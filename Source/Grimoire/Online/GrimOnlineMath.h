#pragma once

#include "CoreMinimal.h"

/** 서버 주소(호스트 + 포트). 호스트는 IPv4 · 이름 · IPv6(대괄호 없이 저장). */
struct GRIMOIRE_API FGrimServerAddress
{
	FString Host;
	int32 Port = 0;

	bool IsIPv6() const { return Host.Contains(TEXT(":")); }
};

/**
 * 접속 · 메뉴의 순수 계산 (docs/systems/world-multiplayer 「세션」). 엔진 상태를 보지 않아 자동화 테스트 Grimoire.Online.* 가 바로 부른다.
 */
struct GRIMOIRE_API FGrimOnlineMath
{
	/** URL 옵션 이름 — 표시 이름(클라이언트 → 서버 로그인 URL) */
	static const TCHAR* NameOptionName;

	/**
	 * 「주소[:포트]」를 읽는다. 앞뒤 공백 무시. 받는 꼴: 1.2.3.4 · 1.2.3.4:7777 · host-name.lan:7777 · [::1]:7777 · ::1(포트 없음).
	 * 포트가 없으면 DefaultPort. 포트는 1~65535 숫자만. 틀리면 false 와 까닭(OutError).
	 */
	static bool ParseAddress(const FString& In, int32 DefaultPort, FGrimServerAddress& Out, FText* OutError = nullptr);

	/** 포트 칸 글(비면 DefaultPort). 1~65535 숫자만. */
	static bool ParsePort(const FString& In, int32 DefaultPort, int32& OutPort, FText* OutError = nullptr);

	/** 「호스트:포트」(IPv6 는 [호스트]:포트) */
	static FString FormatAddress(const FGrimServerAddress& Address);

	/** 세이브 슬롯 이름 검사 — 세이브 시스템과 같은 규칙: 영문 · 숫자 · - · _ 만, 1~64자. 예약 이름(MainMenu 등)도 막는다. */
	static bool ValidateSlotName(const FString& In, FText* OutError = nullptr);

	/** 입력을 슬롯 이름 후보로 다듬는다(공백 → _, 그 밖의 허용 안 되는 글자는 뺌, 64자). 결과가 비면 빈 문자열. */
	static FString SuggestSlotName(const FString& In);

	/** 표시 이름 다듬기: 앞뒤 공백 · 제어 문자 · URL 에 못 쓰는 글자(?=#&/\"') 빼고, 연속 공백 하나로, MaxLen 글자. 한글 그대로. */
	static FString SanitizePlayerName(const FString& In, int32 MaxLen = 20);

	/** URL 옵션 값 인코딩: 영문 · 숫자 · - · _ · . 밖의 글자는 UTF-8 바이트별 %XX. */
	static FString EncodeOptionValue(const FString& In);

	/** EncodeOptionValue 의 반대(잘못된 %XX 는 그대로 둔다). */
	static FString DecodeOptionValue(const FString& In);

	/** 최대 인원 1~Limit(Q11 — 8) */
	static int32 ClampMaxPlayers(int32 Requested, int32 Limit);

	/**
	 * 맵을 여는 URL: <맵>[?listen]?SaveSlot=<슬롯>?MaxPlayers=<N>[?GrimName=<이름>]. 맵은 /Game/... 패키지 경로(또는 오브젝트 경로 — 점 뒤는 뺀다).
	 * 단독(bListen=false)도 슬롯을 붙인다(세이브는 서버 = 혼자인 그 프로세스).
	 */
	static FString MakeHostURL(const FString& MapPath, const FString& Slot, int32 MaxPlayers, bool bListen, const FString& PlayerName);

	/** 접속 URL: <호스트:포트>[?GrimName=<이름>] — 콘솔 open <ip>:<port> 와 같은 길(SetClientTravel). */
	static FString MakeJoinURL(const FGrimServerAddress& Address, const FString& PlayerName);

	/** 로그인 URL(옵션 포함)에서 표시 이름을 꺼낸다(디코딩 · 다듬은 값). 없으면 빈 문자열. */
	static FString ParseNameOption(const FString& RequestURL, int32 MaxLen = 20);

	/** 오브젝트 경로 /Game/A/B.B → 패키지 경로 /Game/A/B (이미 패키지면 그대로) */
	static FString ToPackagePath(const FString& MapPath);

	/** 실패 · 끊김 까닭(엔진 ENetworkFailure 값 · 문자열) → 사람이 읽을 한국어. bWhileConnecting = 아직 들어가기 전. */
	static FText DescribeNetworkFailure(int32 FailureType, const FString& EngineError, bool bWhileConnecting);

	/** 엔진 ETravelFailure 값 → 한국어 */
	static FText DescribeTravelFailure(int32 FailureType, const FString& EngineError);
};
