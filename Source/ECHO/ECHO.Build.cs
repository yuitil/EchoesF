// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ECHO : ModuleRules
{
	public ECHO(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "NavigationSystem",
			"GameplayTasks", "GameplayAbilities", "GameplayTags","MoviePlayer", "Slate", "SlateCore", "UMG"
        });

		//
		PrivateDependencyModuleNames.AddRange(new string[]
			{ "UMG", "Slate", "SlateCore", "Niagara", "LevelSequence", "MovieScene" });
		//

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}