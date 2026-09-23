# 验收记录

日期：2026-09-22。此记录区分实际执行结果和仍需在 Linux 上完成的验收。

## 本机已执行

环境：Windows x64、MSVC 19.29、CMake 3.20、Ninja 1.10、Python 3.12。

| 检查 | 结果 |
| --- | --- |
| CMake 配置和两个标准线程目标的 Debug 构建 | 通过，无编译警告 |
| std_thread CTest | 通过，正常退出并输出完成消息 |
| thread_sync CTest | 通过，核对 40 次操作的 FIFO、数量和容量 |
| BUILD_TESTING=OFF 配置 | 通过，不查找 Python 测试依赖 |
| 同时开启 TSan 和 ASan/UBSan | 按预期拒绝配置，提示使用不同构建目录 |
| Python 测试脚本语法 | 通过 |
| 三份学习文档的本地链接 | 通过 |

本机未安装 WSL，也未提供可连接的 Linux 主机信息。Windows 构建跳过四个 Linux 示例，因此不能将上述 2/2 通过解释为全部六个程序已在 Linux 验收。

## Linux 上待执行

从项目根目录运行以下命令；依赖安装和详细说明见 README。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
cmake -E chdir build ctest --output-on-failure

cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON
cmake --build build-tsan --parallel
cmake -E chdir build-tsan ctest -L thread --output-on-failure

cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN_UBSAN=ON
cmake --build build-asan --parallel
cmake -E chdir build-asan ctest --output-on-failure
```

验收目标：普通构建 17/17 项通过，TSan 的 3 项线程测试通过，ASan/UBSan 的 17 项测试通过且无检测报告。当前尚无这些 Linux 实测结果。

测试包含正常运行、消息边界和 exec 的错误/信号状态传播；不宣称已经穷举资源耗尽等所有系统调用失败情形。
