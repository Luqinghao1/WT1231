/*
 * wt_modelwidget.cpp
 * 文件作用: 压裂水平井复合页岩油模型计算界面实现 (UI层)
 * 功能描述:
 * 1. 实现了用户与模型的交互逻辑。
 * 2. 负责收集界面上的参数，组装成 QMap。
 * 3. 调用 ModelSolver01_06 进行数学计算。
 * 4. 将计算结果显示在 ChartWidget 图表和文本框中。
 */

#include "wt_modelwidget.h"
#include "ui_wt_modelwidget.h"
#include "modelsolver01_06.h" // 引入计算核心
#include "modelmanager.h"     // 可能需要用到其中的通用工具函数
#include "modelparameter.h"   // 用于获取默认参数

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QSplitter>

WT_ModelWidget::WT_ModelWidget(ModelType type, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WT_ModelWidget)
    , m_type(type)
    , m_highPrecision(true)
{
    ui->setupUi(this);
    m_colorList = { Qt::red, Qt::blue, QColor(0,180,0), Qt::magenta, QColor(255,140,0), Qt::cyan };

    // [布局] 设置 Splitter 初始比例 (左 20% : 右 80%)
    QList<int> sizes;
    sizes << 240 << 960;
    ui->splitter->setSizes(sizes);
    ui->splitter->setCollapsible(0, false); // 左侧不可折叠

    // [界面] 设置当前模型名称到选择按钮
    ui->btnSelectModel->setText(getModelName() + "  (点击切换)");

    initUi();
    initChart();
    setupConnections();
    onResetParameters();
}

WT_ModelWidget::~WT_ModelWidget() { delete ui; }

QString WT_ModelWidget::getModelName() const {
    switch(m_type) {
    case Model_1: return "模型1: 变井储+无限大边界";
    case Model_2: return "模型2: 恒定井储+无限大边界";
    case Model_3: return "模型3: 变井储+封闭边界";
    case Model_4: return "模型4: 恒定井储+封闭边界";
    case Model_5: return "模型5: 变井储+定压边界";
    case Model_6: return "模型6: 恒定井储+定压边界";
    default: return "未知模型";
    }
}

void WT_ModelWidget::initUi() {
    // 根据模型类型显示或隐藏特定参数控件
    if (m_type == Model_1 || m_type == Model_2) {
        ui->label_reD->setVisible(false);
        ui->reDEdit->setVisible(false);
    } else {
        ui->label_reD->setVisible(true);
        ui->reDEdit->setVisible(true);
    }

    bool hasStorage = (m_type == Model_1 || m_type == Model_3 || m_type == Model_5);
    ui->label_cD->setVisible(hasStorage);
    ui->cDEdit->setVisible(hasStorage);
    ui->label_s->setVisible(hasStorage);
    ui->sEdit->setVisible(hasStorage);
}

void WT_ModelWidget::initChart() {
    MouseZoom* plot = ui->chartWidget->getPlot();

    plot->setBackground(Qt::white);
    plot->axisRect()->setBackground(Qt::white);

    // 设置对数坐标轴
    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    plot->xAxis->setScaleType(QCPAxis::stLogarithmic); plot->xAxis->setTicker(logTicker);
    plot->yAxis->setScaleType(QCPAxis::stLogarithmic); plot->yAxis->setTicker(logTicker);
    plot->xAxis->setNumberFormat("eb"); plot->xAxis->setNumberPrecision(0);
    plot->yAxis->setNumberFormat("eb"); plot->yAxis->setNumberPrecision(0);

    // 设置字体
    QFont labelFont("Microsoft YaHei", 10, QFont::Bold);
    QFont tickFont("Microsoft YaHei", 9);
    plot->xAxis->setLabel("时间 Time (h)");
    plot->yAxis->setLabel("压力 & 导数 Pressure & Derivative (MPa)");
    plot->xAxis->setLabelFont(labelFont); plot->yAxis->setLabelFont(labelFont);
    plot->xAxis->setTickLabelFont(tickFont); plot->yAxis->setTickLabelFont(tickFont);

    // 设置辅助轴
    plot->xAxis2->setVisible(true); plot->yAxis2->setVisible(true);
    plot->xAxis2->setTickLabels(false); plot->yAxis2->setTickLabels(false);
    connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2, SLOT(setRange(QCPRange)));
    connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2, SLOT(setRange(QCPRange)));
    plot->xAxis2->setScaleType(QCPAxis::stLogarithmic); plot->yAxis2->setScaleType(QCPAxis::stLogarithmic);
    plot->xAxis2->setTicker(logTicker); plot->yAxis2->setTicker(logTicker);

    // 设置网格线
    plot->xAxis->grid()->setVisible(true); plot->yAxis->grid()->setVisible(true);
    plot->xAxis->grid()->setSubGridVisible(true); plot->yAxis->grid()->setSubGridVisible(true);
    plot->xAxis->grid()->setPen(QPen(QColor(220, 220, 220), 1, Qt::SolidLine));
    plot->yAxis->grid()->setPen(QPen(QColor(220, 220, 220), 1, Qt::SolidLine));
    plot->xAxis->grid()->setSubGridPen(QPen(QColor(240, 240, 240), 1, Qt::DotLine));
    plot->yAxis->grid()->setSubGridPen(QPen(QColor(240, 240, 240), 1, Qt::DotLine));

    plot->xAxis->setRange(1e-3, 1e3); plot->yAxis->setRange(1e-3, 1e2);

    // 设置图例
    plot->legend->setVisible(true);
    plot->legend->setFont(QFont("Microsoft YaHei", 9));
    plot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));

    ui->chartWidget->setTitle("复合页岩油储层试井曲线");
}

