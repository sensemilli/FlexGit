// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderCore : ModuleRules
{
	public ShaderCore(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "RHI", "RenderCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateIncludePathModuleNames.AddRange(new string[] { "DerivedDataCache", "TargetPlatform" });

        // NVCHANGE_BEGIN: Add VXGI
        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            PublicDependencyModuleNames.Add("VXGI");
        }
        // NVCHANGE_END: Add VXGI
	}
}