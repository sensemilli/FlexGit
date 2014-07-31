// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class WmfMedia : ModuleRules
	{
		public WmfMedia(TargetInfo Target)
		{
            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                    "Media",
                    "Settings",
				}
            );

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
                    "RenderCore",
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
                    "Media",
					"Settings",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"WmfMedia/Private",
                    "WmfMedia/Private/Player",
                    "WmfMedia/Private/Tracks",
                    "WmfMedia/Private/Wmf",
				}
			);

            if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
            {
                PublicDelayLoadDLLs.Add("shlwapi.dll");
                PublicDelayLoadDLLs.Add("mf.dll");
                PublicDelayLoadDLLs.Add("mfplat.dll");
                PublicDelayLoadDLLs.Add("mfplay.dll");
                PublicDelayLoadDLLs.Add("mfuuid.dll");
            }
		}
	}
}
