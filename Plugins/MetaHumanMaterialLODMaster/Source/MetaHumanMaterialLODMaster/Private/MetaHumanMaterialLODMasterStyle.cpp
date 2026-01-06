#include "MetaHumanMaterialLODMasterStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FMetaHumanMaterialLODMasterStyle::StyleInstance = NULL;

void FMetaHumanMaterialLODMasterStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMetaHumanMaterialLODMasterStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FMetaHumanMaterialLODMasterStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("MetaHumanMaterialLODMasterStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const ISlateStyle& FMetaHumanMaterialLODMasterStyle::Get()
{
	return *StyleInstance;
}

void FMetaHumanMaterialLODMasterStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

TSharedRef< FSlateStyleSet > FMetaHumanMaterialLODMasterStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("MetaHumanMaterialLODMasterStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("MetaHumanMaterialLODMaster")->GetBaseDir() / TEXT("Resources"));

	Style->Set("MetaHumanMaterialLODMaster.OpenPluginWindow", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), FVector2D(40.0f, 40.0f)));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
