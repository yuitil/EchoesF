using UnrealBuildTool;
//
public class ECHOEditor : ModuleRules
{
    public ECHOEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.Add(System.IO.Path.Combine(GetModuleDirectory("Niagara"), "Internal"));
        PrivateIncludePaths.Add(System.IO.Path.Combine(GetModuleDirectory("NiagaraShader"), "Internal"));
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new string[] { "ECHO", "UnrealEd", "AnimGraph", "AnimGraphRuntime", "BlueprintGraph", "AIModule", "AIGraph", "Niagara", "MaterialEditor", "NiagaraShader", "RenderCore", "RHI" });
    }
}
