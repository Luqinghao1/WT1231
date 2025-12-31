/*
 * modelselect.h
 * 文件作用: 模型选择对话框头文件
 * 功能描述: 提供一个对话框供用户选择不同的试井解释模型。
 */

#ifndef MODELSELECT_H
#define MODELSELECT_H

#include <QDialog>

namespace Ui {
class ModelSelect;
}

class ModelSelect : public QDialog
{
    Q_OBJECT

public:
    explicit ModelSelect(QWidget *parent = nullptr);
    ~ModelSelect();

    QString getSelectedModelCode() const;
    QString getSelectedModelName() const;

private slots:
    void onSelectionChanged();
    void onAccepted();

private:
    void initOptions();

private:
    Ui::ModelSelect *ui;
    QString m_selectedModelCode;
    QString m_selectedModelName;
};

#endif // MODELSELECT_H
