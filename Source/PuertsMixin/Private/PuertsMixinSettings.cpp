
#include "PuertsMixinSettings.h"

UPuertsMixinSettings::UPuertsMixinSettings()
	: EditorType(EPuertsMixinEditorType::VSCode)
	, CustomEditorExecutable(TEXT("code.cmd"))
	, MixinRootPath(TEXT("Blueprints"))
	, MainEntryFilePath(TEXT("Main.ts"))
{
}

FString UPuertsMixinSettings::GetEditorCommand() const
{
	switch (EditorType)
	{
	case EPuertsMixinEditorType::VSCode:
		return TEXT("code.cmd");

	case EPuertsMixinEditorType::Custom:
		return CustomEditorExecutable.IsEmpty() ? TEXT("code.cmd") : CustomEditorExecutable;

	default:
		return TEXT("code.cmd");
	}
}
