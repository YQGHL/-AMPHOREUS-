#include <iostream>
#include "LLMClient.h"
#include <windows.h>

int main() {
    // 设置控制台输出为UTF-8编码
    SetConsoleOutputCP(65001);
    
    std::cout << "=== LLM事件生成功能测试 ===" << std::endl;
    std::cout << "API地址: http://192.168.1.105:1234" << std::endl;
    std::cout << "模型: qwen3-4b-thinking-2507" << std::endl;
    std::cout << "超时: 300秒" << std::endl;
    std::cout << "\n正在初始化LLM客户端..." << std::endl;
    
    // 初始化LLM客户端
    if (!LLMClient::getInstance().initialize("config.json")) {
        std::cerr << "LLM客户端初始化失败!" << std::endl;
        return 1;
    }
    
    std::cout << "LLM客户端初始化成功" << std::endl;
    
    // 测试连接
    std::cout << "\n=== 测试API连接 ===" << std::endl;
    if (!LLMClient::getInstance().testConnection()) {
        std::cerr << "API连接测试失败，但可能仍可使用保存的事件" << std::endl;
    } else {
        std::cout << "API连接测试成功!" << std::endl;
    }
    
    // 测试事件生成
    std::cout << "\n=== 测试事件生成 ===" << std::endl;
    std::cout << "正在请求LLM生成包含10个选项的随机事件..." << std::endl;
    
    try {
        LLMClient::RandomEvent event = LLMClient::getInstance().generateRandomEvent();
        
        std::cout << "\n事件生成成功!" << std::endl;
        std::cout << "事件名称: " << event.name << std::endl;
        std::cout << "事件描述: " << event.description << std::endl;
        std::cout << "选项数量: " << event.options.size() << std::endl;
        
        if (!event.options.empty()) {
            std::cout << "\n第一个选项详情:" << std::endl;
            std::cout << "  文本: " << event.options[0].text << std::endl;
            std::cout << "  结果描述: " << event.options[0].outcomeText << std::endl;
            std::cout << "  决策要求向量维度: " << event.options[0].decisionRequirement.size() << std::endl;
            std::cout << "  决策反馈向量维度: " << event.options[0].decisionFeedback.size() << std::endl;
        }
        
        std::cout << "\n=== 测试总结 ===" << std::endl;
        std::cout << "1. LLM客户端初始化: 成功" << std::endl;
        std::cout << "2. API连接测试: " << (LLMClient::getInstance().testConnection() ? "成功" : "失败") << std::endl;
        std::cout << "3. 事件生成: 成功" << std::endl;
        std::cout << "4. 事件验证: " << (event.options.size() == 10 ? "10个选项✓" : "选项数量不匹配") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n事件生成失败: " << e.what() << std::endl;
        std::cerr << "LLM事件生成功能测试失败!" << std::endl;
        return 1;
    }
    
    std::cout << "\nLLM事件生成功能测试通过!" << std::endl;
    std::cout << "\n按Enter键退出...";
    std::cin.get();
    
    return 0;
}