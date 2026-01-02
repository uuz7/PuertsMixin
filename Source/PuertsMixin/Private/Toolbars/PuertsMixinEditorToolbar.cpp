#include "Toolbars/PuertsMixinEditorToolbar.h"
#include "PuertsMixinCommands.h"
#include "PuertsMixinStyle.h"
#include "Engine/Blueprint.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/MessageDialog.h"
#include "UObject/Package.h"
#include "Editor.h"
#include "PuertsMixinSettings.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FPuertsMixinEditorToolbar"

FPuertsMixinEditorToolbar::FPuertsMixinEditorToolbar()
{
}

void FPuertsMixinEditorToolbar::Initialize()
{
	BindCommands();
}

void FPuertsMixinEditorToolbar::BindCommands()
{

}

void FPuertsMixinEditorToolbar::BuildToolbar(FToolBarBuilder& ToolbarBuilder, UObject* InContextObject)
{
	if (!InContextObject)
		return;

	ToolbarBuilder.BeginSection(NAME_None);

	ToolbarBuilder.AddToolBarButton(
		FPuertsMixinCommands::Get().PluginAction,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FPuertsMixinStyle::GetStyleSetName(), "PuertsMixin.PluginAction")
	);

	ToolbarBuilder.EndSection();
}

TSharedRef<FExtender> FPuertsMixinEditorToolbar::GetExtender(UObject* InContextObject)
{
	TSharedRef<FExtender> ToolbarExtender(new FExtender());
	
	TWeakObjectPtr<UObject> WeakContextObject = InContextObject;

	TSharedRef<FUICommandList> LocalCommandList = MakeShared<FUICommandList>();
	LocalCommandList->MapAction(
		FPuertsEnhanceCommands::Get().PluginAction,
		FExecuteAction::CreateLambda([this, WeakContextObject]() { Mixin_Executed(WeakContextObject.Get()); })
	);

	const auto ExtensionDelegate = FToolBarExtensionDelegate::CreateLambda([this, WeakContextObject](FToolBarBuilder& ToolbarBuilder) {
		BuildToolbar(ToolbarBuilder, WeakContextObject.Get());
		});

	// 添加到 "Debugging" 区域之后
	ToolbarExtender->AddToolBarExtension("Debugging", EExtensionHook::After, LocalCommandList, ExtensionDelegate);
	return ToolbarExtender;
}

