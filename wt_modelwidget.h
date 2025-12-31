/*
 * wt_modelwidget.h
 * 文件作用: 压裂水平井复合页岩油模型计算界面头文件 (UI层)
 * 功能描述:
 * 1. 负责管理用户界面交互，包括参数输入、按钮响应和图表展示。
 * 2. 替代原有的 ModelWidget01_06 类。
 * 3. 调用 ModelSolver01_06 进行核心计算。
 * 4. 引用通用的 ChartWidget 组件。
 */

#ifndef WT_MODELWIDGET_H
#define WT_MODELWIDGET_H

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QColor>
#include <tuple>
#include "modelenums.h"
#include "modelsolver01_06.h" // 引用模型求解器类型
#include "chartwidget.h"

namespace Ui {
class WT_ModelWidget;
}

class WT_ModelWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数，接收模型类型
    explicit WT_ModelWidget(ModelType type, QWidget *parent = nullptr);
    ~WT_ModelWidget();

    // 设置是否使用高精度计算(更多的Stehfest项数)
    void setHighPrecision(bool high);

    // 获取当前模型名称（用于显示）
    QString getModelName() const;

signals:
    // 计算完成信号，传递模型类型名称和参数
    void calculationCompleted(const QString& modelType, const QMap<QString, double>& params);

    // 请求模型选择的信号，由管理器接收以切换模型
    void requestModelSelection();

public slots:
    // 界面交互槽函数
    void onCalculateClicked();          // 点击计算按钮
    void onResetParameters();           // 点击重置参数按钮
    void onDependentParamsChanged();    // 相关参数变更(如 L, Lf -> LfD)
    void onShowPointsToggled(bool checked); // 切换显示数据点
    void onExportData();                // 导出数据

private:
    // 初始化界面控件状态
    void initUi();
    // 初始化图表设置
    void initChart();
    // 建立信号槽连接
    void setupConnections();
    // 执行计算流程
    void runCalculation();

    // 辅助工具函数
    QVector<double> parseInput(const QString& text);
    void setInputText(QLineEdit* edit, double value);
    void plotCurve(const ModelCurveData& data, const QString& name, QColor color, bool isSensitivity);

private:
    Ui::WT_ModelWidget *ui;
    ModelType m_type;
    bool m_highPrecision;
    QList<QColor> m_colorList; // 曲线颜色列表

    // 缓存计算结果
    QVector<double> res_tD;
    QVector<double> res_pD;
    QVector<double> res_dpD;
};

#endif // WT_MODELWIDGET_H
