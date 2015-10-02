// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;

public class FLEX : ModuleRules
{
	public FLEX(TargetInfo Target)
	{
		Type = ModuleType.External;

        if (UEBuildConfiguration.bCompileFLEX == false)
        {
            Definitions.Add("WITH_FLEX=0");
            return;
        }

        Definitions.Add("WITH_FLEX=1");

        string FLEXDir = UEBuildConfiguration.UEThirdPartySourceDirectory + "PhysX/FLEX-0.9.0/";
		string FLEXLibDir = FLEXDir + "lib";

        PublicIncludePaths.Add(FLEXDir + "include");

		// Libraries and DLLs for windows platform
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
            PublicLibraryPaths.Add(FLEXLibDir + "/x64");

            if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
            {
                PublicAdditionalLibraries.Add("flexDebug_x64.lib");
                PublicDelayLoadDLLs.Add("flexDebug_x64.dll");
                PublicAdditionalLibraries.Add("flexExtDebug_x64.lib");
                PublicDelayLoadDLLs.Add("flexExtDebug_x64.dll");
            }
            else
            {
                PublicAdditionalLibraries.Add("flexRelease_x64.lib");
                PublicDelayLoadDLLs.Add("flexRelease_x64.dll");
                PublicAdditionalLibraries.Add("flexExtRelease_x64.lib");
                PublicDelayLoadDLLs.Add("flexExtRelease_x64.dll");
            }

            PublicLibraryPaths.Add(FLEXDir + "/Win64");

            string[] RuntimeDependenciesX64 =
			{
				"cudart64_70.dll",
				"flexDebug_x64.dll",
				"flexExtDebug_x64.dll",
				"flexExtRelease_x64.dll",
				"flexRelease_x64.dll",
			};

            string FlexBinariesDir = String.Format("$(EngineDir)/Binaries/ThirdParty/PhysX/FLEX-0.9.0/Win64/");
            foreach (string RuntimeDependency in RuntimeDependenciesX64)
            {
                RuntimeDependencies.Add(new RuntimeDependency(FlexBinariesDir + RuntimeDependency));
            }
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			PublicLibraryPaths.Add(FLEXLibDir + "/win32");

            if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
            {
                PublicAdditionalLibraries.Add("flexDebug_x86.lib");
                PublicDelayLoadDLLs.Add("flexDebug_x86.dll");
                PublicAdditionalLibraries.Add("flexExtDebug_x86.lib");
                PublicDelayLoadDLLs.Add("flexExtDebug_x86.dll");
            }
            else
            {
                PublicAdditionalLibraries.Add("flexRelease_x86.lib");
                PublicDelayLoadDLLs.Add("flexRelease_x86.dll");
                PublicAdditionalLibraries.Add("flexExtRelease_x86.lib");
                PublicDelayLoadDLLs.Add("flexExtRelease_x86.dll");
            }

			PublicLibraryPaths.Add(FLEXDir + "/Win32");

			string[] RuntimeDependenciesX86 =
			{
				"cudart32_70.dll",
				"flexDebug_x86.dll",
				"flexExtDebug_x86.dll",
				"flexExtRelease_x86.dll",
				"flexRelease_x86.dll",
			};

			string FlexBinariesDir = String.Format("$(EngineDir)/Binaries/ThirdParty/PhysX/FLEX-0.9.0/Win32/");
			foreach (string RuntimeDependency in RuntimeDependenciesX86)
			{
				RuntimeDependencies.Add(new RuntimeDependency(FlexBinariesDir + RuntimeDependency));
			}
        }
	}
}
