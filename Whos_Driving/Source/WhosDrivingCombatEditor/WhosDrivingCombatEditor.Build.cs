using UnrealBuildTool;

public class WhosDrivingCombatEditor : ModuleRules
{
	public WhosDrivingCombatEditor(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[] {
			"Whos_Driving", "UnrealEd", "AssetTools", "AssetRegistry", "AnimGraph", "AnimGraphRuntime",
			"BlueprintGraph", "Kismet", "KismetCompiler", "SlateCore"
		});
	}
}
