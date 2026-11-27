using UnrealBuildTool;

public class Gloshaugen : ModuleRules
{
	public Gloshaugen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "NetCore"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "ApplicationCore", "AnimationCore" });
	}
}
