/*
 * modelenums.h
 * 文件作用: 定义项目通用的枚举类型
 * 功能描述: 包含模型类型的枚举定义，供全局使用。
 */

#ifndef MODELENUMS_H
#define MODELENUMS_H

// 定义模型类型枚举
enum ModelType {
    Model_1 = 0, // 无限大 + 变井储
    Model_2,     // 无限大 + 恒定井储
    Model_3,     // 封闭边界 + 变井储
    Model_4,     // 封闭边界 + 恒定井储
    Model_5,     // 定压边界 + 变井储
    Model_6      // 定压边界 + 恒定井储
};

#endif // MODELENUMS_H
