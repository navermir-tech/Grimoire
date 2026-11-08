#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Appearance/GrimAppearanceComponent.h"
#include "Appearance/GrimAppearanceDummy.h"
#include "Appearance/GrimAppearanceSettings.h"
#include "Appearance/GrimAppearanceTypes.h"
#include "Appearance/GrimCharacterCreatorWidget.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/BitReader.h"
#include "Serialization/BitWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace GrimAppearanceTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags PureFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const FName RuleNames[] = { TEXT("Player_Start"), TEXT("Tower_Apprentice"), TEXT("Tower_Instructor"), TEXT("Tower_Librarian"), TEXT("Tower_Examiner") };

	/** 실제 CSV 로 만든 카탈로그(전역과 같은 것) */
	const FGrimAppearanceCatalog& Catalog()
	{
		return UGrimAppearanceSettings::GetCatalog();
	}

	FGrimAppearance SaveRoundTrip(const FGrimAppearance& In)
	{
		FGrimAppearance Copy = In;
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes);
		Copy.Serialize(Writer);
		FGrimAppearance Out;
		FMemoryReader Reader(Bytes);
		Out.Serialize(Reader);
		return Out;
	}

	/** 게임 월드(게임 모드 없음). 소멸자가 정리한다 */
	struct FScopedAppearanceWorld
	{
		UWorld* World = nullptr;

		FScopedAppearanceWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimAppearanceTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FScopedAppearanceWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

using namespace GrimAppearanceTestsLocal;

// 표 여섯 개가 문제 없이 읽히고, 규칙 · 팔레트 · 슬라이더가 스펙(docs/spec/customization) 수만큼 있다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimAppearanceCatalogTest, "Grimoire.Appearance.Catalog", PureFlags)
bool FGrimAppearanceCatalogTest::RunTest(const FString& Parameters)
{
	const UGrimAppearanceSettings* Settings = UGrimAppearanceSettings::Get();
	FString Texts[6];
	const FString* Paths[6] = { &Settings->PartsCSV, &Settings->PiecesCSV, &Settings->PalettesCSV, &Settings->MorphsCSV, &Settings->RulesCSV, &Settings->NpcsCSV };
	for (int32 Index = 0; Index < 6; ++Index)
	{
		TestTrue(FString::Printf(TEXT("%s 를 연다"), **Paths[Index]), FFileHelper::LoadFileToString(Texts[Index], *FPaths::Combine(FPaths::ProjectContentDir(), *Paths[Index])));
	}
	FGrimAppearanceCatalog Built;
	TArray<FString> Problems;
	FGrimAppearanceCatalog::BuildFromCSV(Texts[0], Texts[1], Texts[2], Texts[3], Texts[4], Texts[5], Built, Problems);
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}

	TestEqual(TEXT("얼굴 슬라이더 6"), Built.Morphs.Num(), 6);
	TestEqual(TEXT("생성 화면 얼굴 8"), Built.GetPartsForSlot(EGrimAppearanceSlot::Face, 0, true).Num(), 8);
	TestEqual(TEXT("생성 화면 머리 모양 8"), Built.GetPartsForSlot(EGrimAppearanceSlot::Hair, 0, true).Num(), 8);
	TestEqual(TEXT("피부색 12"), Built.GetColorChoices(GrimAppearancePalettes::Skin, true).Num(), 12);
	TestEqual(TEXT("눈 색 10"), Built.GetColorChoices(GrimAppearancePalettes::Eye, true).Num(), 10);
	TestEqual(TEXT("머리색 12(생성 화면)"), Built.GetColorChoices(GrimAppearancePalettes::Hair, true).Num(), 12);
	TestEqual(TEXT("속성 색 6"), Built.GetPalette(GrimAppearancePalettes::Element).Num(), 6);
	for (const FName& Rule : RuleNames)
	{
		TestNotNull(*FString::Printf(TEXT("규칙 %s"), *Rule.ToString()), Built.FindRule(Rule));
	}
	TestNotNull(TEXT("플레이어 규칙(설정)"), Built.FindRule(Settings->PlayerRule));
	TestTrue(TEXT("마렌(Master) 외형 표"), Built.Npcs.Contains(TEXT("Master")));
	const FGrimAppearancePart* Stole = Built.FindPart(TEXT("Stole_A_Parchment"));
	TestTrue(TEXT("스톨에 서클 고리 소품"), Stole && Stole->Pieces.ContainsByPredicate([&Built](int32 Index) { return Built.Pieces[Index].bRepeatCircle; }));
	const FGrimAppearancePart* Examiner = Built.FindPart(TEXT("Robe_A_Examiner"));
	TestTrue(TEXT("시험관 로브는 NPC 전용"), Examiner && !Examiner->Row.Creator);

	// 시험관 색은 표의 16진수 그대로(sRGB → 선형)
	const FLinearColor Ink = Built.GetColor(GrimAppearancePalettes::Cloth, Built.FindColorIndex(GrimAppearancePalettes::Cloth, TEXT("Cloth_Ink")));
	TestTrue(TEXT("먹빛 남색 #232A45"), Ink.ToFColorSRGB() == FColor(0x23, 0x2A, 0x45));

	// 복제 번호 왕복
	for (const FGrimAppearancePart& Part : Built.Parts)
	{
		if (Built.PartFromNet(Built.PartToNet(Part.Id)) != Part.Id)
		{
			AddError(FString::Printf(TEXT("복제 번호 왕복 실패: %s"), *Part.Id.ToString()));
		}
	}
	TestEqual(TEXT("없는 조각 = 0"), Built.PartToNet(TEXT("Nope")), static_cast<uint16>(0));
	return true;
}

