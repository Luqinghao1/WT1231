/*
 * modelsolver01_06.h
 * 文件作用: 压裂水平井复合页岩油模型的核心算法类
 */

#ifndef MODELSOLVER01_06_H
#define MODELSOLVER01_06_H

#include "modelenums.h"
#include <QMap>
#include <QVector>
#include <QString>
#include <tuple>
#include <functional>

// 类型定义: <时间, 压力, 导数>
using ModelCurveData = std::tuple<QVector<double>, QVector<double>, QVector<double>>;

class ModelSolver01_06
{
public:
    static ModelCurveData calculateTheoreticalCurve(ModelType type,
                                                    const QMap<QString, double>& params,
                                                    const QVector<double>& providedTime = QVector<double>(),
                                                    bool highPrecision = true);

private:
    static void calculatePDandDeriv(const QVector<double>& tD,
                                    const QMap<QString, double>& params,
                                    std::function<double(double, const QMap<QString, double>&)> laplaceFunc,
                                    QVector<double>& outPD,
                                    QVector<double>& outDeriv,
                                    bool highPrecision);

    static double flaplace_composite(double z, const QMap<QString, double>& p, ModelType type);

    static double PWD_composite(double z, double fs1, double fs2, double M12,
                                double LfD, double rmD, double reD, int nf,
                                const QVector<double>& xwD, ModelType type);

    static double scaled_besseli(int v, double x);
    static double gauss15(std::function<double(double)> f, double a, double b);
    static double adaptiveGauss(std::function<double(double)> f, double a, double b, double eps, int depth, int maxDepth);
    static double stefestCoefficient(int i, int N);
    static double factorial(int n);
};

#endif // MODELSOLVER01_06_H