void WT_ModelWidget::setupConnections() {
    connect(ui->calculateButton, &QPushButton::clicked, this, &WT_ModelWidget::onCalculateClicked);
    connect(ui->resetButton, &QPushButton::clicked, this, &WT_ModelWidget::onResetParameters);
    connect(ui->chartWidget, &ChartWidget::exportDataTriggered, this, &WT_ModelWidget::onExportData);
    connect(ui->btnExportDataTab, &QPushButton::clicked, this, &WT_ModelWidget::onExportData);
    connect(ui->LEdit, &QLineEdit::editingFinished, this, &WT_ModelWidget::onDependentParamsChanged);
    connect(ui->LfEdit, &QLineEdit::editingFinished, this, &WT_ModelWidget::onDependentParamsChanged);
    connect(ui->checkShowPoints, &QCheckBox::toggled, this, &WT_ModelWidget::onShowPointsToggled);

    // 转发模型选择按钮信号
    connect(ui->btnSelectModel, &QPushButton::clicked, this, &WT_ModelWidget::requestModelSelection);
}

void WT_ModelWidget::setHighPrecision(bool high) { m_highPrecision = high; }

QVector<double> WT_ModelWidget::parseInput(const QString& text) {
    QVector<double> values;
    QString cleanText = text;
    cleanText.replace("，", ",");
    QStringList parts = cleanText.split(",", Qt::SkipEmptyParts);
    for(const QString& part : parts) {
        bool ok;
        double v = part.trimmed().toDouble(&ok);
        if(ok) values.append(v);
    }
    if(values.isEmpty()) values.append(0.0);
    return values;
}

void WT_ModelWidget::setInputText(QLineEdit* edit, double value) {
    if(!edit) return;
    edit->setText(QString::number(value, 'g', 8));
}

void WT_ModelWidget::onResetParameters() {
    ModelParameter* mp = ModelParameter::instance();

    setInputText(ui->phiEdit, mp->getPhi());
    setInputText(ui->hEdit, mp->getH());
    setInputText(ui->muEdit, mp->getMu());
    setInputText(ui->BEdit, mp->getB());
    setInputText(ui->CtEdit, mp->getCt());
    setInputText(ui->qEdit, mp->getQ());

    setInputText(ui->tEdit, 1000.0);
    setInputText(ui->pointsEdit, 100);

    setInputText(ui->kfEdit, 1e-3);
    setInputText(ui->kmEdit, 1e-4);
    setInputText(ui->LEdit, 1000.0);
    setInputText(ui->LfEdit, 100.0);
    setInputText(ui->nfEdit, 4);
    setInputText(ui->rmDEdit, 4.0);
    setInputText(ui->omga1Edit, 0.4);
    setInputText(ui->omga2Edit, 0.08);
    setInputText(ui->remda1Edit, 0.001);
    setInputText(ui->gamaDEdit, 0.02);

    bool isInfinite = (m_type == Model_1 || m_type == Model_2);
    if (!isInfinite) {
        setInputText(ui->reDEdit, 10.0);
    }

    bool hasStorage = (m_type == Model_1 || m_type == Model_3 || m_type == Model_5);
    if (hasStorage) {
        setInputText(ui->cDEdit, 0.01);
        setInputText(ui->sEdit, 1.0);
    }

    onDependentParamsChanged();
}

