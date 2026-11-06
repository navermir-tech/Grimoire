#include "Base/GrimBaseSettings.h"
#include "Items/GrimItemSettings.h"
#include "HAL/IConsoleManager.h"

namespace GrimBaseSettingsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	FGrimBaseDatabase GBaseDatabase;
	bool GBaseDatabaseLoaded = false;

	FAutoConsoleCommand GReloadBaseCommand(
		TEXT("Grim.Base.Reload"),
		TEXT("건축 조각 · 작물 · 장비 표(CSV)를 다시 읽는다"),
		FConsoleCommandDelegate::CreateStatic(&UGrimBaseSettings::ReloadData));
}

const FGrimBaseDatabase& UGrimBaseSettings::GetDatabase()
{
	using namespace GrimBaseSettingsLocal;
	if (GBaseDatabaseLoaded)
	{
		return GBaseDatabase;
	}
	GBaseDatabaseLoaded = true;

	const UGrimBaseSettings* Settings = Get();
	FString Pieces, Crops, Equipment;
	if (!UGrimItemSettings::LoadContentFile(Settings->BuildPiecesCSV, Pieces))
	{
		UE_LOG(LogGrimBase, Error, TEXT("건축 조각 표를 열 수 없다: %s"), *Settings->BuildPiecesCSV);
	}
	if (!UGrimItemSettings::LoadContentFile(Settings->CropsCSV, Crops))
	{
		UE_LOG(LogGrimBase, Error, TEXT("작물 표를 열 수 없다: %s"), *Settings->CropsCSV);
	}
	if (!UGrimItemSettings::LoadContentFile(Settings->EquipmentCSV, Equipment))
	{
		UE_LOG(LogGrimBase, Error, TEXT("장비 표를 열 수 없다: %s"), *Settings->EquipmentCSV);
	}
	TArray<FString> Problems;
	FGrimBaseDatabase::BuildFromCSV(Pieces, Crops, Equipment, &UGrimItemSettings::GetDatabase(), GBaseDatabase, Problems);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimBase, Error, TEXT("거점 표: %s"), *Problem);
	}
	UE_LOG(LogGrimBase, Log, TEXT("거점 표: 조각 %d · 작물 %d · 장비 %d"), GBaseDatabase.Pieces.Num(), GBaseDatabase.Crops.Num(), GBaseDatabase.Equipment.Num());
	return GBaseDatabase;
}

void UGrimBaseSettings::ReloadData()
{
	GrimBaseSettingsLocal::GBaseDatabaseLoaded = false;
	GetDatabase();
}

FGrimBuildGrid UGrimBaseSettings::MakeGrid(const FVector& Origin, float Yaw) const
{
	FGrimBuildGrid Grid;
	Grid.Origin = Origin;
	Grid.Yaw = Yaw;
	Grid.CellSize = CellSize;
	Grid.WallHeight = WallHeight;
	Grid.SlabThickness = SlabThickness;
	Grid.FloorTopOffset = FloorTopOffset;
	Grid.FoundationDepth = FoundationDepth;
	Grid.MaxLevel = MaxLevel;
	return Grid;
}
