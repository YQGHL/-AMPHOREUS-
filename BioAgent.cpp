#include "BioAgent.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>

BioAgent::BioAgent(int id) 
    : id(id), decisionVector(DECISION_VECTOR_DIMENSIONS, 0.5) {
    rng.seed(std::random_device{}());
    dist = std::uniform_real_distribution<double>(0.0, 1.0);
    
    // 随机初始化决策向量
    randomizeDecisionVector();
}

void BioAgent::updateDecisionVector(const std::vector<double>& feedback) {
    if (feedback.size() != DECISION_VECTOR_DIMENSIONS) {
        return;
    }
    
    // 根据反馈更新决策向量，保持值在0.0-1.0范围内
    for (int i = 0; i < DECISION_VECTOR_DIMENSIONS; ++i) {
        double newValue = decisionVector[i] + feedback[i];
        // 限制在0.0-1.0范围内
        decisionVector[i] = std::max(0.0, std::min(1.0, newValue));
    }
    
    // 标准化决策向量
    normalizeDecisionVector();
}

bool BioAgent::checkDecisionRequirement(const std::vector<double>& requirement) const {
    if (requirement.size() != DECISION_VECTOR_DIMENSIONS) {
        return false;
    }
    
    // 检查每个维度是否满足要求（决策向量值 >= 要求值）
    for (int i = 0; i < DECISION_VECTOR_DIMENSIONS; ++i) {
        if (decisionVector[i] < requirement[i]) {
            return false;
        }
    }
    return true;
}

void BioAgent::randomizeDecisionVector() {
    for (int i = 0; i < DECISION_VECTOR_DIMENSIONS; ++i) {
        decisionVector[i] = dist(rng);
    }
    normalizeDecisionVector();
}

void BioAgent::normalizeDecisionVector() {
    // 确保所有值在0.0-1.0范围内
    for (int i = 0; i < DECISION_VECTOR_DIMENSIONS; ++i) {
        decisionVector[i] = std::max(0.0, std::min(1.0, decisionVector[i]));
    }
}

std::string BioAgent::getDecisionVectorString() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    const char* dimensionNames[DECISION_VECTOR_DIMENSIONS] = {
        "快乐", "悲伤", "愤怒", "恐惧", "厌恶", "惊讶",
        "信任", "期待", "宁静", "效价", "唤醒度", "优势度"
    };
    
    ss << "[";
    for (int i = 0; i < DECISION_VECTOR_DIMENSIONS; ++i) {
        ss << dimensionNames[i] << ": " << decisionVector[i];
        if (i < DECISION_VECTOR_DIMENSIONS - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    
    return ss.str();
}

double BioAgent::calculateSimilarity(const BioAgent& other) const {
    if (decisionVector.empty() || other.decisionVector.empty() || 
        decisionVector.size() != other.decisionVector.size()) {
        return 0.0;
    }
    
    // 计算余弦相似度
    double dotProduct = 0.0;
    double normA = 0.0;
    double normB = 0.0;
    
    for (size_t i = 0; i < decisionVector.size(); ++i) {
        dotProduct += decisionVector[i] * other.decisionVector[i];
        normA += decisionVector[i] * decisionVector[i];
        normB += other.decisionVector[i] * other.decisionVector[i];
    }
    
    if (normA == 0.0 || normB == 0.0) {
        return 0.0;
    }
    
    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}