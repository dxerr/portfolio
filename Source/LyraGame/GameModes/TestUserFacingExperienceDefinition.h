// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "LyraUserFacingExperienceDefinition.h"
#include "TestUserFacingExperienceDefinition.generated.h"

/** Description of settings used to display experiences in the UI and start a new session */
UCLASS(BlueprintType)
class UTestUserFacingExperienceDefinition : public ULyraUserFacingExperienceDefinition
{
	GENERATED_BODY()

public:
	UTestUserFacingExperienceDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
