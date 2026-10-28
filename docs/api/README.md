# 启动器 WebSocket 接口（protocol 2）

正常运行 `CS2Injector.exe` 后，连接 `ws://127.0.0.1:55203`，向已有消息总线发送启动请求。启动器读取 `CS2BootLoader/Config/config.yaml`，每次实际启动都执行环境检查，再复用原有加载顺序并强制附加 `-insecure`。界面按钮及 `auto.yaml` 自动启动也执行该检查；外部调用时保持 `autoLaunch: false`。

启动器的 `WebSocketManager/Config/config.yaml` 默认端口为 55203，只绑定 IPv4 loopback；配置缺失、无效或误填游戏端口 55202 时回退到 55203。游戏内网关仍使用原有 55202 及原绑定行为。发布包内 `CS2Injector/launch-protocol.txt` 为 `2`；旧的命令行参数和结果文件协议不再使用。

## 请求与回执

外层保留 `type`、`payload` 两个字符串。`Launch` 使用现有 `SubscribeAsyncEx<void>` factory：payload 中的前导引号字符串进入 `NetExt::str1`，作为 requestId。

```json
{"type":"CS2BootLoader/Launch","payload":"\"1\""}
```

requestId 是非零 uint64 的规范十进制字符串（1 至 18446744073709551615，无前导零）。每个启动器实例只保存最高已接收 ID 和最近一次结果：相同 ID 回放该结果，小于最高 ID 返回 `stale_request`，缺失或非法 ID 返回 `invalid_request`，均不重新启动。调用方在同一个启动器实例内串行使用递增 ID；断线重连后用原 ID 取回结果，不要直接发新 ID。

网关原有的“已投入消息总线”文本只表示入队。真正结果通过已有 `WebPost` 广播给当前连接，调用方只接收匹配自己 requestId 的 `CS2BootLoader/LaunchResult`，并再次解析 payload 字符串：

```json
{"type":"CS2BootLoader/LaunchResult","payload":"{\"protocol\":2,\"requestId\":\"1\",\"ok\":true,\"stage\":\"resumed\",\"pid\":1234,\"created\":\"134057000000000000\",\"errorCode\":0,\"message\":\"\",\"cleanupPending\":false,\"cleanupErrorCode\":0}"}
```

`pid` 与 `created` 来自本次持有的游戏进程句柄；created 是创建 FILETIME 的十进制字符串，避免 JSON 数值精度丢失。尚未创建进程时为 `0`、`""`；身份读取失败时 created 也可能为空。`resumed` 表示现有远程加载/初始化调用返回且 `ResumeThread` 将主线程挂起计数从 1 降到 0。调用方须核对 protocol、requestId、ok、stage，以及进程的 PID、创建时间和 CS2 路径；窗口、地图、游戏接口及 GSI 就绪另行确认。

## 确认成功后关闭启动器

客户端完成上述成功结果核验后发送相同 requestId：

```json
{"type":"CS2BootLoader/LaunchResult/Ack","payload":"\"1\""}
```

只有 ACK 匹配最近成功结果且仍持有同一个存活游戏进程时，BootLoader 才发送内部 `Win32Starter/Close`。它通过 `PostMessageW(hwnd, WM_CLOSE)` 沿现有窗口销毁及框架关闭流程退出启动器，只释放游戏句柄，保留游戏运行。未 ACK、错误 ID 或失败结果均不会自动关闭窗口。游戏端 [`System/Stop`](stop.md) 语义不变。

## 失败与重试边界

失败保留窗口，结果包含 `errorCode`、`message`、`cleanupPending`、`cleanupErrorCode`。启动阶段为 `environment/launch/identity/load/resume`；请求被拒绝时为 `invalid_request/stale_request/already_running/cleanup_pending`。errorCode 为直接捕获的 Win32 错误码，仅有错误文本时为 0，详细加载错误也会写入日志。

失败只清理本启动器本次创建的游戏；先终止并最多等待 5 秒确认退出。无法确认时保留句柄，返回 `cleanupPending: true`，不能据此假定游戏已经退出。新 ID 会重新核对持有的进程：成功启动的游戏仍存活时返回 `already_running`；失败进程未清理完时返回 `cleanup_pending` 并再次尝试清理。只有已确认进程退出后，后续新 ID 才允许再次启动。同 ID 始终回放缓存结果，不刷新状态；`already_running` 不会终止已成功启动的游戏。

这里没有启动 watchdog、结果文件或全局致命错误接管。沿用的底层注入/远程初始化包含无限等待；若它们不返回，启动结果及后续消息也可能一直无法处理。启动器框架初始化失败也不保证产生协议结果。此外，游戏端现有初始化入口在部分异常路径也会唤醒等待并返回 0，故成功回执不能替代调用方对游戏运行状态的检查。连接断开、没有回执或客户端超时都不能证明启动失败或游戏已清理；不要盲目启动另一个启动器实例。
