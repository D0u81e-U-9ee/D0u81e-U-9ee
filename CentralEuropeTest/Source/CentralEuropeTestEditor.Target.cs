using UnrealBuildTool;

public class CentralEuropeTestEditorTarget : TargetRules
{
	public CentralEuropeTestEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("CentralEuropeTest");
	}
}
