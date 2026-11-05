#include "Online/GrimOnlineMath.h"
#include "Engine/EngineBaseTypes.h"
#include "Net/Core/Connection/NetEnums.h"

#define LOCTEXT_NAMESPACE "GrimOnline"

const TCHAR* FGrimOnlineMath::NameOptionName = TEXT("GrimName");

namespace GrimOnlineMathLocal // 유니티 빌드 이름 충돌 방지
{
	/** 메뉴 · 시스템이 쓰는 이름 — 플레이어가 슬롯으로 고르지 못한다 */
	const TCHAR* ReservedSlots[] = { TEXT("MainMenu"), TEXT("Menu") };

	bool IsAsciiAlnum(TCHAR C)
	{
		return (C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('A') && C <= TEXT('Z')) || (C >= TEXT('0') && C <= TEXT('9'));
	}

	bool IsHexDigit(TCHAR C)
	{
		return (C >= TEXT('0') && C <= TEXT('9')) || (C >= TEXT('a') && C <= TEXT('f')) || (C >= TEXT('A') && C <= TEXT('F'));
	}

	int32 HexValue(TCHAR C)
	{
		if (C >= TEXT('0') && C <= TEXT('9')) return C - TEXT('0');
		if (C >= TEXT('a') && C <= TEXT('f')) return 10 + C - TEXT('a');
		if (C >= TEXT('A') && C <= TEXT('F')) return 10 + C - TEXT('A');
		return -1;
	}

	bool SetError(FText* OutError, const FText& Text)
	{
		if (OutError)
		{
			*OutError = Text;
		}
		return false;
	}

	bool IsValidHostName(const FString& Host)
	{
		if (Host.IsEmpty() || Host.Len() > 253 || Host.StartsWith(TEXT(".")) || Host.EndsWith(TEXT(".")) || Host.StartsWith(TEXT("-")))
		{
			return false;
		}
		for (const TCHAR C : Host)
		{
			if (!IsAsciiAlnum(C) && C != TEXT('.') && C != TEXT('-'))
			{
				return false;
			}
		}
		return true;
	}

	bool IsValidIPv6(const FString& Host)
	{
		int32 Colons = 0;
		for (const TCHAR C : Host)
		{
			if (C == TEXT(':'))
			{
				++Colons;
			}
			else if (!IsHexDigit(C) && C != TEXT('.'))
			{
				return false;
			}
		}
		return Colons >= 2 && Host.Len() <= 45;
	}
}

bool FGrimOnlineMath::ParsePort(const FString& In, int32 DefaultPort, int32& OutPort, FText* OutError)
{
	using namespace GrimOnlineMathLocal;
	const FString Text = In.TrimStartAndEnd();
	if (Text.IsEmpty())
	{
		OutPort = DefaultPort;
		return DefaultPort >= 1 && DefaultPort <= 65535 ? true : SetError(OutError, LOCTEXT("PortDefaultBad", "기본 포트 설정이 잘못됐다"));
	}
	if (Text.Len() > 5)
	{
		return SetError(OutError, LOCTEXT("PortRange", "포트는 1~65535 사이 숫자다"));
	}
	int32 Value = 0;
	for (const TCHAR C : Text)
	{
		if (C < TEXT('0') || C > TEXT('9'))
		{
			return SetError(OutError, LOCTEXT("PortDigits", "포트는 숫자만 쓴다"));
		}
		Value = Value * 10 + (C - TEXT('0'));
	}
	if (Value < 1 || Value > 65535)
	{
		return SetError(OutError, LOCTEXT("PortRange", "포트는 1~65535 사이 숫자다"));
	}
	OutPort = Value;
	return true;
}

