# AMPHOREUS 项目上下文文档

## 项目概述

**AMPHOREUS**（也称为 AMPH0REUS）是一个生物随机性模拟系统，使用 C++ 编写。该系统模拟生物代理的自我迭代与学习过程，采用 Q-learning 强化学习算法，并集成了 LLM（大语言模型）支持。

### 核心特性
- **生物代理 (BioAgent)**：具有 Q-learning 学习能力的智能体
- **模拟环境 (SimulationEnvironment)**：包含随机事件的模拟环境
- **LLM 集成 (LLMClient)**：支持 OpenAI API 交互，生成智能随机事件
- **经验保存**：代理停止运行时自动保存学习经验到 `exp/` 文件夹
- **状态存档**：模拟结束时保存完整状态到 `ws/` 文件夹
- **紧急终止**：支持终端输入 "SAC" 强行终止模拟并暂时存档

## 技术栈
- **编程语言**: C++20
- **构建系统**: CMake 或 Visual Studio 编译器
- **操作系统**: Windows（使用 Windows API 创建目录）
- **编译器**: 支持 C++20 的编译器（MSVC、Clang、GCC）
- **外部依赖**: 可选的 OpenAI API 集成

## 快速开始

### 构建项目
```bash
# 方法1：使用 CMake 构建（推荐）
mkdir build
cd build
cmake ..
cmake --build .

# 方法2：使用 Visual Studio 编译器
# 运行 compile_vs.bat 或 compile_vs_short.bat

# 方法3：使用 PowerShell 脚本
# 运行 build.ps1

# 方法4：使用批处理脚本
# 运行 build.bat
```

### 运行模拟
```bash
# 运行生成的可执行文件
./AMPH0REUS.exe  # 或 ./AMPHOREUS.exe

# 注意：项目名称中的 "0" 是数字零，不是字母 "O"
```

### 示例流程
1. 运行程序后选择菜单选项 1（开始新模拟）
2. 设置模拟参数（步数、随机事件概率）
3. 观察模拟运行，代理将自动学习
4. 模拟结束后检查 `exp/` 和 `ws/` 目录中的输出文件

## 项目结构

```
AMPHOREUS/
├── main.cpp                    # 主程序入口
├── BioAgent.h/cpp             # 生物代理类定义与实现
├── SimulationEnvironment.h/cpp # 模拟环境类定义与实现
├── LLMClient.h/cpp            # LLM客户端类定义与实现（新增）
├── CMakeLists.txt             # CMake 构建配置
├── config.json                # LLM客户端配置文件（新增）
├── README.txt                 # 项目说明文档
├── build.bat                  # 批处理构建脚本
├── build.ps1                  # PowerShell构建脚本
├── compile_vs.bat             # Visual Studio编译脚本
├── compile_vs_short.bat       # 简化版VS编译脚本
├── compile_sim.bat            # 仅编译模拟环境
├── compile_sim_only.bat       # 仅编译模拟环境（简化）
├── compile_test.bat           # 测试编译脚本
├── exp/                       # 经验文件目录（代理学习数据）
├── ws/                        # 存档文件目录（模拟状态）
└── cmake-build-debug/         # CMake构建输出目录
    ├── exp/                   # 实际经验文件目录
    ├── ws/                    # 实际存档文件目录
    └── AMPH0REUS.exe          # 生成的可执行文件
```

## 详细使用说明

### 主菜单选项
1. **开始新模拟** - 设置模拟步数和随机事件概率
   - *盗火者条件*：只有盗火者存活且是唯一幸存者时才能开始新模拟
2. **加载存档** - 从 `ws/` 文件夹加载之前保存的状态
3. **显示当前状态** - 查看当前模拟状态
4. **退出** - 退出程序

### 模拟控制
- **正常结束**: 模拟按设定步数完成后自动保存
- **紧急终止**: 模拟过程中输入 "SAC" 可强行终止并暂时存档
- **自动保存**: 代理停止运行时自动保存学习经验
- **LLM 集成**: 当配置了 OpenAI API 密钥时，使用 LLM 生成智能随机事件

## 技术实现细节

### 生物代理 (BioAgent)
- **学习算法**: Q-learning 强化学习
- **探索策略**: epsilon-greedy 策略
- **状态空间**: 4维（位置、能量系数、时间）
- **动作空间**: 3个动作（左、右、保持）
- **能量系统**: 代理具有能量值，影响生存和学习能力
- **经验管理**: 自动保存和加载学习经验
- **决策向量**: 12维决策向量，用于与 LLM 交互

### 模拟环境 (SimulationEnvironment)
- **代理数量**: 12个生物代理
- **随机事件**: 每步有概率发生随机事件，改变代理状态
- **多线程**: 支持输入处理线程，可响应 SAC 命令
- **状态管理**: 维护模拟计数和代理状态
- **存档系统**: 完整的模拟状态保存和恢复
- **LLM 集成**: 通过 LLMClient 生成智能随机事件

