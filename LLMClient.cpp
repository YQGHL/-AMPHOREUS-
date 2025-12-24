#include "LLMClient.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

// Unicode转换辅助函数
#ifdef _WIN32
namespace {
    std::string wideStringToString(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }
    
    std::wstring stringToWideString(const std::string& str) {
        if (str.empty()) return L"";
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }
}
#endif

// JSON转义辅助函数
namespace {
    std::string escapeJsonString(const std::string& str) {
        std::string escaped;
        escaped.reserve(str.length() * 2); // 粗略估计
        for (char c : str) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '/':  escaped += "\\/"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    // 转义控制字符 (0x00-0x1F)
                    if (c >= 0 && c <= 0x1F) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        escaped += buf;
                    } else {
                        escaped += c;
                    }
                    break;
            }
        }
        return escaped;
    }
}

// JSON解析简化
#include <map>
#include <string>

// 单例实例
LLMClient& LLMClient::getInstance() {
    static LLMClient instance;
    return instance;
}

bool LLMClient::initialize(const std::string& configPath) {
    simulationMode = true; // 默认模拟模式
    apiKey = "";
    model = "gpt-3.5-turbo";
    baseUrl = "https://api.openai.com/v1";
    systemPrompt = "你是一个情感决策模拟系统的事件生成器。请生成一个随机事件，用于12维决策向量的模拟。事件应该包含：\n1. 事件名称（简短描述性名称）\n2. 事件描述（详细说明情境）\n3. 10个选项，每个选项包括：\n   - 选项文本（简短描述选择）\n   - 12维决策要求向量（每个维度值在0.0-1.0之间）\n   - 12维决策反馈向量（每个维度值在-0.2到0.2之间）\n   - 结果描述文本（选择后的结果）\n\n12维决策向量包括：快乐、悲伤、愤怒、恐惧、厌恶、惊讶、信任、期待、宁静、效价、唤醒度、优势度。\n\n请用JSON格式回复，包含以下结构：\n{\n  \"name\": \"事件名称\",\n  \"description\": \"事件描述\",\n  \"options\": [\n    {\n      \"text\": \"选项文本\",\n      \"decisionRequirement\": [0.1, 0.2, ...], // 12个值\n      \"decisionFeedback\": [0.05, -0.1, ...], // 12个值\n      \"outcomeText\": \"结果描述\"\n    }\n  ]\n}";
    timeoutSeconds = 30;
    maxRetries = 3;
    
    // 尝试读取配置文件
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "警告: 无法打开配置文件 " << configPath << "，使用模拟模式。" << std::endl;
        return true; // 模拟模式仍然可用
    }
    
    try {
        // 简单JSON解析（实际项目应使用JSON库）
        std::string line;
        while (std::getline(configFile, line)) {
            if (line.find("\"openai_api_key\"") != std::string::npos) {
                size_t colonPos = line.find(':');
                size_t quoteStart = line.find('"', colonPos + 1);
                size_t quoteEnd = line.find('"', quoteStart + 1);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    apiKey = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                }
            }
            else if (line.find("\"openai_model\"") != std::string::npos) {
                size_t colonPos = line.find(':');
                size_t quoteStart = line.find('"', colonPos + 1);
                size_t quoteEnd = line.find('"', quoteStart + 1);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    model = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                }
            }
            else if (line.find("\"openai_base_url\"") != std::string::npos) {
                size_t colonPos = line.find(':');
                size_t quoteStart = line.find('"', colonPos + 1);
                size_t quoteEnd = line.find('"', quoteStart + 1);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    baseUrl = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                }
            }
            else if (line.find("\"llm_timeout_seconds\"") != std::string::npos) {
                size_t colonPos = line.find(':');
                size_t valueStart = line.find_first_not_of(" \t", colonPos + 1);
                size_t valueEnd = line.find_last_not_of(" \t,}");
                if (valueStart != std::string::npos) {
                    timeoutSeconds = std::stoi(line.substr(valueStart, valueEnd - valueStart + 1));
                }
            }
            else if (line.find("\"max_retries\"") != std::string::npos) {
                size_t colonPos = line.find(':');
                size_t valueStart = line.find_first_not_of(" \t", colonPos + 1);
                size_t valueEnd = line.find_last_not_of(" \t,}");
                if (valueStart != std::string::npos) {
                    maxRetries = std::stoi(line.substr(valueStart, valueEnd - valueStart + 1));
                }
            }
            else if (line.find("\"llm_system_prompt\"") != std::string::npos) {
                size_t colonPos = line.find(':');
                size_t quoteStart = line.find('"', colonPos + 1);
                size_t quoteEnd = line.find('"', quoteStart + 1);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    systemPrompt = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    // 处理转义字符
                    size_t escapePos;
                    while ((escapePos = systemPrompt.find("\\n")) != std::string::npos) {
                        systemPrompt.replace(escapePos, 2, "\n");
                    }
                    while ((escapePos = systemPrompt.find("\\\"")) != std::string::npos) {
                        systemPrompt.replace(escapePos, 2, "\"");
                    }
                    while ((escapePos = systemPrompt.find("\\\\")) != std::string::npos) {
                        systemPrompt.replace(escapePos, 2, "\\");
                    }
                }
            }
        }
        
        configFile.close();
        
        // 决定是否使用模拟模式
        // 规则：如果baseUrl指向本地服务，或apiKey不是默认值，则尝试API模式
        simulationMode = true; // 默认模拟模式
        
        if (!baseUrl.empty()) {
            // 检查是否是本地LLM服务（不需要API密钥）
            bool isLocalService = (baseUrl.find("localhost") != std::string::npos) ||
                                 (baseUrl.find("127.0.0.1") != std::string::npos) ||
                                 (baseUrl.find("192.168.") != std::string::npos) ||
                                 (baseUrl.find("10.") != std::string::npos) ||
                                 (baseUrl.find("172.") != std::string::npos);
            
            // 检查是否是默认OpenAI URL
            bool isDefaultOpenAI = (baseUrl.find("api.openai.com") != std::string::npos);
            
            if (isLocalService) {
                // 本地LLM服务，即使apiKey为默认值也尝试连接
                simulationMode = false;
                std::cout << "检测到本地LLM服务，将尝试API连接" << std::endl;
            } else if (isDefaultOpenAI && apiKey != "your_api_key_here" && !apiKey.empty()) {
                // OpenAI服务且有有效API密钥
                simulationMode = false;
                std::cout << "检测到有效OpenAI API配置" << std::endl;
            } else if (!isDefaultOpenAI && !baseUrl.empty()) {
                // 其他自定义API端点，尝试连接
                simulationMode = false;
                std::cout << "检测到自定义API端点，将尝试连接" << std::endl;
            }
        }
        
        // 加载已保存的LLM生成事件
        loadSavedEvents();
        
        if (!simulationMode) {
            std::cout << "LLM客户端初始化成功，使用API模式。" << std::endl;
            std::cout << "已加载 " << savedEvents.size() << " 个保存的LLM事件作为备用" << std::endl;
        } else {
            std::cout << "LLM客户端使用模拟模式。" << std::endl;
            std::cout << "已加载 " << savedEvents.size() << " 个保存的LLM事件备用" << std::endl;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "配置文件解析错误: " << e.what() << "，使用模拟模式。" << std::endl;
        simulationMode = true;
        return true;
    }
}

