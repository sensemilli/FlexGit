// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class D3D11RHI : ModuleRules
{
	public D3D11RHI(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Runtime/Windows/D3D11RHI/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Engine",
				"RHI",
				"RenderCore",
				"ShaderCore",
				"UtilityShaders",
			}
			);

		AddThirdPartyPrivateStaticDependencies(Target, "DX11");
        AddThirdPartyPrivateStaticDependencies(Target, "NVAPI");
        AddThirdPartyPrivateStaticDependencies(Target, "AMD");

        // NVCHANGE_BEGIN: Add VXGI
        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            AddThirdPartyPrivateStaticDependencies(Target, "VXGI");
        }
        // NVCHANGE_END: Add VXGI

        // NVCHANGE_BEGIN: Add HBAO+
        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            AddThirdPartyPrivateStaticDependencies(Target, "GFSDK_SSAO");
        }
        // NVCHANGE_END: Add HBAO+

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateIncludePathModuleNames.AddRange(new string[] { "TaskGraph" });
		}
	}
}
