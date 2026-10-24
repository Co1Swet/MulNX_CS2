# 非交互启动

`CS2Injector.exe --launch-result "D:\制作资料\launch-result.json"` 供外部程序调用：读取 `CS2BootLoader/Config/config.yaml`，执行环境检查并复用原启动流程，附加 `-insecure`，完成后退出启动器。普通启动及 `auto.yaml` 保持原行为。

结果文件须为尚不存在的绝对磁盘路径，父目录须已存在。每次调用使用新路径，串行启动；调用方等待启动器退出后读取 UTF-8 JSON。退出码：`0` 成功、`1` 启动失败、`2` 参数或结果路径无效。

```json
{"protocol":1,"ok":true,"stage":"resumed","pid":1234,"created":"134057000000000000","cleanupPending":false}
```

`created` 为进程创建 FILETIME 的十进制字符串。调用方核对 PID、创建时间和 CS2 路径；`resumed` 表示加载调用返回且主线程已恢复，窗口、GSI、地图及控制网关就绪由调用方另行确认。成功后游戏继续运行。

失败回执包含 `stage`、`errorCode`、`message`、`cleanupPending`，阶段为 `initialize/environment/launch/identity/load/shutdown`。系统错误码取自当次失败，只有错误文本时为 `0`。框架致命错误也写回回执；新入口设 90 秒超时，失败或成功回执写入失败时清理本次创建的游戏。清理尚未确认时返回 `cleanupPending:true` 及 `cleanupErrorCode`。

调用方校验协议与退出码，空文件、超时或结果不一致均作为失败处理，由用户决定下一次启动。
