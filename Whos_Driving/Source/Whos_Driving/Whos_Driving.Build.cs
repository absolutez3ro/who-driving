// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Whos_Driving : ModuleRules
{
	public Whos_Driving(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
            "GameplayTasks",
            "Slate",
			"AnimGraphRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Whos_Driving",
			"Whos_Driving/Variant_Horror",
			"Whos_Driving/Variant_Horror/UI",
			"Whos_Driving/Variant_Shooter",
			"Whos_Driving/Variant_Shooter/AI",
			"Whos_Driving/Variant_Shooter/UI",
			"Whos_Driving/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
