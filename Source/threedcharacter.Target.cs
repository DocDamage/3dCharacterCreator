using UnrealBuildTool;

public class threedcharacterTarget : TargetRules
{
    public threedcharacterTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("threedcharacter");
    }
}
