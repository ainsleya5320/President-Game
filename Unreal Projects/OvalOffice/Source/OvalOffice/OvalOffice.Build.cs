using UnrealBuildTool;

public class OvalOffice : ModuleRules
{
	public OvalOffice(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		// The simulation core is plain C++17 with its own anonymous helpers; keep translation units separate.
		bUseUnity = false;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		RuntimeDependencies.Add("$(ProjectDir)/Content/FourYears/Intro/*.png", StagedFileType.NonUFS);
	}
}