// 저장 형식: 이름으로 쓰고 읽어 같다. 없어진 조각 이름은 Sanitize 가 비운다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimAppearanceSerializationTest, "Grimoire.Appearance.Serialization", PureFlags)
bool FGrimAppearanceSerializationTest::RunTest(const FString& Parameters)
{
	for (const FName& Rule : RuleNames)
	{
		const FGrimAppearance Original = FGrimAppearanceMath::GenerateFromRule(Catalog(), Rule, 42);
		TestTrue(*FString::Printf(TEXT("%s 생성됨"), *Rule.ToString()), Original.IsSet());
		FGrimAppearance Loaded = SaveRoundTrip(Original);
		TestFalse(*FString::Printf(TEXT("%s 읽은 뒤 고칠 것 없음"), *Rule.ToString()), FGrimAppearanceMath::Sanitize(Loaded, Catalog(), Original.Rule.IsNone()));
		TestTrue(*FString::Printf(TEXT("%s 저장 왕복 같음"), *Rule.ToString()), Loaded == Original);
		TestEqual(*FString::Printf(TEXT("%s 해시 같음"), *Rule.ToString()), Loaded.GetHash(), Original.GetHash());
	}

	// 다음 버전에서 조각이 없어졌다 → 그 칸은 비고 나머지는 그대로
	FGrimAppearance Player = FGrimAppearanceMath::GenerateFromRule(Catalog(), TEXT("Player_Start"), 7);
	Player.SetPart(EGrimAppearanceSlot::Robe, TEXT("Robe_Removed_In_Future"));
	Player.SetPart(EGrimAppearanceSlot::Face, TEXT("Face_Removed_In_Future"));
	FGrimAppearance Loaded = SaveRoundTrip(Player);
	TestEqual(TEXT("모르는 이름도 일단 읽는다"), Loaded.GetPart(EGrimAppearanceSlot::Robe), FName(TEXT("Robe_Removed_In_Future")));
	TestTrue(TEXT("Sanitize 가 고친다"), FGrimAppearanceMath::Sanitize(Loaded, Catalog(), true));
	TestTrue(TEXT("없어진 로브 → 비움"), Loaded.GetPart(EGrimAppearanceSlot::Robe).IsNone());
	TestFalse(TEXT("얼굴은 비울 수 없다 → 첫 얼굴"), Loaded.GetPart(EGrimAppearanceSlot::Face).IsNone());
	TestEqual(TEXT("스톨은 그대로"), Loaded.GetPart(EGrimAppearanceSlot::Stole), Player.GetPart(EGrimAppearanceSlot::Stole));

	// 깨진 바이트는 오류로 끝난다(무한 읽기 없음)
	TArray<uint8> Garbage = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0x7F };
	FMemoryReader Reader(Garbage);
	FGrimAppearance Broken;
	Broken.Serialize(Reader);
	TestTrue(TEXT("깨진 바이트 = 오류 표시"), Reader.IsError() || !Broken.Rule.IsNone() || Broken.Parts.Num() == GrimAppearanceSlotCount);
	return true;
}

