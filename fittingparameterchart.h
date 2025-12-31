/*
 * fittingparameterchart.h
 * 文件作用: 拟合参数表格管理类头文件
 * 功能描述:
 * 1. 管理拟合界面左侧的参数表格 (QTableWidget)。
 * 2. 负责参数的显示、读取、更新和模型切换时的参数重置。
 */

#ifndef FITTINGPARAMETERCHART_H
#define FITTINGPARAMETERCHART_H

#include <QObject>
#include <QTableWidget>
#include <QList>
#include <QMap>
#include "modelparameter.h"
#include "modelenums.h" // [新增] 引入公共枚举

class ModelManager; // 前向声明

// 定义拟合参数结构体
struct FitParameter {
    QString name;       // 参数英文名 (如 "C", "Skin")
    QString displayName;// 显示中文名
    double value;       // 当前值
    double min;         // 下限
    double max;         // 上限
    bool isFit;         // 是否参与拟合
    bool isVisible;     // 是否在当前模型中可见
};

class FittingParameterChart : public QObject
{
    Q_OBJECT
public:
    explicit FittingParameterChart(QTableWidget* table, QObject *parent = nullptr);

    // 设置模型管理器
    void setModelManager(ModelManager* manager);

    // 根据模型类型重置参数（修改点：使用 ModelType）
    void resetParams(ModelType type);

    // 切换模型（修改点：使用 ModelType）
    void switchModel(ModelType newType);

    // 从表格更新参数到内部列表
    void updateParamsFromTable();

    // 获取当前参数列表
    QList<FitParameter> getParameters() const;

    // 设置参数列表到表格
    void setParameters(const QList<FitParameter>& params);

    // 静态工具：获取参数的显示信息（符号、单位等）
    static void getParamDisplayInfo(const QString& name, QString& displayName, QString& symbol, QString& uniSymbol, QString& unit);

private:
    // 初始化表格表头
    void initTable();
    // 刷新表格显示
    void refreshTable();

private:
    QTableWidget* m_table;
    ModelManager* m_modelManager;
    QList<FitParameter> m_params;
};

#endif // FITTINGPARAMETERCHART_H
