#include "SimulationEnvironment.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <windows.h>
#include <chrono>
#include <ctime>
#include <cmath>
#include <conio.h>  // 用于_kbhit和_getch
#include <cstdlib>  // 用于system()
#include <thread>   // 用于this_thread::sleep_for
#include <iomanip>

// 构造函数
SimulationEnvironment::SimulationEnvironment() 
    : running(false), eventCount(0), randomEventProb(0.3) {
    rng.seed(std::random_device{}());
    probDist = std::uniform_real_distribution<double>(0.0, 1.0);
    
    // 初始化代理
    agents.resize(NUM_AGENTS);
    for (int i = 0; i < NUM_AGENTS; ++i) {
        agents[i] = BioAgent(i);
    }
    
    // 初始化事件系统
    initializeEvents();
    
    // 初始化LLM客户端（可选）
    LLMClient::getInstance().initialize("config.json");
}

// 析构函数
SimulationEnvironment::~SimulationEnvironment() {
    stopSimulation();
}

// 初始化模拟环境
void SimulationEnvironment::initialize() {
    // 创建必要的目录（如果不存在）
    CreateDirectoryA("exp", NULL);
    CreateDirectoryA("ws", NULL);
    
    // 重置事件计数
    eventCount = 0;
    eventHistory.clear();
    
    std::cout << "模拟环境初始化完成，共有 " << NUM_AGENTS << " 个代理。" << std::endl;
}

// 运行事件模拟
void SimulationEnvironment::runEventSimulation(int numEvents) {
    if (running) {
        std::cout << "模拟已经在运行中。" << std::endl;
        return;
    }
    
    running = true;
    eventCount = 0;
    eventHistory.clear();
    
    std::cout << "开始事件模拟，计划执行 " << numEvents << " 个事件。" << std::endl;
    std::cout << "随机事件概率: " << randomEventProb << std::endl;
    std::cout << "==========================================" << std::endl;
    
    for (int i = 0; i < numEvents && running; ++i) {
        std::cout << "\n事件 #" << (i + 1) << ":" << std::endl;
        
        // 生成随机事件
        ChoiceEvent event = generateRandomEvent();
        
        // 处理事件
        processEvent(event);
        
        eventCount++;
        
        // 显示当前代理状态
        if ((i + 1) % 5 == 0) {
            std::cout << "\n--- 第 " << (i + 1) << " 个事件后的代理状态 ---" << std::endl;
            for (int j = 0; j < std::min(3, NUM_AGENTS); ++j) {
                std::cout << "代理 " << j << ": " << getAgentDecisionVectorString(j) << std::endl;
            }
        }
    }
    
    running = false;
    std::cout << "\n==========================================" << std::endl;
    std::cout << "事件模拟完成，共处理 " << eventCount << " 个事件。" << std::endl;
    
    // 保存事件历史
    saveEventHistory();
}

// 运行交互式模拟（实时显示代理状态）
void SimulationEnvironment::runInteractiveSimulation(int numEvents) {
    if (running) {
        std::cout << "模拟已经在运行中。" << std::endl;
        return;
    }
    
    running = true;
    eventCount = 0;
    eventHistory.clear();
    
    std::cout << "开始交互式模拟，计划执行 " << numEvents << " 个事件。" << std::endl;
    std::cout << "随机事件概率: " << randomEventProb << std::endl;
    std::cout << "按任意键开始，模拟过程中按 'q' 键退出..." << std::endl;
    std::cin.get();
    
    // 清屏并显示初始状态
    system("cls");
    
    for (int i = 0; i < numEvents && running; ++i) {
        // 生成并处理事件，但不显示事件详情
        ChoiceEvent event = generateRandomEvent();
        
        // 随机选择一个代理参与事件
        int agentId = getRandomInt(0, NUM_AGENTS - 1);
        
        // 代理选择选项
        int optionIndex = selectOptionForAgent(agents[agentId], event);
        
        if (optionIndex >= 0 && optionIndex < event.options.size()) {
            // 应用事件结果
            applyEventOutcome(agentId, event.options[optionIndex]);
            
            // 记录事件（但不显示）
            std::stringstream record;
            record << "事件#" << eventCount + 1 << ": " << event.name 
                   << " | 代理#" << agentId << " 选择了: " << event.options[optionIndex].text;
            recordEvent(record.str());
        }
        
        eventCount++;
        
        // 更新显示
        std::cout << "==========================================" << std::endl;
        std::cout << "     决策向量与随机事件模拟系统 (交互模式)     " << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "事件进度: " << (i + 1) << " / " << numEvents << std::endl;
        std::cout << "当前事件: " << event.name << std::endl;
        std::cout << "参与代理: " << agentId << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "所有代理的当前状态:" << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        
        // 显示所有代理的详细状态
        auto statusList = getAllAgentsDetailedStatus();
        for (const auto& status : statusList) {
            std::cout << status << std::endl;
        }
        
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "按 'q' 键退出模拟，或等待下一个事件..." << std::endl;
        
        // 检查用户输入（非阻塞）
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 'q' || ch == 'Q') {
                std::cout << "\n用户请求退出模拟。" << std::endl;
                break;
            }
        }
        
        // 短暂暂停以便观察
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // 清屏为下一次更新做准备
        system("cls");
    }
    
    running = false;
    
    // 最终状态显示
    system("cls");
    std::cout << "==========================================" << std::endl;
    std::cout << "     交互式模拟完成     " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "共处理 " << eventCount << " 个事件。" << std::endl;
    std::cout << "\n最终代理状态:" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    auto finalStatus = getAllAgentsDetailedStatus();
    for (const auto& status : finalStatus) {
        std::cout << status << std::endl;
    }
    
    // 保存事件历史
    saveEventHistory();
    
    std::cout << "\n按任意键返回主菜单..." << std::endl;
    std::cin.get();
}