void FPuertsMixinEditorToolbar::Mixin_Executed(UObject* InContextObject)
{
	UBlueprint* TargetBP = Cast<UBlueprint>(InContextObject);
	if (!TargetBP)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No Blueprint context found.")));
		return;
	}

	// Blueprint 名称（用于 TS class 名）
	FString BPName = TargetBP->GetName();

	// Blueprint 所在 Package 路径，例如：/Game/NPC/Services/BTS_Shoot
	FString PackageName = TargetBP->GetOutermost()->GetName();

	// 去掉 /Game/ 前缀，得到相对于 Content 的路径
	FString RelativePath = PackageName;
	if (RelativePath.StartsWith(TEXT("/Game/")))
	{
		RelativePath.RightChopInline(6); // 移除 "/Game/"
	}

	// 获取设置
	const UPuertsMixinSettings* Settings = GetDefault<UPuertsMixinSettings>();

	// 获取配置的 Mixin 根路径（相对于 TypeScript 目录）
	FString MixinRoot = Settings->MixinRootPath;
	if (MixinRoot.IsEmpty())
	{
		MixinRoot = TEXT("Blueprints");
	}

	// 构造绝对路径：
	// 输出路径 = <Project>/TypeScript/<MixinRoot>/<蓝图相对路径>.ts
	// 例如：<Project>/TypeScript/src/blueprints/NPC/Services/BTS_Shoot.ts
	FString AbsFilePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("TypeScript"),
		MixinRoot,
		RelativePath + TEXT(".ts")
	);

	// 标准化路径分隔符
	FPaths::MakeStandardFilename(AbsFilePath);

	// 确保目录存在
	FString FileDir = FPaths::GetPath(AbsFilePath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*FileDir))
	{
		PlatformFile.CreateDirectoryTree(*FileDir);
	}

	// 如果文件不存在，则生成 TS 模板
	if (!PlatformFile.FileExists(*AbsFilePath))
	{
		// 根据 Blueprint 的层级深度计算 Mixin 的相对导入路径
		// 例如：
		// MixinRoot = "src/blueprints", RelativePath = "NPC/Services/BTS_Shoot"
		// 文件位置：TypeScript/src/blueprints/NPC/Services/BTS_Shoot.ts
		// 需要回溯到 TypeScript 目录，然后找到 Mixin
		// 路径：../../../../Mixin
		
		TArray<FString> PathParts;
		RelativePath.ParseIntoArray(PathParts, TEXT("/"), true);

		// 计算 MixinRoot 的深度
		TArray<FString> RootParts;
		MixinRoot.ParseIntoArray(RootParts, TEXT("/"), true);

		// 总深度 = MixinRoot 深度 + 蓝图路径深度
		int32 Depth = PathParts.Num() + RootParts.Num();

		FString MixinImportPath = TEXT("..");
		for (int32 i = 1; i < Depth; ++i)
		{
			MixinImportPath += TEXT("/..");
		}
		MixinImportPath += TEXT("/Mixin");

		// Blueprint 生成类路径（Puerts 使用）
		FString ClassPath = TargetBP->GetPathName() + TEXT("_C");

		// 生成 TS 类型路径（UE.Game.[目录].[蓝图名].[蓝图名]_C）
		FString TsTypePath = TEXT("UE.Game");
		if (PathParts.Num() > 0)
		{
			for (int32 i = 0; i < PathParts.Num() - 1; ++i)
			{
				TsTypePath += TEXT(".");
				TsTypePath += PathParts[i];
			}
			const FString& BPShortName = PathParts.Last();
			TsTypePath += TEXT(".");
			TsTypePath += BPShortName;
			TsTypePath += TEXT(".");
			TsTypePath += BPShortName;
			TsTypePath += TEXT("_C");
		}
		else
		{
			TsTypePath += TEXT(".");
			TsTypePath += BPName;
			TsTypePath += TEXT(".");
			TsTypePath += BPName;
			TsTypePath += TEXT("_C");
		}

		// 查找并应用模板
		// 1. 从当前类开始向上遍历继承链
		// 2. 查找 Config/TSTemplates/<ClassName>.ts
		// 3. 优先查找 ProjectConfigDir，其次查找 PluginConfigDir
		static FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("PuertsMixin"))->GetBaseDir();

		UClass* Class = TargetBP->GeneratedClass;
		FString Content;
		bool bTemplateFound = false;

		for (auto TemplateClass = Class; TemplateClass; TemplateClass = TemplateClass->GetSuperClass())
		{
			// 处理类名后缀（_C）
			FString TemplateClassName = TemplateClass->GetName();
			if (TemplateClassName.EndsWith(TEXT("_C")))
			{
				TemplateClassName.LeftChopInline(2);
			}

			// 构造相对路径：Config/TSTemplates/ClassName.ts
			FString RelativeFilePath = FPaths::Combine(
				TEXT("Config/TSTemplates"), TemplateClassName + TEXT(".ts")
			);

			// 检查项目配置目录
			FString FullFilePath = FPaths::Combine(FPaths::ProjectConfigDir(), RelativeFilePath);
			if (!FPaths::FileExists(FullFilePath))
			{
				// 检查插件配置目录
				FullFilePath = FPaths::Combine(BaseDir, RelativeFilePath);
			}

			// 如果找到了模板文件
			if (FPaths::FileExists(FullFilePath))
			{
				if (FFileHelper::LoadFileToString(Content, *FullFilePath))
				{
					// 替换占位符
					Content = Content.Replace(TEXT("{BPName}"), *BPName)
						.Replace(TEXT("{PackageName}"), *PackageName)
						.Replace(TEXT("{MixinImportPath}"), *MixinImportPath)
						.Replace(TEXT("{AssetPath}"), *ClassPath)
						.Replace(TEXT("{TsTypePath}"), *TsTypePath);

					bTemplateFound = true;
					break;
				}
			}
		}

		// 如果没有找到特定模板，使用默认 Object 模板
		if (!bTemplateFound)
		{
			// 尝试加载 Object.ts 作为默认回退
			FString DefaultTemplatePath = FPaths::Combine(BaseDir, TEXT("Config/TSTemplates/Object.ts"));
			if (FPaths::FileExists(DefaultTemplatePath))
			{
				if (FFileHelper::LoadFileToString(Content, *DefaultTemplatePath))
				{
					Content = Content.Replace(TEXT("{BPName}"), *BPName)
						.Replace(TEXT("{PackageName}"), *PackageName)
						.Replace(TEXT("{MixinImportPath}"), *MixinImportPath)
						.Replace(TEXT("{AssetPath}"), *ClassPath)
						.Replace(TEXT("{TsTypePath}"), *TsTypePath);
					bTemplateFound = true;
				}
			}
		}

		// 如果依然没有内容（极少情况），使用硬编码的回退内容
		if (!bTemplateFound)
		{
			Content = FString::Printf(
				TEXT(
					"import * as UE from \"ue\";\n"
					"import mixin from \"%s\";\n\n"
					"const AssetPath = \"%s\";\n\n"
					"@mixin(AssetPath)\n"
					"export class %s implements %s {\n"
					"}\n"
				),
				*MixinImportPath,
				*ClassPath,
				*BPName,
				*TsTypePath
			);
		}

		// 写入文件
		FFileHelper::SaveStringToFile(Content, *AbsFilePath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

		// -------------------------------------------------------------------------
		// 自动注册 Mixin 类到入口文件
		// -------------------------------------------------------------------------
		
		// 获取入口文件路径（相对于 TypeScript 目录）
		FString MainEntryPath = Settings->MainEntryFilePath;
		if (MainEntryPath.IsEmpty())
		{
			MainEntryPath = TEXT("Main.ts");
		}

		// Main 文件绝对路径：<Project>/TypeScript/<MainEntryPath>
		// 例如：<Project>/TypeScript/src/main.ts
		FString MainFilePath = FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("TypeScript"),
			MainEntryPath
		);
		FPaths::MakeStandardFilename(MainFilePath);

		if (PlatformFile.FileExists(*MainFilePath))
		{
			FString MainContent;
			if (FFileHelper::LoadFileToString(MainContent, *MainFilePath))
			{
				// 计算相对导入路径
				// 例如：
				// Generated TS: TypeScript/src/blueprints/NPC/Services/BTS_Shoot.ts
				// Main TS: TypeScript/src/main.ts
				// 需要计算从 main.ts 到 BTS_Shoot.ts 的相对路径
				// 结果: ./blueprints/NPC/Services/BTS_Shoot

				// 获取 Main 文件所在目录（相对于 TypeScript）
				FString MainDir = FPaths::GetPath(MainEntryPath);
				// 标准化路径分隔符
				MainDir.ReplaceInline(TEXT("\\"), TEXT("/"));
				FString MixinRootCopy = MixinRoot;
				MixinRootCopy.ReplaceInline(TEXT("\\"), TEXT("/"));

				// 分解路径
				TArray<FString> MainDirParts;
				if (!MainDir.IsEmpty())
				{
					MainDir.ParseIntoArray(MainDirParts, TEXT("/"), true);
				}

				TArray<FString> MixinRootParts;
				MixinRootCopy.ParseIntoArray(MixinRootParts, TEXT("/"), true);

				// 找到共同父目录
				int32 CommonDepth = 0;
				int32 MinDepth = FMath::Min(MainDirParts.Num(), MixinRootParts.Num());
				for (int32 i = 0; i < MinDepth; ++i)
				{
					if (MainDirParts[i] == MixinRootParts[i])
					{
						CommonDepth++;
					}
					else
					{
						break;
					}
				}

				// 计算相对路径
				FString ImportPath;

				// 从 Main 文件目录向上回溯到共同父目录
				int32 UpLevels = MainDirParts.Num() - CommonDepth;
				if (UpLevels == 0)
				{
					ImportPath = TEXT(".");
				}
				else
				{
					ImportPath = TEXT("..");
					for (int32 i = 1; i < UpLevels; ++i)
					{
						ImportPath += TEXT("/..");
					}
				}

				// 添加 MixinRoot 中未共享的部分
				for (int32 i = CommonDepth; i < MixinRootParts.Num(); ++i)
				{
					ImportPath += TEXT("/") + MixinRootParts[i];
				}

				// 添加蓝图相对路径
				ImportPath += TEXT("/") + RelativePath;
				// 将 Windows 路径分隔符替换为 /
				ImportPath.ReplaceInline(TEXT("\\"), TEXT("/"));

				// 检查是否已经包含该导入
				if (!MainContent.Contains(ImportPath))
				{
					FString ImportStmt = FString::Printf(TEXT("import '%s';\r\n"), *ImportPath);

					// 追加到文件末尾（也可以考虑插入到开头，但追加更安全不易破坏结构）
					MainContent += ImportStmt;

					FFileHelper::SaveStringToFile(MainContent, *MainFilePath);
				}
			}
		}
	}


	// 从配置获取编辑器启动命令与类型
	const FString EditorCommand = Settings->GetEditorCommand();
	const EPuertsMixinEditorType EditorType = Settings->EditorType;

	// 根据类型构造命令行
	FString ProcExecutable;
	FString ProcArgs;

	if (EditorType == EPuertsMixinEditorType::VSCode)
	{
		// 标准 VSCode 类编辑器：-r -g "file:line"
		const FString CmdArgs = FString::Printf(TEXT("-r -g \"%s:%d\""), *AbsFilePath, 1);

		// 检查是否是脚本文件
		if (EditorCommand.EndsWith(TEXT(".cmd")) || EditorCommand.EndsWith(TEXT(".bat")))
		{
			ProcExecutable = TEXT("cmd.exe");
			ProcArgs = FString::Printf(TEXT("/c %s %s"), *EditorCommand, *CmdArgs);
		}
		else
		{
			ProcExecutable = EditorCommand;
			ProcArgs = CmdArgs;
		}
	}
	else // Custom
	{
		// 自定义编辑器
		FString CmdArgs = FString::Printf(TEXT("\"%s\""), *AbsFilePath); // 默认只传文件路径

		// 如果自定义编辑器也支持 code 风格参数，可以在这里扩展逻辑，暂时按最安全的方式处理

		if (EditorCommand.EndsWith(TEXT(".cmd")) || EditorCommand.EndsWith(TEXT(".bat")))
		{
			ProcExecutable = TEXT("cmd.exe");
			ProcArgs = FString::Printf(TEXT("/c %s %s"), *EditorCommand, *CmdArgs);
		}
		else
		{
			ProcExecutable = EditorCommand;
			ProcArgs = CmdArgs;
		}
	}

	// 执行外部进程
	{
		FProcHandle Handle = FPlatformProcess::CreateProc(
			*ProcExecutable,
			*ProcArgs,
			true,
			false,
			false,
			nullptr,
			0,
			nullptr,
			nullptr
		);

		// 1. 执行 Puerts.Gen 命令，确保 TS 类型定义是最新的
		// 这样生成的 TS 文件中引用的 Blueprint 类型才有效
		if (GEditor)
		{
			GEditor->Exec(nullptr, TEXT("Puerts.Gen"), *GLog);
		}

		if (!Handle.IsValid())
		{
			FMessageDialog::Open(
				EAppMsgType::Ok, FText::Format(
					FText::FromString(TEXT("Failed to launch editor: {0}")), FText::FromString(EditorCommand)
				)
			);
		}
	}
}

#undef LOCTEXT_NAMESPACE
