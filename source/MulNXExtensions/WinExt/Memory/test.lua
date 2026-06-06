alloc(newmem,2048,"client.dll"+B80CD9) 
label(returnhere)
label(originalcode)
label(exit)

newmem: //这是已分配内存，您拥有读取、写入和执行权限
//将您的代码放在这里
// 保存寄存器
  push rax
  push rsi

  // 修改 rsi 指向的 CViewSetup 结构体
  mov dword ptr [rsi+4A0], (float)-1000.0   // X 坐标
  mov dword ptr [rsi+4A4], (float)-650.0   // Y 坐标
  mov dword ptr [rsi+4A8], (float)100.0   // Z 坐标

  // 修改角度 (Pitch, Yaw, Roll)
  mov dword ptr [rsi+4B8], (float)30.0      // Pitch
  mov dword ptr [rsi+4BC], (float)0.0     // Yaw
  mov dword ptr [rsi+4C0], (float)0.0      // Roll

  // 修改 FOV
  mov dword ptr [rsi+498], (float)90.0     // FOV

  // 恢复寄存器
  pop rsi
  pop rax

originalcode:
mov rcx,[client.dll+2303AE8]

exit:
jmp returnhere

"client.dll"+B80CD9:
jmp newmem
nop 2
returnhere:

