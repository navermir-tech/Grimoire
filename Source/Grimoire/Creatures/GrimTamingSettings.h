#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "Creatures/GrimTamingTypes.h"
#include "GrimTamingSettings.generated.h"

/** 포획 확률 수치(스펙 「길들이기」 — 조건 몫은 더하고, 등급 · 거대 · 서클은 곱한다(Q118)) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCaptureTuning
{
	GENERATED_BODY()

	/** 기본(파크툼을 맞히기만 해도) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture", meta = (ClampMin = "0", ClampMax = "1"))
	float BaseChance = 0.3f;

	/** 약해짐(체력 ≤ 생물 설정 WeakenedHealthRatio) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float WeakenedBonus = 0.2f;

	/** 약한 정도: 문턱 아래로 더 내려갈수록 이만큼까지(체력 0 에 가까울 때) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float WeakenedDepthBonus = 0.15f;

	/** 잠듦(AI 상태 Sleep · 기절) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float AsleepBonus = 0.15f;

	/** 지침(Creature.State.Exhausted) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float ExhaustedBonus = 0.1f;

	/** 먹이를 미리 줌 · 좋아하는 먹이(둘 중 큰 것 하나만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float FedBonus = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float FavoriteFoodBonus = 0.2f;

	/** 먹이 준 기록을 기억하는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture", meta = (Units = "s"))
	float FedMemorySeconds = 120.f;

	/** 속성 일치(파크툼 + 그 생물 속성) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float ElementMatchBonus = 0.25f;

	/** 완드 속성 친화: (그 생물 주속성 위력 배율 − 1) × 이 값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float WandAffinityScale = 0.5f;

	/** 완드 친화 몫 상한 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	float WandAffinityMaxBonus = 0.15f;

	/**
	 * 서클 배율(Q118 — 팰월드 포획 곡선을 빌림). 차 = 내 서클 − 종의 요구 서클.
	 * 차 ≥ 0: min(1 + 차 × CircleSurplusScalePerCircle, CircleSurplusMaxScale) — 요구 서클 이상이면 넉넉하고 조금씩 더 오른다.
	 * −CircleDeficitWindow < 차 < 0: (1 + 차 / CircleDeficitWindow) ^ CircleDeficitExponent — 팰월드 곡선의 아래 반쪽(9제곱)처럼 가파르게 떨어진다.
	 * 차 ≤ −CircleDeficitWindow: 0(정확히) — 시도 불가(CircleTooLow 알림, 굴리지 않는다).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Circle", meta = (ClampMin = "0"))
	float CircleSurplusScalePerCircle = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Circle", meta = (ClampMin = "1"))
	float CircleSurplusMaxScale = 1.3f;

	/** 이만큼 모자라면 확률이 정확히 0. 4 면 요구 7서클 전설은 6서클 아주 낮음 · 5서클 거의 0 · 3서클 이하 0 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Circle", meta = (ClampMin = "1"))
	float CircleDeficitWindow = 4.f;

	/** 모자란 쪽 곡선의 지수(팰월드 9). 클수록 한 단계만 모자라도 크게 떨어진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture|Circle", meta = (ClampMin = "1"))
	float CircleDeficitExponent = 9.f;

	/** 등급 깎기(곱): 확률 × (1 − 값). 소형 · 중형 · 대형 · 전설 — 팰월드의 종별 포획 배율 자리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
	TArray<float> SizeClassPenalty = { 0.f, 0.15f, 0.35f, 0.7f };

	/** 거대 개체(우두머리) 깎기(곱 — Q119): 확률 × (1 − 값). 팰월드 우두머리 ×0.7 과 같다. 파크툼 막시마(뒤)가 잘 듣는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture", meta = (ClampMin = "0", ClampMax = "1"))
	float GiantPenalty = 0.3f;

	/** 최종 상한(하한은 없다 — 서클이 너무 모자라면 정확히 0) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxChance = 0.95f;

	/** 실패하면 화내 덤비는 시간(Creature.State.Enraged — 겁 많은 종도 덤빈다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture", meta = (Units = "s"))
	float EnrageSeconds = 6.f;

	/** 길들이면 체력을 이 비율까지 채운다(0 = 그대로) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture", meta = (ClampMin = "0", ClampMax = "1"))
	float TamedHealToRatio = 0.5f;
};

/** 사건 하나의 기본 변화량 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimLoyaltyEventDelta
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	EGrimLoyaltyEvent Event = EGrimLoyaltyEvent::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float Delta = 0.f;
};

/** 사건 하나에 성격이 곱하는 배율 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimLoyaltyEventScale
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	EGrimLoyaltyEvent Event = EGrimLoyaltyEvent::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float Scale = 1.f;
};

/** 성격(Q27) 하나 — 충성도가 오르내리는 배율. 사건별 배율이 있으면 그것, 없으면 오름 · 내림 배율 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimPersonalityTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	EGrimCreaturePersonality Personality = EGrimCreaturePersonality::None;

	/** 화면 이름(먹보 · 일벌레 …) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	FString DisplayName;

	/** 오르는 사건 모두에 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	float GainScale = 1.f;

	/** 내리는 사건 모두에 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	float LossScale = 1.f;

	/** 사건별(위 둘을 덮는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	TArray<FGrimLoyaltyEventScale> EventScales;

	/** 작업 속도 배율(M5 일꾼 자리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality")
	float WorkSpeedScale = 1.f;
};

/** 충성도 단계 하나의 효과 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimLoyaltyStageTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	EGrimLoyaltyStage Stage = EGrimLoyaltyStage::Normal;

	/** 화면 이름(충직 · 보통 · 불만 · 반항 · 떠남) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	FString DisplayName;

	/** 이 값 이상이면 이 단계(가장 높은 맞는 단계) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float MinLoyalty = 0.f;

	/** 명령(따라오기 · 기다리기 · 타기)을 따를 확률 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (ClampMin = "0", ClampMax = "1"))
	float Obedience = 1.f;

	/** 탑승 속도 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float MountSpeedScale = 1.f;

	/** 작업 속도 배율(M5 일꾼 자리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float WorkSpeedScale = 1.f;

	/** 타려 할 때 거부할 확률 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (ClampMin = "0", ClampMax = "1"))
	float MountRefuseChance = 0.f;

	/** 탄 동안 BuckCheckSeconds 마다 떨어뜨릴 확률 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (ClampMin = "0", ClampMax = "1"))
	float BuckChance = 0.f;
};

/** 충성도 수치(스펙 「충성도」) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimLoyaltyTuning
{
	GENERATED_BODY()

	/** 길들인 직후 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float InitialLoyalty = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	TArray<FGrimLoyaltyEventDelta> EventDeltas;

	/** 높은 단계부터 적어도 된다(계산이 MinLoyalty 로 고른다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	TArray<FGrimLoyaltyStageTuning> Stages;

	/** 서버 충성도 갱신 주기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (ClampMin = "0.5", Units = "s"))
	float TickSeconds = 5.f;

	/** 먹이 없이 이만큼 지나면 굶김, 그 뒤 StarveRepeatSeconds 마다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float StarveSeconds = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float StarveRepeatSeconds = 180.f;

	/** 기다리기 · 잠으로 이만큼 쉬면 쉼 한 번 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float RestSeconds = 120.f;

	/** 이 체력 비율 아래로 이만큼 내버려 두면 다친 채 내버려 둠 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (ClampMin = "0", ClampMax = "1"))
	float InjuredHealthRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float InjuredNeglectSeconds = 180.f;

	/** 치료로 칠 체력 오름(최대 체력 비율) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (ClampMin = "0", ClampMax = "1"))
	float HealMinRatio = 0.1f;

	/** 고단함: 싸움 · 도망 · 탑승 중 초당 쌓임, 쉴 때 초당 풀림, 상한(= 지침) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float ExertionPerSecond = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float ExertionRecoveryPerSecond = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	float ExertionMax = 90.f;

	/** 지쳤는데 계속 쓰면 이만큼마다 혹사 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float OverworkRepeatSeconds = 30.f;

	/** 싸워 이김: 쓰러진 생물이 이 거리 안이면 「함께」 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "cm"))
	float WinTogetherRadius = 2500.f;

	/** 함께 탐험: 지난 탐험 지점에서 주인 곁으로 이만큼 옮겨 가면 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "cm"))
	float ExploreDistance = 6000.f;

	/** 맞는 환경 · 맞지 않는 환경이 이만큼 이어지면 한 번 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float GoodEnvironmentSeconds = 240.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float BadEnvironmentSeconds = 60.f;

	/** 함께 지낸 날: 이만큼(실시간 초 — 하루 길이)마다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float DaySeconds = 1440.f;

	/** 다툼: 다른 생물과 싸움이 시작될 때 한 번, 이만큼 안에서는 다시 세지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float QuarrelRepeatSeconds = 60.f;

	/** 주인 부재: 이 거리 밖에 이만큼 있으면, 그 뒤 AbsentRepeatSeconds 마다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "cm"))
	float AbsentDistance = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float AbsentSeconds = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float AbsentRepeatSeconds = 300.f;

	/** 접속하지 않은 시간: 한 시간마다 주인 부재 한 번, 상한 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty")
	int32 OfflineAbsentMaxEvents = 5;

	/** 기절: 체력이 0 이 되면 죽지 않고 이만큼 쓰러져 있다가 이 비율로 일어난다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float KnockoutSeconds = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (ClampMin = "0.01", ClampMax = "1"))
	float KnockoutReviveRatio = 0.2f;

	/** 탄 동안 반항 단계면 이만큼마다 떨어뜨릴지 굴린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (Units = "s"))
	float BuckCheckSeconds = 8.f;

	/** 최근 까닭 줄 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loyalty", meta = (ClampMin = "1"))
	int32 RecentReasonCount = 3;
};

/** 길들인 생물 명령 · 따라오기 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTamedBehaviorTuning
{
	GENERATED_BODY()

	/** 주인에게서 이만큼 안이면 멈춘다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow", meta = (Units = "cm"))
	float FollowDistance = 350.f;

	/** 이만큼 밖이면 달려온다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow", meta = (Units = "cm"))
	float FollowRunDistance = 900.f;

	/** 이만큼 밖이면 주인 곁으로 옮겨 온다(지형에 걸림 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow", meta = (Units = "cm"))
	float FollowTeleportDistance = 5000.f;

	/** 접속 · 불러오기 때 주인 곁 이 거리에 놓는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow", meta = (Units = "cm"))
	float SummonDistance = 300.f;

	/** F 메뉴를 여는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow", meta = (Units = "cm"))
	float InteractRadius = 400.f;

	/** 서버가 허용하는 거리 여유 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow", meta = (Units = "cm"))
	float ServerReachSlack = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow")
	FKey MenuKey = EKeys::F;

	/** 이름 최대 길이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow", meta = (ClampMin = "1"))
	int32 NicknameMaxLength = 16;
};

/** 탈것 종 하나(안장 · 속도 · 카메라) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimMountProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount")
	FName SpeciesId;

	/** 안장 자리(생물 캡슐 가운데 기준, 크기 1 일 때 cm). 메시에 Saddle 소켓이 생기면 그것을 먼저 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount")
	FVector SaddleOffset = FVector(0.f, 0.f, 120.f);

	/** 탑승 속도 = 종 달리기 속도 × 이 값 × 충성도 단계 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount")
	float SpeedScale = 1.f;

	/** 탄 동안 카메라 붐을 이만큼 늘린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount", meta = (Units = "cm"))
	float CameraArmExtra = 180.f;

	/** 내릴 때 생물 오른쪽으로 이만큼 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount", meta = (Units = "cm"))
	float DismountSideOffset = 160.f;
};

USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimMountTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount")
	TArray<FGrimMountProfile> Profiles;

	/** 탄 사람 입력이 이만큼 오지 않으면 멈춘다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount", meta = (Units = "s"))
	float InputTimeoutSeconds = 0.3f;

	/** 원격 클라이언트가 입력을 보내는 간격 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount", meta = (Units = "s"))
	float InputSendInterval = 0.033f;

	/** 물 위 달리기: 물 볼륨 수면 위 이 높이를 지킨다(캡슐 바닥 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount", meta = (Units = "cm"))
	float WaterRunSurfaceOffset = 2.f;

	/** 이 이름의 액터 · 컴포넌트 태그가 붙은 표면을 「물 표면」으로 본다(물 볼륨이 없는 맵의 자리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mount")
	FName WaterSurfaceTag = TEXT("Grim.Water");
};

/** 종 하나의 먹이(포획 · 충성도) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureFoodRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	FName SpeciesId;

	/** 좋아하는 먹이(아이템 id) — 두 배 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	TArray<FName> FavoriteItems;

	/** 먹는 아이템 묶음(아이템 표 Groups) — 비우면 먹이 습성 기본(DietGroups) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	TArray<FName> AcceptedGroups;
};

/** 먹이 습성 → 먹는 묶음(종 규칙이 없을 때) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimDietGroups
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	EGrimCreatureDiet Diet = EGrimCreatureDiet::Herbivore;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	TArray<FName> Groups;
};

/**
 * 길들이기 · 충성도 · 탑승 수치 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimTamingSettings] (docs/systems/creatures 「길들이기」).
 * 종의 요구 서클 · 잘 듣는 파크툼 · 탈것 여부는 종 정의(DA_Creature_*), 파크툼 주문 수치는 DA_Spell_Pactum*(Scripts/Magic/make_pactum_spells.py).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Taming"))
class GRIMOIRE_API UGrimTamingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimTamingSettings* Get() { return GetDefault<UGrimTamingSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, Category = "Capture")
	FGrimCaptureTuning Capture;

	UPROPERTY(Config, EditAnywhere, Category = "Loyalty")
	FGrimLoyaltyTuning Loyalty;

	/** 성격(Q27) — 개체 기록의 Personality 로 고른다. 없으면 배율 1 */
	UPROPERTY(Config, EditAnywhere, Category = "Loyalty")
	TArray<FGrimPersonalityTuning> Personalities;

	UPROPERTY(Config, EditAnywhere, Category = "Behavior")
	FGrimTamedBehaviorTuning Behavior;

	UPROPERTY(Config, EditAnywhere, Category = "Mount")
	FGrimMountTuning Mount;

	UPROPERTY(Config, EditAnywhere, Category = "Food")
	TArray<FGrimCreatureFoodRule> FoodRules;

	UPROPERTY(Config, EditAnywhere, Category = "Food")
	TArray<FGrimDietGroups> DietGroups;

	/** 숙련도 추가 효과(ProficiencyExtraEffect.EffectTag)가 이 태그면 포획 확률 몫 */
	UPROPERTY(Config, EditAnywhere, Category = "Capture", meta = (Categories = "Taming"))
	FGameplayTag CaptureBonusEffectTag;

	/** 길들이면 퀘스트 이벤트 앞 이름 — 뒤에 크기 분류(Small …) */
	UPROPERTY(Config, EditAnywhere, Category = "Quest")
	FString TamedEventPrefix = TEXT("Creature.Tamed.");

	const FGrimPersonalityTuning* FindPersonality(EGrimCreaturePersonality Personality) const;
	const FGrimLoyaltyStageTuning* FindStage(EGrimLoyaltyStage Stage) const;
	const FGrimMountProfile* FindMountProfile(FName SpeciesId) const;
	const FGrimCreatureFoodRule* FindFoodRule(FName SpeciesId) const;
	/** 종이 먹는 묶음(종 규칙 → 먹이 습성 기본) */
	TArray<FName> GetAcceptedGroups(FName SpeciesId, EGrimCreatureDiet Diet) const;
	float GetEventDelta(EGrimLoyaltyEvent Event) const;
};