bool LLMClient::testConnection() {
    if (simulationMode) {
        std::cout << "模拟模式: 连接测试通过。" << std::endl;
        return true;
    }
    
    // 实际API测试
    std::cout << "\n=== 测试API连接 ===" << std::endl;
    std::cout << "API地址: " << baseUrl << std::endl;
    std::cout << "模型: " << model << std::endl;
    std::cout << "超时: " << timeoutSeconds << "秒" << std::endl;
    std::cout << "正在发送测试请求..." << std::endl;
    
    // 尝试发送一个简单的请求
    std::string testBody = "{\"model\": \"" + model + "\", \"messages\": [{\"role\": \"user\", \"content\": \"Hello\"}], \"stream\": false}";
    std::cout << "测试请求体: " << testBody << std::endl;
    std::string response = sendRequest("v1/chat/completions", testBody);
    
    if (response.empty()) {
        std::cout << "\nAPI连接测试失败: 服务器无响应" << std::endl;
        std::cout << "可能原因: " << std::endl;
        std::cout << "1. 服务器未运行在 " << baseUrl << std::endl;
        std::cout << "2. 网络连接问题" << std::endl;
        std::cout << "3. 防火墙阻止了连接" << std::endl;
        return false;
    }
    
    // 检查响应是否包含错误
    if (response.find("\"error\"") != std::string::npos) {
        std::cout << "\nAPI连接测试失败: 服务器返回错误" << std::endl;
        std::cout << "响应内容: " << (response.length() > 500 ? response.substr(0, 500) + "..." : response) << std::endl;
        return false;
    }
    
    std::cout << "\nAPI连接测试成功!" << std::endl;
    std::cout << "服务器响应正常，可以继续使用LLM功能" << std::endl;
    return true;
}

LLMClient::RandomEvent LLMClient::generateRandomEvent() {
    // 首先尝试使用API生成事件
    if (!simulationMode) {
        try {
            // 使用配置中的systemPrompt
            std::string prompt = systemPrompt;
            
            // 构建请求体
            std::string requestBody = "{\"model\": \"" + model + "\", \"messages\": [{\"role\": \"system\", \"content\": \"你是一个情感决策模拟系统的事件生成器。\"}, {\"role\": \"user\", \"content\": \"" + escapeJsonString(prompt) + "\"}], \"stream\": false, \"max_tokens\": 1500}";
            
            std::cout << "LLMClient: 正在向LLM请求生成包含10个选项的事件..." << std::endl;
            std::string response = sendRequest("v1/chat/completions", requestBody);
            
            if (response.empty()) {
                std::cerr << "LLMClient: API响应为空，使用备用事件" << std::endl;
                // 尝试使用保存的事件
                return getSavedRandomEvent();
            }
            
            // 尝试解析LLM响应为JSON事件
            RandomEvent event = parseEventResponse(response);
            
            // 验证事件是否符合要求（10个选项，12维向量）
            if (validateEvent(event)) {
                std::cout << "LLMClient: 成功使用API生成有效事件，正在保存..." << std::endl;
                // 保存事件到文件，以便后续使用
                saveEventToFile(event);
                // 添加到内存缓存
                savedEvents.push_back(event);
                std::cout << "LLMClient: 事件已保存并缓存" << std::endl;
                return event;
            } else {
                std::cerr << "LLMClient: LLM生成的事件无效，使用备用事件" << std::endl;
                return getSavedRandomEvent();
            }
            
        } catch (const std::exception& e) {
            std::cerr << "LLMClient: Exception in generateRandomEvent: " << e.what() << ", using saved events" << std::endl;
            return getSavedRandomEvent();
        }
    }
    
    // 模拟模式或无API连接时，使用保存的事件或生成模拟事件
    return getSavedRandomEvent();
}