### LLM 客户端 (LLMClient) - 新增功能
- **API 集成**: 支持 OpenAI API（GPT-3.5-turbo 等模型）
- **随机事件生成**: 使用 LLM 生成有意义的随机事件和选项
- **决策支持**: 当代理无法做出决策时，使用 LLM 提供建议
- **模拟模式**: 当没有 API 密钥时，使用内置模拟模式
- **配置管理**: 通过 config.json 文件管理 API 设置

### 文件系统
- **经验文件**: `exp/agent_{id}_experience.txt`（版本 v4.0）
  - 包含代理的 Q 表、能量状态、重要事件记录、模拟计数
- **存档文件**: `ws/archive_{count}.txt`
  - 包含完整模拟状态、所有代理信息、时间戳
- **配置文件**: `config.json`
  - OpenAI API 配置、超时设置、重试次数
- **文件格式**: 文本格式，便于调试和分析

## 配置说明

### config.json 配置
```json
{
  "openai_api_key": "your_api_key_here",
  "openai_model": "gpt-3.5-turbo",
  "openai_base_url": "https://api.openai.com/v1",
  "decision_vector_dimensions": 12,
  "initial_decision_vector_value": 1.0,
  "llm_timeout_seconds": 30,
  "max_retries": 3
}
```

**配置说明**：
- `openai_api_key`: 你的 OpenAI API 密钥（留空则使用模拟模式）
- `openai_model`: 使用的 LLM 模型
- `decision_vector_dimensions`: 决策向量维度（默认为12）
- `llm_timeout_seconds`: API 请求超时时间
- `max_retries`: 最大重试次数

## 开发约定

### 代码风格
- 使用 C++20 标准
- 头文件使用 `#pragma once`
- 类成员变量使用驼峰命名法
- 包含必要的错误处理和边界检查
- 使用异常处理确保程序稳定性
- 添加详细的代码注释

### 构建配置
- CMake 最低版本要求 4.1
- 设置 C++20 标准
- 可执行文件名为 `AMPH0REUS`（数字0）
- 包含所有必要的源文件依赖
- 支持多种构建方式（CMake、VS编译器、脚本）

### 平台依赖
- 使用 Windows API (`windows.h`) 创建目录
- 设置控制台代码页为 UTF-8 支持中文输出
- 确保目录创建和文件操作兼容性
- 支持 Visual Studio 开发工具链

## 学习算法参数

### Q-learning 参数
- **学习率 (learningRate)**: 0.1
- **折扣因子 (discountFactor)**: 0.9
- **初始探索率 (explorationRate)**: 1.0
- **探索衰减 (explorationDecay)**: 0.995
- **最小探索率 (minExplorationRate)**: 0.01

### 代理参数
- **初始能量**: 1000.0
- **最大能量**: 1000.0
- **能量消耗**: 每步消耗 1.0 能量
- **死亡条件**: 能量 ≤ 0
- **状态维度**: 4（位置 x, 位置 y, 能量系数, 时间）
- **动作数量**: 3（左移, 右移, 保持）
- **决策向量维度**: 12（用于 LLM 交互）

## 构建脚本说明

### 可用构建脚本
1. **build.bat** - 基本的 Visual Studio 编译
2. **build.ps1** - PowerShell 编译脚本，输出到 cl_output.txt
3. **compile_vs.bat** - 完整的 Visual Studio 编译（包含 LLMClient）
4. **compile_vs_short.bat** - 简化版 Visual Studio 编译
5. **compile_sim.bat** - 仅编译 SimulationEnvironment
6. **compile_sim_only.bat** - 简化版模拟环境编译
7. **compile_test.bat** - 测试编译

### 构建输出文件
- **cl_output.txt** - PowerShell 编译输出日志
- **compile_output.txt** - 编译输出
- **sim_compile.txt** - 模拟环境编译输出
- **sim_errors.txt** - 模拟环境编译错误
- **test_errors.txt** - 测试编译错误

## 调试与测试

### 调试输出
- 程序启动时输出 "程序启动..."
- 关键操作有日志输出到标准错误流
- 可查看代理的 Q 表进行调试：`agent.getQTable()`
- 模拟状态检查：`env.isRunning()`, `agent.isAlive()`
- LLM 连接测试：`LLMClient::getInstance().testConnection()`

### 状态监控
```cpp
// 检查模拟运行状态
if (env.isRunning()) {
    std::cout << "模拟正在运行..." << std::endl;
}

// 检查代理存活状态
if (agent.isAlive()) {
    std::cout << "代理存活，当前能量: " << agent.getEnergy() << std::endl;
}

// 检查 LLM 连接
if (LLMClient::getInstance().testConnection()) {
    std::cout << "LLM 连接正常" << std::endl;
}
```

