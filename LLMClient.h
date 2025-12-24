#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

// LLM客户端，用于与OpenAI API交互
class LLMClient {
public:
    // 单例模式获取实例
    static LLMClient& getInstance();
    
    // 初始化LLM客户端，从配置文件加载设置
    bool initialize(const std::string& configPath = "config.json");
    
    // 获取随机事件描述和选项
    // 返回: 事件描述和选项列表，每个选项包括文本和决策要求向量
    struct EventOption {
        std::string text;
        std::vector<double> decisionRequirement; // 12维决策要求
        std::vector<double> decisionFeedback;    // 12维决策反馈
        std::string outcomeText;
    };
    
    struct RandomEvent {
        std::string name;
        std::string description;
        std::vector<EventOption> options;
    };
    
    // 生成随机事件
    RandomEvent generateRandomEvent();
    
    // 当没有符合的选项时，提交决策向量给LLM，获取选择
    // 参数: agentId, 决策向量(12维), 事件描述, 选项列表
    // 返回: 选择的选项索引，或-1表示无法选择
    int getLLMChoice(int agentId, const std::vector<double>& decisionVector,
                     const std::string& eventDescription,
                     const std::vector<EventOption>& options);
    
    // 检查API连接
    bool testConnection();
    
    // 获取保存的LLM生成事件（用于模拟模式下的备用事件）
    RandomEvent getSavedRandomEvent();
    
private:
    LLMClient() = default;
    LLMClient(const LLMClient&) = delete;
    LLMClient& operator=(const LLMClient&) = delete;
    
    // OpenAI API配置
    std::string apiKey;
    std::string model;
    std::string baseUrl;
    std::string systemPrompt;  // 系统提示，用于生成随机事件
    int timeoutSeconds;
    int maxRetries;
    
    // 模拟模式（当没有API密钥时）
    bool simulationMode;
    
    // 保存的LLM生成事件（作为备用事件）
    std::vector<RandomEvent> savedEvents;
    
    // 发送HTTP请求到OpenAI API
    std::string sendRequest(const std::string& endpoint, const std::string& body);
    
    // 解析LLM响应
    RandomEvent parseEventResponse(const std::string& response);
    int parseChoiceResponse(const std::string& response);
    
    // 生成模拟事件（当simulationMode为true时）
    RandomEvent generateSimulatedEvent();
    
    // 生成模拟选择
    int generateSimulatedChoice(int agentId, const std::vector<double>& decisionVector,
                                const std::vector<EventOption>& options);
    
    // 保存事件到文件
    void saveEventToFile(const RandomEvent& event);
    
    // 从文件加载保存的事件
    void loadSavedEvents();
    
    // 检查事件是否符合要求（10个选项，每个选项有12维向量）
    bool validateEvent(const RandomEvent& event);
    
    // 从JSON内容解析事件（用于加载保存的文件）
    RandomEvent parseEventFromJsonContent(const std::string& jsonContent);
};