int LLMClient::getLLMChoice(int agentId, const std::vector<double>& decisionVector,
                           const std::string& eventDescription,
                           const std::vector<EventOption>& options) {
    if (simulationMode) {
        return generateSimulatedChoice(agentId, decisionVector, options);
    }
    
    // 实际API调用
    try {
        if (options.empty()) {
            return -1;
        }
        
        // 构建提示
        std::string prompt = "作为情感决策代理#" + std::to_string(agentId) + "，请根据以下情况做出选择：\n";
        prompt += "事件描述: " + eventDescription + "\n";
        prompt += "可用选项:\n";
        for (size_t i = 0; i < options.size(); ++i) {
            prompt += std::to_string(i + 1) + ". " + options[i].text + " (结果: " + options[i].outcomeText + ")\n";
        }
        prompt += "请只返回选择的选项编号（1-" + std::to_string(options.size()) + "），不要包含其他文字。";
        
        // 构建请求体
        std::string requestBody = "{\"model\": \"" + model + "\", \"messages\": [{\"role\": \"user\", \"content\": \"" + escapeJsonString(prompt) + "\"}], \"stream\": false, \"max_tokens\": 50}";
        
        std::string response = sendRequest("v1/chat/completions", requestBody);
        
        if (response.empty()) {
            std::cerr << "LLMClient: Empty response for choice, falling back to simulation" << std::endl;
            return generateSimulatedChoice(agentId, decisionVector, options);
        }
        
        // 尝试解析响应中的数字
        size_t contentPos = response.find("\"content\"");
        if (contentPos == std::string::npos) {
            std::cerr << "LLMClient: No content in choice response, falling back to simulation" << std::endl;
            return generateSimulatedChoice(agentId, decisionVector, options);
        }
        
        size_t quoteStart = response.find('"', contentPos + 9);
        size_t quoteEnd = response.find('"', quoteStart + 1);
        if (quoteStart == std::string::npos || quoteEnd == std::string::npos) {
            std::cerr << "LLMClient: Malformed choice response, falling back to simulation" << std::endl;
            return generateSimulatedChoice(agentId, decisionVector, options);
        }
        
        std::string content = response.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        
        // 提取数字
        int choice = -1;
        for (char c : content) {
            if (c >= '1' && c <= '9') {
                choice = c - '0';
                break;
            }
        }
        
        if (choice >= 1 && choice <= options.size()) {
            std::cout << "LLMClient: API选择选项 " << choice << std::endl;
            return choice - 1; // 转换为0-based索引
        } else {
            std::cerr << "LLMClient: Invalid choice from API: " << content << ", falling back to simulation" << std::endl;
            return generateSimulatedChoice(agentId, decisionVector, options);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "LLMClient: Exception in getLLMChoice: " << e.what() << ", falling back to simulation" << std::endl;
        return generateSimulatedChoice(agentId, decisionVector, options);
    }
}

// 模拟事件生成
LLMClient::RandomEvent LLMClient::generateSimulatedEvent() {
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> eventDist(0, 7);
    
    RandomEvent event;
    int eventType = eventDist(rng);
    
    // 简化的事件类型
    const char* eventNames[] = {
        "情感挑战", "决策困境", "社交互动", "自我反思",
        "环境适应", "压力应对", "目标设定", "风险决策"
    };
    
    const char* eventDescriptions[] = {
        "面对复杂的情感状况，需要做出选择",
        "遇到困难的决策，需要在不同选项之间权衡",
        "在社交场合中需要做出适当的反应",
        "需要反思自己的行为和情感状态",
        "环境发生变化，需要适应新的情况",
        "面临压力，需要找到应对策略",
        "需要设定新的目标并做出决策",
        "面临风险，需要在安全与机会之间选择"
    };
    
    event.name = eventNames[eventType];
    event.description = eventDescriptions[eventType];
    
    // 生成2-3个选项
    std::uniform_int_distribution<int> optionCountDist(2, 3);
    int optionCount = optionCountDist(rng);
    
    const char* optionTexts[] = {
        "注重情感表达", "保持理性思考", "寻求平衡", "冒险尝试",
        "谨慎行事", "依赖直觉", "参考他人意见", "坚持原则"
    };
    
    const char* outcomes[] = {
        "这个选择带来了新的视角",
        "选择的结果符合预期",
        "这个决定引发了进一步的思考",
        "选择导致了有趣的发展",
        "决定带来了情感上的满足",
        "这个选择促进了个人成长"
    };
    
    for (int i = 0; i < optionCount; ++i) {
        EventOption option;
        
        // 生成选项文本
        option.text = optionTexts[(i + eventType) % 8];
        
        // 生成12维决策要求（随机）
        option.decisionRequirement.resize(12, 0.0);
        std::uniform_real_distribution<double> reqDist(0.0, 0.6);
        for (int d = 0; d < 12; ++d) {
            option.decisionRequirement[d] = reqDist(rng);
        }
        
        // 生成12维决策反馈（随机，在-0.2到0.2之间）
        option.decisionFeedback.resize(12, 0.0);
        std::uniform_real_distribution<double> feedbackDist(-0.2, 0.2);
        for (int d = 0; d < 12; ++d) {
            option.decisionFeedback[d] = feedbackDist(rng);
        }
        
        // 结果描述
        option.outcomeText = outcomes[(i + eventType) % 6];
        
        event.options.push_back(option);
    }
    
    return event;
}

// 模拟选择生成
int LLMClient::generateSimulatedChoice(int agentId, const std::vector<double>& decisionVector,
                                      const std::vector<EventOption>& options) {
    if (options.empty()) {
        return -1;
    }
    
    static std::mt19937 rng(std::random_device{}());
    
    // 首先检查是否有满足决策要求的选项
    std::vector<int> availableOptions;
    for (int i = 0; i < options.size(); ++i) {
        bool requirementMet = true;
        const auto& req = options[i].decisionRequirement;
        if (req.size() != decisionVector.size()) {
            continue;
        }
        
        for (int d = 0; d < req.size(); ++d) {
            if (decisionVector[d] < req[d]) {
                requirementMet = false;
                break;
            }
        }
        
        if (requirementMet) {
            availableOptions.push_back(i);
        }
    }
    
    // 如果有满足要求的选项，随机选择一个
    if (!availableOptions.empty()) {
        std::uniform_int_distribution<int> dist(0, availableOptions.size() - 1);
        return availableOptions[dist(rng)];
    }
    
    // 如果没有满足要求的选项，根据决策向量相似度选择
    // 计算每个选项的决策向量与代理决策向量的余弦相似度
    std::vector<double> similarities(options.size(), 0.0);
    for (int i = 0; i < options.size(); ++i) {
        const auto& req = options[i].decisionRequirement;
        if (req.size() != decisionVector.size()) {
            similarities[i] = 0.0;
            continue;
        }
        
        double dot = 0.0, normA = 0.0, normB = 0.0;
        for (int d = 0; d < decisionVector.size(); ++d) {
            dot += decisionVector[d] * req[d];
            normA += decisionVector[d] * decisionVector[d];
            normB += req[d] * req[d];
        }
        
        if (normA > 0.0 && normB > 0.0) {
            similarities[i] = dot / (sqrt(normA) * sqrt(normB));
        } else {
            similarities[i] = 0.0;
        }
    }
    
    // 选择相似度最高的选项
    int bestOption = 0;
    double bestSimilarity = similarities[0];
    for (int i = 1; i < similarities.size(); ++i) {
        if (similarities[i] > bestSimilarity) {
            bestSimilarity = similarities[i];
            bestOption = i;
        }
    }
    
    // 如果所有相似度都为0，随机选择
    if (bestSimilarity <= 0.0) {
        std::uniform_int_distribution<int> dist(0, options.size() - 1);
        return dist(rng);
    }
    
    return bestOption;
}

// 发送HTTP请求（完整实现）
std::string LLMClient::sendRequest(const std::string& endpoint, const std::string& body) {
#ifdef _WIN32
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    
    try {
        // 构建完整URL
        std::string fullUrl = baseUrl;
        if (!endpoint.empty() && endpoint[0] != '/') {
            fullUrl += "/";
        }
        fullUrl += endpoint;
        
        // 调试输出
        std::cout << "LLMClient: 发送请求到 URL: " << fullUrl << std::endl;
        std::cout << "LLMClient: 端点: " << endpoint << std::endl;
        std::cout << "LLMClient: 请求体前100字符: " << (body.length() > 100 ? body.substr(0, 100) + "..." : body) << std::endl;
        
        // 解析URL
        URL_COMPONENTS urlComp;
        ZeroMemory(&urlComp, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);
        
        urlComp.dwSchemeLength = -1;
        urlComp.dwHostNameLength = -1;
        urlComp.dwUrlPathLength = -1;
        urlComp.dwExtraInfoLength = -1;
        
        std::wstring wideUrl = stringToWideString(fullUrl);
        
        if (!WinHttpCrackUrl(wideUrl.c_str(), wideUrl.length(), 0, &urlComp)) {
            std::cerr << "LLMClient: Failed to parse URL: " << fullUrl << std::endl;
            return "";
        }
        
        // 提取主机和端口
        std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
        INTERNET_PORT port = urlComp.nPort;
        
        // 提取路径
        std::wstring path;
        if (urlComp.dwUrlPathLength > 0) {
            path = std::wstring(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
        } else {
            path = L"/";
        }
        
        // 创建会话
        hSession = WinHttpOpen(L"LLMClient/1.0", 
                               WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME, 
                               WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            std::cerr << "LLMClient: Failed to create WinHTTP session" << std::endl;
            return "";
        }
        
        // 设置超时（将秒转换为毫秒）
        DWORD timeoutMs = timeoutSeconds * 1000;
        WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);
        
        // 连接到服务器
        hConnect = WinHttpConnect(hSession, hostName.c_str(), port, 0);
        if (!hConnect) {
            std::cerr << "LLMClient: Failed to connect to server" << std::endl;
            WinHttpCloseHandle(hSession);
            return "";
        }
        
        // 创建请求
        hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
                                     NULL, WINHTTP_NO_REFERER,
                                     WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) {
            std::cerr << "LLMClient: Failed to create request" << std::endl;
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }
        
        // 准备请求头
        std::wstring headers = L"Content-Type: application/json; charset=utf-8";
        if (!apiKey.empty()) {
            headers += L"\r\nAuthorization: Bearer " + stringToWideString(apiKey);
        }
        
        // 发送请求
        if (!WinHttpSendRequest(hRequest, headers.c_str(), headers.length(),
                               (LPVOID)body.c_str(), body.length(),
                               body.length(), 0)) {
            std::cerr << "LLMClient: Failed to send request" << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }
        
        // 接收响应
        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            std::cerr << "LLMClient: Failed to receive response" << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }
        
        // 读取响应数据
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        std::string response;
        
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                std::cerr << "LLMClient: Error querying data available" << std::endl;
                break;
            }
            
            if (dwSize == 0) break;
            
            std::vector<char> buffer(dwSize + 1);
            if (!WinHttpReadData(hRequest, (LPVOID)buffer.data(), dwSize, &dwDownloaded)) {
                std::cerr << "LLMClient: Error reading data" << std::endl;
                break;
            }
            
            buffer[dwDownloaded] = '\0';
            response.append(buffer.data());
            
        } while (dwSize > 0);
        
        // 调试输出：显示响应信息
        std::cout << "LLMClient: 收到响应，长度: " << response.length() << " 字节" << std::endl;
        if (!response.empty()) {
            std::cout << "LLMClient: 响应前200字符: " << (response.length() > 200 ? response.substr(0, 200) + "..." : response) << std::endl;
        } else {
            std::cout << "LLMClient: 响应为空" << std::endl;
        }
        
        // 清理资源
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        
        return response;
        
    } catch (const std::exception& e) {
        std::cerr << "LLMClient: Exception in sendRequest: " << e.what() << std::endl;
        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        if (hSession) WinHttpCloseHandle(hSession);
        return "";
    }
