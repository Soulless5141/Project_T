// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Project_T : ModuleRules
{
    public Project_T(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // EOSópÇÃàÀë∂ÉÇÉWÉÖÅ[ÉãÇí«â¡
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "OnlineSubsystem",
            "OnlineSubsystemEOS",
            "OnlineSubsystemUtils"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
    }
}