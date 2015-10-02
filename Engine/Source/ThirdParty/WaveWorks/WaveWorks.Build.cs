// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WaveWorks : ModuleRules
{
	public WaveWorks(TargetInfo Target)
	{
		Type = ModuleType.External;

		Definitions.Add("WITH_WAVEWORKS=1");
        Definitions.Add("GFSDK_WAVEWORKS_DLL");

		string WaveWorksDir = UEBuildConfiguration.UEThirdPartySourceDirectory + "WaveWorks/";

        string Platform = "unknown";
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            Platform = "win64";
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            Platform = "win32";
        }

        string FileName = "gfsdk_waveworks";
        if (Target.Configuration == UnrealTargetConfiguration.Debug)
            FileName += "_debug";
        FileName += "." + Platform;

        PublicIncludePaths.Add(WaveWorksDir + "inc");
        PublicLibraryPaths.Add(WaveWorksDir + "lib/" + Platform);
        PublicAdditionalLibraries.Add(FileName + ".lib");
        PublicDelayLoadDLLs.Add(FileName + ".dll");
	}
}
