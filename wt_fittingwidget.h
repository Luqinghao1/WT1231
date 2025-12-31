/*
 * wt_fittingwidget.h
 * 文件作用: 试井拟合分析主界面类的头文件
 * 功能描述:
 * 1. 负责拟合界面的整体布局和交互。
 * 2. 包含图表显示、参数调整、数据导入导出等功能。
 * 3. 使用 QtConcurrent 进行后台拟合计算。
 */

#ifndef WT_FITTINGWIDGET_H
#define WT_FITTINGWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QFutureWatcher>
#include <QMap>
#include <QVector>
#include <QJsonObject>

#include "modelmanager.h"
#include "fittingparameterchart.h"
#include "mousezoom.h"
#include "chartsetting1.h"
#include "paramselectdialog.h"
#include "modelenums.h" // [新增] 引入公共枚举

namespace Ui {
class FittingWidget;
}

class FittingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FittingWidget(QWidget *parent = nullptr);
    ~FittingWidget();

    // 设置模型管理器 (用于调用计算算法)
    void setModelManager(ModelManager* m);

    // 设置项目数据模型 (用于加载观测数据)
    void setProjectDataModel(QStandardItemModel* model);

    // 更新基础参数 (从 ModelParameter 单例)
    void updateBasicParameters();

    // 加载/保存状态 (用于项目文件的保存和恢复)
    QJsonObject getJsonState() const;
    void loadFittingState(const QJsonObject& root);

    // 设置当前观测数据
    void setObservedDataToCurrent(const QVector<double>& t, const QVector<double>& p, const QVector<double>& d) {
        setObservedData(t, p, d);
    }

    // 加载所有保存的拟合状态（如有历史记录）
    void loadAllFittingStates() {}

    // 重置分析
    void resetAnalysis() {
        m_obsTime.clear(); m_obsDeltaP.clear(); m_obsDerivative.clear();
        m_plot->clearGraphs();
        setupPlot();
        initializeDefaultModel();
    }

signals:
    // 拟合迭代更新信号 (用于更新界面曲线)
    void sigIterationUpdated(double error, const QMap<QString, double>& currentParams,
                             const QVector<double>& t, const QVector<double>& p, const QVector<double>& d);

    // 进度信号
    void sigProgress(int percent);

    // 请求保存项目
    void sigRequestSave();

    // 拟合完成信号
    // [修改] 参数类型改为 ModelType
    void fittingCompleted(ModelType modelType, const QMap<QString, double>& parameters);

private slots:
    // 界面按钮槽函数
    void on_btnLoadData_clicked();      // 加载数据
    void on_btnSelectParams_clicked();  // 选择参数
    void on_btnRunFit_clicked();        // 开始拟合
    void on_btnStop_clicked();          // 停止拟合
    void on_btnImportModel_clicked();   // 刷新/生成曲线
    void on_btnResetParams_clicked();   // 重置参数
    void on_btnResetView_clicked();     // 重置视图
    void on_btnChartSettings_clicked(); // 图表设置
    void on_btn_modelSelect_clicked();  // 选择模型
    void on_btnExportData_clicked();    // 导出参数
    void on_btnExportChart_clicked();   // 导出图表
    void on_btnExportReport_clicked();  // 导出报告
    void on_btnSaveFit_clicked();       // 保存拟合

    void onSliderWeightChanged(int value);
    void onIterationUpdate(double err, const QMap<QString,double>& p, const QVector<double>& t, const QVector<double>& p_curve, const QVector<double>& d_curve);
    void onFitFinished();

private:
    void initUi();
    void setupPlot();
    void initializeDefaultModel();

    void setObservedData(const QVector<double>& t, const QVector<double>& deltaP, const QVector<double>& d);
    void updateModelCurve();
    void plotCurves(const QVector<double>& t, const QVector<double>& p, const QVector<double>& d, bool isModel);

    QString getPlotImageBase64();

    // --- 拟合算法相关 ---
    // [修改] 参数类型改为 ModelType
    void runOptimizationTask(ModelType modelType, QList<FitParameter> fitParams, double weight);

    // [修改] 参数类型改为 ModelType
    void runLevenbergMarquardtOptimization(ModelType modelType, QList<FitParameter> params, double weight);

    // [修改] 参数类型改为 ModelType
    QVector<double> calculateResiduals(const QMap<QString, double>& params, ModelType modelType, double weight);

    // [修改] 参数类型改为 ModelType
    QVector<QVector<double>> computeJacobian(const QMap<QString, double>& params, const QVector<double>& residuals, const QVector<int>& fitIndices, ModelType modelType, const QList<FitParameter>& currentFitParams, double weight);

    QVector<double> solveLinearSystem(const QVector<QVector<double>>& A, const QVector<double>& b);
    double calculateSumSquaredError(const QVector<double>& residuals);

private:
    Ui::FittingWidget *ui;
    ModelManager* m_modelManager;
    QStandardItemModel* m_projectModel;

    FittingParameterChart* m_paramChart;
    MouseZoom* m_plot;
    QCPTextElement* m_plotTitle;

    // [修改] 参数类型改为 ModelType
    ModelType m_currentModelType;

    QVector<double> m_obsTime;
    QVector<double> m_obsDeltaP;
    QVector<double> m_obsDerivative;

    bool m_isFitting;
    bool m_stopRequested;
    QFutureWatcher<void> m_watcher;
};

#endif // WT_FITTINGWIDGET_H
