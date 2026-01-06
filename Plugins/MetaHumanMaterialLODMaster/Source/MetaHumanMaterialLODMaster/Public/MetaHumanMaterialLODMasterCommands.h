#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "MetaHumanMaterialLODMasterStyle.h"

class FMetaHumanMaterialLODMasterCommands : public TCommands<FMetaHumanMaterialLODMasterCommands>
{
public:

	FMetaHumanMaterialLODMasterCommands()
		: TCommands<FMetaHumanMaterialLODMasterCommands>(TEXT("MetaHumanMaterialLODMaster"), NSLOCTEXT("Contexts", "MetaHumanMaterialLODMaster", "MetaHumanMaterialLODMaster Plugin"), NAME_None, FMetaHumanMaterialLODMasterStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};
