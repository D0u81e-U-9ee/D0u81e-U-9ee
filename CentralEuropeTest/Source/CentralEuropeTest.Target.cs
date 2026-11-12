using UnrealBuildTool;

public class CentralEuropeTestTarget : TargetRules
{
	public CentralEuropeTestTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CentralEuropeTest");
	}
}