bool FGrimOnlineMath::ParseAddress(const FString& In, int32 DefaultPort, FGrimServerAddress& Out, FText* OutError)
{
	using namespace GrimOnlineMathLocal;
	const FString Text = In.TrimStartAndEnd();
	if (Text.IsEmpty())
	{
		return SetError(OutError, LOCTEXT("AddressEmpty", "주소를 적어 주세요(예: 192.168.0.10 또는 192.168.0.10:7777)"));
	}

	FString Host;
	FString PortText;
	if (Text.StartsWith(TEXT("[")))
	{
		int32 Close = INDEX_NONE;
		if (!Text.FindChar(TEXT(']'), Close))
		{
			return SetError(OutError, LOCTEXT("AddressBracket", "IPv6 주소의 ] 가 없다"));
		}
		Host = Text.Mid(1, Close - 1);
		const FString Rest = Text.Mid(Close + 1);
		if (!Rest.IsEmpty())
		{
			if (!Rest.StartsWith(TEXT(":")))
			{
				return SetError(OutError, LOCTEXT("AddressAfterBracket", "] 뒤에는 :포트 만 올 수 있다"));
			}
			PortText = Rest.Mid(1);
			if (PortText.IsEmpty())
			{
				return SetError(OutError, LOCTEXT("AddressPortEmpty", ": 뒤에 포트가 없다"));
			}
		}
		if (!IsValidIPv6(Host))
		{
			return SetError(OutError, LOCTEXT("AddressBadIPv6", "IPv6 주소가 올바르지 않다"));
		}
	}
	else
	{
		int32 Colons = 0;
		for (const TCHAR C : Text)
		{
			Colons += C == TEXT(':') ? 1 : 0;
		}
		if (Colons >= 2)
		{
			// 대괄호 없는 IPv6 — 포트는 따로 받는다.
			Host = Text;
			if (!IsValidIPv6(Host))
			{
				return SetError(OutError, LOCTEXT("AddressBadIPv6", "IPv6 주소가 올바르지 않다"));
			}
		}
		else
		{
			if (Colons == 1)
			{
				Text.Split(TEXT(":"), &Host, &PortText);
				if (PortText.IsEmpty())
				{
					return SetError(OutError, LOCTEXT("AddressPortEmpty", ": 뒤에 포트가 없다"));
				}
			}
			else
			{
				Host = Text;
			}
			if (!IsValidHostName(Host))
			{
				return SetError(OutError, LOCTEXT("AddressBadHost", "주소에는 영문 · 숫자 · 점 · - 만 쓴다(예: 192.168.0.10)"));
			}
		}
	}

	int32 Port = 0;
	if (!ParsePort(PortText, DefaultPort, Port, OutError))
	{
		return false;
	}
	Out.Host = Host;
	Out.Port = Port;
	return true;
}

FString FGrimOnlineMath::FormatAddress(const FGrimServerAddress& Address)
{
	return Address.IsIPv6()
		? FString::Printf(TEXT("[%s]:%d"), *Address.Host, Address.Port)
		: FString::Printf(TEXT("%s:%d"), *Address.Host, Address.Port);
}

bool FGrimOnlineMath::ValidateSlotName(const FString& In, FText* OutError)
{
	using namespace GrimOnlineMathLocal;
	if (In.IsEmpty())
	{
		return SetError(OutError, LOCTEXT("SlotEmpty", "슬롯 이름을 적어 주세요"));
	}
	if (In.Len() > 64)
	{
		return SetError(OutError, LOCTEXT("SlotLong", "슬롯 이름은 64자까지다"));
	}
	for (const TCHAR C : In)
	{
		if (!IsAsciiAlnum(C) && C != TEXT('-') && C != TEXT('_'))
		{
			return SetError(OutError, LOCTEXT("SlotChars", "슬롯 이름에는 영문 · 숫자 · - · _ 만 쓴다(폴더 이름이 된다)"));
		}
	}
	for (const TCHAR* Reserved : ReservedSlots)
	{
		if (In.Equals(Reserved, ESearchCase::IgnoreCase))
		{
			return SetError(OutError, LOCTEXT("SlotReserved", "그 이름은 게임이 쓴다 — 다른 이름을 골라 주세요"));
		}
	}
	return true;
}

FString FGrimOnlineMath::SuggestSlotName(const FString& In)
{
	using namespace GrimOnlineMathLocal;
	FString Out;
	for (const TCHAR C : In.TrimStartAndEnd())
	{
		if (Out.Len() >= 64)
		{
			break;
		}
		if (IsAsciiAlnum(C) || C == TEXT('-') || C == TEXT('_'))
		{
			Out.AppendChar(C);
		}
		else if (C == TEXT(' ') && !Out.EndsWith(TEXT("_")))
		{
			Out.AppendChar(TEXT('_'));
		}
	}
	return Out;
}

FString FGrimOnlineMath::SanitizePlayerName(const FString& In, int32 MaxLen)
{
	FString Out;
	bool bPendingSpace = false;
	for (const TCHAR C : In)
	{
		// 탭 · 줄바꿈도 공백 하나로. 그 밖의 제어 문자와 URL 에 못 쓰는 글자는 뺀다.
		if (FChar::IsWhitespace(C))
		{
			bPendingSpace = !Out.IsEmpty();
			continue;
		}
		if (C < 0x20 || C == 0x7F || FCString::Strchr(TEXT("?=#&/\\\"'<>|%"), C) != nullptr)
		{
			continue;
		}
		if (bPendingSpace)
		{
			if (Out.Len() + 1 >= MaxLen)
			{
				break;
			}
			Out.AppendChar(TEXT(' '));
			bPendingSpace = false;
		}
		if (Out.Len() >= MaxLen)
		{
			break;
		}
		Out.AppendChar(C);
	}
	return Out;
}

