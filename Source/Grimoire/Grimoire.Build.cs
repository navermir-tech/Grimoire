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
			"SlateCore"
		});
		PublicDependencyModuleNames.Add("AIModule"); // M3 갈래 C — 생물 AI 컨트롤러(AAIController)

		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils", "Sockets", "EngineSettings" }); // M3 갈래 J 접속 · 메뉴
	}
}
