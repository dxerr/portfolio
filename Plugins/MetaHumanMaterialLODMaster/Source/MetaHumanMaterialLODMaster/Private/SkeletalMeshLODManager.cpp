#include "SkeletalMeshLODManager.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Engine/SkinnedAssetCommon.h"

void USkeletalMeshLODManager::UpgradeSkeletalMeshLODMaterial(USkeletalMesh* SkeletalMesh, int32 LODIndex, int32 SectionIndex, UMaterialInterface* NewMaterial)
{
	if (!SkeletalMesh || !NewMaterial) return;

	// Ensure we have valid LOD Info
	if (!SkeletalMesh->IsValidLODIndex(LODIndex)) return;
	
	FSkeletalMeshLODInfo* LODInfo = SkeletalMesh->GetLODInfo(LODIndex);
	if (!LODInfo) return;

	// 1. Find or Add the Material to the Global Materials List
	int32 MaterialIndex = INDEX_NONE;
	const TArray<FSkeletalMaterial>& Materials = SkeletalMesh->GetMaterials();
	
	// Check if material already exists
	for (int32 i = 0; i < Materials.Num(); ++i)
	{
		if (Materials[i].MaterialInterface == NewMaterial)
		{
			MaterialIndex = i;
			break;
		}
	}

	// If not found, add it
	if (MaterialIndex == INDEX_NONE)
	{
		SkeletalMesh->GetMaterials().Add(FSkeletalMaterial(NewMaterial));
		MaterialIndex = SkeletalMesh->GetMaterials().Num() - 1;
	}

	// 2. Update LODMaterialMap
	// LODMaterialMap maps Section Index -> Global Material Index
	// Ideally, ensure keys cover the section index
	if (LODInfo->LODMaterialMap.Num() <= SectionIndex)
	{
		LODInfo->LODMaterialMap.SetNum(SectionIndex + 1);
		// Initialize new entries to avoid remapping (INDEX_NONE) or default behavior
		for (int32 i = 0; i < LODInfo->LODMaterialMap.Num(); ++i)
		{
			 if (i != SectionIndex && LODInfo->LODMaterialMap[i] == 0) // Basic init check
			 {
				 LODInfo->LODMaterialMap[i] = INDEX_NONE; 
			 }
		}
	}

	LODInfo->LODMaterialMap[SectionIndex] = MaterialIndex;

	// 3. Apply Changes
	SkeletalMesh->PostEditChange();
	SkeletalMesh->MarkPackageDirty();
	
	// Force Re-init of render resources (Important for Editor visual update)
	SkeletalMesh->InitResources();
}
