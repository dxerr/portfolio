#include "SMetaHumanMaterialLODMasterWidget.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/STextComboBox.h"
#include "PropertyCustomizationHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "GroomAsset.h"
#include "SkeletalMeshLODManager.h"
#include "GroomLODManager.h"

#define LOCTEXT_NAMESPACE "SMetaHumanMaterialLODMasterWidget"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMetaHumanMaterialLODMasterWidget::Construct(const FArguments& InArgs)
{
	MainContent = SNew(SVerticalBox);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UObject::StaticClass()) // We handle filter manually or allow both
			.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
			{
				SelectedAsset = AssetData.GetAsset();
				Refresh();
			})
			.DisplayThumbnail(true)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(5)
		[
			MainContent.ToSharedRef()
		]
	];
}

void SMetaHumanMaterialLODMasterWidget::Refresh()
{
	MainContent->ClearChildren();

	UObject* Asset = SelectedAsset.Get();
	if (!Asset)
	{
		MainContent->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("NoAsset", "Please select a Skeletal Mesh or Groom Asset."))
			];
		return;
	}

	if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Asset))
	{
		// Render Skeletal Mesh UI
		// Simple grid: Rows = LODs
		for (int32 LODIdx = 0; LODIdx < Mesh->GetNumSourceModels(); ++LODIdx)
		{
			TSharedPtr<SHorizontalBox> Row = SNew(SHorizontalBox);
			Row->AddSlot().AutoWidth().Padding(5)[ SNew(STextBlock).Text(FText::Format(LOCTEXT("LOD", "LOD {0}"), LODIdx)) ];
			
			// For simplicity, we assume Section 0. Real implementation would iterate sections.
			Row->AddSlot().FillWidth(1.0f).Padding(5)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UMaterialInterface::StaticClass())
				.OnObjectChanged_Lambda([Mesh, LODIdx](const FAssetData& MatData)
				{
					if (UMaterialInterface* Mat = Cast<UMaterialInterface>(MatData.GetAsset()))
					{
						USkeletalMeshLODManager::UpgradeSkeletalMeshLODMaterial(Mesh, LODIdx, 0, Mat);
					}
				})
			];
			
			MainContent->AddSlot().AutoHeight()[ Row.ToSharedRef() ];
		}
	}
	else if (UGroomAsset* Groom = Cast<UGroomAsset>(Asset))
	{
		// Render Groom UI
		// Assume usage for Cards (often LOD 2+)
		MainContent->AddSlot().AutoHeight()[ SNew(STextBlock).Text(LOCTEXT("GroomInfo", "Groom Logic (Set Material for Cards)")) ];
		
		for (int32 LODIdx = 0; LODIdx < Groom->GetLODCount(); ++LODIdx)
		{
			TSharedPtr<SHorizontalBox> Row = SNew(SHorizontalBox);
			Row->AddSlot().AutoWidth().Padding(5)[ SNew(STextBlock).Text(FText::Format(LOCTEXT("LOD", "LOD {0}"), LODIdx)) ];
			
			Row->AddSlot().FillWidth(1.0f).Padding(5)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UMaterialInterface::StaticClass())
				.OnObjectChanged_Lambda([Groom, LODIdx](const FAssetData& MatData)
				{
					if (UMaterialInterface* Mat = Cast<UMaterialInterface>(MatData.GetAsset()))
					{
						// Assume Group 0 for now
						UGroomLODManager::SetGroomCardMaterial(Groom, 0, LODIdx, Mat);
					}
				})
			];
			MainContent->AddSlot().AutoHeight()[ Row.ToSharedRef() ];
		}
	}
	else
	{
		MainContent->AddSlot().AutoHeight()[ SNew(STextBlock).Text(LOCTEXT("InvalidAsset", "Invalid Asset Type Selected.")) ];
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
