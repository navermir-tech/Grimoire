#include "Factions/GrimTowerNPC.h"
#include "Factions/GrimDialogueWidget.h"
#include "Factions/GrimFactionTypes.h"
#include "Quest/GrimQuestSettings.h"
#include "Quest/GrimQuestTrackerWidget.h"
#include "Quest/GrimQuestTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "GrimTowerNPC"

AGrimTowerNPC::AGrimTowerNPC()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.f;
	// 서버 RPC 인자로 가리키려면(배치 액터는 이름으로) 복제 액터여야 한다.
	bReplicates = true;
	SetNetUpdateFrequency(1.f);

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(40.f, 92.f);
	Capsule->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Capsule);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	auto MakePart = [this](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Capsule);
		if (Mesh)
		{
			Part->SetStaticMesh(Mesh);
		}
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};
	// 기본 도형은 100cm. 캡슐 가운데(키의 반)가 원점.
	Body = MakePart(TEXT("Body"), CylinderMesh.Object, FVector(0.f, 0.f, -20.f), FVector(0.7f, 0.7f, 1.45f));
	Head = MakePart(TEXT("Head"), SphereMesh.Object, FVector(0.f, 0.f, 70.f), FVector(0.4f));
	Hat = MakePart(TEXT("Hat"), ConeMesh.Object, FVector(0.f, 0.f, 112.f), FVector(0.55f, 0.55f, 0.6f));
}

void AGrimTowerNPC::BeginPlay()
{
	Super::BeginPlay();
	ApplyColors();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
	}
	if (!GetNpcDef())
	{
		UE_LOG(LogGrimQuest, Warning, TEXT("%s: NPC 표에 %s 가 없다"), *GetName(), *NpcId.ToString());
	}
}

void AGrimTowerNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Dialogue)
	{
		Dialogue->CloseConversation();
		Dialogue->RemoveFromParent();
		Dialogue = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AGrimTowerNPC::ApplyColors()
{
	auto Tint = [](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (Part && Part->GetStaticMesh())
		{
			if (UMaterialInstanceDynamic* Material = Part->CreateAndSetMaterialInstanceDynamic(0))
			{
				Material->SetVectorParameterValue(TEXT("Color"), Color);
			}
		}
	};
	Tint(Body, RobeColor);
	Tint(Head, FLinearColor(0.85f, 0.7f, 0.58f));
	Tint(Hat, RobeColor * 0.7f);
}

const FGrimNpcDef* AGrimTowerNPC::GetNpcDef() const
{
	return FGrimDialogueDatabase::Get().FindNpc(NpcId);
}

FText AGrimTowerNPC::GetDisplayName() const
{
	const FGrimNpcDef* Def = GetNpcDef();
	return Def ? FText::FromString(Def->DisplayName) : FText::FromName(NpcId);
}

bool AGrimTowerNPC::OffersService(FName Service) const
{
	const FGrimNpcDef* Def = GetNpcDef();
	return Def && Def->Services.Contains(Service);
}

bool AGrimTowerNPC::IsWithinTalkRange(const APawn* Pawn, float Slack) const
{
	if (!Pawn)
	{
		return false;
	}
	const float Radius = UGrimQuestSettings::Get()->TalkRadius + FMath::Max(0.f, Slack);
	return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

void AGrimTowerNPC::BuildLines(FName Context, const FGrimQuestLog& Log, TArray<FString>& OutLines) const
{
	OutLines.Reset();
	if (const FGrimDialogueEntry* Entry = FGrimDialogueDatabase::Get().Select(NpcId, Context, Log))
	{
		OutLines = Entry->Lines;
	}
	if (OutLines.Num() == 0)
	{
		OutLines.Add(TEXT("..."));
	}
}

AGrimTowerNPC* AGrimTowerNPC::FindById(const UWorld* World, FName InNpcId)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AGrimTowerNPC> It(World); It; ++It)
	{
		if (It->GetNpcId() == InNpcId)
		{
			return *It;
		}
	}
	return nullptr;
}