// 添加用户自定义事件
void SimulationEnvironment::addUserEvent(const std::string& name, const std::string& description,
                                        const std::vector<std::tuple<std::string, std::vector<double>, std::string>>& options) {
    ChoiceEvent newEvent;
    newEvent.name = name;
    newEvent.description = description;
    
    for (const auto& optionTuple : options) {
        EventOption option;
        option.text = std::get<0>(optionTuple);
        option.decisionRequirement = std::get<1>(optionTuple);
        option.outcomeText = std::get<2>(optionTuple);
        
        // 生成随机反馈向量
        option.decisionFeedback = generateRandomFeedbackVector();
        
        newEvent.options.push_back(option);
    }
    
    userEvents.push_back(newEvent);
    std::cout << "用户事件 '" << name << "' 已添加，包含 " << newEvent.options.size() << " 个选项。" << std::endl;
}

// 获取指定代理的决策向量字符串
std::string SimulationEnvironment::getAgentDecisionVectorString(int agentId) const {
    if (agentId < 0 || agentId >= agents.size()) {
        return "无效的代理ID";
    }
    return agents[agentId].getDecisionVectorString();
}

// 获取所有代理的详细状态
std::vector<std::string> SimulationEnvironment::getAllAgentsDetailedStatus() const {
    std::vector<std::string> statusList;
    
    for (const auto& agent : agents) {
        std::stringstream ss;
        ss << "代理 " << agent.getId() << ": ";
        
        const std::vector<double>& decisionVec = agent.getDecisionVector();
        const char* dimensionNames[BioAgent::DECISION_VECTOR_DIMENSIONS] = {
            "快乐", "悲伤", "愤怒", "恐惧", "厌恶", "惊讶",
            "信任", "期待", "宁静", "效价", "唤醒度", "优势度"
        };
        
        ss << "[";
        for (int i = 0; i < BioAgent::DECISION_VECTOR_DIMENSIONS; ++i) {
            ss << std::fixed << std::setprecision(2);
            ss << dimensionNames[i] << ":" << decisionVec[i];
            if (i < BioAgent::DECISION_VECTOR_DIMENSIONS - 1) {
                ss << ", ";
            }
        }
        ss << "]";
        
        statusList.push_back(ss.str());
    }
    
    return statusList;
}

// 初始化事件系统
void SimulationEnvironment::initializeEvents() {
    events.clear();
    userEvents.clear();
    
    // 添加一些默认事件
    ChoiceEvent event1;
    event1.name = "情感冲突";
    event1.description = "代理面临情感冲突，需要在不同情感维度之间做出选择";
    
    EventOption option1a;
    option1a.text = "选择快乐路径";
    option1a.decisionRequirement = {0.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    option1a.decisionFeedback = {0.1, -0.05, -0.05, -0.05, -0.05, 0.0, 0.0, 0.0, 0.05, 0.05, 0.0, 0.0};
    option1a.outcomeText = "选择了快乐，心情变好但可能忽略了其他情感";
    
    EventOption option1b;
    option1b.text = "保持情感平衡";
    option1b.decisionRequirement = {0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3};
    option1b.decisionFeedback = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.05, 0.05, 0.1, 0.0, 0.0, 0.0};
    option1b.outcomeText = "保持了情感平衡，获得内心的宁静";
    
    event1.options = {option1a, option1b};
    events.push_back(event1);
    
    std::cout << "事件系统初始化完成，已加载 " << events.size() << " 个默认事件。" << std::endl;
}