FString FGrimOnlineMath::EncodeOptionValue(const FString& In)
{
	using namespace GrimOnlineMathLocal;
	FString Out;
	const FTCHARToUTF8 Utf8(*In);
	const uint8* Bytes = reinterpret_cast<const uint8*>(Utf8.Get());
	for (int32 Index = 0; Index < Utf8.Length(); ++Index)
	{
		const uint8 Byte = Bytes[Index];
		const TCHAR C = static_cast<TCHAR>(Byte);
		if (Byte < 0x80 && (IsAsciiAlnum(C) || C == TEXT('-') || C == TEXT('_') || C == TEXT('.')))
		{
			Out.AppendChar(C);
		}
		else
		{
			Out += FString::Printf(TEXT("%%%02X"), Byte);
		}
	}
	return Out;
}

FString FGrimOnlineMath::DecodeOptionValue(const FString& In)
{
	using namespace GrimOnlineMathLocal;
	TArray<uint8> Bytes;
	Bytes.Reserve(In.Len());
	FString Out;
	auto FlushBytes = [&Bytes, &Out]()
	{
		if (Bytes.Num() > 0)
		{
			Bytes.Add(0);
			Out += UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()));
			Bytes.Reset();
		}
	};
	for (int32 Index = 0; Index < In.Len(); ++Index)
	{
		const TCHAR C = In[Index];
		if (C == TEXT('%') && Index + 2 < In.Len() && HexValue(In[Index + 1]) >= 0 && HexValue(In[Index + 2]) >= 0)
		{
			Bytes.Add(static_cast<uint8>(HexValue(In[Index + 1]) * 16 + HexValue(In[Index + 2])));
			Index += 2;
			continue;
		}
		FlushBytes();
		Out.AppendChar(C);
	}
	FlushBytes();
	return Out;
}

int32 FGrimOnlineMath::ClampMaxPlayers(int32 Requested, int32 Limit)
{
	return FMath::Clamp(Requested, 1, FMath::Max(1, Limit));
}

FString FGrimOnlineMath::ToPackagePath(const FString& MapPath)
{
	FString Package = MapPath.TrimStartAndEnd();
	int32 Dot = INDEX_NONE;
	if (Package.FindLastChar(TEXT('.'), Dot))
	{
		int32 Slash = INDEX_NONE;
		Package.FindLastChar(TEXT('/'), Slash);
		if (Dot > Slash)
		{
			Package.LeftInline(Dot);
		}
	}
	return Package;
}

FString FGrimOnlineMath::MakeHostURL(const FString& MapPath, const FString& Slot, int32 MaxPlayers, bool bListen, const FString& PlayerName)
{
	FString URL = ToPackagePath(MapPath);
	if (bListen)
	{
		URL += TEXT("?listen");
	}
	URL += FString::Printf(TEXT("?SaveSlot=%s?MaxPlayers=%d"), *Slot, FMath::Max(1, MaxPlayers));
	const FString Name = SanitizePlayerName(PlayerName);
	if (!Name.IsEmpty())
	{
		URL += FString::Printf(TEXT("?%s=%s"), NameOptionName, *EncodeOptionValue(Name));
	}
	return URL;
}

FString FGrimOnlineMath::MakeJoinURL(const FGrimServerAddress& Address, const FString& PlayerName)
{
	FString URL = FormatAddress(Address);
	const FString Name = SanitizePlayerName(PlayerName);
	if (!Name.IsEmpty())
	{
		URL += FString::Printf(TEXT("?%s=%s"), NameOptionName, *EncodeOptionValue(Name));
	}
	return URL;
}

FString FGrimOnlineMath::ParseNameOption(const FString& RequestURL, int32 MaxLen)
{
	// 옵션은 ? 로 나뉜다(FURL 과 같은 규칙). 값은 EncodeOptionValue 로 싼 것.
	const FString Key = FString(NameOptionName) + TEXT("=");
	TArray<FString> Parts;
	RequestURL.ParseIntoArray(Parts, TEXT("?"), true);
	for (const FString& Part : Parts)
	{
		if (Part.StartsWith(Key, ESearchCase::IgnoreCase))
		{
			return SanitizePlayerName(DecodeOptionValue(Part.Mid(Key.Len())), MaxLen);
		}
	}
	return FString();
}

