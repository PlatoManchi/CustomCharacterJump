// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CustomMovement : ModuleRules
{
	public CustomMovement(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
