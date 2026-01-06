#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SMetaHumanMaterialLODMasterWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMetaHumanMaterialLODMasterWidget)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
private:
	TSharedRef<SWidget> MakeWidgetForAsset(UObject* Asset);
	void Refresh();
	
	// State
	TWeakObjectPtr<UObject> SelectedAsset;
	TSharedPtr<SVerticalBox> MainContent;
};
