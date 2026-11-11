using UnrealBuildTool;

public class CentralEuropeTest : ModuleRules
{
	public CentralEuropeTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "PhysicsCore"
		});
	}
}
