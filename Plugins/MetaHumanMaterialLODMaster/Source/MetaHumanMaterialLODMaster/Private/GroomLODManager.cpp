#include "GroomLODManager.h"
#include "GroomAsset.h"
#include "GroomAssetCards.h"

void UGroomLODManager::SetGroomCardMaterial(UGroomAsset* GroomAsset, int32 GroupIndex, int32 LODIndex, UMaterialInterface* CardMaterial)
{
	if (!GroomAsset || !CardMaterial) return;

	// 1. Define a unique Slot Name for this override
	// Naming convention: Cards_GroupX_LODY
	FString SlotNameStr = FString::Printf(TEXT("Cards_Group%d_LOD%d"), GroupIndex, LODIndex);
	FName SlotName(*SlotNameStr);

	// 2. Add or Update the Material Slot in HairGroupsMaterials
	bool bFoundSlot = false;
	TArray<FHairGroupsMaterial>& Materials = GroomAsset->GetHairGroupsMaterials();
	
	for (FHairGroupsMaterial& Entry : Materials)
	{
		if (Entry.SlotName == SlotName)
		{
			Entry.Material = CardMaterial;
			bFoundSlot = true;
			break;
		}
	}

	if (!bFoundSlot)
	{
		FHairGroupsMaterial NewEntry;
		NewEntry.SlotName = SlotName;
		NewEntry.Material = CardMaterial;
		Materials.Add(NewEntry);
	}

	// 3. Assign this SlotName to the Cards Geometry configuration for this LOD
	TArray<FHairGroupsCardsSourceDescription>& CardsSource = GroomAsset->GetHairGroupsCards();
	bool bUpdated = false;

	for (FHairGroupsCardsSourceDescription& CardDesc : CardsSource)
	{
		// Check if this Card description applies to our Group and LOD
		// Note: Logic depends on how Groom setup their Cards. 
		// Often 'LODIndex' in SourceDescription might be -1 (valid for all) or specific.
		if (CardDesc.GroupIndex == GroupIndex && (CardDesc.LODIndex == LODIndex || CardDesc.LODIndex == -1))
		{
			CardDesc.MaterialSlotName = SlotName;
			bUpdated = true;
			// If it was -1 (shared), you might want to duplicate this entry for specifically this LOD
			// but for simplicity here we assume 1:1 mapping or user intent to override all.
		}
	}

	if (bUpdated)
	{
		GroomAsset->Modify();
#if WITH_EDITOR
		GroomAsset->UpdateResource(); // Triggers rebuild of render data
#endif
		GroomAsset->MarkPackageDirty();
	}
}
