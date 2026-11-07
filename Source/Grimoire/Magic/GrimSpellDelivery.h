#pragma once

#include "CoreMinimal.h"
#include "GrimSpellDelivery.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimMagic, Log, All);

/**
 * 원뿔(Cone) 마법의 거리별 자극 세기 (spell-feel 「전달 방식」).
 * NearDistance 안 = NearIntensity, 거기서 사거리 끝까지 FarIntensity 로 줄어든다(Exponent 1 = 직선).
 * 미풍: 바로 앞은 돌풍(2.5 이상 = 불 꺼짐), 멀리는 미풍(0.3~2.5 = 불 키움).
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimConeFalloff
{
	GENERATED_BODY()

	/** 이 거리 안은 NearIntensity 그대로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cone", meta = (ClampMin = "0", Units = "cm"))
	float NearDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cone", meta = (ClampMin = "0"))
	float NearIntensity = 1.f;

	/** 사거리 끝(ConeRange)에서의 세기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cone", meta = (ClampMin = "0"))
	float FarIntensity = 1.f;

	/** 줄어드는 곡선. 1 = 직선, 1 보다 크면 가까이서 오래 세다가 멀리서 빨리 준다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cone", meta = (ClampMin = "0.01"))
	float Exponent = 1.f;
};

/**
 * 전달 방식(Bolt · Stream · Cone)의 순수 계산. 액터 · 월드를 모른다 — 어빌리티 · 투사체 · 연출(MagicFX) · 테스트가 같은 식을 쓴다.
 * 테스트: Grimoire.Magic.Delivery.*
 */
namespace GrimSpellDelivery
{
	/** 발사 시작점과 방향 */
	struct FLaunch
	{
		FVector Start = FVector::ZeroVector;
		FVector Direction = FVector::ForwardVector;
		/** 가까운 대상이라 가슴 높이에서 쏜다 */
		bool bFromChest = false;
	};

	/**
	 * 카메라 ↔ 완드 시차 보정 (spell-feel 「조준 — 시차 보정」).
	 * 방향 = 시작점 → 조준점. 조준점이 CloseDistance 안이면 시작점을 완드 끝 대신 가슴 높이로(어깨 너머로 엉뚱하게 나가지 않게).
	 * 조준점이 시작점 뒤(시선 방향 기준)면 시선 방향 그대로.
	 */
	GRIMOIRE_API FLaunch ComputeLaunch(const FVector& Muzzle, const FVector& Chest, const FVector& AimPoint, const FVector& ViewDirection, float CloseDistance);

	/**
	 * Bolt 유도: 속도 방향을 대상 쪽으로 초당 MaxTurnDegPerSec 만큼까지 돌린다(속력은 그대로).
	 * 대상이 MaxAngleDeg 보다 옆 · 뒤에 있으면(빗나감) 돌리지 않는다 — 유턴하지 않는 "살짝 휨"(Q16).
	 */
	GRIMOIRE_API FVector SteerTowards(const FVector& Velocity, const FVector& Position, const FVector& TargetPoint, float MaxTurnDegPerSec, float MaxAngleDeg, float DeltaTime);

	/**
	 * Stream 포물선이 Target 을 지나도록 하는 발사 방향(낮은 호). GravityZ 는 음수(cm/s²).
	 * 닿을 수 없으면 false — OutDirection 은 그 방향으로 가장 멀리 가는 45° (조준이 45° 보다 높으면 조준 방향 그대로).
	 */
	GRIMOIRE_API bool SolveLaunchDirection(const FVector& Start, const FVector& Target, float Speed, float GravityZ, FVector& OutDirection);

	/** 포물선 위의 점 Segments + 1 개(0 ~ TotalTime 등간격). 구간 i = Points[i] → Points[i+1] */
	GRIMOIRE_API void ComputeArcPoints(const FVector& Start, const FVector& Velocity, float GravityZ, float TotalTime, int32 Segments, TArray<FVector>& OutPoints);

	/**
	 * 포물선이 Point 까지 가는 데 걸린 시간 — 수평 거리 ÷ 수평 속력. 거의 수직으로 쏴서 수평 속력이 없으면 Fallback.
	 * 줄기 연출(Niagara)이 판정 호가 막힌 곳에서 물방울을 끝내게 한다. 결과는 [0, Fallback].
	 */
	GRIMOIRE_API float CalcArcTimeToPoint(const FVector& Start, const FVector& Velocity, const FVector& Point, float Fallback);

	/**
	 * 볼트 발사 빛줄기 길이(Q85): 완드 끝부터 볼트가 이미 가 있는 곳(Gap — 원격 지연을 당긴 만큼)과 BoltSpeed × Seconds 중 긴 쪽,
	 * 그 사이에 맞을 곳이 있으면(HitDistance > 0) 거기까지. 0 이하면 긋지 않는다.
	 */
	GRIMOIRE_API float CalcLaunchStreakLength(float Gap, float BoltSpeed, float Seconds, float HitDistance);

	/** 원뿔 판정: Point 가 Origin 에서 Range 안이고 Direction 과의 각이 HalfAngleDeg 이하 */
	GRIMOIRE_API bool IsInCone(const FVector& Origin, const FVector& Direction, float HalfAngleDeg, float Range, const FVector& Point);

	/** 원뿔 거리별 세기. Range = ConeRange */
	GRIMOIRE_API float ConeIntensityAt(const FGrimConeFalloff& Falloff, float Range, float Distance);

	/** Stream 의 대상별 자극 주기: 처음 닿으면 바로, 그 뒤로는 Interval 마다. LastApplyTime 이 없으면 처음 */
	GRIMOIRE_API bool IsStimulusDue(const double* LastApplyTime, double Now, float Interval);

	/** 서버가 시전을 받은 뒤 기다릴 시간과 투사체를 앞으로 당길 시간 */
	struct FLagSplit
	{
		float WaitSeconds = 0.f;
		float AdvanceSeconds = 0.f;
	};

	/**
	 * 지연 보정 (spell-feel 「네트워크」): 시전자는 누르고 Windup 뒤에 가짜 볼트를 쏜다. 서버는 OneWayLatency 늦게 안다.
	 * → 남은 Windup 만큼 기다리고, 이미 지나 버린 시간은 투사체를 그만큼 앞에서 시작(상한 MaxCompensation).
	 */
	GRIMOIRE_API FLagSplit SplitLag(float OneWayLatency, float Windup, float MaxCompensation);

	/**
	 * 서버의 TargetData 검증(치트 방지 최소한): 시선 시작이 폰 근처(MaxOriginError)이고, 조준점이 폰에서 MaxDistance 안이며,
	 * 시선 시작 → 조준점 방향이 서버가 아는 폰 조준 방향과 MaxAngleDeg 안.
	 */
	GRIMOIRE_API bool IsClientAimPlausible(const FVector& ClientViewOrigin, const FVector& ClientAimPoint, const FVector& ServerPawnLocation, const FVector& ServerAimDirection, float MaxOriginError, float MaxAngleDeg, float MaxDistance);
}