void AGrimTowerNPC::BlockReopenFor(float Seconds)
{
	ReopenBlockedUntil = FPlatformTime::Seconds() + Seconds;
}

void AGrimTowerNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->IsLocalController())
		{
			HandleLocalPlayer(PlayerController);
			break;
		}
	}
}

void AGrimTowerNPC::HandleLocalPlayer(APlayerController* PlayerController)
{
	const APawn* Pawn = PlayerController->GetPawn();
	const bool bInRange = IsWithinTalkRange(Pawn);

	if (!Dialogue && bInRange && PlayerController->GetLocalPlayer())
	{
		Dialogue = CreateWidget<UGrimDialogueWidget>(PlayerController, UGrimDialogueWidget::StaticClass());
		if (Dialogue)
		{
			Dialogue->AddToViewport(20);
		}
	}
	if (!Dialogue)
	{
		return;
	}

	if (Dialogue->IsConversationOpen())
	{
		// 멀어지면 닫는다(여유를 조금 준다).
		if (!IsWithinTalkRange(Pawn, 150.f))
		{
			Dialogue->CloseConversation();
		}
		return;
	}

	if (!bInRange)
	{
		if (bPromptShown)
		{
			Dialogue->HidePrompt();
			bPromptShown = false;
		}
		return;
	}

	const FKey& Key = UGrimQuestSettings::Get()->TalkKey;
	if (!bPromptShown)
	{
		Dialogue->ShowPrompt(FText::Format(LOCTEXT("Prompt", "[{0}] {1}에게 말 걸기"), Key.GetDisplayName(), GetDisplayName()));
		bPromptShown = true;
	}
	if (FPlatformTime::Seconds() >= ReopenBlockedUntil && PlayerController->WasInputKeyJustPressed(Key))
	{
		bPromptShown = false;
		Dialogue->HidePrompt();
		Dialogue->OpenConversation(this);
	}
}

// ───── 게시판 ─────

AGrimQuestBoard::AGrimQuestBoard()
{
	NpcId = TEXT("QuestBoard");
	RobeColor = FLinearColor(0.42f, 0.28f, 0.16f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Capsule->InitCapsuleSize(60.f, 100.f);
	if (CubeMesh.Succeeded())
	{
		Body->SetStaticMesh(CubeMesh.Object);
		Head->SetStaticMesh(CubeMesh.Object);
	}
	// 판(160×10×110)과 다리 하나. 앞(+X)에서 읽는다.
	Body->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	Body->SetRelativeScale3D(FVector(0.1f, 1.6f, 1.1f));
	Head->SetRelativeLocation(FVector(0.f, 0.f, -65.f));
	Head->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.7f));
	Hat->SetVisibility(false);
	Hat->SetHiddenInGame(true);
}

void AGrimQuestBoard::BuildLines(FName Context, const FGrimQuestLog& Log, TArray<FString>& OutLines) const
{
	Super::BuildLines(Context, Log, OutLines);
	if (Context != GrimDialogueContexts::Talk)
	{
		return;
	}
	const FGrimQuestDatabase& Database = UGrimQuestSettings::GetQuestDatabase();
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float ServerTime = GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : 0.f;
	for (const FGrimQuestProgress& Progress : Log.Active)
	{
		FString Title, Text;
		UGrimQuestTrackerWidget::DescribeProgress(Database, Progress, ServerTime, Title, Text);
		OutLines.Add(FString::Printf(TEXT("「%s」 — %s"), *Title, *Text.Replace(TEXT("\n"), TEXT(" "))));
	}
	if (Log.Completed.Num() > 0)
	{
		TArray<FString> Titles;
		for (const FName QuestId : Log.Completed)
		{
			const FGrimQuestDef* Def = Database.Find(QuestId);
			Titles.Add(Def ? Def->Title : QuestId.ToString());
		}
		OutLines.Add(FString::Printf(TEXT("끝낸 일: %s"), *FString::Join(Titles, TEXT(", "))));
	}
}

#undef LOCTEXT_NAMESPACE
