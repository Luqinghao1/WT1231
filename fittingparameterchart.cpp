/*
 * fittingparameterchart.cpp
 * 文件作用: 拟合参数表格管理类实现文件
 * 功能描述:
 * 1. 实现了参数表格的初始化、数据填充和交互逻辑。
 * 2. 定义了不同模型下参数的默认值和可见性。
 */

#include "fittingparameterchart.h"
#include "modelmanager.h"
#include <QHeaderView>
#include <QCheckBox>
#include <QDebug>

FittingParameterChart::FittingParameterChart(QTableWidget* table, QObject *parent)
    : QObject(parent), m_table(table), m_modelManager(nullptr)
{
    initTable();
}

void FittingParameterChart::setModelManager(ModelManager* manager)
{
    m_modelManager = manager;
}

void FittingParameterChart::initTable()
{
    if(!m_table) return;

    QStringList headers;
    headers << "拟合" << "参数" << "数值" << "下限" << "上限";
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(headers);

    // 设置列宽比例
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 40);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
}

// [修改] 参数类型改为 ModelType
void FittingParameterChart::resetParams(ModelType type)
{
    m_params.clear();

    // 获取全局默认参数
    ModelParameter* mp = ModelParameter::instance();

    // 定义所有可能的参数
    // 格式: {Name, DisplayName, Value, Min, Max, IsFit, IsVisible}

    // 1. 基础参数
    m_params.append({"kf", "内区渗透率", 1.0, 0.001, 1000.0, true, true});
    m_params.append({"km", "外区渗透率", 0.1, 0.0001, 100.0, true, true});
    m_params.append({"L", "水平井长", mp->getL(), 100.0, 5000.0, false, true});
    m_params.append({"Lf", "裂缝半长", 100.0, 10.0, 1000.0, true, true});
    m_params.append({"nf", "裂缝条数", 4.0, 1.0, 50.0, false, true}); // 通常为整数，不拟合

    // 2. 双重介质参数
    m_params.append({"omega1", "内区储容比", 0.1, 0.001, 1.0, true, true});
    m_params.append({"omega2", "外区储容比", 0.01, 0.001, 1.0, true, true});
    m_params.append({"lambda1", "窜流系数", 1e-6, 1e-9, 1.0, true, true});

    // 3. 几何参数
    m_params.append({"rmD", "复合半径", 5.0, 1.1, 100.0, true, true});

    // 4. 外边界 (仅对封闭/定压模型可见)
    bool hasBoundary = (type == Model_3 || type == Model_4 || type == Model_5 || type == Model_6);
    m_params.append({"reD", "外边界半径", 20.0, 5.0, 5000.0, true, hasBoundary});

    // 5. 井储与表皮 (仅对变井储模型可见)
    bool hasStorage = (type == Model_1 || type == Model_3 || type == Model_5);
    m_params.append({"cD", "无因次井储", 0.01, 1e-5, 1000.0, true, hasStorage});
    m_params.append({"S", "表皮系数", 0.0, -5.0, 50.0, true, hasStorage});

    // 6. 压敏 (全部可见)
    m_params.append({"gamaD", "压敏系数", 0.0, 0.0, 0.5, true, true});

    refreshTable();
}

// [修改] 参数类型改为 ModelType
void FittingParameterChart::switchModel(ModelType newType)
{
    // 切换模型时，更新参数的可见性
    // 例如：从无限大模型切到封闭边界，reD 应该显示出来

    bool hasBoundary = (newType == Model_3 || newType == Model_4 || newType == Model_5 || newType == Model_6);
    bool hasStorage = (newType == Model_1 || newType == Model_3 || newType == Model_5);

    for(int i=0; i<m_params.size(); ++i) {
        if(m_params[i].name == "reD") {
            m_params[i].isVisible = hasBoundary;
        }
        else if(m_params[i].name == "cD" || m_params[i].name == "S") {
            m_params[i].isVisible = hasStorage;
        }
    }
    refreshTable();
}

