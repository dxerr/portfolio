#include "MetaHumanMaterialLODMaster.h"
#include "MetaHumanMaterialLODMasterStyle.h"
#include "MetaHumanMaterialLODMasterCommands.h"
#include "SMetaHumanMaterialLODMasterWidget.h" // We will create this Slate Widget next
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName MetaHumanMaterialLODMasterTabName("MetaHumanMaterialLODMaster");

#define LOCTEXT_NAMESPACE "FMetaHumanMaterialLODMasterModule"

void FMetaHumanMaterialLODMasterModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FMetaHumanMaterialLODMasterStyle::Initialize();
	FMetaHumanMaterialLODMasterStyle::ReloadTextures();

	FMetaHumanMaterialLODMasterCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FMetaHumanMaterialLODMasterCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FMetaHumanMaterialLODMasterModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMetaHumanMaterialLODMasterModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(MetaHumanMaterialLODMasterTabName, FOnSpawnTab::CreateRaw(this, &FMetaHumanMaterialLODMasterModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("MetaHumanMaterialLODMasterTabTitle", "MetaHuman Material LOD Master"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FMetaHumanMaterialLODMasterModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FMetaHumanMaterialLODMasterStyle::Shutdown();

	FMetaHumanMaterialLODMasterCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MetaHumanMaterialLODMasterTabName);
}

TSharedRef<SDockTab> FMetaHumanMaterialLODMasterModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SMetaHumanMaterialLODMasterWidget)
		];
}

void FMetaHumanMaterialLODMasterModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(MetaHumanMaterialLODMasterTabName);
}

void FMetaHumanMaterialLODMasterModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FMetaHumanMaterialLODMasterCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FMetaHumanMaterialLODMasterCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMetaHumanMaterialLODMasterModule, MetaHumanMaterialLODMaster)
