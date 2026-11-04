// 마법 디버그 콘솔 명령 — 헤드리스 리슨 서버 + 클라이언트로 완드 부착 복제를 확인할 때 쓴다.

#include "Magic/GrimWand.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

DEFINE_LOG_CATEGORY_STATIC(LogGrimMagicDebug, Log, All);

namespace
{
	void DumpWandsInWorld(UWorld* World)
	{
		const TCHAR* NetMode = World->GetNetMode() == NM_Client ? TEXT("Client") : World->GetNetMode() == NM_ListenServer ? TEXT("ListenServer") : TEXT("Server");
		for (TActorIterator<AGrimWand> It(World); It; ++It)
		{
			const USceneComponent* Parent = It->GetRootComponent()->GetAttachParent();
			const FName Socket = It->GetRootComponent()->GetAttachSocketName();
			// 손 소켓에서 완드 원점까지 거리 — 제대로 붙었으면 0 이다.
			const float Offset = Parent ? FVector::Dist(Parent->GetSocketLocation(Socket), It->GetActorLocation()) : -1.f;
			UE_LOG(LogGrimMagicDebug, Display, TEXT("[%s] %s owner=%s parent=%s socket=%s offset=%.1fcm muzzle=%s"),
				NetMode, *It->GetName(), *GetNameSafe(It->GetOwner()),
				Parent ? *FString::Printf(TEXT("%s.%s"), *GetNameSafe(Parent->GetOwner()), *Parent->GetName()) : TEXT("None"),
				*Socket.ToString(), Offset, *It->GetMuzzleTransform().GetLocation().ToCompactString());

			// 쥐는 자세를 정할 때 쓰는 값: 손 소켓 공간에서 본 엄지 · 가운뎃손가락 · 완드 끝 · 폰 앞 방향.
			if (const USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(Parent))
			{
				const FTransform Hand = SkelMesh->GetSocketTransform(Socket);
				auto Local = [&Hand](const FVector& World) { return Hand.InverseTransformPositionNoScale(World).GetSafeNormal(); };
				UE_LOG(LogGrimMagicDebug, Display, TEXT("[%s]   hand space: thumb=%s middle=%s muzzle=%s pawnFwd=%s pawnUp=%s"), NetMode,
					*Local(SkelMesh->GetSocketLocation(TEXT("thumb_02_r"))).ToCompactString(),
					*Local(SkelMesh->GetSocketLocation(TEXT("middle_02_r"))).ToCompactString(),
					*Local(It->GetMuzzleTransform().GetLocation()).ToCompactString(),
					*Hand.InverseTransformVectorNoScale(SkelMesh->GetOwner()->GetActorForwardVector()).ToCompactString(),
					*Hand.InverseTransformVectorNoScale(SkelMesh->GetOwner()->GetActorUpVector()).ToCompactString());
			}
		}
	}

	/** Grim.Magic.DumpWands [Delay] — 이 프로세스에서 본 완드의 부착 부모 · 소켓 · 손에서 떨어진 거리 */
	void DumpWandsCommand(const TArray<FString>& Args)
	{
		auto RunNow = []()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World(); World && World->IsGameWorld())
				{
					DumpWandsInWorld(World);
				}
			}
		};
		const float Delay = Args.IsValidIndex(0) ? FCString::Atof(*Args[0]) : 0.f;
		if (Delay <= 0.f)
		{
			RunNow();
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([RunNow](float)
		{
			RunNow();
			return false;
		}), Delay);
	}

	FAutoConsoleCommand GDumpWandsCommand(
		TEXT("Grim.Magic.DumpWands"),
		TEXT("이 프로세스에서 본 완드의 부착 부모 · 소켓 · 손에서 떨어진 거리를 로그로. [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DumpWandsCommand));
}

#endif