// 복제 형식: 비트로 쓰고 읽어 같고, 작다(대략 40바이트).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimAppearanceNetSerializeTest, "Grimoire.Appearance.NetSerialize", PureFlags)
bool FGrimAppearanceNetSerializeTest::RunTest(const FString& Parameters)
{
	for (const FName& Rule : RuleNames)
	{
		for (int32 Seed : { 1, 99, -12345 })
		{
			FGrimAppearance Original = FGrimAppearanceMath::GenerateFromRule(Catalog(), Rule, Seed);
			FBitWriter Writer(0, true);
			bool bOk = false;
			Original.NetSerialize(Writer, nullptr, bOk);
			TestTrue(TEXT("쓰기 성공"), bOk && !Writer.IsError());

			FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
			FGrimAppearance Received;
			bOk = false;
			Received.NetSerialize(Reader, nullptr, bOk);
			TestTrue(*FString::Printf(TEXT("%s#%d 읽기 성공"), *Rule.ToString(), Seed), bOk && !Reader.IsError());
			TestTrue(*FString::Printf(TEXT("%s#%d 왕복 같음"), *Rule.ToString(), Seed), Received == Original);
			TestTrue(*FString::Printf(TEXT("%s#%d 64바이트 이하(%d비트)"), *Rule.ToString(), Seed, static_cast<int32>(Writer.GetNumBits())), Writer.GetNumBytes() <= 64);
		}
	}
	// 빈 외형도 왕복(아직 정해지지 않은 폰)
	FGrimAppearance Empty;
	FBitWriter Writer(0, true);
	bool bOk = false;
	Empty.NetSerialize(Writer, nullptr, bOk);
	FBitReader Reader(Writer.GetData(), Writer.GetNumBits());
	FGrimAppearance Received;
	Received.NetSerialize(Reader, nullptr, bOk);
	TestFalse(TEXT("빈 외형은 빈 채로"), Received.IsSet());
	return true;
}

