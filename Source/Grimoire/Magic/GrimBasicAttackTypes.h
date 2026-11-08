#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GrimBasicAttackTypes.generated.h"

class UGrimSpellDefinition;

/**
 * 기본 공격(R26) 수치 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimBasicAttackSettings].
 * 마법 자체(피해 · 속도 · 사거리 · 색)는 정의 DA_Spell_BasicAttack(Scripts/Magic/make_basic_attack.py), 여기는 연타 리듬 · 강타 · 주먹질.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Basic Attack"))
class GRIMOIRE_API UGrimBasicAttackSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimBasicAttackSettings* Get() { return GetDefault<UGrimBasicAttackSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/**
	 * 기본 공격 정의 — 시작 지급이 아니라 튜토리얼 첫 단계의 「기본 공격 스킬북」(AGrimSkillbook::TeachesSpell)으로 배운다.
	 * 여기는 그 정의가 어느 것인지 알려 주는 자리(튜토리얼 · 테스트 · 디버그가 읽는다). 배우면 그리모어 맨 앞, 잊지 않음.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Spell")
	TSoftObjectPtr<UGrimSpellDefinition> BasicAttackSpell;

	/** 연타 리듬: 한 발 사이 최소 간격(초). 쿨다운이 아니라 휘두르는 속도 — 이보다 빨리 누르면 무시 */
	UPROPERTY(Config, EditAnywhere, Category = "Rhythm", meta = (ClampMin = "0", Units = "s"))
	float AttackInterval = 0.3f;

	/** 서버가 원격 클라이언트의 간격을 볼 때 봐주는 시간(네트워크 흔들림) */
	UPROPERTY(Config, EditAnywhere, Category = "Rhythm", meta = (ClampMin = "0", Units = "s"))
	float ServerIntervalTolerance = 0.08f;

	/**
	 * 연타 창(Q120): **마지막 발(또는 사이에 쓴 슬롯 마법 · 회피 · 방패)부터** 이 시간 안에 다시 쏘면 이어진다 — 쏘는 동안은 끊기지 않는다.
	 * 넘기면 1부터(전투를 벗어나면 자연히 처음으로). ComboLength 번째가 강타.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Combo", meta = (ClampMin = "0", Units = "s"))
	float ComboWindow = 1.f;

	/** 몇 번째가 강타인가(호그와트 = 4) */
	UPROPERTY(Config, EditAnywhere, Category = "Combo", meta = (ClampMin = "1"))
	int32 ComboLength = 4;

	/** 강타 피해 배율(정의의 Damage ×) */
	UPROPERTY(Config, EditAnywhere, Category = "Combo", meta = (ClampMin = "0"))
	float HeavyDamageMultiplier = 3.f;

	/** 강타 밀어냄: 캐릭터는 이 속도로 날아간다(수평 · 위, cm/s) */
	UPROPERTY(Config, EditAnywhere, Category = "Combo", meta = (ClampMin = "0", Units = "cm/s"))
	float HeavyKnockbackSpeed = 900.f;

	UPROPERTY(Config, EditAnywhere, Category = "Combo", meta = (ClampMin = "0", Units = "cm/s"))
	float HeavyKnockbackUpSpeed = 250.f;

	/** 강타 밀어냄: 물리로 움직이는 가벼운 사물에 주는 속도 변화(질량 무관, cm/s) */
	UPROPERTY(Config, EditAnywhere, Category = "Combo", meta = (ClampMin = "0", Units = "cm/s"))
	float HeavyPhysicsSpeed = 600.f;

	/** 강타 볼트 크기 배율(보이는 것만) */
	UPROPERTY(Config, EditAnywhere, Category = "Combo", meta = (ClampMin = "0.1"))
	float HeavyBoltScale = 1.6f;

	/** 주먹질(UGrimFistAbility — 기본 공격을 배우기 전 · 완드 없음 · 부러진 완드, Q4, 임시): 앞으로 짧은 구 판정 */
	UPROPERTY(Config, EditAnywhere, Category = "Fist", meta = (ClampMin = "0", Units = "cm"))
	float FistRange = 130.f;

	UPROPERTY(Config, EditAnywhere, Category = "Fist", meta = (ClampMin = "1", Units = "cm"))
	float FistRadius = 35.f;

	UPROPERTY(Config, EditAnywhere, Category = "Fist", meta = (ClampMin = "0"))
	float FistDamage = 3.f;

	/** 주먹질이 사물에 주는 무속성 「충격」 자극 세기 */
	UPROPERTY(Config, EditAnywhere, Category = "Fist", meta = (ClampMin = "0"))
	float FistStimulusIntensity = 0.5f;
};

/** 연타 상태(기기마다 — 서버는 서버 시계, 시전자는 자기 시계로 같은 규칙을 센다) */
struct FGrimBasicAttackCombo
{
	/** 마지막으로 친 시각(연타 리듬 — AttackInterval) */
	double LastAttackTime = -1.0e9;
	/** 연타 창을 잰 마지막 시각 = 마지막 발, 또는 그 뒤 창 안에서 쓴 슬롯 마법 · 회피 · 방패(Q120 — 끊지도 세지도 않는다) */
	double LastActivityTime = -1.0e9;
	/** 마지막으로 친 것이 몇 번째였나(0 = 아직 없음) */
	int32 Step = 0;
};

/** 기본 공격의 순수 계산 (자동화 테스트 Grimoire.Magic.BasicAttack.*) */
struct GRIMOIRE_API FGrimBasicAttackMath
{
	/** 연타 리듬: 마지막 발에서 Interval(− Tolerance) 이 지났나 */
	static bool IsReady(double Now, double LastAttackTime, float Interval, float Tolerance = 0.f);

	/** 이번 발이 몇 번째인가(1..Length): 처음 · 앞이 강타였음 · 창(마지막 발 · 활동부터 Window)이 지났음 → 1, 아니면 앞 + 1 */
	static int32 NextStep(const FGrimBasicAttackCombo& Combo, double Now, float Window, int32 Length);

	/** 연타 중인가(이어 친 발이 있고 창이 아직 안 지났다) */
	static bool IsChainAlive(const FGrimBasicAttackCombo& Combo, double Now, float Window);

	/**
	 * 사이에 다른 행동(슬롯 마법 · 회피 · 방패)을 했다 — 연타가 살아 있으면 창만 다시 잰다(번째는 그대로).
	 * 이미 끊긴 연타는 되살리지 않는다. 살렸으면 true.
	 */
	static bool KeepAlive(FGrimBasicAttackCombo& Combo, double Now, float Window);

	/** 친다: 상태를 고치고 이번 번째를 돌려준다 */
	static int32 Register(FGrimBasicAttackCombo& Combo, double Now, float Window, int32 Length);

	/** 강타인가(Length 번째) */
	static bool IsHeavy(int32 Step, int32 Length);

	/** HUD 에 보일 번째: 창 안이면 마지막 번째, 지났으면 0(강타 뒤는 그대로 보였다가 창이 지나면 0) */
	static int32 VisibleStep(const FGrimBasicAttackCombo& Combo, double Now, float Window);

	/** 피해 배율: 강타면 HeavyMultiplier, 아니면 1 */
	static float DamageMultiplier(int32 Step, int32 Length, float HeavyMultiplier);

	/** 밀어냄 속도: 날아온 방향의 수평 성분 × Speed + 위로 UpSpeed. 방향이 수직이면 수평은 0 */
	static FVector KnockbackVelocity(const FVector& TravelDirection, float Speed, float UpSpeed);
};
