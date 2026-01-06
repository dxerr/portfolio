#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SkeletalMeshLODManager.generated.h"

class USkeletalMesh;
class UMaterialInterface;

UCLASS()
class USkeletalMeshLODManager : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Apply a specific Material Interface to a Section within a specific LOD.
	 */
	static void UpgradeSkeletalMeshLODMaterial(USkeletalMesh* SkeletalMesh, int32 LODIndex, int32 SectionIndex, UMaterialInterface* NewMaterial);
};
