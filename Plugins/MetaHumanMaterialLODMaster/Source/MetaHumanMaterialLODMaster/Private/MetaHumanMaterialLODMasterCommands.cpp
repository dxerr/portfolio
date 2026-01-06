#include "MetaHumanMaterialLODMasterCommands.h"

#define LOCTEXT_NAMESPACE "FMetaHumanMaterialLODMasterModule"

void FMetaHumanMaterialLODMasterCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "MetaHuman Material LOD Master", "Bring up MetaHuman Material LOD Master window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
