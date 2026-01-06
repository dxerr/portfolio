#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GroomLODManager.generated.h"

class UGroomAsset;
class UMaterialInterface;

UCLASS()
class UGroomLODManager : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Assign a material to the Cards geometry for a specific LOD.
	 */
	static void SetGroomCardMaterial(UGroomAsset* GroomAsset, int32 GroupIndex, int32 LODIndex, UMaterialInterface* CardMaterial);
};
