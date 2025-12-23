import * as UE from "ue";
import mixin from "../../../Mixin";
import { $Nullable } from "puerts";

// 蓝图类路径（Puerts绑定路径）
const AssetPath = "/Game/NPC/Services/BTS_SetFocus.BTS_SetFocus_C";

// 创建一个继承 TS 类（或其他类）的接口（用于类型提示）
export interface BTS_SetFocus extends UE.Game.NPC.Services.BTS_SetFocus.BTS_SetFocus_C {}

// 创建一个继承 TS 的本体类，并通过 mixin 将蓝图逻辑混入
@mixin(AssetPath)
export class BTS_SetFocus implements UE.Game.NPC.Services.BTS_SetFocus.BTS_SetFocus_C {
	ReceiveTickAI(
		OwnerController: $Nullable<UE.AIController>,
		ControlledPawn: $Nullable<UE.Pawn>,
		DeltaSeconds: number
	): void {}
}