// 씨앗이 같으면 같은 사람, 씨앗이 다르면 여러 사람. 결과는 늘 그 규칙 안.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimAppearanceNpcDeterminismTest, "Grimoire.Appearance.NpcDeterminism", PureFlags)
bool FGrimAppearanceNpcDeterminismTest::RunTest(const FString& Parameters)
{
	for (const FName& RuleId : RuleNames)
	{
		const FGrimAppearanceRule* Rule = Catalog().FindRule(RuleId);
		if (!TestNotNull(*RuleId.ToString(), Rule))
		{
			continue;
		}
		TSet<uint32> Distinct;
		TSet<FName> Faces;
		int32 BodyCounts[2] = { 0, 0 };
		for (int32 Seed = 1; Seed <= 60; ++Seed)
		{
			const FGrimAppearance A = FGrimAppearanceMath::GenerateFromRule(Catalog(), RuleId, Seed);
			const FGrimAppearance B = FGrimAppearanceMath::GenerateFromRule(Catalog(), RuleId, Seed);
			if (A != B)
			{
				AddError(FString::Printf(TEXT("%s#%d: 같은 씨앗인데 다르다"), *RuleId.ToString(), Seed));
			}
			Distinct.Add(A.GetHash());
			++BodyCounts[FMath::Min<uint8>(A.BodyType, 1)];
			Faces.Add(A.GetPart(EGrimAppearanceSlot::Face));

			// 칸마다 그 규칙의 후보 안
			for (int32 SlotIndex = 0; SlotIndex < GrimAppearanceSlotCount; ++SlotIndex)
			{
				const FName Part = A.Parts[SlotIndex];
				const bool bAllowed = Rule->Slots[SlotIndex].ContainsByPredicate([Part](const FGrimAppearanceCandidate& C) { return C.Part == Part; })
					|| (Part.IsNone() && Rule->Slots[SlotIndex].Num() == 0)
					|| (SlotIndex == static_cast<int32>(EGrimAppearanceSlot::Face) && Rule->Slots[SlotIndex].Num() == 0);
				if (!bAllowed)
				{
					AddError(FString::Printf(TEXT("%s#%d: %s 칸 %s 는 규칙 밖"), *RuleId.ToString(), Seed,
						*FGrimAppearanceMath::SlotName(static_cast<EGrimAppearanceSlot>(SlotIndex)), *Part.ToString()));
				}
			}
			if (A.Circle < Rule->Row.CircleMin || A.Circle > Rule->Row.CircleMax)
			{
				AddError(FString::Printf(TEXT("%s#%d: 서클 %d 가 범위 밖"), *RuleId.ToString(), Seed, A.Circle));
			}
			if (Rule->Elements.Num() > 0 && !Rule->Elements.Contains(A.Element))
			{
				AddError(FString::Printf(TEXT("%s#%d: 속성 %d 가 규칙 밖"), *RuleId.ToString(), Seed, A.Element));
			}
			if (Rule->Row.Player)
			{
				FGrimAppearance Copy = A;
				TestFalse(*FString::Printf(TEXT("%s#%d 플레이어 규칙 결과는 플레이어 검사를 통과"), *RuleId.ToString(), Seed),
					FGrimAppearanceMath::Sanitize(Copy, Catalog(), true));
			}
		}
		TestTrue(*FString::Printf(TEXT("%s: 60명 중 서로 다른 사람 40+ (%d)"), *RuleId.ToString(), Distinct.Num()), Distinct.Num() >= 40);
		TestTrue(*FString::Printf(TEXT("%s: 얼굴 5종+ (%d)"), *RuleId.ToString(), Faces.Num()), Faces.Num() >= 5);
		// 이웃 씨앗(1~60)이 한쪽 체형으로 몰리지 않는다(가중치 1:1 → 각 15명 넘게)
		TestTrue(*FString::Printf(TEXT("%s: 체형 A %d · B %d 고르게"), *RuleId.ToString(), BodyCounts[0], BodyCounts[1]), BodyCounts[0] > 15 && BodyCounts[1] > 15);
	}

	// 같은 씨앗이라도 규칙이 다르면 다른 사람
	TestTrue(TEXT("규칙마다 다른 사람"), FGrimAppearanceMath::GenerateFromRule(Catalog(), TEXT("Tower_Librarian"), 5).GetPart(EGrimAppearanceSlot::Robe)
		!= FGrimAppearanceMath::GenerateFromRule(Catalog(), TEXT("Tower_Examiner"), 5).GetPart(EGrimAppearanceSlot::Robe));

	// 이름 있는 NPC: 씨앗 = 이름 해시(대소문자 무관) → 늘 같은 얼굴, 규칙 = 표
	const FGrimAppearance Maren1 = FGrimAppearanceMath::GenerateForNpc(Catalog(), TEXT("Master"), TEXT("Tower_Apprentice"));
	const FGrimAppearance Maren2 = FGrimAppearanceMath::GenerateForNpc(Catalog(), TEXT("master"), TEXT("Tower_Apprentice"));
	TestTrue(TEXT("마렌은 늘 같은 얼굴"), Maren1 == Maren2);
	TestEqual(TEXT("마렌 규칙 = 교관"), Maren1.Rule, FName(TEXT("Tower_Instructor")));
	TestEqual(TEXT("마렌 씨앗 = 이름 해시"), Maren1.Seed, FGrimAppearanceMath::MakeSeed(TEXT("Master")));
	TestEqual(TEXT("이름 해시는 문자열 CRC(실행마다 같다)"), FGrimAppearanceMath::MakeSeed(TEXT("Master")), static_cast<int32>(FCrc::StrCrc32(TEXT("master"))));
	const FGrimAppearance Unknown = FGrimAppearanceMath::GenerateForNpc(Catalog(), TEXT("NoSuchNpc"), TEXT("Tower_Apprentice"));
	TestEqual(TEXT("표에 없는 NPC = 기본 규칙"), Unknown.Rule, FName(TEXT("Tower_Apprentice")));
	TestTrue(TEXT("없는 규칙 = 빈 외형"), !FGrimAppearanceMath::GenerateFromRule(Catalog(), TEXT("NoSuchRule"), 1).IsSet());
	return true;
}

