using UnrealBuildTool;
//
public class ECHOEditor : ModuleRules
{
    public ECHOEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new string[] { "ECHO", "UnrealEd", "AnimGraph", "AnimGraphRuntime", "BlueprintGraph", "AIModule" });
    }
}
