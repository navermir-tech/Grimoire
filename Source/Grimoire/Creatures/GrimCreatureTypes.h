#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "GrimCreatureTypes.generated.h"

class APlayerState;

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimCreatures, Log, All);

/** 크기 분류(스펙 「분류」). 튜토리얼 이벤트 Creature.Killed.<분류> 의 이름이기도 하다. */
UENUM(BlueprintType)
enum class EGrimCreatureSizeClass : uint8
{
	Small,
	Medium,
	Large,
	Legendary
};

UENUM(BlueprintType)
enum class EGrimCreatureDiet : uint8
{
	Herbivore,
	Carnivore,
	Manavore
};

/** 성격의 큰 갈래(AI 반응). 개체 성격(Q27)과는 다르다 — 종 전체의 기질. */
UENUM(BlueprintType)
enum class EGrimCreatureTemperament : uint8
{
	/** 겁 많음 — 위협이 보이면 도망 */
	Timid,
	/** 온순 — 맞으면 맞서 싸운다 */
	Docile,
	/** 공격적 — 가까이 오면 먼저 덤빈다 */
	Aggressive
};

/** 활동 시간. 밖이면 잠든다(bHideOutsideActiveTime 이면 사라진다). */
UENUM(BlueprintType)
enum class EGrimCreatureActivity : uint8
{
	Diurnal,
	Nocturnal,
	/** 낮 + 해 질 녘(설정 DuskStartHour~DuskEndHour) */
	DayAndDusk,
	Always
};

UENUM(BlueprintType)
enum class EGrimCreatureSex : uint8
{
	Female,
	Male
};

/** 성별 부위: 이 부위가 어느 성별에 붙나 */
UENUM(BlueprintType)
enum class EGrimCreatureSexFilter : uint8
{
	Any,
	FemaleOnly,
	MaleOnly
};

/** 임시 모델 부위 모양 — 엔진 기본 도형(/Engine/BasicShapes). */
UENUM(BlueprintType)
enum class EGrimCreatureShape : uint8
{
	Cube,
	Sphere,
	Cylinder,
	Cone
};

/** 개체 성격(Q27) — 충성도 배율은 UGrimTamingSettings::Personalities(갈래 T). 값을 더하면 굴림 결과가 바뀐다(RollPersonality 가 수로 고른다). */
UENUM(BlueprintType)
enum class EGrimCreaturePersonality : uint8
{
	None,
	Glutton,
	Workaholic,
	Coward,
	Brawler,
	Sleepyhead
};

/** 물 위 이동(켈피 「물 위 달리기」 자리) */
UENUM(BlueprintType)
enum class EGrimCreatureWaterMovement : uint8
{
	/** 물에 들어가면 보통 헤엄 */
	Swim,
	/** 물 볼륨 위를 달린다(부력으로 수면에 뜬다) */
	RunOnSurface
};

/** AI 상태. 복제된다(연출 · 덤프용). */
UENUM(BlueprintType)
enum class EGrimCreatureAIState : uint8
{
	Idle,
	Wander,
	Graze,
	Flee,
	Fight,
	Sleep,
	Hunt,
	Dead,
	// ── 길들인 생물(갈래 T) ──
	/** 주인을 따라간다 */
	Follow,
	/** 기다리기 명령 — 제자리 */
	Stay,
	/** 누군가 타고 있다(탄 사람 입력으로 움직임) */
	Ridden
};