FText FGrimOnlineMath::DescribeNetworkFailure(int32 FailureType, const FString& EngineError, bool bWhileConnecting)
{
	FText Base;
	// 서버가 보낸 거절 글은 종류와 상관없이 먼저 본다 — 가득 참은 접속 전이면 PendingConnectionFailure 로 온다.
	if (EngineError.Contains(TEXT("Server full"), ESearchCase::IgnoreCase))
	{
		FailureType = ENetworkFailure::FailureReceived;
	}
	switch (static_cast<ENetworkFailure::Type>(FailureType))
	{
	case ENetworkFailure::NetDriverAlreadyExists:
	case ENetworkFailure::NetDriverCreateFailure:
		Base = LOCTEXT("NetDriverCreate", "네트워크를 시작하지 못했다");
		break;
	case ENetworkFailure::NetDriverListenFailure:
		Base = LOCTEXT("NetListen", "서버를 열지 못했다 — 포트가 이미 쓰이고 있을 수 있다");
		break;
	case ENetworkFailure::ConnectionLost:
		Base = bWhileConnecting
			? LOCTEXT("LostConnecting", "서버에 닿지 못했다 — 주소 · 포트 · 방화벽을 확인해 주세요")
			: LOCTEXT("Lost", "서버와의 연결이 끊겼다(호스트가 서버를 닫았거나 네트워크 문제)");
		break;
	case ENetworkFailure::ConnectionTimeout:
		Base = bWhileConnecting
			? LOCTEXT("TimeoutConnecting", "서버가 답하지 않는다(시간 초과) — 주소 · 포트 · 방화벽을 확인해 주세요")
			: LOCTEXT("Timeout", "서버가 답하지 않아 연결이 끊겼다(시간 초과)");
		break;
	case ENetworkFailure::FailureReceived:
		if (EngineError.Contains(TEXT("full"), ESearchCase::IgnoreCase))
		{
			Base = LOCTEXT("ServerFull", "서버가 가득 찼다 — 자리가 나면 다시 해 주세요");
		}
		else if (EngineError.Contains(TEXT("closed"), ESearchCase::IgnoreCase))
		{
			Base = LOCTEXT("HostClosed", "호스트가 서버를 닫았다");
		}
		else
		{
			Base = LOCTEXT("Rejected", "서버가 접속을 거절했다");
		}
		break;
	case ENetworkFailure::OutdatedClient:
		Base = LOCTEXT("OutdatedClient", "내 게임 버전이 서버보다 낮다 — 게임을 업데이트해 주세요");
		break;
	case ENetworkFailure::OutdatedServer:
		Base = LOCTEXT("OutdatedServer", "서버의 게임 버전이 낮다 — 호스트가 업데이트해야 한다");
		break;
	case ENetworkFailure::NetGuidMismatch:
	case ENetworkFailure::NetChecksumMismatch:
		Base = LOCTEXT("Mismatch", "서버와 게임 데이터가 다르다 — 같은 버전인지 확인해 주세요");
		break;
	case ENetworkFailure::PendingConnectionFailure:
	default:
		Base = bWhileConnecting ? LOCTEXT("ConnectFailed", "접속하지 못했다") : LOCTEXT("Disconnected", "연결이 끊겼다");
		break;
	}
	const FString Detail = EngineError.TrimStartAndEnd();
	return Detail.IsEmpty() ? Base : FText::Format(LOCTEXT("WithDetail", "{0} ({1})"), Base, FText::FromString(Detail));
}

FText FGrimOnlineMath::DescribeTravelFailure(int32 FailureType, const FString& EngineError)
{
	FText Base;
	switch (static_cast<ETravelFailure::Type>(FailureType))
	{
	case ETravelFailure::NoLevel:
	case ETravelFailure::LoadMapFailure:
	case ETravelFailure::PackageMissing:
		Base = LOCTEXT("TravelMap", "맵을 열지 못했다");
		break;
	case ETravelFailure::InvalidURL:
		Base = LOCTEXT("TravelURL", "주소가 올바르지 않다");
		break;
	case ETravelFailure::PackageVersion:
		Base = LOCTEXT("TravelVersion", "서버와 게임 버전이 다르다");
		break;
	case ETravelFailure::PendingNetGameCreateFailure:
		Base = LOCTEXT("TravelPending", "서버에 접속을 시작하지 못했다");
		break;
	case ETravelFailure::CheatCommands:
		Base = LOCTEXT("TravelCheat", "치트 명령을 쓴 뒤라 서버를 열 수 없다 — 게임을 다시 켜 주세요");
		break;
	default:
		Base = LOCTEXT("TravelOther", "이동하지 못했다");
		break;
	}
	const FString Detail = EngineError.TrimStartAndEnd();
	return Detail.IsEmpty() ? Base : FText::Format(LOCTEXT("WithDetail", "{0} ({1})"), Base, FText::FromString(Detail));
}

#undef LOCTEXT_NAMESPACE
