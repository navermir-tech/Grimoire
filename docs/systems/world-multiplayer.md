# 월드 · 멀티플레이 · 세이브

> 스펙: [세계 · 환경](../spec/world) (R12), [멀티플레이](../spec/multiplayer) (R13)

## 오픈월드

- **World Partition** 한 레벨(`L_World`). 셀 스트리밍 + 서버는 플레이어 주변만 로드(플레이어 위치 기반 스트리밍 소스).
- **데이터 레이어**: 바이옴 장식, 사냥꾼 거점(이벤트로 켜고 끔), 계절(▢).
- 랜드스케이프 + 지역 기후 볼륨. 지역 간 경계는 자연 지형(산맥 · 강)으로 숨긴다.
- 대형 사물(나무 · 바위)은 HLOD. 상호작용 사물(양초 · 모닥불)은 액터 + Dormancy.

## 인스턴스 던전

### 결정: 같은 서버 프로세스 안에서 파티별 **레벨 인스턴스**를 띄운다

팰월드처럼 서버는 하나다(R13). UE 에 "인스턴스 던전" 기본 기능은 없으므로:

1. 던전 레벨(`L_Dungeon_*`)은 World Partition 밖의 **독립 레벨**로 만든다.
2. 입장: 서버가 `ULevelStreamingDynamic::LoadLevelInstance(DungeonLevel, Transform = 먼 좌표 슬롯 N)` 으로 **월드 밖 빈 슬롯**(예: X 축으로 100 km 간격)에 로드. 슬롯은 `UGrimDungeonSubsystem` 이 관리(파티 ID ↔ 슬롯).
3. 파티원을 슬롯 좌표로 텔레포트. 다른 파티는 거리가 멀어 **Net Relevancy** 로 자동 격리(보이지도 복제되지도 않음).
4. 던전 상태(보스 · 보상)는 인스턴스 액터들 안에 있고, 마지막 파티원이 나가면 `Unload` → 사라진다(스펙대로 진행 상태 비유지).
5. 환경: 인스턴스는 자체 하늘 / 조명(실내) — 날씨 서브시스템은 던전 슬롯 영역을 `Indoor` 로 취급.

- 제약 · 대응: 큰 월드 좌표(UE5 는 LWC 로 안전), 슬롯 수 상한(동시 파티 수 = 최대 인원이면 충분), 던전 레벨은 World Partition 미사용(단일 레벨, 크기 제한).
- 대안으로 검토했으나 보류: 별도 서버 프로세스(복잡), 서버 트래블(오픈월드가 끊김).

## 멀티플레이

### 서버 형태

| 형태 | 언제 | 빌드 |
|---|---|---|
| **리슨 서버**(호스트 플레이) | M3 부터 기본 | 게임 클라이언트 빌드에서 `?listen` 으로 맵 열기 |
| 전용 서버 | M6 이후 | `GrimoireServer.Target.cs`(Type = Server) 추가. 런처 엔진은 전용 서버 타깃을 빌드할 수 없으므로 그때 엔진 소스 빌드 전환 검토 |
| 싱글 | 항상 | 혼자 있는 리슨 서버(코드 경로 동일) |

### 접속

- M3: `OnlineSubsystemNull` — 메인 메뉴에서 "서버 열기 / IP 로 참가 / LAN 찾기"(`FindSessions`).
- Steam 출시 시 `OnlineSubsystemSteam`(또는 EOS)로 교체 — 세션 인터페이스는 같게 추상화(`UGrimSessionSubsystem`).
- 플레이어 식별: `FUniqueNetId` → 저장용 `FGuid PlayerId` 매핑(Null 서브시스템에서는 로컬 생성 GUID 를 설정 파일에 보관).

### 서버 권위 규칙 (모든 시스템 공통)

1. 상태를 바꾸는 함수는 `HasAuthority()` 에서만 실행. 클라이언트는 `Server_*`(Reliable) 요청 또는 GAS 어빌리티.
2. 복제는 **상태**(`Replicated` / `OnRep_`). 연출은 상태 변화에서 파생. 일회성 임팩트만 `NetMulticast` Unreliable 또는 GameplayCue.
3. GAS 예측: 이동 · 시전 시작 · 로컬 이펙트는 예측, 피해 · 상태 변화 · 상호작용 결과는 서버 확정 후 복제.
4. 관련성 · 휴면: 사물은 `DORM_DormantAll`, 변화 시 `FlushNetDormancy`. 생물 · 플레이어는 거리 기반 관련성(`NetCullDistanceSquared` 종별).
5. 대역폭 예산: 플레이어당 약 100 KB/s 상한을 목표. 상호작용 상태는 FastArray, 인벤토리는 소유자 전용.
6. 치트 방지는 최소(협동 게임): 서버 검증만, 난독화 없음.

### 서버 옵션 `FGrimServerSettings` (GameState 복제)

하루 길이, PvP(Q13), 최대 인원(Q11), 하드코어(Q7), 생존 배율, 경험치 배율, 번식 속도. 호스트 UI 와 `ServerSettings.ini`.

## 세이브 `UGrimSaveSubsystem` (서버 전용)

- **세계 세이브** `WorldSave_<슬롯>`: 시간 · 날씨, 거점 전부(조각 · 시설 · 인벤토리 · 밭), 길들인 생물, 야생 개체군 레코드, 사물의 속성 상태(지속형만 — 켜진 양초 등), 퀘스트 · 세력 상태. 던전은 저장하지 않는다.
- **플레이어 세이브** `Player_<PlayerId>`: 서클 · 배운 마법 · 어트리뷰트 · 인벤토리 · 장비 · 위치 · 소유 거점 ID.
- 방식: 각 저장 대상이 `IGrimSaveable` 구현(`FGuid SaveId`, `Serialize(FArchive&)`), 서브시스템이 수집 → `FMemoryWriter` + `SaveGame` 플래그 직렬화 → 백그라운드 스레드에서 파일 쓰기(`Saved/SaveGames/<슬롯>/`). 자동 저장 5분 + 종료 시 + 플레이어 퇴장 시(해당 플레이어).
- 로드: 서버 시작 → 세계 복원(액터 재스폰은 ID 로 데이터 에셋 조회) → 플레이어 접속 시 `Player_<Id>` 적용. 버전 필드 + 마이그레이션 함수.
- Q12(서버 간 캐릭터 이동)는 플레이어 세이브를 서버 밖으로 내보내는 기능 — 미결.

## M1 범위

싱글 리슨 서버 구조(PIE 2 클라이언트로 복제 검증), World Partition 작은 지도 1개, 세이브는 플레이어 위치 · 마법만.