/** 색 영역 하나(등 · 배 · 무늬 · 뿔 · 눈 …). 팔레트에서 색 하나가 정해진다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureColorRegion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	FName RegionName;

	/** 기본 팔레트(4~8색) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	TArray<FLinearColor> BasePalette;

	/** 비어 있지 않으면 수컷은 이 팔레트(수컷 깃은 화려하게) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	TArray<FLinearColor> MalePalette;

	/** 비어 있지 않으면 암컷은 이 팔레트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	TArray<FLinearColor> FemalePalette;

	/** 끄면 흔들림 없이 팔레트 색 그대로(눈처럼 빛나는 영역) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color")
	bool bJitter = true;
};

/** 임시 모델 부위 하나 — 기본 도형 + 색 영역 + 성별 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureBodyPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FName PartName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	EGrimCreatureShape Shape = EGrimCreatureShape::Cube;

	/** 캡슐 가운데 기준(크기 1 일 때, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FRotator Rotation = FRotator::ZeroRotator;

	/** 기본 도형(100cm) 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	FVector Scale = FVector(0.3f);

	/** 색 영역 번호(ColorRegions 의 인덱스) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0", ClampMax = "5"))
	int32 ColorRegion = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	EGrimCreatureSexFilter Sex = EGrimCreatureSexFilter::Any;

	/** 그림자 걸음 중에도 보인다(불씨 같은 눈) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	bool bVisibleWhenHidden = false;
};

/** 전리품 한 줄 — 아이템 id(갈래 I 의 아이템 표) · 개수 · 확률 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 MaxCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 1.f;
};

/** 생물 공격 하나 = 속성 하나(주속성 기술 · 부속성 기술이 따로) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureAttack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName AttackName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (Categories = "Element"))
	FGameplayTag Element;

	/** 기본 피해(개체 힘 배율 × 상성이 곱해진다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Damage = 5.f;

	/** 대상 캡슐 겉면까지의 거리(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0", Units = "cm"))
	float Range = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 2.f;

	/** 넉백 속도(cm/s, 0 = 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Knockback = 0.f;

	/** 대상에 주는 속성 자극 세기(0 = 자극 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float StimulusIntensity = 0.5f;

	/** 여러 공격이 함께 준비됐을 때 고를 가중치 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float Weight = 1.f;
};

/** 색 영역 하나의 개체 색 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureRegionColor
{
	GENERATED_BODY()

	/** 팔레트 번호(돌연변이면 변이 팔레트의 번호) */
	UPROPERTY(BlueprintReadOnly, Category = "Color")
	uint8 PaletteIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Color")
	bool bMutant = false;

	/** 흔들림까지 들어간 최종 색 */
	UPROPERTY(BlueprintReadOnly, Category = "Color")
	FLinearColor Color = FLinearColor::White;
};

/**
 * 개체 기록(R23) — 생성 · 부화 때 서버가 굴리고 복제 · 접힌 기록 · (다음 묶음) 세이브가 이것 그대로를 쓴다.
 * 길들이기(OwnerId · Loyalty · Nickname)는 UGrimCreatureTamingComponent 가 바꾸고 주인 세이브(Creatures.Owned)가 이것 그대로를 쓴다(갈래 T).
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	FName SpeciesId;

	/** 굴린 씨앗 — 같은 씨앗 + 같은 종 = 같은 개체. 네트 확인에서 서버 · 클라이언트 개체를 잇는 이름으로도 쓴다 */
	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	int32 Seed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	EGrimCreatureSex Sex = EGrimCreatureSex::Female;

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	TArray<FGrimCreatureRegionColor> Colors;

	/** 0.9~1.1, 거대 1.3~1.5 */
	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	float SizeScale = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	bool bGiant = false;

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	bool bMutant = false;

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	EGrimCreaturePersonality Personality = EGrimCreaturePersonality::None;

	/** 체력 · 힘 배율(크기 ±5% · 거대 배율) */
	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	float StatScale = 1.f;

	// ── 길들이기(갈래 T) ──

	/** 길들인 주인의 PlayerId(세이브 Player_<PlayerId> 와 같은 값 — FGrimPlayerIdentity). 야생 = 빈 문자열 */
	UPROPERTY(BlueprintReadOnly, Category = "Creature|Taming")
	FString OwnerId;

	/** 0~100, 길들인 직후 50(스펙). 야생은 0 */
	UPROPERTY(BlueprintReadOnly, Category = "Creature|Taming")
	float Loyalty = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Creature|Taming")
	FString Nickname;

	bool IsValid() const { return !SpeciesId.IsNone(); }
	bool IsTamed() const { return !OwnerId.IsEmpty(); }
};

/** 떨어진 전리품 하나(굴린 결과) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureLootDrop
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Count = 0;
};

/** 죽음 기록 — 복제되어 클라이언트도 같은 전리품 · 죽인 이를 안다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureDeath
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	bool bDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	TArray<FGrimCreatureLootDrop> Drops;

	/** 죽인 플레이어(없으면 다른 생물 · 환경) */
	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	TObjectPtr<APlayerState> KillerPlayer;

	/** 죽인 것의 이름(로그 · 덤프) */
	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	FString KillerName;
};

/** 접힌 개체 — 플레이어가 멀어 액터 없이 남은 기록(위치 · 유전 · 체력) */
USTRUCT()
struct GRIMOIRE_API FGrimFoldedCreature
{
	GENERATED_BODY()

	UPROPERTY()
	FGrimCreatureInstance Instance;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	float HealthRatio = 1.f;

	UPROPERTY()
	int32 HerdId = 0;
};

/** 먹이 관계 표의 한 줄(Content/Grimoire/Data/CreatureEcology.csv). 행 이름은 아무거나. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureEcologyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	FString Note;

	/** 쫓는 종 id */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	FName Predator;

	/** 쫓기는 종 id */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	FName Prey;

	/** 클수록 먼저 고른다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	int32 Priority = 0;
};
