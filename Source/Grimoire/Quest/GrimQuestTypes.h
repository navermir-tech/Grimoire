#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GrimQuestTypes.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimQuest, Log, All);

/** 퀘스트 종류. 시험(Trial)은 마법탑 NPC 가 시작하고, 통과하면 서클이 오른다. */
UENUM(BlueprintType)
enum class EGrimQuestKind : uint8
{
	Main,
	Trial,
	Side
};

/**
 * 목표 종류. 「조회」형은 서버가 주기적으로 세계를 보고(사물 상태 · 마법서 · 서클 · 위치), 「신호」형은 일이 생길 때 센다(대화 · 밤 넘김 · 표적 명중).
 * 한 번 채운 목표는 그 단계가 끝날 때까지 채워진 채로 남는다(양초를 켰다가 꺼져도 「켰다」).
 */
UENUM(BlueprintType)
enum class EGrimQuestObjectiveType : uint8
{
	/** 조회: 태그 Target 을 가진 사물 중 상태 State 의 세기가 Compare Value 인 것이 Count 개 이상 */
	ElementalState,
	/** 조회: 마법 Target(정의 이름 DA_Spell_<Target>)을 배웠다 */
	KnowSpell,
	/** 조회: 마법 Target 의 숙련도 등급 번호가 Value 이상(D=0, C=1 …) */
	ProficiencyGrade,
	/** 조회: 서클이 Value 이상 */
	ReachCircle,
	/** 조회: 태그 Target 을 가진 표지 액터에서 Value cm 안(0 이면 설정 기본 반경) */
	ReachLocation,
	/** 신호: NPC Target 과 대화를 마쳤다 */
	TalkToNpc,
	/** 신호: 잠든 채로 밤 건너뛰기를 맞았다 */
	SleepThroughNight,
	/** 신호: 태그 Target 을 가진 시험 표적을 마법으로 맞혔다(Count 번) */
	HitTarget,
	/** 조회: 퀘스트 Target 을 끝냈다 */
	CompleteQuest,
	/**
	 * 신호: 다른 시스템이 알리는 이벤트 Target(Count 번) — UGrimQuestComponent::NotifyQuestEvent.
	 * 아직 없는 시스템(생물 · 전리품 · 길들이기 M3, 작물 · 요리 · 거점 M4)이 생기면 그쪽이 부른다. 그 전엔 콘솔(Grim.Quest.At … Event <Id>).
	 */
	Event,
	/** 신호: 태그 Target 을 가진 과녁을 부쉈다(AGrimTrialTarget::HitsToBreak) */
	BreakTarget,
	/** 조회: 어트리뷰트 Target(이름 — Fatigue · Hunger …)이 Compare Value */
	Attribute
};

UENUM(BlueprintType)
enum class EGrimQuestCompare : uint8
{
	AtLeast,
	AtMost
};

