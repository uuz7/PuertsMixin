// 命令定义头文件：声明插件按钮命令
// 规范：头文件使用 /** ... */ 注释函数与变量
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "PuertsMixinStyle.h"

class FPuertsMixinCommands : public TCommands<FPuertsMixinCommands>
{
public:

	FPuertsMixinCommands()
		: TCommands<FPuertsMixinCommands>(
			TEXT("PuertsMixin"), NSLOCTEXT("Contexts", "PuertsMixin", "PuertsMixin Plugin"), NAME_None,
			FPuertsMixinStyle::GetStyleSetName()
		)
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:

	/** Mixin 按钮命令信息（用于在工具栏显示与绑定行为） */
	TSharedPtr<FUICommandInfo> PluginAction;
};
