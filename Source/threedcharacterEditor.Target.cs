using UnrealBuildTool;

public class threedcharacterEditorTarget : TargetRules
{
    public threedcharacterEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("threedcharacter");
    }
}