/** Content/Grimoire/Data/Quests.csv 의 한 행. 행 이름 = 퀘스트 ID. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimQuestRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FString Summary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	EGrimQuestKind Kind = EGrimQuestKind::Main;

	/** 플레이어가 들어오면 바로 시작(설정 AutoStartMaps 의 맵에서만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	bool AutoStart = false;

	/** 시험이면 통과했을 때 오르는 서클(0 = 시험 아님). 지금 서클 + 1 일 때만 시작할 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 TrialCircle = 0;

	/** 시험을 시작하려면 승급 조건(UGrimProgressionSettings — 배운 마법 수 · 등급 합)도 맞아야 하나 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	bool RequireCircleConditions = true;

	/** 시작 조건: 이 퀘스트를 끝냈거나, 진행 중이고 단계가 RequiresStep 이상 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName RequiresQuest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 RequiresStep = 0;

	/** 끝냈을 때 경험치(레벨 갈래의 AwardExperience — 통합 때 잇는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 RewardXP = 0;

	/** 끝냈을 때 스킬북 하나를 쓴 것처럼(UseSkillbook) — 마법 ID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName RewardSpell;

	/** 끝나면 이어서 시작할 퀘스트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName NextQuest;
};

/** QuestSteps.csv 의 한 행(행 이름은 자유 — 보통 <퀘스트>_<단계>). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimQuestStepRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName Quest;

	/** 0 부터 빈틈 없이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 Step = 0;

	/** 추적 창에 뜨는 단계 설명 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FString Text;

	/** 0 보다 크면 이 초 안에 끝내야 한다 — 넘기면 퀘스트 실패(시험) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	float TimeLimitSeconds = 0.f;

	/** 단계가 시작될 때 시각을 옮긴다. 비우면 그대로. "Dusk+0.2"(해넘이 + 0.2시간) · "Dawn" · "21.5"(시) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FString TimeOfDayOnStart;

	/** 이 태그의 시험 소품: 단계가 시작되면 처음 상태로 되돌리고(사물) 켠다(표적), 끝나면 끈다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName PropsTag;

	/**
	 * 단계가 시작될 때 플레이어에게 주는 것을 | 로. 인벤토리(M4) 전이라 월드에 놓는다.
	 *  Skillbook:<마법 ID> — 스킬북(다가가면 읽힌다), Bedroll — 침낭, Item:<ID> — 아이템(M4 전엔 기록만).
	 *  뒤에 @<태그> 를 붙이면 그 태그 액터 곁에, 없으면 플레이어 앞에.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FString Give;
};

/** QuestObjectives.csv 의 한 행. 한 단계의 목표를 모두 채우면 다음 단계. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimQuestObjectiveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName Quest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 Step = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	EGrimQuestObjectiveType Type = EGrimQuestObjectiveType::TalkToNpc;

	/** 액터 태그 · 마법 ID · NPC ID · 퀘스트 ID (종류마다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName Target;

	/** ElementalState 의 상태 태그(State.Burning …) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName State;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	EGrimQuestCompare Compare = EGrimQuestCompare::AtLeast;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	float Value = 0.f;

	/** 몇 개(사물 수 · 명중 수 · 대화 수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 Count = 1;

	/** 추적 창의 목표 한 줄(비우면 단계 설명만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FString Text;
};

struct GRIMOIRE_API FGrimQuestObjectiveDef
{
	EGrimQuestObjectiveType Type = EGrimQuestObjectiveType::TalkToNpc;
	FName Target;
	FName State;
	EGrimQuestCompare Compare = EGrimQuestCompare::AtLeast;
	float Value = 0.f;
	int32 Count = 1;
	FString Text;

	/** 신호형(대화 · 밤 넘김 · 명중) — 조회하지 않고 신호로만 센다 */
	bool IsSignalDriven() const;
};

/** 단계 시작 때 주는 것 하나(FGrimQuestStepRow::Give) */
struct GRIMOIRE_API FGrimQuestGive
{
	enum class EKind : uint8 { Skillbook, Bedroll, Item };
	EKind Kind = EKind::Item;
	/** 마법 ID · 아이템 ID(침낭은 비움) */
	FName Id;
	/** 이 태그 액터 곁에(None = 플레이어 앞) */
	FName AtTag;

	/** "Skillbook:Ember@Tag" 하나를 읽는다. 못 읽으면 false. */
	static bool Parse(const FString& Text, FGrimQuestGive& Out);
};

struct GRIMOIRE_API FGrimQuestStepDef
{
	FString Text;
	float TimeLimitSeconds = 0.f;
	FString TimeOfDayOnStart;
	FName PropsTag;
	TArray<FGrimQuestGive> Gives;
	TArray<FGrimQuestObjectiveDef> Objectives;
};

struct GRIMOIRE_API FGrimQuestDef
{
	FName Id;
	FString Title;
	FString Summary;
	EGrimQuestKind Kind = EGrimQuestKind::Main;
	bool bAutoStart = false;
	int32 TrialCircle = 0;
	bool bRequireCircleConditions = true;
	FName RequiresQuest;
	int32 RequiresStep = 0;
	int32 RewardXP = 0;
	FName RewardSpell;
	FName NextQuest;
	TArray<FGrimQuestStepDef> Steps;

	bool IsTrial() const { return TrialCircle > 0; }
};

/** 세 표(퀘스트 · 단계 · 목표)를 묶은 것. 표 검사도 여기서(Grimoire.Quest.Table). */
struct GRIMOIRE_API FGrimQuestDatabase
{
	/** 표 순서대로 */
	TArray<FGrimQuestDef> Quests;

	const FGrimQuestDef* Find(FName QuestId) const;

	/** 통과하면 TargetCircle 이 되는 시험 */
	const FGrimQuestDef* FindTrialForCircle(int32 TargetCircle) const;

	/** 표 셋을 읽어 Out 을 채운다. 문제(빈 단계 · 모르는 퀘스트 · 잘못된 태그 …)는 OutProblems — 하나라도 있으면 false. */
	static bool Build(const UDataTable* QuestTable, const UDataTable* StepTable, const UDataTable* ObjectiveTable, FGrimQuestDatabase& Out, TArray<FString>& OutProblems);

	/** CSV 문자열 셋으로 Build(테스트 · 로더 공용). */
	static bool BuildFromCSV(const FString& QuestsCSV, const FString& StepsCSV, const FString& ObjectivesCSV, FGrimQuestDatabase& Out, TArray<FString>& OutProblems);

