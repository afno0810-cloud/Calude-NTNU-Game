using UnrealBuildTool;

public class GloshaugenEditorTarget : TargetRules
{
	public GloshaugenEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Gloshaugen");
	}
}
