#pragma once

#include "BioAgent.h"
#include "LLMClient.h"
#include <string>
#include <vector>
#include <random>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <chrono>

class SimulationEnvironment {
public:
    // 常量：代理数量
    static constexpr int NUM_AGENTS = 12;
    
    // 构造函数
    SimulationEnvironment();
    ~SimulationEnvironment();
    
    // 初始化模拟环境
    void initialize();
    
    // 运行事件模拟
    void runEventSimulation(int numEvents = 10);
    
    // 运行交互式模拟（实时显示代理状态）
    void runInteractiveSimulation(int numEvents = 10);
    
    // 停止模拟
    void stopSimulation() { running = false; }
    
    // 检查模拟是否在运行
    bool isRunning() const { return running; }
    
    // 设置随机事件概率
    void setRandomEventProbability(double prob) { randomEventProb = prob; }
    
    // 添加用户自定义事件
    void addUserEvent(const std::string& name, const std::string& description,
                     const std::vector<std::tuple<std::string, std::vector<double>, std::string>>& options);
    
    // 获取事件历史
    const std::vector<std::string>& getEventHistory() const { return eventHistory; }
    
    // 获取代理列表
    const std::vector<BioAgent>& getAgents() const { return agents; }
    
    // 获取指定代理的决策向量字符串
    std::string getAgentDecisionVectorString(int agentId) const;
    
    // 获取所有代理的详细状态（用于交互式显示）
    std::vector<std::string> getAllAgentsDetailedStatus() const;

private:
    // 简化的事件选项定义
    struct EventOption {
        std::string text;                      // 选项文本
        std::vector<double> decisionRequirement; // 12维决策要求向量
        std::vector<double> decisionFeedback;    // 12维决策反馈向量
        std::string outcomeText;               // 结果描述文本
    };
    
    // 简化的事件定义
    struct ChoiceEvent {
        std::string name;                      // 事件名称
        std::string description;               // 事件描述
        std::vector<EventOption> options;      // 可用选项（2-4个）
    };
    
    // 生物代理集合
    std::vector<BioAgent> agents;
    
    // 模拟状态
    std::atomic<bool> running;
    int eventCount;
    
    // 随机事件参数
    double randomEventProb;
    mutable std::mt19937 rng;
    std::uniform_real_distribution<double> probDist;
    
    // 事件系统
    std::vector<ChoiceEvent> events;
    std::vector<ChoiceEvent> userEvents; // 用户自定义事件
    std::vector<std::string> eventHistory; // 事件历史记录
    
    // 内部方法
    void initializeEvents();
    ChoiceEvent generateRandomEvent();
    void processEvent(const ChoiceEvent& event);
    int selectOptionForAgent(const BioAgent& agent, const ChoiceEvent& event);
    void applyEventOutcome(int agentId, const EventOption& option);
    
    // 简化的事件生成方法
    ChoiceEvent generateSimpleEvent();
    ChoiceEvent generateLLMEvent(); // 使用LLM生成事件
    
    // 随机数生成辅助方法
    double getRandomDouble(double min, double max);
    int getRandomInt(int min, int max);
    
    // 决策向量辅助方法
    std::vector<double> generateRandomDecisionVector();
    std::vector<double> generateRandomFeedbackVector();
    
    // 事件历史记录
    void recordEvent(const std::string& eventRecord);
    void saveEventHistory();
};