	/**
	 * 단계의 TimeOfDayOnStart 를 하루 비율(0~1)로. 비었으면 false(옮기지 않음).
	 * "Dusk" · "Dawn" · "Noon" · "Midnight" 에 +/− 시간(시)을 붙이거나, 시(0~24) 숫자 하나. 못 읽으면 bOutValid = false.
	 */
	static bool ParseTimeOfDay(const FString& Text, float DayRatio, float& OutTimeOfDay, bool& bOutValid);
};

/** 진행 중인 퀘스트 하나(소유 클라이언트에 복제 · 플레이어 세이브). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimQuestProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FName QuestId;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	int32 Step = 0;

	/** 지금 단계의 목표별 센 값(목표 순서) */
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TArray<int32> Counts;

	/** 시간 제한 단계가 끝나는 서버 시각(GameState 서버 월드 초). 0 = 제한 없음 */
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	float Deadline = 0.f;
};

USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimQuestLog
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TArray<FGrimQuestProgress> Active;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TArray<FName> Completed;

	const FGrimQuestProgress* FindActive(FName QuestId) const;
	bool IsActive(FName QuestId) const { return FindActive(QuestId) != nullptr; }
	bool IsCompleted(FName QuestId) const { return Completed.Contains(QuestId); }
};

/** 진행이 바뀐 일(모델이 내놓고, 컴포넌트가 보상 · 소품 · 시각 · 알림으로 처리한다). */
enum class EGrimQuestChangeType : uint8
{
	QuestStarted,
	StepStarted,
	/** 단계가 끝났다(채움 · 실패 모두) — 소품을 끈다 */
	StepEnded,
	QuestCompleted,
	QuestFailed
};

struct GRIMOIRE_API FGrimQuestChange
{
	EGrimQuestChangeType Type = EGrimQuestChangeType::QuestStarted;
	FName QuestId;
	int32 Step = 0;
};

/** 신호형 목표에 들어오는 일 하나 */
struct GRIMOIRE_API FGrimQuestSignal
{
	EGrimQuestObjectiveType Type = EGrimQuestObjectiveType::TalkToNpc;
	FName Target;
	int32 Amount = 1;
};

/** 조회형 목표의 지금 값(0~Count)을 세계에서 읽는 쪽. 서버는 월드로, 테스트는 가짜로. */
class GRIMOIRE_API IGrimQuestFacts
{
public:
	virtual ~IGrimQuestFacts() = default;
	virtual int32 QueryObjective(const FGrimQuestObjectiveDef& Objective) const = 0;
};

enum class EGrimQuestStartResult : uint8
{
	Started,
	UnknownQuest,
	AlreadyActive,
	AlreadyCompleted,
	PrerequisiteMissing
};

/**
 * 퀘스트 진행 순수 계산 — 액터 · 월드를 모른다(Grimoire.Quest.Model).
 * 시작 → (신호 · 조회로 목표 채움) → 단계 넘김 → 끝(보상은 부른 쪽) → NextQuest 시작. 시간 제한을 넘기면 실패(진행에서 빠짐, 다시 시작할 수 있다).
 */
struct GRIMOIRE_API FGrimQuestModel
{
	static EGrimQuestStartResult CanStart(const FGrimQuestDatabase& Database, const FGrimQuestLog& Log, FName QuestId);

	static EGrimQuestStartResult StartQuest(const FGrimQuestDatabase& Database, FGrimQuestLog& Log, FName QuestId, float Now, TArray<FGrimQuestChange>& OutChanges);

	/** 신호형 목표를 센다(지금 단계에서 종류 · Target 이 맞는 것). 하나라도 셌으면 true. 단계 넘김은 Advance 가. */
	static bool ApplySignal(const FGrimQuestDatabase& Database, FGrimQuestLog& Log, const FGrimQuestSignal& Signal);

	/** 조회형 목표를 다시 읽고(Facts 가 없으면 건너뜀), 시간 제한을 보고, 채운 단계를 넘긴다. 무엇이든 바뀌었으면 true. */
	static bool Advance(const FGrimQuestDatabase& Database, FGrimQuestLog& Log, const IGrimQuestFacts* Facts, float Now, TArray<FGrimQuestChange>& OutChanges);

	/** 퀘스트를 실패로 뺀다(시간 초과 · 포기). */
	static bool FailQuest(const FGrimQuestDatabase& Database, FGrimQuestLog& Log, FName QuestId, TArray<FGrimQuestChange>& OutChanges);

	static bool Compare(EGrimQuestCompare Compare, float Value, float Threshold);
	static bool IsObjectiveMet(const FGrimQuestObjectiveDef& Objective, int32 Count) { return Count >= FMath::Max(1, Objective.Count); }
	static bool IsStepMet(const FGrimQuestStepDef& Step, const TArray<int32>& Counts);
};
