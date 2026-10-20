using UnrealBuildTool;
public class EgnisDefEditor : ModuleRules
{
    public EgnisDefEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "Kismet", "BlueprintGraph", "EgnisDef", "InputCore" });
    }
}

