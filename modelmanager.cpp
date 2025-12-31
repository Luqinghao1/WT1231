/*
 * modelmanager.cpp
 * 文件作用: 模型管理类实现文件
 * 功能描述:
 * 1. 实例化 WT_ModelWidget 并将其加入界面布局。
 * 2. 实现 calculateTheoreticalCurve，直接调用 ModelSolver01_06 的静态方法，
 * 从而实现了界面与算法的解耦，提高了拟合计算的效率和安全性。
 */

#include "modelmanager.h"
#include "modelparameter.h"
#include <QVBoxLayout>
#include <QDebug>
#include <cmath>

ModelManager::ModelManager(QObject *parent)
    : QObject(parent)
    , m_highPrecision(true)
{
}

ModelManager::~ModelManager()
{
    // 界面控件由 Qt 父子对象机制自动管理内存，此处无需手动 delete widgets
}

void ModelManager::initializeModels(QWidget* parentContainer)
{
    // 清理旧列表（如果重新初始化）
    m_modelWidgets.clear();

    // 确保父容器有布局管理器
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(parentContainer->layout());
    if (!layout) {
        layout = new QVBoxLayout(parentContainer);
        parentContainer->setLayout(layout);
    }

    // 循环创建 6 种复合模型界面 (WT_ModelWidget)
    // 使用公共枚举 Model_1 到 Model_6
    for (int i = 0; i < 6; ++i) {
        ModelType type = static_cast<ModelType>(i);

        // 使用新的界面类 WT_ModelWidget
        WT_ModelWidget* widget = new WT_ModelWidget(type, parentContainer);

        // 默认隐藏，由外部逻辑控制显示哪一个
        widget->hide();

        layout->addWidget(widget);
        m_modelWidgets.append(widget);

        // 连接信号：当界面计算完成时，通知 Manager（可用于日志或后续处理）
        connect(widget, &WT_ModelWidget::calculationCompleted,
                this, &ModelManager::onModelCalculationFinished);
    }

    // 默认显示第一个模型
    if (!m_modelWidgets.isEmpty()) {
        m_modelWidgets.first()->show();
    }
}

// 核心修改点：计算理论曲线
// 不再调用 Widget 的方法，而是直接调用 Solver 的静态方法
ModelCurveData ModelManager::calculateTheoreticalCurve(ModelType type,
                                                       const QMap<QString, double>& params,
                                                       const QVector<double>& providedTime)
{
    // 直接调用纯数学计算类，传入当前管理的精度设置
    return ModelSolver01_06::calculateTheoreticalCurve(type, params, providedTime, m_highPrecision);
}

void ModelManager::setHighPrecision(bool high)
{
    m_highPrecision = high;
    // 同时更新所有界面显示的精度设置（如果有必要同步）
    for(auto w : m_modelWidgets) {
        w->setHighPrecision(high);
    }
}

void ModelManager::updateAllModelsBasicParameters()
{
    // 通知所有界面重置/更新基础参数
    for (auto widget : m_modelWidgets) {
        widget->onResetParameters();
    }
}

void ModelManager::clearCache()
{
    // 清理缓存逻辑（如有）
}

void ModelManager::onModelCalculationFinished(const QString& modelType, const QMap<QString, double>& params)
{
    emit calculationCompleted(modelType, params);
}

// 静态辅助函数：生成对数时间步长
QVector<double> ModelManager::generateLogTimeSteps(int nPoints, double logStart, double logEnd)
{
    QVector<double> t;
    if (nPoints < 2) return t;

    double step = (logEnd - logStart) / (nPoints - 1);
    for (int i = 0; i < nPoints; ++i) {
        t.append(pow(10.0, logStart + i * step));
    }
    return t;
}

// 静态辅助函数：获取模型名称
QString ModelManager::getModelTypeName(ModelType type)
{
    switch(type) {
    case Model_1: return "模型1: 变井储+无限大边界";
    case Model_2: return "模型2: 恒定井储+无限大边界";
    case Model_3: return "模型3: 变井储+封闭边界";
    case Model_4: return "模型4: 恒定井储+封闭边界";
    case Model_5: return "模型5: 变井储+定压边界";
    case Model_6: return "模型6: 恒定井储+定压边界";
    default: return "未知模型";
    }
}
