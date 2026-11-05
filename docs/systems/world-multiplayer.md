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
- 플레이어 식별: 저장용 `PlayerId` — 아래 [세이브 · PlayerId](#playerid).

### 서버 권위 규칙 (모든 시스템 공통)

1. 상태를 바꾸는 함수는 `HasAuthority()` 에서만 실행. 클라이언트는 `Server_*`(Reliable) 요청 또는 GAS 어빌리티.
2. 복제는 **상태**(`Replicated` / `OnRep_`). 연출은 상태 변화에서 파생. 일회성 임팩트만 `NetMulticast` Unreliable 또는 GameplayCue.
3. GAS 예측: 이동 · 시전 시작 · 로컬 이펙트는 예측, 피해 · 상태 변화 · 상호작용 결과는 서버 확정 후 복제.
4. 관련성 · 휴면: 사물은 `DORM_DormantAll`, 변화 시 `FlushNetDormancy`. 생물 · 플레이어는 거리 기반 관련성(`NetCullDistanceSquared` 종별).
5. 대역폭 예산: 플레이어당 약 100 KB/s 상한을 목표. 상호작용 상태는 FastArray, 인벤토리는 소유자 전용.
6. 치트 방지는 최소(협동 게임): 서버 검증만, 난독화 없음.

### 서버 옵션 `FGrimServerSettings` (GameState 복제)

하루 길이, 최대 인원(8, Q11), 하드코어(Q7), 생존 배율, 경험치 배율, 번식 속도. 호스트 UI 와 `ServerSettings.ini`.

## 세이브 `UGrimSaveSubsystem` (서버 전용)

세이브 v1(M2 갈래 V). 코드는 `Source/Grimoire/Save/`, 공통 인터페이스는 `Core/GrimSaveTypes.h` 의 `IGrimSaveParticipant`.

### 무엇이 어디에

- **세계 세이브** `WorldSave_<슬롯>`: 월드 서브시스템 · 레벨에 놓인 액터(와 컴포넌트)의 `World` 몫, 게임 중 스폰된 저장 대상 액터(클래스 · 이름 · 위치 + 그 몫), 게임 중 없어진 놓인 액터 이름. 던전은 저장하지 않는다.
- **플레이어 세이브** `Player_<PlayerId>`: PlayerState · 그 컴포넌트 · 폰의 `Player` 몫 + 세이브 시스템이 맡는 `Player.Attributes` · `Player.Transform`. 캐릭터는 세계 슬롯에 묶인다 — 다른 서버로 서클 · 배운 마법 · 그리모어 기록만 옮기는 서버 옵션은 나중(Q12).
- 나중 묶음의 몫(거점 · 길들인 생물 · 개체군 · 인벤토리 · 레벨 · 성장 트리 · 퀘스트 · 세력)은 그 시스템이 인터페이스를 구현하면 저절로 모인다.

| SaveId | 범위 | 구현 | v1 내용 |
|---|---|---|---|
| `Magic.Spellbook` | 플레이어 | `UGrimSpellbookComponent` | 배운 마법(에셋 경로 · 숙련도 XP, 배운 순서) · 선택 · 단축 슬롯 · 보이는 세트. 읽을 때 서클은 보지 않는다 |
| `Magic.Progression` | 플레이어 | `UGrimProgressionComponent` | 서클(Circle 기본값) |
| `Survival.Vitals` | 플레이어 | `UGrimSurvivalComponent` | 최대 HP · HP · 배고픔 · 체온 · 피곤함 · 질병 저항(기본값), 배탈 · 감기 남은 초, 젖은 채 추운 누적, 부활 지점(위치). 잠 · 쓰러짐은 저장 안 함 |
| `Player.Attributes` | 플레이어 | `UGrimPlayerAttributesSaveParticipant`(Save) | 설정 `PlayerAttributes` 의 어트리뷰트 기본값 — 지금은 마나 현재값 |
| `Player.Transform` | 플레이어 | `UGrimPlayerTransformSaveParticipant`(Save) | 폰 위치 · 바라보는 방향(설정 `bRestorePlayerTransform`). 퇴장 때는 폰이 Logout 보다 먼저 사라지므로 세이브 시스템이 매 틱 기억한 마지막 위치 |
| `Environment.TimeOfDay` | 세계 | `UGrimTimeOfDaySubsystem` | 누적 일수(하루 길이는 서버 옵션이라 안 함) |
| `Environment.Weather` | 세계 | `UGrimWeatherSubsystem` | 지역마다 id · 날씨 · 그 날씨가 된 지 몇 초. 읽으면 블렌드 없이 |
| `Elemental.<액터>` | 세계 | `UGrimElementalStateComponent` | 지속 상태(남은 시간 없음 — 켜진 양초 · 탄 풀)와 전파. 젖음 같은 시간 제한 상태는 안 함 |
| `Skillbook.<액터>` | 세계 | `AGrimSkillbook` | 가르칠 마법 · 쓰였나 |
| `Bed.<액터>` | 세계 | `AGrimBed` | 침낭인가 |

### 모으는 규칙

- 플레이어: PlayerState 와 컴포넌트, 폰과 컴포넌트 중 `GetSaveScope() == Player` + Save 의 두 도우미.
- 세계: 월드 서브시스템 + **놓인 액터**(`IsNetStartupActor` 또는 `RF_WasLoaded` — 이름이 레벨 안에서 고정)와 그 컴포넌트의 `World` 몫. 스폰된 액터는 **액터 스스로** 참여자일 때만: 프레임워크 액터(AInfo · 폰 · 컨트롤러 — 게임 모드가 만든다)는 그냥 레코드, 그 밖(침대 · 스킬북)은 「스폰 액터」로 저장해 불러올 때 같은 이름 · 클래스 · 위치로 다시 스폰하고 `FinishSpawning` 전에 몫을 읽힌다. 컴포넌트만 참여자인 스폰 액터(플레이어 폰의 속성 상태)는 저장하지 않는다.
- 놓인 참여자 액터가 게임 중 없어지면(`OnActorDestroyed` — 집은 스킬북) 이름을 「없앤 액터」에 적고 그 몫은 다시 쓰지 않는다. 불러오면 그 액터를 다시 없앤다.

### 레코드 · 버전

- 파일 = `UGrimSaveGame`(USaveGame) — 형식 버전 · 슬롯 · PlayerId · 저장 시각 · 게임 버전 · 맵 · 저장 횟수 + 참여자별 `FGrimSaveRecord { SaveId, Version, Bytes }` 배열. 바이트는 참여자가 `FArchive` 에 직접 쓴 것(에셋은 경로 문자열).
- **모르는 레코드는 버리지 않는다**: 다시 쓸 때 지난 파일의 레코드 순서를 지키며 지금 참여자가 쓴 것으로만 바꾸고, 참여자가 없는 레코드(다른 버전 · 아직 없는 시스템 · 스트리밍으로 안 올라온 액터)는 그대로 둔다. 새 id 는 끝에(`FGrimSaveRecordMath::Merge`).
- 버전: 읽을 때 저장 버전 ≤ 코드 버전이면 `ReadSaveData(Ar, 저장 버전)` — 옛 형식은 참여자가 옮긴다. 저장 버전이 더 높으면 읽지 않고, 다시 쓸 때도 그 레코드를 덮지 않는다(옛 실행 파일이 새 데이터를 지우지 않게). 0 이하는 잘못된 레코드.
- 읽는 순서: 설정 `PlayerLoadOrder` · `WorldLoadOrder` 에 적은 id 가 먼저(서클 → 생존 → 마법서 → 어트리뷰트 → 위치), 나머지는 찾은 순서.

### 파일 · 백업

- 위치: `Saved/SaveGames/Grimoire/<슬롯>/WorldSave_<슬롯>.sav` · `Player_<PlayerId>.sav`(설정 `SaveDirectory`). 슬롯 · PlayerId 는 영문 · 숫자 · `-` · `_` 만(64자).
- 형식: 봉투(매직 `GRMS` · 봉투 버전 · 페이로드 크기 · CRC32, 16바이트) + `UGameplayStatics::SaveGameToMemory` 결과.
- 쓰기: `<파일>.tmp` 에 쓰고 → 지금 파일이 온전하면 `<파일>.bak` 으로(직전 파일 하나 보관, 설정 `bKeepBackup`) — 깨진 파일은 좋은 백업을 밀어내지 않고 버린다 → tmp 를 제자리로. 지금은 게임 스레드에서 바로 쓴다(파일이 몇 KB) — 거점 · 개체군이 커지면 백그라운드 쓰기로.
- 읽기: 본 파일이 깨졌으면(봉투 · CRC · 역직렬화 실패) `.bak`. 둘 다 깨졌으면 지우지 않고 `.corrupt-<시각>` 으로 옆에 치우고 새로 시작한다.

### 언제

- **불러오기**: 세계는 월드 `BeginPlay` 다음 틱(놓인 액터의 초기 상태 뒤 — 월드 서브시스템의 `OnWorldBeginPlay` 는 액터 BeginPlay 보다 먼저라서). 플레이어는 폰이 붙고 ASC 가 그 폰으로 초기화된 뒤 `PlayerLoadDelayTicks`(2) 틱 — 마법서의 시작 마법 · 생존 초기값이 먼저 들어간 위에 덮는다.
- **저장**: 자동(`AutoSaveIntervalSeconds` 300초, 실제 시간) · 맵을 떠날 때와 종료(`FWorldDelegates::OnWorldBeginTearDown` — `quit` 도 이 길) · 플레이어 퇴장(`FGameModeEvents::GameModeLogoutEvent`, 그 플레이어만) · 콘솔 `Grim.Save.Now`. 불러오기 전에는 저장하지 않는다(기본 상태가 세이브를 덮지 않게).
- **슬롯**: URL 옵션 `?SaveSlot=<이름>` → 없으면 `DefaultSlot`(Default). PIE 는 `PIESlot` 을 채워야 저장한다(기본 비움 — 에디터 시험이 세이브에 섞이지 않게). 무인 실행(`-unattended` — 헤드리스 확인 스크립트 · 자동화 테스트)은 `?SaveSlot=` 이 있을 때만 저장한다(다른 스크립트가 지난 실행을 이어받지 않게). `-NoGrimSave` 로 끈다.

### 복제

- 읽기는 서버에서 각 시스템의 보통 함수로 상태를 바꾸므로 평소 복제 길을 탄다: 어트리뷰트(ASC) · 마법서 속성 · 숙련도 FastArray · 시간/날씨 복제 액터(`ForceNetUpdate`).
- 사물(`DORM_DormantAll`)의 속성 상태는 `SetIntensity` · `Remove` 가 바꾸기 **전에** `FlushNetDormancy` 한다(`FGrimElementalStateArray::NotifyPreChange` — 바꾼 뒤에 깨우면 변화가 기준에 묻힌다). 게임 중 `Grim.Save.Load` 로 꺼진 양초를 다시 켜도 원격 클라이언트에 보인다(`check_save.ps1` C3).
- 마법서는 읽기 전에 소유 클라이언트에 `Client_SaveRestored`(Reliable — 속성 복제보다 먼저 간다)를 보내 곧 올 숙련도 변화를 「등급 오름」으로 알리지 않게 한다.

### PlayerId

- OnlineSubsystemNull 의 UniqueNetId 는 첫 인스턴스 · 에디터 밖일 때만 「호스트 이름-로그인 id」로 고정되고, PIE · 같은 PC 의 두 번째 인스턴스에서는 실행마다 새 GUID 다(엔진 `OnlineIdentityNull.cpp` `GenerateRandomUserId`). 그래서 저장 키로 쓸 수 없다.
- 대신 각 PC 가 GUID 를 한 번 만들어 `GameUserSettings.ini` `[/Script/Grimoire.GrimLocalPlayer] PlayerId` 에 보관하고, 접속할 때 로그인 URL 옵션 `GrimPlayerId=<id>` 로 보낸다 — `UGrimLocalPlayer::GetGameLoginOptions`(`DefaultEngine.ini` `LocalPlayerClassName`). 엔진이 `NMT_Login` 에 붙이고 서버는 `UNetConnection::RequestURL` 에서 읽는다.
- 서버의 결정 순서(`FGrimPlayerIdentity::ResolveServerSide`): 로컬(호스트) = 자기 id → 원격 = 로그인 옵션 → 없으면 UniqueNetId(`Net-…`) → 그것도 없으면 이름(`Name-…`). 명령줄 `-GrimPlayerId=<id>` 가 보관 값보다 앞선다(시험 · 같은 PC 의 여러 인스턴스). PIE 둘째 창부터는 `-PIE<n>` 이 붙는다. 같은 id 가 이미 접속해 있으면 `_2` 를 붙여 서로 덮지 않게 한다.
- 협동 게임이라 id 위조는 막지 않는다(치트 방지 최소 — 서버 권위 규칙 6). Steam · EOS 로 바꾸면 고정 UniqueNetId 를 먼저 쓴다.

### 콘솔 (서버 · 호스트 프로세스)

| 명령 | 하는 일 |
|---|---|
| `Grim.Save.Now [지연초]` | 세계 + 접속한 플레이어 저장 |
| `Grim.Save.Load [지연초]` | 디스크에서 세계 + 접속한 플레이어를 다시 읽어 적용(저장 뒤 새로 생긴 것은 지우지 않음) |
| `Grim.Save.Dump [지연초]` | 슬롯 · 레코드(보존 여부) · 스폰/없앤 액터 · 플레이어 · 파일 상태 |
| `Grim.Save.Delete [슬롯]` | 슬롯 폴더를 지운다(지금 슬롯이면 끌 때 지금 상태가 새로 저장된다) |
| `Grim.Save.At <초> <명령…>` | (개발) 그 프로세스가 준비(놀이 시작 + 로컬 폰)된 때부터 몇 초 뒤 명령 하나 — 헤드리스 시동 시간이 들쭉날쭉해도 순서가 지켜진다 |
| `Grim.Save.AtLogins <N> <초> <명령…>` | (개발) 서버: 원격 접속이 N 번에 이른 때부터 몇 초 뒤 |
| `Grim.Save.Quit [지연초]` | (개발) `quit` — 종료 저장 경로 시험 |
| `Grim.Save.SetAttr <이름> <값> [지연초]` | (개발) 호스트 어트리뷰트 기본값 |

- 확인: 자동화 `Grimoire.Save.*`(병합 · 버전 · 보존 · 읽는 순서 · id 다듬기 · 봉투 · 왕복 · 백업 · 속성 상태 · 모으기), 헤드리스 `Scripts/Save/check_save.ps1`(저장 → 끄기 → 다시 켜기, 2인 다시 접속, 게임 중 불러오기 복제).
- 남은 일: World Partition 스트리밍으로 나중에 올라오는 액터에 몫 읽히기(지금은 시작 때 있는 액터만 — 안 올라온 액터의 레코드는 보존만 된다), 백그라운드 쓰기, 에셋을 경로 대신 Asset Manager ID 로, 서버 간 캐릭터 옮기기(Q12).

## M1 범위

싱글 리슨 서버 구조(PIE 2 클라이언트로 복제 검증), World Partition 작은 지도 1개, 세이브는 플레이어 위치 · 마법만(M2 세이브 v1 이 위 표대로 넓혔다).
