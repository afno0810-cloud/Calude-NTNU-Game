using UnrealBuildTool;

public class GloshaugenTarget : TargetRules
{
	public GloshaugenTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Gloshaugen");
	}
}
