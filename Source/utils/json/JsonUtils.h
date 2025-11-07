#pragma once
#include "pch.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>

namespace engine {
    // JSON解析工具类，处理特殊字符和转义情况
    class JsonUtils {
    public:
        // 预处理JSON字符串，处理特殊字符和转义
        static std::string preprocessJsonString(const std::string& rawJson) {
            std::string processed = rawJson;
            
            // 处理常见的转义问题
            processed = handleEscapedCharacters(processed);
            
            // 处理非标准JSON注释（如果存在）
            processed = removeJsonComments(processed);
            
            // 处理尾随逗号（非标准JSON）
            processed = removeTrailingCommas(processed);
            
            return processed;
        }
        
        // 安全解析JSON，带有详细的错误信息
        static nlohmann::json safeParseJson(const std::string& jsonStr) {
            try {
                // 预处理JSON字符串
                std::string processed = preprocessJsonString(jsonStr);
                
                // 解析JSON
                return nlohmann::json::parse(processed);
            } 
            catch (const nlohmann::json::parse_error& e) {
                // 提供更详细的错误信息
                std::string errorMsg = "JSON Parse Error: ";
                errorMsg += e.what();
                
                // 尝试提供错误位置信息
                if (e.id == 101) { // syntax error
                    errorMsg += "\nThis might be caused by:";
                    errorMsg += "\n- Missing or extra commas";
                    errorMsg += "\n- Unescaped characters in strings";
                    errorMsg += "\n- Mismatched brackets or braces";
                    errorMsg += "\n- Invalid Unicode sequences";
                }
                
                // 记录错误并重新抛出
                spdlog::error(errorMsg);
                throw;
            }
        }
        
        // 从文件安全加载JSON
        static nlohmann::json loadJsonFromFile(const std::string& filePath) {
            try {
                std::ifstream file(filePath);
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open file: " + filePath);
                }
                
                // 读取整个文件内容
                std::string content((std::istreambuf_iterator<char>(file)), 
                                   std::istreambuf_iterator<char>());
                
                // 安全解析
                return safeParseJson(content);
            } 
            catch (const std::exception& e) {
                std::string errorMsg = "Failed to load JSON from file: " + filePath;
                errorMsg += "\nError: " + std::string(e.what());
                spdlog::error(errorMsg);
                throw;
            }
        }
        
        // 验证JSON结构是否符合预期
        static bool validateJsonStructure(const nlohmann::json& j, const std::vector<std::string>& requiredFields) {
            for (const auto& field : requiredFields) {
                if (!j.contains(field)) {
                    spdlog::error("Missing required field in JSON: {}", field);
                    return false;
                }
            }
            return true;
        }
        
    private:
        // 处理转义字符
        static std::string handleEscapedCharacters(const std::string& jsonStr) {
            std::string result = jsonStr;
            bool inString = false;
            
            for (size_t i = 0; i < result.size(); ++i) {
                if (result[i] == '"' && (i == 0 || result[i-1] != '\\')) {
                    inString = !inString;
                }
                
                // 在字符串内部且不是转义字符的情况下，替换反斜杠
                if (inString && result[i] == '\\' && (i+1 >= result.size() || result[i+1] != '"')) {
                    result[i] = '/';
                }
            }
            
            return result;
        }
        
        // 移除JSON注释（如果存在）
        static std::string removeJsonComments(const std::string& jsonStr) {
            std::string result;
            bool inString = false;
            bool inLineComment = false;
            bool inBlockComment = false;
            
            for (size_t i = 0; i < jsonStr.size(); ++i) {
                char c = jsonStr[i];
                char nextChar = (i + 1 < jsonStr.size()) ? jsonStr[i + 1] : '\0';
                
                if (inString) {
                    result += c;
                    if (c == '\\' && nextChar == '"') {
                        // 转义的引号
                        result += nextChar;
                        i++;
                    } else if (c == '"') {
                        // 字符串结束
                        inString = false;
                    }
                } else if (inLineComment) {
                    if (c == '\n') {
                        // 行注释结束
                        inLineComment = false;
                        result += c;
                    }
                } else if (inBlockComment) {
                    if (c == '*' && nextChar == '/') {
                        // 块注释结束
                        inBlockComment = false;
                        i++; // 跳过'/'
                    }
                } else {
                    if (c == '"') {
                        // 字符串开始
                        inString = true;
                        result += c;
                    } else if (c == '/' && nextChar == '/') {
                        // 行注释开始
                        inLineComment = true;
                        i++; // 跳过第二个'/'
                    } else if (c == '/' && nextChar == '*') {
                        // 块注释开始
                        inBlockComment = true;
                        i++; // 跳过'*'
                    } else {
                        result += c;
                    }
                }
            }
            
            return result;
        }
        
        // 移除尾随逗号
        static std::string removeTrailingCommas(const std::string& jsonStr) {
            std::string result = jsonStr;
            
            // 移除对象和数组中的尾随逗号
            // 匹配 }, 或 ] 后面跟着空白字符然后是 } 或 ]
            std::regex trailingCommaPattern(R"((,)\s*([}\]]))");
            result = std::regex_replace(result, trailingCommaPattern, "$2");
            
            return result;
        }
    };
}