void WT_ModelWidget::onDependentParamsChanged() {
    double L = parseInput(ui->LEdit->text()).first();
    double Lf = parseInput(ui->LfEdit->text()).first();
    if (L > 1e-9) setInputText(ui->LfDEdit, Lf / L);
    else setInputText(ui->LfDEdit, 0.0);
}

void WT_ModelWidget::onShowPointsToggled(bool checked) {
    MouseZoom* plot = ui->chartWidget->getPlot();
    for(int i = 0; i < plot->graphCount(); ++i) {
        if (checked) plot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5));
        else plot->graph(i)->setScatterStyle(QCPScatterStyle::ssNone);
    }
    plot->replot();
}

void WT_ModelWidget::onCalculateClicked() {
    ui->calculateButton->setEnabled(false);
    ui->calculateButton->setText("计算中...");
    QCoreApplication::processEvents();
    runCalculation();
    ui->calculateButton->setEnabled(true);
    ui->calculateButton->setText("开始计算");
}

void WT_ModelWidget::runCalculation() {
    MouseZoom* plot = ui->chartWidget->getPlot();
    plot->clearGraphs();

    // 1. 收集界面参数
    QMap<QString, QVector<double>> rawParams;
    rawParams["phi"] = parseInput(ui->phiEdit->text());
    rawParams["h"] = parseInput(ui->hEdit->text());
    rawParams["mu"] = parseInput(ui->muEdit->text());
    rawParams["B"] = parseInput(ui->BEdit->text());
    rawParams["Ct"] = parseInput(ui->CtEdit->text());
    rawParams["q"] = parseInput(ui->qEdit->text());
    rawParams["t"] = parseInput(ui->tEdit->text());

    rawParams["kf"] = parseInput(ui->kfEdit->text());
    rawParams["km"] = parseInput(ui->kmEdit->text());
    rawParams["L"] = parseInput(ui->LEdit->text());
    rawParams["Lf"] = parseInput(ui->LfEdit->text());
    rawParams["nf"] = parseInput(ui->nfEdit->text());
    rawParams["rmD"] = parseInput(ui->rmDEdit->text());
    rawParams["omega1"] = parseInput(ui->omga1Edit->text());
    rawParams["omega2"] = parseInput(ui->omga2Edit->text());
    rawParams["lambda1"] = parseInput(ui->remda1Edit->text());
    rawParams["gamaD"] = parseInput(ui->gamaDEdit->text());

    if (ui->reDEdit->isVisible()) rawParams["reD"] = parseInput(ui->reDEdit->text());
    else rawParams["reD"] = {0.0};

    if (ui->cDEdit->isVisible()) {
        rawParams["cD"] = parseInput(ui->cDEdit->text());
        rawParams["S"] = parseInput(ui->sEdit->text());
    } else {
        rawParams["cD"] = {0.0};
        rawParams["S"] = {0.0};
    }

    // 2. 检查是否有敏感性参数（输入了多个值）
    QString sensitivityKey = "";
    QVector<double> sensitivityValues;
    for(auto it = rawParams.begin(); it != rawParams.end(); ++it) {
        if(it.key() == "t") continue; // 时间不作为敏感性参数
        if(it.value().size() > 1) {
            sensitivityKey = it.key();
            sensitivityValues = it.value();
            break;
        }
    }
    bool isSensitivity = !sensitivityKey.isEmpty();

    // 3. 准备基准参数
    QMap<QString, double> baseParams;
    for(auto it = rawParams.begin(); it != rawParams.end(); ++it) {
        baseParams[it.key()] = it.value().isEmpty() ? 0.0 : it.value().first();
    }
    baseParams["N"] = m_highPrecision ? 8.0 : 4.0;

    // 计算无因次缝长
    if(baseParams["L"] > 1e-9) baseParams["LfD"] = baseParams["Lf"] / baseParams["L"];
    else baseParams["LfD"] = 0;

    // 4. 准备时间步长
    int nPoints = ui->pointsEdit->text().toInt();
    if(nPoints < 5) nPoints = 5;
    double maxTime = baseParams.value("t", 1000.0);
    if(maxTime < 1e-3) maxTime = 1000.0;
    // 使用 ModelManager 工具函数或自行实现对数时间生成
    QVector<double> t = ModelManager::generateLogTimeSteps(nPoints, -3.0, log10(maxTime));

    int iterations = isSensitivity ? sensitivityValues.size() : 1;
    iterations = qMin(iterations, (int)m_colorList.size());

    QString resultTextHeader = QString("计算完成 (%1)\n").arg(getModelName());
    if(isSensitivity) resultTextHeader += QString("敏感性参数: %1\n").arg(sensitivityKey);

    // 5. 循环计算
    for(int i = 0; i < iterations; ++i) {
        QMap<QString, double> currentParams = baseParams;
        double val = 0;
        if (isSensitivity) {
            val = sensitivityValues[i];
            currentParams[sensitivityKey] = val;
            // 如果修改了 L 或 Lf，重新计算 LfD
            if (sensitivityKey == "L" || sensitivityKey == "Lf") {
                if(currentParams["L"] > 1e-9) currentParams["LfD"] = currentParams["Lf"] / currentParams["L"];
            }
        }

        // 调用 ModelSolver 计算核心 (替代原有的内部计算逻辑)
        ModelCurveData res = ModelSolver01_06::calculateTheoreticalCurve(m_type, currentParams, t, m_highPrecision);

        // 保存最后一次结果用于显示
        res_tD = std::get<0>(res);
        res_pD = std::get<1>(res);
        res_dpD = std::get<2>(res);

        QColor curveColor = isSensitivity ? m_colorList[i] : Qt::red;
        QString legendName;
        if (isSensitivity) legendName = QString("%1 = %2").arg(sensitivityKey).arg(val);
        else legendName = "理论曲线";

        plotCurve(res, legendName, curveColor, isSensitivity);
    }

    // 6. 显示结果文本
    QString resultText = resultTextHeader;
    resultText += "t(h)\t\tDp(MPa)\t\tdDp(MPa)\n";
    for(int i=0; i<res_pD.size(); ++i) {
        resultText += QString("%1\t%2\t%3\n").arg(res_tD[i],0,'e',4).arg(res_pD[i],0,'e',4).arg(res_dpD[i],0,'e',4);
    }
    ui->resultTextEdit->setText(resultText);

    ui->chartWidget->getPlot()->rescaleAxes();
    if(plot->xAxis->range().lower <= 0) plot->xAxis->setRangeLower(1e-3);
    if(plot->yAxis->range().lower <= 0) plot->yAxis->setRangeLower(1e-3);
    plot->replot();

    onShowPointsToggled(ui->checkShowPoints->isChecked());
    emit calculationCompleted(getModelName(), baseParams);
}

