#pragma once

#include <string>
#include <vector>
#include <random>

class BioAgent {
public:
    // 决策向量维度常量
    static constexpr int DECISION_VECTOR_DIMENSIONS = 12;
    
    // 决策向量维度枚举
    enum DecisionVectorDimension { 
        HAPPINESS,      // 快乐
        SADNESS,        // 悲伤
        ANGER,          // 愤怒
        FEAR,           // 恐惧
        DISGUST,        // 厌恶
        SURPRISE,       // 惊讶
        TRUST,          // 信任
        ANTICIPATION,   // 期待
        PEACEFULNESS,   // 宁静
        VALENCE,        // 效价
        AROUSAL,        // 唤醒度
        DOMINANCE       // 优势度
    };
    
    // 构造函数
    BioAgent(int id = -1);
    
    // 获取代理ID
    int getId() const { return id; }
    
    // 设置代理ID
    void setId(int newId) { id = newId; }
    
    // 获取决策向量
    const std::vector<double>& getDecisionVector() const { return decisionVector; }
    
    // 设置决策向量
    void setDecisionVector(const std::vector<double>& decisions) { 
        if (decisions.size() == DECISION_VECTOR_DIMENSIONS) {
            decisionVector = decisions;
        }
    }
    
    // 根据反馈更新决策向量（反馈是12维向量，正值增加，负值减少）
    void updateDecisionVector(const std::vector<double>& feedback);
    
    // 检查决策向量是否满足要求（每个维度 >= 要求值）
    bool checkDecisionRequirement(const std::vector<double>& requirement) const;
    
    // 获取指定维度的决策值
    double getDecisionValue(DecisionVectorDimension dimension) const {
        if (dimension >= 0 && dimension < DECISION_VECTOR_DIMENSIONS) {
            return decisionVector[dimension];
        }
        return 0.0;
    }
    
    // 设置指定维度的决策值
    void setDecisionValue(DecisionVectorDimension dimension, double value) {
        if (dimension >= 0 && dimension < DECISION_VECTOR_DIMENSIONS) {
            decisionVector[dimension] = value;
        }
    }
    
    // 随机初始化决策向量
    void randomizeDecisionVector();
    
    // 标准化决策向量（确保所有值在0.0-1.0范围内）
    void normalizeDecisionVector();
    
    // 获取决策向量的字符串表示
    std::string getDecisionVectorString() const;
    
    // 计算与另一个代理决策向量的相似度
    double calculateSimilarity(const BioAgent& other) const;

private:
    // 代理ID
    int id;
    
    // 12维决策向量：快乐、悲伤、愤怒、恐惧、厌恶、惊讶、信任、期待、宁静、效价、唤醒度、优势度（每个维度范围0.0-1.0）
    std::vector<double> decisionVector;
    
    // 随机数生成器
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
};