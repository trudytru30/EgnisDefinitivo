using UnrealBuildTool;

public class EgnisEditor : ModuleRules
{
    public EgnisEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "InputCore",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "LevelEditor",
                "PropertyEditor",
                "EgnisDef"
            }
        );
    }
}