void WT_ModelWidget::plotCurve(const ModelCurveData& data, const QString& name, QColor color, bool isSensitivity) {
    MouseZoom* plot = ui->chartWidget->getPlot();

    const QVector<double>& t = std::get<0>(data);
    const QVector<double>& p = std::get<1>(data);
    const QVector<double>& d = std::get<2>(data);

    QCPGraph* graphP = plot->addGraph();
    graphP->setData(t, p);
    graphP->setPen(QPen(color, 2, Qt::SolidLine));

    QCPGraph* graphD = plot->addGraph();
    graphD->setData(t, d);

    if (isSensitivity) {
        graphD->setPen(QPen(color, 2, Qt::DashLine));
        graphP->setName(name);
        graphD->removeFromLegend();
    } else {
        graphP->setPen(QPen(Qt::red, 2));
        graphP->setName("压力");
        graphD->setPen(QPen(Qt::blue, 2));
        graphD->setName("压力导数");
    }
}

void WT_ModelWidget::onExportData() {
    if (res_tD.isEmpty()) return;
    QString defaultDir = ModelParameter::instance()->getProjectPath();
    if(defaultDir.isEmpty()) defaultDir = ".";
    QString path = QFileDialog::getSaveFileName(this, "导出CSV数据", defaultDir + "/CalculatedData.csv", "CSV Files (*.csv)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        out << "t,Dp,dDp\n";
        for (int i = 0; i < res_tD.size(); ++i) {
            double dp = (i < res_dpD.size()) ? res_dpD[i] : 0.0;
            out << res_tD[i] << "," << res_pD[i] << "," << dp << "\n";
        }
        f.close();
        QMessageBox::information(this, "导出成功", "数据文件已保存");
    }
}