// 서버 검사: 플레이어는 NPC 전용 조각 · 색을 못 입고, 체형 · 범위가 맞춰진다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimAppearanceSanitizeTest, "Grimoire.Appearance.Sanitize", PureFlags)
bool FGrimAppearanceSanitizeTest::RunTest(const FString& Parameters)
{
	FGrimAppearance Cheater = FGrimAppearanceMath::GenerateFromRule(Catalog(), TEXT("Tower_Examiner"), 3);
	Cheater.BodyType = 9;
	Cheater.SkinColor = 250;
	Cheater.ClothSecondary = static_cast<uint8>(Catalog().FindColorIndex(GrimAppearancePalettes::Cloth, TEXT("Cloth_Ivory")));
	Cheater.Element = 77;
	Cheater.Circle = 200;
	Cheater.Morphs.SetNum(2);
	Cheater.SetPart(EGrimAppearanceSlot::Hat, TEXT("Robe_A_Apprentice")); // 칸이 다른 조각

	FGrimAppearance AsPlayer = Cheater;
	TestTrue(TEXT("고칠 것이 있다"), FGrimAppearanceMath::Sanitize(AsPlayer, Catalog(), true));
	TestEqual(TEXT("체형 0~1"), AsPlayer.BodyType, static_cast<uint8>(1));
	TestTrue(TEXT("시험관 로브(NPC 전용) 벗겨짐"), AsPlayer.GetPart(EGrimAppearanceSlot::Robe).IsNone());
	TestTrue(TEXT("원통 모자(NPC 전용) 벗겨짐 · 칸이 다른 조각도"), AsPlayer.GetPart(EGrimAppearanceSlot::Hat).IsNone());
	TestTrue(TEXT("어깨 망토(NPC 전용) 벗겨짐"), AsPlayer.GetPart(EGrimAppearanceSlot::Cloak).IsNone());
	TestEqual(TEXT("스톨은 그대로"), AsPlayer.GetPart(EGrimAppearanceSlot::Stole), FName(TEXT("Stole_A_Parchment")));
	TestTrue(TEXT("피부색 범위 안"), Catalog().GetPalette(GrimAppearancePalettes::Skin).IsValidIndex(AsPlayer.SkinColor));
	TestTrue(TEXT("상아색(NPC 전용) → 고를 수 있는 색"), Catalog().GetPalette(GrimAppearancePalettes::Cloth)[AsPlayer.ClothSecondary].bCreator);
	TestTrue(TEXT("속성 0~6"), AsPlayer.Element <= FGrimAppearance::ElementCount);
	TestEqual(TEXT("서클 0~10"), AsPlayer.Circle, static_cast<uint8>(FGrimAppearance::MaxCircle));
	TestEqual(TEXT("슬라이더 수 = 표"), AsPlayer.Morphs.Num(), Catalog().Morphs.Num());
	TestTrue(TEXT("플레이어는 규칙 · 씨앗 없음"), AsPlayer.Rule.IsNone() && AsPlayer.Seed == 0);

	FGrimAppearance AsNpc = Cheater;
	FGrimAppearanceMath::Sanitize(AsNpc, Catalog(), false);
	TestEqual(TEXT("NPC 는 시험관 로브 그대로"), AsNpc.GetPart(EGrimAppearanceSlot::Robe), FName(TEXT("Robe_A_Examiner")));
	TestEqual(TEXT("NPC 는 규칙 이름 유지"), AsNpc.Rule, FName(TEXT("Tower_Examiner")));

	FGrimAppearance Fresh;
	FGrimAppearanceMath::Sanitize(Fresh, Catalog(), true);
	TestTrue(TEXT("빈 외형도 얼굴은 생긴다"), !Fresh.GetPart(EGrimAppearanceSlot::Face).IsNone());

	// 얼굴을 바꾸면 슬라이더가 그 얼굴 기본값으로
	FGrimAppearance Face = Fresh;
	Face.SetPart(EGrimAppearanceSlot::Face, TEXT("Face_03"));
	FGrimAppearanceMath::ApplyFaceDefaults(Face, Catalog());
	TestEqual(TEXT("각진 턱 = 턱 너비 0.5"), Face.GetMorph(Catalog().FindMorph(TEXT("JawWidth"))), 0.5f, 0.01f);
	return true;
}

