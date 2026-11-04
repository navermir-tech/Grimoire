using UnrealBuildTool;

public class Grimoire : ModuleRules
{
	public Grimoire(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore"
		});

		// Magic system: if we adopt the Gameplay Ability System, add
		// "GameplayAbilities", "GameplayTags", "GameplayTasks" here and enable the plugin in Grimoire.uproject.
	}
}
