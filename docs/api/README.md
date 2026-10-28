# WebSocket 启动

正常启动 `CS2Injector.exe`，连接 `ws://127.0.0.1:55203`（仅本机）。
发送 `{"type":"CS2BootLoader/Launch","payload":""}`，复用现有消息总线和启动流程。
每次启动前检查环境，`auto.yaml` 自动启动也执行检查。
返回的“已投入消息总线”只表示入队，不表示启动成功。
Mizar 继续使用现有的进程与 GSI 检查确认游戏状态；游戏内网关仍使用 55202。
