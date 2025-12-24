#include <iostream>
#include "SimulationEnvironment.h"
#include <limits>
#include <cstdlib>
#include <windows.h>
#include <iomanip>

void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void displayAgentInfo(const SimulationEnvironment& env) {
    std::cout << "\n当前代理状态：" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    const auto& agents = env.getAgents();
    for (size_t i = 0; i < agents.size() && i < 5; ++i) {
        std::cout << "代理 " << i << ": " << env.getAgentDecisionVectorString(i) << std::endl;
    }
    
    if (agents.size() > 5) {
        std::cout << "... 共 " << agents.size() << " 个代理" << std::endl;
    }
}

void displayDetailedAgentInfo(const SimulationEnvironment& env) {
    std::cout << "\n详细代理状态：" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    
    auto detailedStatus = env.getAllAgentsDetailedStatus();
    for (const auto& status : detailedStatus) {
        std::cout << status << std::endl;
    }
}

int main() {
    try {
        // 设置控制台代码页为 UTF-8，支持中文输出
        SetConsoleOutputCP(65001);
        SetConsoleCP(65001);
        
        std::cout << "==========================================" << std::endl;
        std::cout << "     决策向量与随机事件模拟系统 (AMPHOREUS)     " << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "系统已简化：只保留决策向量和随机事件功能" << std::endl;
        std::cout << std::endl;
        
        SimulationEnvironment env;
        env.initialize();
        
        bool running = true;
        while (running) {
            std::cout << std::endl;
            std::cout << "主菜单：" << std::endl;
            std::cout << "1. 运行交互式模拟（实时显示代理状态）" << std::endl;
            std::cout << "2. 运行事件模拟（显示事件详情）" << std::endl;
            std::cout << "3. 显示代理状态（简要）" << std::endl;
            std::cout << "4. 显示详细代理状态" << std::endl;
            std::cout << "5. 设置事件概率" << std::endl;
            std::cout << "6. 添加自定义事件" << std::endl;
            std::cout << "7. 查看事件历史" << std::endl;
            std::cout << "8. 退出" << std::endl;
            std::cout << "输入选项 (1-8): ";
            
            int choice;
            std::cin >> choice;
            clearInputBuffer();
            
            switch (choice) {
                case 1: {
                    std::cout << "请输入要运行的事件数量 (默认20): ";
                    int numEvents = 20;
                    std::string input;
                    std::getline(std::cin, input);
                    
                    if (!input.empty()) {
                        try {
                            numEvents = std::stoi(input);
                            if (numEvents < 1) numEvents = 20;
                        } catch (...) {
                            numEvents = 20;
                        }
                    }
                    
                    std::cout << "开始运行交互式模拟，共 " << numEvents << " 个事件..." << std::endl;
                    env.runInteractiveSimulation(numEvents);
                    break;
                }
                
                case 2: {
                    std::cout << "请输入要运行的事件数量 (默认10): ";
                    int numEvents = 10;
                    std::string input;
                    std::getline(std::cin, input);
                    
                    if (!input.empty()) {
                        try {
                            numEvents = std::stoi(input);
                            if (numEvents < 1) numEvents = 10;
                        } catch (...) {
                            numEvents = 10;
                        }
                    }
                    
                    std::cout << "开始运行 " << numEvents << " 个随机事件..." << std::endl;
                    env.runEventSimulation(numEvents);
                    break;
                }
                
                case 3: {
                    displayAgentInfo(env);
                    break;
                }
                
                case 4: {
                    displayDetailedAgentInfo(env);
                    break;
                }
                
                case 5: {
                    std::cout << "当前随机事件概率: " << std::endl;
                    std::cout << "请输入新的随机事件概率 (0.0-1.0): ";
                    double prob;
                    if (std::cin >> prob && prob >= 0.0 && prob <= 1.0) {
                        env.setRandomEventProbability(prob);
                        std::cout << "随机事件概率已设置为: " << prob << std::endl;
                    } else {
                        std::cout << "输入无效，概率保持不变。" << std::endl;
                    }
                    clearInputBuffer();
                    break;
                }
                
                case 6: {
                    std::cout << "添加自定义事件功能暂未实现。" << std::endl;
                    std::cout << "TODO: 实现自定义事件添加界面" << std::endl;
                    break;
                }
                
                case 7: {
                    const auto& history = env.getEventHistory();
                    std::cout << "\n事件历史记录 (" << history.size() << " 个事件):" << std::endl;
                    std::cout << "------------------------------------------" << std::endl;
                    
                    if (history.empty()) {
                        std::cout << "暂无事件历史。" << std::endl;
                    } else {
                        for (size_t i = 0; i < history.size() && i < 10; ++i) {
                            std::cout << history[i] << std::endl;
                        }
                        if (history.size() > 10) {
                            std::cout << "... 还有 " << (history.size() - 10) << " 个事件" << std::endl;
                        }
                    }
                    break;
                }
                
                case 8: {
                    running = false;
                    std::cout << "退出系统..." << std::endl;
                    break;
                }
                
                default: {
                    std::cout << "无效选项，请重新输入。" << std::endl;
                    break;
                }
            }
        }
        
        std::cout << "感谢使用决策向量与随机事件模拟系统！" << std::endl;
        
        // 在 Windows 上暂停，防止窗口立即关闭
        #ifdef _WIN32
        std::cout << "按任意键退出..." << std::endl;
        std::cin.get();
        #endif
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "程序发生异常: " << e.what() << std::endl;
        #ifdef _WIN32
        std::cerr << "按任意键退出..." << std::endl;
        std::cin.get();
        #endif
        return 1;
    }
    catch (...) {
        std::cerr << "程序发生未知异常" << std::endl;
        #ifdef _WIN32
        std::cerr << "按任意键退出..." << std::endl;
        std::cin.get();
        #endif
        return 1;
    }
}