#else
    std::cerr << "LLMClient: HTTP requests require Windows platform" << std::endl;
    return "";
#endif
}

// 解析LLM响应
LLMClient::RandomEvent LLMClient::parseEventResponse(const std::string& response) {
    RandomEvent event;
    
    std::cout << "LLMClient: 开始解析LLM响应..." << std::endl;
    
    try {
        // 首先尝试从OpenAI兼容响应中提取content字段
        // OpenAI格式: {"choices": [{"message": {"content": "..."}}]}
        std::string content;
        
        // 查找choices数组
        size_t choicesPos = response.find("\"choices\"");
        if (choicesPos != std::string::npos) {
            // 找到第一个choice中的message
            size_t messagePos = response.find("\"message\"", choicesPos);
            if (messagePos != std::string::npos) {
                // 在message对象中查找content
                size_t contentPos = response.find("\"content\"", messagePos);
                if (contentPos != std::string::npos) {
                    // 找到content字段的开始引号
                    size_t quoteStart = response.find('"', contentPos + 9); // 跳过"content":
                    if (quoteStart != std::string::npos) {
                        size_t quoteEnd = response.find('"', quoteStart + 1);
                        if (quoteEnd != std::string::npos) {
                            content = response.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                        }
                    }
                }
            }
        }
        
        // 如果通过choices没有找到，尝试直接查找content字段（旧格式）
        if (content.empty()) {
            size_t contentPos = response.find("\"content\"");
            if (contentPos == std::string::npos) {
                std::cerr << "LLMClient: 响应中未找到content字段" << std::endl;
                return generateSimulatedEvent();
            }
            
            // 找到content字段的开始引号
            size_t quoteStart = response.find('"', contentPos + 9); // 跳过"content":
            if (quoteStart == std::string::npos) {
                std::cerr << "LLMClient: 格式错误：未找到content字段的开始引号" << std::endl;
                return generateSimulatedEvent();
            }
            
            // 找到content字段的结束引号
            size_t quoteEnd = response.find('"', quoteStart + 1);
            if (quoteEnd == std::string::npos) {
                std::cerr << "LLMClient: 格式错误：未找到content字段的结束引号" << std::endl;
                return generateSimulatedEvent();
            }
            
            content = response.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
        
        // 现在content应该是我们请求的JSON字符串
        // 我们需要解析这个JSON字符串
        std::cout << "LLMClient: 提取到content长度：" << content.length() << std::endl;
        
        // 简单的JSON解析辅助函数
        auto extractJsonString = [](const std::string& json, const std::string& key) -> std::string {
            size_t keyPos = json.find("\"" + key + "\"");
            if (keyPos == std::string::npos) return "";
            
            size_t colonPos = json.find(':', keyPos);
            if (colonPos == std::string::npos) return "";
            
            size_t quoteStart = json.find('"', colonPos);
            if (quoteStart == std::string::npos) return "";
            
            size_t quoteEnd = json.find('"', quoteStart + 1);
            if (quoteEnd == std::string::npos) return "";
            
            return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        };
        
        auto extractJsonArray = [](const std::string& json, const std::string& key) -> std::string {
            size_t keyPos = json.find("\"" + key + "\"");
            if (keyPos == std::string::npos) return "";
            
            size_t colonPos = json.find(':', keyPos);
            if (colonPos == std::string::npos) return "";
            
            // 找到数组的开始括号 '['
            size_t bracketStart = json.find('[', colonPos);
            if (bracketStart == std::string::npos) return "";
            
            // 找到匹配的结束括号 ']'
            int bracketCount = 1;
            size_t pos = bracketStart + 1;
            for (; pos < json.length(); ++pos) {
                if (json[pos] == '[') bracketCount++;
                else if (json[pos] == ']') {
                    bracketCount--;
                    if (bracketCount == 0) break;
                }
            }
            
            if (bracketCount != 0) return "";
            
            return json.substr(bracketStart, pos - bracketStart + 1);
        };
        
        // 从content中提取事件名称和描述
        event.name = extractJsonString(content, "name");
        event.description = extractJsonString(content, "description");
        
        if (event.name.empty() || event.description.empty()) {
            std::cerr << "LLMClient: 无法解析事件名称或描述" << std::endl;
            return generateSimulatedEvent();
        }
        
        std::cout << "LLMClient: 解析到事件：" << event.name << std::endl;
        
        // 提取options数组
        std::string optionsArray = extractJsonArray(content, "options");
        if (optionsArray.empty()) {
            std::cerr << "LLMClient: 无法解析options数组" << std::endl;
            return generateSimulatedEvent();
        }
        
        // 解析每个选项
        size_t optionStart = 0;
        while (true) {
            // 查找选项对象的开始 '{'
            optionStart = optionsArray.find('{', optionStart);
            if (optionStart == std::string::npos) break;
            
            // 查找选项对象的结束 '}'
            int braceCount = 1;
            size_t optionEnd = optionStart + 1;
            for (; optionEnd < optionsArray.length(); ++optionEnd) {
                if (optionsArray[optionEnd] == '{') braceCount++;
                else if (optionsArray[optionEnd] == '}') {
                    braceCount--;
                    if (braceCount == 0) break;
                }
            }
            
            if (braceCount != 0) break;
            
            std::string optionJson = optionsArray.substr(optionStart, optionEnd - optionStart + 1);
            
            // 提取选项字段
            EventOption option;
            option.text = extractJsonString(optionJson, "text");
            option.outcomeText = extractJsonString(optionJson, "outcomeText");
            
            // 提取decisionRequirement数组
            std::string reqArray = extractJsonArray(optionJson, "decisionRequirement");
            if (!reqArray.empty() && reqArray.length() > 2) {
                // 解析数组 [0.1, 0.2, ...]
                std::vector<double> requirements;
                size_t start = reqArray.find('[') + 1;
                size_t end = reqArray.find(']');
                if (end != std::string::npos && end > start) {
                    std::string values = reqArray.substr(start, end - start);
                    size_t pos = 0;
                    while (pos < values.length()) {
                        // 跳过空格
                        while (pos < values.length() && (values[pos] == ' ' || values[pos] == ',' || values[pos] == '\n' || values[pos] == '\t')) pos++;
                        if (pos >= values.length()) break;
                        
                        size_t valueEnd = values.find_first_of(", ]", pos);
                        if (valueEnd == std::string::npos) break;
                        
                        std::string valueStr = values.substr(pos, valueEnd - pos);
                        try {
                            requirements.push_back(std::stod(valueStr));
                        } catch (...) {
                            requirements.push_back(0.0);
                        }
                        
                        pos = valueEnd;
                    }
                }
                option.decisionRequirement = requirements;
            }
            
            // 提取decisionFeedback数组
            std::string feedbackArray = extractJsonArray(optionJson, "decisionFeedback");
            if (!feedbackArray.empty() && feedbackArray.length() > 2) {
                // 解析数组 [0.1, -0.1, ...]
                std::vector<double> feedbacks;
                size_t start = feedbackArray.find('[') + 1;
                size_t end = feedbackArray.find(']');
                if (end != std::string::npos && end > start) {
                    std::string values = feedbackArray.substr(start, end - start);
                    size_t pos = 0;
                    while (pos < values.length()) {
                        // 跳过空格
                        while (pos < values.length() && (values[pos] == ' ' || values[pos] == ',' || values[pos] == '\n' || values[pos] == '\t')) pos++;
                        if (pos >= values.length()) break;
                        
                        size_t valueEnd = values.find_first_of(", ]", pos);
                        if (valueEnd == std::string::npos) break;
                        
                        std::string valueStr = values.substr(pos, valueEnd - pos);
                        try {
                            feedbacks.push_back(std::stod(valueStr));
                        } catch (...) {
                            feedbacks.push_back(0.0);
                        }
                        
                        pos = valueEnd;
                    }
                }
                option.decisionFeedback = feedbacks;
            }
            
            // 如果向量长度不正确，使用默认值
            if (option.decisionRequirement.size() != 12) {
                option.decisionRequirement = std::vector<double>(12, 0.5);
            }
            if (option.decisionFeedback.size() != 12) {
                option.decisionFeedback = std::vector<double>(12, 0.1);
            }
            
            event.options.push_back(option);
            optionStart = optionEnd + 1;
        }
        
        std::cout << "LLMClient: 解析完成，找到 " << event.options.size() << " 个选项" << std::endl;
        
        if (event.options.empty()) {
            std::cerr << "LLMClient: 未解析到任何选项" << std::endl;
            return generateSimulatedEvent();
        }
        
        return event;
        
    } catch (const std::exception& e) {
        std::cerr << "LLMClient: 解析LLM响应时异常: " << e.what() << std::endl;
        return generateSimulatedEvent();
    }
}

// 解析选择响应（占位符）
int LLMClient::parseChoiceResponse(const std::string& response) {
    return 0;
}

// 检查事件是否符合要求（10个选项，每个选项有12维向量）
bool LLMClient::validateEvent(const RandomEvent& event) {
    if (event.name.empty() || event.description.empty()) {
        return false;
    }
    
    // 检查选项数量 - 用户要求10个选项
    if (event.options.size() != 10) {
        std::cerr << "LLMClient: 事件选项数量不正确，期望10个，实际" << event.options.size() << "个" << std::endl;
        return false;
    }
    
    // 检查每个选项
    for (size_t i = 0; i < event.options.size(); ++i) {
        const auto& option = event.options[i];
        if (option.text.empty() || option.outcomeText.empty()) {
            std::cerr << "LLMClient: 选项" << i << "文本为空" << std::endl;
            return false;
        }
        
        if (option.decisionRequirement.size() != 12) {
            std::cerr << "LLMClient: 选项" << i << "决策要求向量维度不正确，期望12，实际" << option.decisionRequirement.size() << std::endl;
            return false;
        }
        
        if (option.decisionFeedback.size() != 12) {
            std::cerr << "LLMClient: 选项" << i << "决策反馈向量维度不正确，期望12，实际" << option.decisionFeedback.size() << std::endl;
            return false;
        }
        
        // 检查值范围
        for (size_t j = 0; j < 12; ++j) {
            if (option.decisionRequirement[j] < 0.0 || option.decisionRequirement[j] > 1.0) {
                std::cerr << "LLMClient: 选项" << i << "决策要求向量值超出范围[0.0, 1.0]: " << option.decisionRequirement[j] << std::endl;
                return false;
            }
            
            if (option.decisionFeedback[j] < -0.2 || option.decisionFeedback[j] > 0.2) {
                std::cerr << "LLMClient: 选项" << i << "决策反馈向量值超出范围[-0.2, 0.2]: " << option.decisionFeedback[j] << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

// 保存事件到文件
void LLMClient::saveEventToFile(const RandomEvent& event) {
    // 首先验证事件
    if (!validateEvent(event)) {
        std::cerr << "LLMClient: 事件验证失败，不保存" << std::endl;
        return;
    }
    
    // 创建保存目录（如果不存在）
    #ifdef _WIN32
    CreateDirectoryA("llm_events", NULL);
    #endif
    
    // 生成文件名（使用时间戳和事件名称）
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &now_time);
    char filename[256];
    strftime(filename, sizeof(filename), "llm_events/event_%Y%m%d_%H%M%S.json", &tm);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "LLMClient: 无法打开文件保存事件: " << filename << std::endl;
        return;
    }
    
    // 保存为JSON格式
    file << "{\n";
    file << "  \"name\": \"" << escapeJsonString(event.name) << "\",\n";
    file << "  \"description\": \"" << escapeJsonString(event.description) << "\",\n";
    file << "  \"options\": [\n";
    
    for (size_t i = 0; i < event.options.size(); ++i) {
        const auto& option = event.options[i];
        file << "    {\n";
        file << "      \"text\": \"" << escapeJsonString(option.text) << "\",\n";
        file << "      \"outcomeText\": \"" << escapeJsonString(option.outcomeText) << "\",\n";
        
        file << "      \"decisionRequirement\": [";
        for (size_t j = 0; j < option.decisionRequirement.size(); ++j) {
            file << option.decisionRequirement[j];
            if (j < option.decisionRequirement.size() - 1) {
                file << ", ";
            }
        }
        file << "],\n";
        
        file << "      \"decisionFeedback\": [";
        for (size_t j = 0; j < option.decisionFeedback.size(); ++j) {
            file << option.decisionFeedback[j];
            if (j < option.decisionFeedback.size() - 1) {
                file << ", ";
            }
        }
        file << "]\n";
        
        file << "    }";
        if (i < event.options.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "LLMClient: 事件已保存到 " << filename << std::endl;
}

// 辅助函数：从JSON内容解析事件（用于加载保存的文件）
LLMClient::RandomEvent LLMClient::parseEventFromJsonContent(const std::string& jsonContent) {
    RandomEvent event;
    
    try {
        // 重用parseEventResponse中的提取函数
        auto extractJsonString = [](const std::string& json, const std::string& key) -> std::string {
            size_t keyPos = json.find("\"" + key + "\"");
            if (keyPos == std::string::npos) return "";
            
            size_t colonPos = json.find(':', keyPos);
            if (colonPos == std::string::npos) return "";
            
            size_t quoteStart = json.find('"', colonPos);
            if (quoteStart == std::string::npos) return "";
            
            size_t quoteEnd = json.find('"', quoteStart + 1);
            if (quoteEnd == std::string::npos) return "";
            
            return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        };
        
        auto extractJsonArray = [](const std::string& json, const std::string& key) -> std::string {
            size_t keyPos = json.find("\"" + key + "\"");
            if (keyPos == std::string::npos) return "";
            
            size_t colonPos = json.find(':', keyPos);
            if (colonPos == std::string::npos) return "";
            
            // 找到数组的开始括号 '['
            size_t bracketStart = json.find('[', colonPos);
            if (bracketStart == std::string::npos) return "";
            
            // 找到匹配的结束括号 ']'
            int bracketCount = 1;
            size_t pos = bracketStart + 1;
            for (; pos < json.length(); ++pos) {
                if (json[pos] == '[') bracketCount++;
                else if (json[pos] == ']') {
                    bracketCount--;
                    if (bracketCount == 0) break;
                }
            }
            
            if (bracketCount != 0) return "";
            
            return json.substr(bracketStart, pos - bracketStart + 1);
        };
        
        // 提取事件名称和描述
        event.name = extractJsonString(jsonContent, "name");
        event.description = extractJsonString(jsonContent, "description");
        
        if (event.name.empty() || event.description.empty()) {
            throw std::runtime_error("无法解析事件名称或描述");
        }
        
        // 提取options数组
        std::string optionsArray = extractJsonArray(jsonContent, "options");
        if (optionsArray.empty()) {
            throw std::runtime_error("无法解析options数组");
        }
        
        // 解析每个选项
        size_t optionStart = 0;
        while (true) {
            // 查找选项对象的开始 '{'
            optionStart = optionsArray.find('{', optionStart);
            if (optionStart == std::string::npos) break;
            
            // 查找选项对象的结束 '}'
            int braceCount = 1;
            size_t optionEnd = optionStart + 1;
            for (; optionEnd < optionsArray.length(); ++optionEnd) {
                if (optionsArray[optionEnd] == '{') braceCount++;
                else if (optionsArray[optionEnd] == '}') {
                    braceCount--;
                    if (braceCount == 0) break;
                }
            }
            
            if (braceCount != 0) break;
            
            std::string optionJson = optionsArray.substr(optionStart, optionEnd - optionStart + 1);
            
            // 提取选项字段
            EventOption option;
            option.text = extractJsonString(optionJson, "text");
            option.outcomeText = extractJsonString(optionJson, "outcomeText");
            
            // 提取decisionRequirement数组
            std::string reqArray = extractJsonArray(optionJson, "decisionRequirement");
            if (!reqArray.empty() && reqArray.length() > 2) {
                std::vector<double> requirements;
                size_t start = reqArray.find('[') + 1;
                size_t end = reqArray.find(']');
                if (end != std::string::npos && end > start) {
                    std::string values = reqArray.substr(start, end - start);
                    size_t pos = 0;
                    while (pos < values.length()) {
                        while (pos < values.length() && (values[pos] == ' ' || values[pos] == ',' || values[pos] == '\n' || values[pos] == '\t')) pos++;
                        if (pos >= values.length()) break;
                        
                        size_t valueEnd = values.find_first_of(", ]", pos);
                        if (valueEnd == std::string::npos) break;
                        
                        std::string valueStr = values.substr(pos, valueEnd - pos);
                        try {
                            requirements.push_back(std::stod(valueStr));
                        } catch (...) {
                            requirements.push_back(0.0);
                        }
                        
                        pos = valueEnd;
                    }
                }
                option.decisionRequirement = requirements;
            }
            
            // 提取decisionFeedback数组
            std::string feedbackArray = extractJsonArray(optionJson, "decisionFeedback");
            if (!feedbackArray.empty() && feedbackArray.length() > 2) {
                std::vector<double> feedbacks;
                size_t start = feedbackArray.find('[') + 1;
                size_t end = feedbackArray.find(']');
                if (end != std::string::npos && end > start) {
                    std::string values = feedbackArray.substr(start, end - start);
                    size_t pos = 0;
                    while (pos < values.length()) {
                        while (pos < values.length() && (values[pos] == ' ' || values[pos] == ',' || values[pos] == '\n' || values[pos] == '\t')) pos++;
                        if (pos >= values.length()) break;
                        
                        size_t valueEnd = values.find_first_of(", ]", pos);
                        if (valueEnd == std::string::npos) break;
                        
                        std::string valueStr = values.substr(pos, valueEnd - pos);
                        try {
                            feedbacks.push_back(std::stod(valueStr));
                        } catch (...) {
                            feedbacks.push_back(0.0);
                        }
                        
                        pos = valueEnd;
                    }
                }
                option.decisionFeedback = feedbacks;
            }
            
            // 如果向量长度不正确，使用默认值
            if (option.decisionRequirement.size() != 12) {
                option.decisionRequirement = std::vector<double>(12, 0.5);
            }
            if (option.decisionFeedback.size() != 12) {
                option.decisionFeedback = std::vector<double>(12, 0.1);
            }
            
            event.options.push_back(option);
            optionStart = optionEnd + 1;
        }
        
        if (event.options.empty()) {
            throw std::runtime_error("未解析到任何选项");
        }
        
        return event;
        
    } catch (const std::exception& e) {
        std::cerr << "LLMClient: 解析JSON内容时异常: " << e.what() << std::endl;
        throw; // 重新抛出异常
    }
}

// 从文件加载保存的事件
void LLMClient::loadSavedEvents() {
    savedEvents.clear();
    
    std::cout << "LLMClient: 开始加载保存的事件..." << std::endl;
    
    // 检查目录是否存在
    #ifdef _WIN32
    DWORD dirAttr = GetFileAttributesA("llm_events");
    if (dirAttr == INVALID_FILE_ATTRIBUTES || !(dirAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        std::cout << "LLMClient: llm_events目录不存在，无保存事件" << std::endl;
        return;
    }
    
    // 使用Windows API遍历目录
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("llm_events\\*.json", &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "LLMClient: 未找到.json事件文件" << std::endl;
        return;
    }
    
    int loadedCount = 0;
    int errorCount = 0;
    
    do {
        // 跳过目录
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        
        std::string filename = "llm_events\\" + std::string(findData.cFileName);
        std::cout << "LLMClient: 加载事件文件: " << filename << std::endl;
        
        try {
            // 读取文件内容
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "LLMClient: 无法打开文件: " << filename << std::endl;
                errorCount++;
                continue;
            }
            
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string fileContent = buffer.str();
            file.close();
            
            // 解析事件
            RandomEvent event = parseEventFromJsonContent(fileContent);
            
            // 验证事件
            if (validateEvent(event)) {
                savedEvents.push_back(event);
                loadedCount++;
                std::cout << "LLMClient: 成功加载事件: " << event.name << std::endl;
            } else {
                std::cerr << "LLMClient: 事件验证失败: " << filename << std::endl;
                errorCount++;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "LLMClient: 加载文件时异常 " << filename << ": " << e.what() << std::endl;
            errorCount++;
        }
        
    } while (FindNextFileA(hFind, &findData) != 0);
    
    FindClose(hFind);
    
    std::cout << "LLMClient: 事件加载完成，成功加载 " << loadedCount << " 个事件，" 
              << errorCount << " 个错误，总共 " << savedEvents.size() << " 个保存事件" << std::endl;
    
#else
    std::cout << "LLMClient: 文件遍历需要Windows平台" << std::endl;
#endif
}

// 获取保存的LLM生成事件（用于模拟模式下的备用事件）
LLMClient::RandomEvent LLMClient::getSavedRandomEvent() {
    if (savedEvents.empty()) {
        // 如果没有保存的事件，返回模拟事件
        std::cout << "LLMClient: 无保存事件，返回模拟事件" << std::endl;
        return generateSimulatedEvent();
    }
    
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, savedEvents.size() - 1);
    int index = dist(rng);
    
    std::cout << "LLMClient: 使用保存的事件 #" << index << ": " << savedEvents[index].name << std::endl;
    return savedEvents[index];
}