void FittingParameterChart::refreshTable()
{
    m_table->clearContents();

    // 计算可见行数
    int visibleCount = 0;
    for(const auto& p : m_params) if(p.isVisible) visibleCount++;

    m_table->setRowCount(visibleCount);

    int row = 0;
    for(int i=0; i<m_params.size(); ++i) {
        if(!m_params[i].isVisible) continue;

        FitParameter& p = m_params[i];

        // 1. Checkbox (拟合?)
        QTableWidgetItem* checkItem = new QTableWidgetItem();
        checkItem->setCheckState(p.isFit ? Qt::Checked : Qt::Unchecked);
        // 存储参数在列表中的索引，方便反向更新
        checkItem->setData(Qt::UserRole, i);
        m_table->setItem(row, 0, checkItem);

        // 2. 参数名
        QTableWidgetItem* nameItem = new QTableWidgetItem(p.displayName + " (" + p.name + ")");
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable); // 不可编辑
        // 存储参数 Key
        nameItem->setData(Qt::UserRole, p.name);
        m_table->setItem(row, 1, nameItem);

        // 3. 数值
        QTableWidgetItem* valItem = new QTableWidgetItem(QString::number(p.value));
        m_table->setItem(row, 2, valItem);

        // 4. 下限
        QTableWidgetItem* minItem = new QTableWidgetItem(QString::number(p.min));
        m_table->setItem(row, 3, minItem);

        // 5. 上限
        QTableWidgetItem* maxItem = new QTableWidgetItem(QString::number(p.max));
        m_table->setItem(row, 4, maxItem);

        row++;
    }
}

void FittingParameterChart::updateParamsFromTable()
{
    // 遍历表格行，更新 m_params
    for(int row=0; row<m_table->rowCount(); ++row) {
        QTableWidgetItem* checkItem = m_table->item(row, 0);
        int paramIndex = checkItem->data(Qt::UserRole).toInt();

        if(paramIndex >= 0 && paramIndex < m_params.size()) {
            FitParameter& p = m_params[paramIndex];

            p.isFit = (checkItem->checkState() == Qt::Checked);

            bool ok;
            double v = m_table->item(row, 2)->text().toDouble(&ok);
            if(ok) p.value = v;

            double minV = m_table->item(row, 3)->text().toDouble(&ok);
            if(ok) p.min = minV;

            double maxV = m_table->item(row, 4)->text().toDouble(&ok);
            if(ok) p.max = maxV;
        }
    }
}

QList<FitParameter> FittingParameterChart::getParameters() const
{
    return m_params;
}

void FittingParameterChart::setParameters(const QList<FitParameter>& params)
{
    // 更新现有参数的值
    for(const auto& newP : params) {
        for(auto& oldP : m_params) {
            if(oldP.name == newP.name) {
                oldP.value = newP.value;
                oldP.min = newP.min;
                oldP.max = newP.max;
                oldP.isFit = newP.isFit;
                // isVisible 不覆盖，由当前模型决定
                break;
            }
        }
    }
    refreshTable();
}

void FittingParameterChart::getParamDisplayInfo(const QString& name, QString& displayName, QString& symbol, QString& uniSymbol, QString& unit)
{
    // 简单的映射表
    if(name == "kf") { displayName="内区渗透率"; symbol="kf"; uniSymbol="k<sub>f</sub>"; unit="mD"; }
    else if(name == "km") { displayName="外区渗透率"; symbol="km"; uniSymbol="k<sub>m</sub>"; unit="mD"; }
    else if(name == "L") { displayName="井长"; symbol="L"; uniSymbol="L"; unit="m"; }
    else if(name == "Lf") { displayName="缝长"; symbol="Lf"; uniSymbol="L<sub>f</sub>"; unit="m"; }
    else if(name == "omega1") { displayName="内区储容比"; symbol="ω1"; uniSymbol="ω<sub>1</sub>"; unit="无因次"; }
    else if(name == "omega2") { displayName="外区储容比"; symbol="ω2"; uniSymbol="ω<sub>2</sub>"; unit="无因次"; }
    else if(name == "lambda1") { displayName="窜流系数"; symbol="λ1"; uniSymbol="λ<sub>1</sub>"; unit="无因次"; }
    else if(name == "rmD") { displayName="复合半径"; symbol="rmD"; uniSymbol="r<sub>mD</sub>"; unit="无因次"; }
    else if(name == "reD") { displayName="外边界半径"; symbol="reD"; uniSymbol="r<sub>eD</sub>"; unit="无因次"; }
    else if(name == "cD") { displayName="井储系数"; symbol="CD"; uniSymbol="C<sub>D</sub>"; unit="无因次"; }
    else if(name == "S") { displayName="表皮系数"; symbol="S"; uniSymbol="S"; unit="无因次"; }
    else if(name == "gamaD") { displayName="压敏系数"; symbol="γD"; uniSymbol="γ<sub>D</sub>"; unit="无因次"; }
    else { displayName=name; symbol=name; uniSymbol=name; unit=""; }
}
