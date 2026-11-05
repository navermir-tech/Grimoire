#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Online/GrimOnlineMath.h"
#include "Online/GrimOnlineSettings.h"
#include "Engine/EngineBaseTypes.h"
#include "Net/Core/Connection/NetEnums.h"

namespace GrimOnlineTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags OnlineTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	bool Parse(const TCHAR* In, FGrimServerAddress& Out)
	{
		return FGrimOnlineMath::ParseAddress(In, 7777, Out);
	}
}

// 주소 읽기: 포트 있음 · 없음 · 이름 · IPv6 · 공백 · 틀린 꼴.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimOnlineAddressTest, "Grimoire.Online.ParseAddress", GrimOnlineTestsLocal::OnlineTestFlags)
bool FGrimOnlineAddressTest::RunTest(const FString& Parameters)
{
	using namespace GrimOnlineTestsLocal;
	FGrimServerAddress A;

	TestTrue(TEXT("IPv4 만"), Parse(TEXT("192.168.0.10"), A));
	TestEqual(TEXT("IPv4 호스트"), A.Host, FString(TEXT("192.168.0.10")));
	TestEqual(TEXT("포트 없으면 기본"), A.Port, 7777);

	TestTrue(TEXT("IPv4:포트"), Parse(TEXT(" 127.0.0.1:17790 "), A));
	TestEqual(TEXT("앞뒤 공백 무시"), A.Host, FString(TEXT("127.0.0.1")));
	TestEqual(TEXT("포트"), A.Port, 17790);

	TestTrue(TEXT("이름:포트"), Parse(TEXT("my-pc.lan:7778"), A));
	TestEqual(TEXT("이름"), A.Host, FString(TEXT("my-pc.lan")));
	TestEqual(TEXT("이름 포트"), A.Port, 7778);

	TestTrue(TEXT("[IPv6]:포트"), Parse(TEXT("[::1]:7000"), A));
	TestEqual(TEXT("IPv6 호스트"), A.Host, FString(TEXT("::1")));
	TestEqual(TEXT("IPv6 포트"), A.Port, 7000);
	TestEqual(TEXT("IPv6 다시 쓰기"), FGrimOnlineMath::FormatAddress(A), FString(TEXT("[::1]:7000")));

	TestTrue(TEXT("대괄호 없는 IPv6"), Parse(TEXT("fe80::1"), A));
	TestEqual(TEXT("대괄호 없는 IPv6 = 기본 포트"), A.Port, 7777);

	FText Error;
	TestFalse(TEXT("빈 주소"), FGrimOnlineMath::ParseAddress(TEXT("   "), 7777, A, &Error));
	TestFalse(TEXT("빈 주소 까닭"), Error.IsEmpty());
	TestFalse(TEXT("포트 0"), Parse(TEXT("1.2.3.4:0"), A));
	TestFalse(TEXT("포트 65536"), Parse(TEXT("1.2.3.4:65536"), A));
	TestFalse(TEXT("포트 글자"), Parse(TEXT("1.2.3.4:77a"), A));
	TestFalse(TEXT(": 뒤 비었음"), Parse(TEXT("1.2.3.4:"), A));
	TestFalse(TEXT("공백 섞인 호스트"), Parse(TEXT("my pc:7777"), A));
	TestFalse(TEXT("URL 옵션 섞임"), Parse(TEXT("1.2.3.4?listen"), A));
	TestFalse(TEXT("] 없음"), Parse(TEXT("[::1:7000"), A));
	TestFalse(TEXT("] 뒤 쓰레기"), Parse(TEXT("[::1]x"), A));

	int32 Port = 0;
	TestTrue(TEXT("포트 칸 비면 기본"), FGrimOnlineMath::ParsePort(TEXT(""), 7777, Port));
	TestEqual(TEXT("포트 칸 기본값"), Port, 7777);
	TestTrue(TEXT("포트 칸 65535"), FGrimOnlineMath::ParsePort(TEXT("65535"), 7777, Port));
	TestFalse(TEXT("포트 칸 음수"), FGrimOnlineMath::ParsePort(TEXT("-1"), 7777, Port));
	return true;
}

