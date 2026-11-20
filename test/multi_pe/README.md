# 四 PE common-entry ELF 回归

`run_startup.py` 编译当前 TileOP adapter、指定的 compiler-rt worker 和 `startup.c`，
然后使用 SuperScalarModel 的 `gfrun_hosted_startup_test` 执行 ELF。测试包含 PE0 先到、
worker 先到、缺失 PE0 阻塞后 abort/relaunch、初始化返回错误后 abort/relaunch。
检查独立 PE ID/栈/TLS、进程初始化一次、每次 invocation 准备、初始化可见性和正式 End。
它验证 SSM.PTO v1 freestanding 平台，不代表 Linux 设备或完整 musl 的运行验收。

```bash
python3 test/multi_pe/run_startup.py \
  --compiler /home/wanglulu/developer/0.59/build-llvm/bin/clang \
  --sysroot /home/wanglulu/developer/linx-toolchain-build/output/linx_blockisa_llvm_musl/sysroot \
  --worker /home/wanglulu/developer/0.59/llvm-project/compiler-rt/lib/builtins/linx64v5/group_worker.c \
  --runtime-archive /home/wanglulu/developer/0.59/build-compiler-rt-multi-pe/lib/linux/libclang_rt.builtins-linx64v5.a \
  --model-test /home/wanglulu/developer/0.59/ssm-validation/bin/gfrun_hosted_startup_test \
  --output /home/wanglulu/developer/0.59/multi-pe-validation/startup
```

输出目录记录编译/链接/运行命令、日志、反汇编和输入/ELF SHA256。
模型版本也必须在验收记录中注明。`startup_report[4]` 是测试请求失败的输入，
不用于伪造到达或 release；测试 runner 不修改生产 runtime 的控制流程。