### 文件验证
- 存档加载时验证文件完整性
- 经验文件格式一致性检查（版本 v4.0）
- 目录权限和可访问性验证
- 配置文件格式验证

### 常见问题排查
1. **构建失败**：确保 CMake 版本 ≥ 4.1，编译器支持 C++20
2. **运行时错误**：检查 `exp/` 和 `ws/` 目录是否可写
3. **代理不学习**：查看经验文件是否正常生成
4. **存档加载失败**：验证存档文件格式和内容
5. **LLM 连接失败**：检查 config.json 配置和网络连接
6. **名称混淆**：注意 `AMPH0REUS`（数字0）和 `AMPHOREUS`（字母O）的区别

## 注意事项

### 重要提醒
1. **随机性**: 模拟包含随机事件，每次运行结果可能不同
2. **目录权限**: 确保程序有权限创建 `exp/` 和 `ws/` 目录
3. **文件访问**: 避免在模拟运行时手动修改经验文件
4. **内存管理**: 注意多线程环境下的资源管理
5. **兼容性**: 使用 Windows 特定 API，在其他平台可能需要调整
6. **性能考虑**: 大量代理或长时间模拟可能影响性能
7. **LLM 成本**: 使用 OpenAI API 可能产生费用，注意使用量
8. **网络依赖**: LLM 功能需要网络连接

### 最佳实践
- 定期清理 `exp/` 和 `ws/` 目录中的旧文件
- 备份重要的存档文件
- 监控模拟过程中的资源使用情况
- 使用版本控制管理源代码和重要存档
- 在测试环境中验证 LLM 功能后再用于生产
- 合理配置 API 超时和重试参数

## 扩展建议

### 功能增强
1. **图形界面**：添加图形界面显示模拟过程
2. **更多算法**：支持更多强化学习算法（如 DQN、PPO）
3. **环境复杂度**：增加状态维度或环境变量
4. **统计分析**：添加详细的统计分析和可视化
5. **跨平台**：支持 Linux/macOS 平台
6. **本地 LLM**：集成本地 LLM 模型（如 Llama、Phi）
7. **多语言支持**：支持更多语言的事件生成

### 性能优化
1. **并行计算**：利用多线程加速模拟过程
2. **内存优化**：优化 Q 表存储和访问
3. **文件压缩**：对经验文件和存档进行压缩
4. **增量学习**：支持增量式经验加载和学习
5. **缓存机制**：缓存 LLM 响应，减少 API 调用
6. **批量处理**：批量处理代理决策，提高效率

### 维护要点
- 保持 CMake 配置的简洁和可维护性
- 定期更新依赖和编译工具链
- 编写单元测试确保核心功能稳定
- 维护清晰的代码文档和注释
- 监控 API 使用情况和成本
- 保持向后兼容性，特别是存档文件格式

## API 参考

### BioAgent 类关键方法
- `initialize(int stateSize, int actionSize)` - 初始化代理
- `chooseAction(const std::vector<double>& state)` - 选择动作
- `learn(...)` - Q-learning 更新
- `saveExperience(const std::string& filepath)` - 保存经验
- `loadExperience(const std::string& filepath)` - 加载经验
- `getDecisionVector()` - 获取决策向量（12维）
- `updateDecisionVector(...)` - 更新决策向量

### SimulationEnvironment 类关键方法
- `initialize(int stateSize, int actionSize)` - 初始化环境
- `runSimulation()` - 运行模拟
- `stopSimulation(bool forced)` - 停止模拟
- `saveArchive(const std::string& filepath)` - 保存存档
- `loadArchive(const std::string& filepath)` - 加载存档
- `canStartNewSimulation()` - 检查是否可以开始新模拟
- `getSimulationCount()` - 获取模拟计数

### LLMClient 类关键方法 - 新增
- `getInstance()` - 获取单例实例
- `initialize(const std::string& configPath)` - 初始化客户端
- `generateRandomEvent()` - 生成随机事件
- `getLLMChoice(...)` - 获取 LLM 决策建议
- `testConnection()` - 测试 API 连接
- `isSimulationMode()` - 检查是否处于模拟模式

---

*本文档基于项目代码和实际项目状态生成，最后更新于 2025年12月23日*

**版本历史**：
- v1.0: 初始版本，基础 Q-learning 模拟
- v2.0: 添加经验保存和存档系统
- v3.0: 添加多线程和紧急终止功能
- v4.0: 集成 LLM 客户端，支持智能随机事件生成
- 当前版本：v4.0+（包含多种构建脚本和配置优化）