// 슬롯 이름: 세이브 시스템과 같은 규칙(영문 · 숫자 · - · _, 64자) + 예약 이름.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimOnlineSlotNameTest, "Grimoire.Online.SlotName", GrimOnlineTestsLocal::OnlineTestFlags)
bool FGrimOnlineSlotNameTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("영문 · 숫자 · _ · -"), FGrimOnlineMath::ValidateSlotName(TEXT("My_World-2")));
	TestFalse(TEXT("빈 이름"), FGrimOnlineMath::ValidateSlotName(TEXT("")));
	TestFalse(TEXT("공백"), FGrimOnlineMath::ValidateSlotName(TEXT("My World")));
	TestFalse(TEXT("한글"), FGrimOnlineMath::ValidateSlotName(TEXT("내세계")));
	TestFalse(TEXT("경로 글자"), FGrimOnlineMath::ValidateSlotName(TEXT("../x")));
	TestFalse(TEXT("65자"), FGrimOnlineMath::ValidateSlotName(FString::ChrN(65, TEXT('a'))));
	TestTrue(TEXT("64자"), FGrimOnlineMath::ValidateSlotName(FString::ChrN(64, TEXT('a'))));
	TestFalse(TEXT("예약 MainMenu(대소문자 무시)"), FGrimOnlineMath::ValidateSlotName(TEXT("mainmenu")));

	TestEqual(TEXT("후보: 공백 → _"), FGrimOnlineMath::SuggestSlotName(TEXT(" My  World! ")), FString(TEXT("My_World")));
	TestEqual(TEXT("후보: 한글은 빠짐"), FGrimOnlineMath::SuggestSlotName(TEXT("내세계")), FString());
	return true;
}

// 표시 이름 다듬기 · URL 옵션 인코딩 왕복(한글).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimOnlinePlayerNameTest, "Grimoire.Online.PlayerName", GrimOnlineTestsLocal::OnlineTestFlags)
bool FGrimOnlinePlayerNameTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("앞뒤 · 연속 공백"), FGrimOnlineMath::SanitizePlayerName(TEXT("  불꽃   마법사 ")), FString(TEXT("불꽃 마법사")));
	TestEqual(TEXT("URL 글자 빼기"), FGrimOnlineMath::SanitizePlayerName(TEXT("a?b=c#d&e/f")), FString(TEXT("abcdef")));
	TestEqual(TEXT("길이"), FGrimOnlineMath::SanitizePlayerName(TEXT("abcdefghij"), 4), FString(TEXT("abcd")));
	TestEqual(TEXT("제어 문자"), FGrimOnlineMath::SanitizePlayerName(TEXT("a\tb\nc")), FString(TEXT("a b c")));

	const FString Name = TEXT("시험 손님_1");
	const FString Encoded = FGrimOnlineMath::EncodeOptionValue(Name);
	TestFalse(TEXT("인코딩에 공백 없음"), Encoded.Contains(TEXT(" ")));
	TestFalse(TEXT("인코딩에 ? 없음"), Encoded.Contains(TEXT("?")));
	TestTrue(TEXT("영문 · _ 는 그대로"), Encoded.EndsWith(TEXT("_1")));
	TestEqual(TEXT("왕복"), FGrimOnlineMath::DecodeOptionValue(Encoded), Name);
	TestEqual(TEXT("잘못된 %XX 는 그대로"), FGrimOnlineMath::DecodeOptionValue(TEXT("a%zzb%4")), FString(TEXT("a%zzb%4")));

	// 서버가 로그인 URL 에서 꺼내기 — 엔진이 붙이는 다른 옵션(Name · GrimPlayerId) 사이에서.
	const FString Login = FString::Printf(TEXT("/Game/Map?Name=DESKTOP-1234?GrimName=%s?GrimPlayerId=ABC"), *Encoded);
	TestEqual(TEXT("로그인 URL 의 GrimName"), FGrimOnlineMath::ParseNameOption(Login), Name);
	TestEqual(TEXT("옵션 없음"), FGrimOnlineMath::ParseNameOption(TEXT("/Game/Map?Name=X")), FString());
	return true;
}

