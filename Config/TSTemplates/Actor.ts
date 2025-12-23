import * as UE from "ue";
import mixin from "{MixinImportPath}";

// 蓝图类路径（Puerts绑定路径）
const AssetPath = "{AssetPath}";

// 创建一个继承 TS 类（或其他类）的接口（用于类型提示）
export interface {BPName}extends{TsTypePath}{}

// 创建一个继承 TS 的本体类，并通过 mixin 将蓝图逻辑混入
@mixin(AssetPath)
export class {BPName}implements{TsTypePath}
{

	// Actor 生命周期：开始播放
	ReceiveBeginPlay():void {
		console.log(`[Mixin] {BPName} ReceiveBeginPlay`);
	}

	// Actor 生命周期：每帧更新
	ReceiveTick(DeltaSeconds:number):oid {}

	// Actor 生命周期：结束播放
	ReceiveEndPlay(EndPlayReason
:
	UE.EEndPlayReason
):
	void {
		console.log(`[Mixin] {BPName} ReceiveEndPlay`);
	}
}
