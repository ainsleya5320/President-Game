using UnrealBuildTool;

public class OvalOfficeTarget : TargetRules
{
	public OvalOfficeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("OvalOffice");
	}
}