// URL 만들기: 리슨 · 혼자 · 참가.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimOnlineURLTest, "Grimoire.Online.URL", GrimOnlineTestsLocal::OnlineTestFlags)
bool FGrimOnlineURLTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("리슨"), FGrimOnlineMath::MakeHostURL(TEXT("/Game/Grimoire/Maps/L_Tower.L_Tower"), TEXT("World1"), 8, true, TEXT("Host")),
		FString(TEXT("/Game/Grimoire/Maps/L_Tower?listen?SaveSlot=World1?MaxPlayers=8?GrimName=Host")));
	TestEqual(TEXT("혼자(listen 없음, 이름 없음)"), FGrimOnlineMath::MakeHostURL(TEXT("/Game/Grimoire/Maps/L_Tower"), TEXT("Solo"), 1, false, TEXT("")),
		FString(TEXT("/Game/Grimoire/Maps/L_Tower?SaveSlot=Solo?MaxPlayers=1")));

	FGrimServerAddress Address;
	Address.Host = TEXT("127.0.0.1");
	Address.Port = 17790;
	TestEqual(TEXT("참가"), FGrimOnlineMath::MakeJoinURL(Address, TEXT("Guest")), FString(TEXT("127.0.0.1:17790?GrimName=Guest")));
	Address.Host = TEXT("::1");
	TestEqual(TEXT("참가 IPv6"), FGrimOnlineMath::MakeJoinURL(Address, TEXT("")), FString(TEXT("[::1]:17790")));

	TestEqual(TEXT("오브젝트 경로 → 패키지"), FGrimOnlineMath::ToPackagePath(TEXT("/Game/A/B.B")), FString(TEXT("/Game/A/B")));
	TestEqual(TEXT("패키지는 그대로"), FGrimOnlineMath::ToPackagePath(TEXT("/Game/A.x/B")), FString(TEXT("/Game/A.x/B")));

	TestEqual(TEXT("인원 위"), FGrimOnlineMath::ClampMaxPlayers(12, 8), 8);
	TestEqual(TEXT("인원 아래"), FGrimOnlineMath::ClampMaxPlayers(0, 8), 1);
	TestEqual(TEXT("설정 최대 = 8(Q11)"), UGrimOnlineSettings::Get()->MaxPlayersLimit, 8);
	return true;
}

// 실패 · 끊김 메시지(엔진 값 → 한국어).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimOnlineFailureTextTest, "Grimoire.Online.FailureText", GrimOnlineTestsLocal::OnlineTestFlags)
bool FGrimOnlineFailureTextTest::RunTest(const FString& Parameters)
{
	const FString Full = FGrimOnlineMath::DescribeNetworkFailure(ENetworkFailure::FailureReceived, TEXT("Server full."), true).ToString();
	TestTrue(TEXT("가득 참"), Full.Contains(TEXT("가득")));
	TestTrue(TEXT("엔진 글을 덧붙임"), Full.Contains(TEXT("Server full.")));
	// 접속 전에 가득 참은 엔진이 PendingConnectionFailure 로 보낸다(헤드리스 check_join 에서 확인).
	TestTrue(TEXT("가득 참(접속 전 종류)"), FGrimOnlineMath::DescribeNetworkFailure(ENetworkFailure::PendingConnectionFailure, TEXT("Server full."), true).ToString().Contains(TEXT("가득")));
	TestTrue(TEXT("호스트가 닫음"), FGrimOnlineMath::DescribeNetworkFailure(ENetworkFailure::FailureReceived, TEXT("Host closed the connection."), false).ToString().Contains(TEXT("호스트가 서버를 닫았다")));
	const FString Lost = FGrimOnlineMath::DescribeNetworkFailure(ENetworkFailure::ConnectionLost, TEXT(""), false).ToString();
	TestTrue(TEXT("접속 뒤 끊김"), Lost.Contains(TEXT("끊겼다")));
	TestFalse(TEXT("엔진 글 없으면 괄호 없음"), Lost.Contains(TEXT("(엔진")));
	TestTrue(TEXT("접속 전 시간 초과"), FGrimOnlineMath::DescribeNetworkFailure(ENetworkFailure::ConnectionTimeout, TEXT(""), true).ToString().Contains(TEXT("방화벽")));
	TestTrue(TEXT("포트 사용 중"), FGrimOnlineMath::DescribeNetworkFailure(ENetworkFailure::NetDriverListenFailure, TEXT(""), false).ToString().Contains(TEXT("포트")));
	TestTrue(TEXT("버전"), FGrimOnlineMath::DescribeNetworkFailure(ENetworkFailure::OutdatedClient, TEXT(""), true).ToString().Contains(TEXT("버전")));
	TestTrue(TEXT("맵 열기 실패"), FGrimOnlineMath::DescribeTravelFailure(ETravelFailure::LoadMapFailure, TEXT("")).ToString().Contains(TEXT("맵")));
	return true;
}

#endif
