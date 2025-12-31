/*
 * modelmanager.h
 * 文件作用: 模型管理类头文件
 * 功能描述:
 * 1. 负责管理所有的试井解释模型（目前为复合模型01-06）。
 * 2. 充当“工厂”角色，在界面上创建并初始化具体的模型界面(WT_ModelWidget)。
 * 3. 作为“计算中转站”，为拟合模块(FittingWidget)提供理论曲线计算服务，底层调用 ModelSolver。
 * 4. 维护模型计算的全局设置（如精度控制）。
 */

#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QWidget>
#include <tuple>

// 引入公共枚举和新拆分的类
#include "modelenums.h"
#include "wt_modelwidget.h"
#include "modelsolver01_06.h"

class ModelManager : public QObject
{
    Q_OBJECT
public:
    explicit ModelManager(QObject *parent = nullptr);
    ~ModelManager();

    // 初始化所有模型界面，并将它们添加到父容器(stackedWidget)中
    void initializeModels(QWidget* parentContainer);

    // 获取特定模型的显示名称 (静态工具函数)
    static QString getModelTypeName(ModelType type);

    // 生成对数时间步长 (静态工具函数，供各模块通用)
    static QVector<double> generateLogTimeSteps(int nPoints, double logStart, double logEnd);

    // 设置计算精度 (True=高精度/更多Stehfest项, False=低精度/速度快)
    // 供拟合模块在迭代过程中调用以加速
    void setHighPrecision(bool high);

    // 计算理论曲线 (代理函数)
    // 拟合模块调用此函数，内部直接调用 ModelSolver 进行纯数学计算
    ModelCurveData calculateTheoreticalCurve(ModelType type,
                                             const QMap<QString, double>& params,
                                             const QVector<double>& providedTime = QVector<double>());

    // 更新所有模型的基础参数 (当项目参数变更时调用)
    void updateAllModelsBasicParameters();

    // 清除缓存 (如有)
    void clearCache();

signals:
    // 计算完成信号
    void calculationCompleted(const QString& modelType, const QMap<QString, double>& results);

public slots:
    // 响应具体模型界面的计算完成信号
    void onModelCalculationFinished(const QString& modelType, const QMap<QString, double>& params);

private:
    // 管理的所有模型界面实例列表
    QList<WT_ModelWidget*> m_modelWidgets;

    // 当前计算精度设置
    bool m_highPrecision;
};

#endif // MODELMANAGER_H