// 生成随机事件
SimulationEnvironment::ChoiceEvent SimulationEnvironment::generateRandomEvent() {
    // 优先尝试使用LLM生成事件
    try {
        std::cout << "正在尝试从LLM获取随机事件..." << std::endl;
        ChoiceEvent llmEvent = generateLLMEvent();
        std::cout << "成功从LLM获取事件: " << llmEvent.name << std::endl;
        return llmEvent;
    } catch (const std::exception& e) {
        std::cerr << "LLM事件生成失败: " << e.what() << std::endl;
        std::cout << "回退到简单事件生成..." << std::endl;
        return generateSimpleEvent();
    }
}

// 处理事件
void SimulationEnvironment::processEvent(const ChoiceEvent& event) {
    std::cout << "\n事件: " << event.name << std::endl;
    std::cout << "描述: " << event.description << std::endl;
    std::cout << "选项:" << std::endl;
    
    for (size_t i = 0; i < event.options.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << event.options[i].text << std::endl;
    }
    
    // 随机选择一个代理参与事件
    int agentId = getRandomInt(0, NUM_AGENTS - 1);
    std::cout << "代理 " << agentId << " 参与此事件。" << std::endl;
    
    // 代理选择选项
    int optionIndex = selectOptionForAgent(agents[agentId], event);
    
    if (optionIndex >= 0 && optionIndex < event.options.size()) {
        std::cout << "代理选择了选项: " << event.options[optionIndex].text << std::endl;
        std::cout << "结果: " << event.options[optionIndex].outcomeText << std::endl;
        
        // 应用事件结果
        applyEventOutcome(agentId, event.options[optionIndex]);
        
        // 记录事件
        std::stringstream record;
        record << "事件#" << eventCount + 1 << ": " << event.name 
               << " | 代理#" << agentId << " 选择了: " << event.options[optionIndex].text
               << " | 结果: " << event.options[optionIndex].outcomeText;
        recordEvent(record.str());
    } else {
        std::cout << "代理无法做出选择。" << std::endl;
    }
}

// 为代理选择选项
int SimulationEnvironment::selectOptionForAgent(const BioAgent& agent, const ChoiceEvent& event) {
    // 首先检查是否有满足决策要求的选项
    std::vector<int> validOptions;
    for (size_t i = 0; i < event.options.size(); ++i) {
        if (agent.checkDecisionRequirement(event.options[i].decisionRequirement)) {
            validOptions.push_back(i);
        }
    }
    
    if (!validOptions.empty()) {
        // 随机选择一个有效选项
        return validOptions[getRandomInt(0, validOptions.size() - 1)];
    }
    
    // 如果没有满足要求的选项，使用LLM帮助选择
    if (LLMClient::getInstance().testConnection()) {
        std::vector<double> decisionVec = agent.getDecisionVector();
        std::vector<LLMClient::EventOption> llmOptions;
        
        for (const auto& option : event.options) {
            LLMClient::EventOption llmOption;
            llmOption.text = option.text;
            llmOption.decisionRequirement = option.decisionRequirement;
            llmOption.decisionFeedback = option.decisionFeedback;
            llmOption.outcomeText = option.outcomeText;
            llmOptions.push_back(llmOption);
        }
        
        return LLMClient::getInstance().getLLMChoice(
            agent.getId(), decisionVec, event.description, llmOptions);
    }
    
    // 如果LLM也不可用，随机选择
    return getRandomInt(0, event.options.size() - 1);
}

// 应用事件结果
void SimulationEnvironment::applyEventOutcome(int agentId, const EventOption& option) {
    if (agentId < 0 || agentId >= agents.size()) {
        return;
    }
    
    agents[agentId].updateDecisionVector(option.decisionFeedback);
    
    // 显示决策向量变化
    std::cout << "代理 " << agentId << " 的决策向量已更新。" << std::endl;
}

