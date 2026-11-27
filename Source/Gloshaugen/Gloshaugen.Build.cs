using UnrealBuildTool;

public class Gloshaugen : ModuleRules
{
	public Gloshaugen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "NetCore",
			"AIModule", "NavigationSystem", "GameplayTasks"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "ApplicationCore", "AnimationCore",
			"RenderCore", "RHI" });   // RenderCore/RHI: frame timings in Grafikk.Foto (GSFoto.cpp)
	}
}