// 조립: 규칙대로 만든 사람에 소품 · 서클 고리가 붙고, 체형을 바꾸면 몸 메시가 바뀐다. 마네킹 뼈 기준 위치도 남긴다(CSV 오프셋 다듬기용).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimAppearanceAssembleTest, "Grimoire.Appearance.Assemble", PureFlags)
bool FGrimAppearanceAssembleTest::RunTest(const FString& Parameters)
{
	FScopedAppearanceWorld Scoped;
	AGrimAppearanceDummy* Dummy = Scoped.World->SpawnActor<AGrimAppearanceDummy>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("더미"), Dummy))
	{
		return false;
	}
	UGrimAppearanceComponent* Appearance = Dummy->GetAppearanceComponent();
	USkeletalMeshComponent* Leader = Appearance->GetLeaderMesh();
	TestNotNull(TEXT("리더 메시"), Leader);

	Dummy->GenerateAppearance(TEXT("Tower_Examiner"), 7);
	TestTrue(TEXT("몸이 조립됨"), Appearance->HasBuiltBody());
	TestEqual(TEXT("시험관 고리 7"), Appearance->GetRingCount(), 7);
	TestTrue(TEXT("소품이 붙음"), Appearance->GetStaticPieceCount() > 20);
	const int32 FirstCount = Appearance->GetStaticPieceCount();

	// 미리보기는 정본을 바꾸지 않는다
	FGrimAppearance Preview = Appearance->GetAppearance();
	Preview.Circle = 2;
	Appearance->PreviewAppearance(Preview);
	TestEqual(TEXT("미리보기 고리 2"), Appearance->GetRingCount(), 2);
	TestEqual(TEXT("정본 서클은 7 그대로"), Appearance->GetAppearance().Circle, static_cast<uint8>(7));
	Appearance->ClearPreview();
	TestEqual(TEXT("미리보기 끝 → 고리 7"), Appearance->GetRingCount(), 7);
	TestEqual(TEXT("다시 조립해도 소품 수 같음(지난 것을 지움)"), Appearance->GetStaticPieceCount(), FirstCount);

	// 체형 B → Quinn
	FGrimAppearance BodyB = Appearance->GetAppearance();
	BodyB.BodyType = 1;
	Appearance->SetAppearance(BodyB);
	const UGrimAppearanceSettings* Settings = UGrimAppearanceSettings::Get();
	if (Leader && Settings->BodyMeshes.Num() > 1)
	{
		TestEqual(TEXT("체형 B 몸 메시"), Leader->GetSkeletalMeshAsset(), Settings->BodyMeshes[1].LoadSynchronous());
	}

	// 빈 외형 → 조각 없음
	Appearance->SetAppearance(FGrimAppearance());
	TestEqual(TEXT("빈 외형은 소품 0"), Appearance->GetStaticPieceCount(), 0);

	// 마네킹 기준 자세의 뼈 위치(메시 공간) — 자리 표시 오프셋을 다듬을 때 본다
	if (const USkeletalMesh* Mesh = Leader ? Leader->GetSkeletalMeshAsset() : nullptr)
	{
		for (const TCHAR* Bone : { TEXT("pelvis"), TEXT("spine_03"), TEXT("spine_05"), TEXT("head"), TEXT("hand_l"), TEXT("hand_r"), TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l") })
		{
			const int32 Index = Mesh->GetRefSkeleton().FindBoneIndex(Bone);
			if (Index != INDEX_NONE)
			{
				const FTransform Transform = FAnimationRuntime::GetComponentSpaceTransformRefPose(Mesh->GetRefSkeleton(), Index);
				AddInfo(FString::Printf(TEXT("뼈 %s: %s"), Bone, *Transform.GetLocation().ToCompactString()));
			}
			else
			{
				AddError(FString::Printf(TEXT("마네킹에 뼈 %s 가 없다(AppearancePieces.csv)"), Bone));
			}
		}
	}
	Dummy->Destroy();
	return true;
}

// Q102: 생성 화면 · 거울 카메라 — 캐릭터 정면에서 보고, 얼굴 칸은 얼굴 가까이 · 옷 칸은 전신, 돌려도 같은 거리.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimAppearanceCreatorCameraTest, "Grimoire.Appearance.CreatorCamera", GrimAppearanceTestsLocal::PureFlags)
bool FGrimAppearanceCreatorCameraTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("얼굴 칸 → 얼굴"), FGrimCreatorCameraMath::FocusForSlot(EGrimAppearanceSlot::Face), EGrimCreatorFocus::Face);
	TestEqual(TEXT("머리 칸 → 얼굴"), FGrimCreatorCameraMath::FocusForSlot(EGrimAppearanceSlot::Hair), EGrimCreatorFocus::Face);
	TestEqual(TEXT("수염 칸 → 얼굴"), FGrimCreatorCameraMath::FocusForSlot(EGrimAppearanceSlot::Beard), EGrimCreatorFocus::Face);
	TestEqual(TEXT("로브 칸 → 전신"), FGrimCreatorCameraMath::FocusForSlot(EGrimAppearanceSlot::Robe), EGrimCreatorFocus::Body);
	TestEqual(TEXT("신발 칸 → 전신"), FGrimCreatorCameraMath::FocusForSlot(EGrimAppearanceSlot::Boots), EGrimCreatorFocus::Body);
	TestEqual(TEXT("모자 칸 → 전신(옷)"), FGrimCreatorCameraMath::FocusForSlot(EGrimAppearanceSlot::Hat), EGrimCreatorFocus::Body);

	// 캐릭터가 +X 를 본다(FrontYaw 0) → 카메라는 +X 쪽 앞에 서서 −X 를 본다(정면).
	const FVector Pivot(100.f, 200.f, 300.f);
	const FTransform Front = FGrimCreatorCameraMath::ComputePose(Pivot, 0.f, 0.f, 0.f, 90.f, 0.f);
	TestTrue(TEXT("정면: 캐릭터 앞(+X)에 선다"), Front.GetLocation().X > Pivot.X + 89.f);
	TestTrue(TEXT("정면: 캐릭터 쪽(−X)을 본다"), FVector::DotProduct(Front.GetRotation().Vector(), FVector(-1.f, 0.f, 0.f)) > 0.999f);
	TestEqual(TEXT("정면: 거리"), FVector::Dist(Front.GetLocation(), Pivot), 90.0, 0.01);

	// 돌려도(끌기) 같은 거리, 90° 돌면 옆에 선다.
	const FTransform Side = FGrimCreatorCameraMath::ComputePose(Pivot, 0.f, 90.f, 0.f, 90.f, 0.f);
	TestEqual(TEXT("돌려도 같은 거리"), FVector::Dist(Side.GetLocation(), Pivot), 90.0, 0.01);
	TestTrue(TEXT("90° → 옆"), FMath::Abs(Side.GetLocation().X - Pivot.X) < 0.1 && FMath::Abs(Side.GetLocation().Y - Pivot.Y) > 89.0);

	// 위에서 내려다보면(Pitch −) 카메라가 위에 있다.
	TestTrue(TEXT("내려다보기 → 위"), FGrimCreatorCameraMath::ComputePose(Pivot, 0.f, 0.f, -20.f, 90.f, 0.f).GetLocation().Z > Pivot.Z);

	// 왼쪽 판에 가리지 않게: 캐릭터(Pivot)가 카메라 기준 오른쪽(+Y 축)에 있다.
	const FTransform Shifted = FGrimCreatorCameraMath::ComputePose(Pivot, 0.f, 0.f, 0.f, 400.f, 70.f);
	const FVector Local = Shifted.InverseTransformPosition(Pivot);
	TestTrue(TEXT("캐릭터는 화면 오른쪽"), Local.Y > 69.f && Local.X > 399.f);

	// 캐릭터가 다른 쪽을 보고 있어도 정면(FrontYaw 따라감).
	const FTransform Turned = FGrimCreatorCameraMath::ComputePose(Pivot, 90.f, 0.f, 0.f, 90.f, 0.f);
	TestTrue(TEXT("캐릭터가 +Y 를 보면 카메라는 +Y 쪽"), Turned.GetLocation().Y > Pivot.Y + 89.f);

	// ini: 얼굴은 가깝고 전신은 멀다
	const FGrimCreatorCameraTuning& Tuning = UGrimAppearanceSettings::Get()->CreatorCamera;
	TestTrue(TEXT("ini: 얼굴 < 전신 거리"), Tuning.FaceDistance < Tuning.BodyDistance);
	TestTrue(TEXT("ini: 끌기 감도"), Tuning.DragYawPerPixel > 0.f);
	TestTrue(TEXT("ini: 각 범위"), Tuning.MinPitch < Tuning.DefaultPitch && Tuning.DefaultPitch < Tuning.MaxPitch);
	return true;
}

#endif
