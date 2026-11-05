using UnrealBuildTool;

public class Grimoire : ModuleRules
{
	public Grimoire(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Include as "Core/GrimGameMode.h", "Magic/...", etc. relative to this module folder.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"NetCore",
			"Niagara",
			"DeveloperSettings",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"RHI",        // 성능 측정(Core/Perf): GPU 프레임 시간 · 드로 콜
			"RenderCore"  // 성능 측정: 게임 · 렌더 · RHI 스레드 시간
		});
		PublicDependencyModuleNames.Add("AIModule"); // M3 갈래 C — 생물 AI 컨트롤러(AAIController)

		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils", "Sockets", "EngineSettings" }); // M3 갈래 J 접속 · 메뉴

		// 갈래 W(지형 1차): 랜드스케이프 · 폴리지 · 레이아웃 JSON · 미리보기 그림, 에디터에서만 브러시 볼륨(UnrealEd)
		PrivateDependencyModuleNames.AddRange(Target.bBuildEditor ? new string[] { "Landscape", "Foliage", "Json", "JsonUtilities", "ImageCore", "UnrealEd" } : new string[] { "Landscape", "Foliage", "Json", "JsonUtilities", "ImageCore" });
	}
}