// 生成简单事件
SimulationEnvironment::ChoiceEvent SimulationEnvironment::generateSimpleEvent() {
    ChoiceEvent event;
    
    // 事件主题列表
    std::vector<std::string> eventThemes = {
        "情感挑战", "道德困境", "社交互动", "自我反思", 
        "环境适应", "压力应对", "目标设定", "风险决策"
    };
    
    std::vector<std::string> eventDescriptions = {
        "面对复杂的情感状况，需要做出选择",
        "遇到道德困境，需要在不同价值观之间权衡",
        "在社交场合中需要做出适当的反应",
        "需要反思自己的行为和情感状态",
        "环境发生变化，需要适应新的情况",
        "面临压力，需要找到应对策略",
        "需要设定新的目标并做出决策",
        "面临风险，需要在安全与机会之间选择"
    };
    
    int themeIndex = getRandomInt(0, eventThemes.size() - 1);
    event.name = eventThemes[themeIndex];
    event.description = eventDescriptions[themeIndex];
    
    // 生成2-3个选项
    int numOptions = getRandomInt(2, 3);
    for (int i = 0; i < numOptions; ++i) {
        EventOption option;
        
        std::vector<std::string> optionTexts = {
            "注重情感表达", "保持理性思考", "寻求平衡", "冒险尝试",
            "谨慎行事", "依赖直觉", "参考他人意见", "坚持原则"
        };
        
        std::vector<std::string> outcomes = {
            "这个选择带来了新的视角",
            "选择的结果符合预期",
            "这个决定引发了进一步的思考",
            "选择导致了有趣的发展",
            "决定带来了情感上的满足",
            "这个选择促进了个人成长"
        };
        
        option.text = optionTexts[getRandomInt(0, optionTexts.size() - 1)];
        option.decisionRequirement = generateRandomDecisionVector();
        option.decisionFeedback = generateRandomFeedbackVector();
        option.outcomeText = outcomes[getRandomInt(0, outcomes.size() - 1)];
        
        event.options.push_back(option);
    }
    
    return event;
}

// 使用LLM生成事件
SimulationEnvironment::ChoiceEvent SimulationEnvironment::generateLLMEvent() {
    // 尝试从LLM获取事件
    LLMClient::RandomEvent llmEvent = LLMClient::getInstance().generateRandomEvent();
    
    // 检查事件是否有效
    if (llmEvent.name.empty() || llmEvent.options.empty()) {
        throw std::runtime_error("LLM返回的事件无效");
    }
    
    ChoiceEvent event;
    event.name = llmEvent.name;
    event.description = llmEvent.description;
    
    for (const auto& llmOption : llmEvent.options) {
        EventOption option;
        option.text = llmOption.text;
        option.decisionRequirement = llmOption.decisionRequirement;
        option.decisionFeedback = llmOption.decisionFeedback;
        option.outcomeText = llmOption.outcomeText;
        event.options.push_back(option);
    }
    
    return event;
}

// 随机数生成辅助方法
double SimulationEnvironment::getRandomDouble(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

int SimulationEnvironment::getRandomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

// 生成随机决策向量
std::vector<double> SimulationEnvironment::generateRandomDecisionVector() {
    std::vector<double> vector(BioAgent::DECISION_VECTOR_DIMENSIONS);
    for (int i = 0; i < BioAgent::DECISION_VECTOR_DIMENSIONS; ++i) {
        vector[i] = getRandomDouble(0.0, 0.8); // 要求值通常较低
    }
    return vector;
}

// 生成随机反馈向量
std::vector<double> SimulationEnvironment::generateRandomFeedbackVector() {
    std::vector<double> vector(BioAgent::DECISION_VECTOR_DIMENSIONS);
    for (int i = 0; i < BioAgent::DECISION_VECTOR_DIMENSIONS; ++i) {
        // 反馈值在-0.2到0.2之间
        vector[i] = getRandomDouble(-0.2, 0.2);
    }
    return vector;
}

// 记录事件
void SimulationEnvironment::recordEvent(const std::string& eventRecord) {
    eventHistory.push_back(eventRecord);
}

// 保存事件历史
void SimulationEnvironment::saveEventHistory() {
    std::ofstream file("ws/event_history.txt");
    if (file.is_open()) {
        file << "事件模拟历史记录" << std::endl;
        
        // 获取当前时间并转换为可读格式
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        file << "模拟时间: " << std::ctime(&now_time);
        
        file << "事件总数: " << eventCount << std::endl;
        file << "==========================================" << std::endl;
        
        for (const auto& record : eventHistory) {
            file << record << std::endl;
        }
        
        file.close();
        std::cout << "事件历史已保存到 ws/event_history.txt" << std::endl;
    }
}