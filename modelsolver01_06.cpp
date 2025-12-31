/*
 * modelsolver01_06.cpp
 * 文件作用: 压裂水平井复合页岩油模型核心算法实现
 * 功能描述:
 * 1. 实现了基于点源函数和叠加原理的压裂水平井压力响应计算。
 * 2. 使用 Eigen 库求解线性方程组。
 * 3. 使用 Boost 库计算 Bessel 函数。
 * 4. 实现了 Stehfest 数值反演算法将拉普拉斯空间解转换回实空间。
 */

#include "modelsolver01_06.h"
#include "modelmanager.h" // 用于获取对数时间步长生成函数
#include "pressurederivativecalculator.h" // 用于计算导数

#include <Eigen/Dense>
#include <boost/math/special_functions/bessel.hpp>
#include <cmath>
#include <algorithm>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 计算理论曲线的主入口
ModelCurveData ModelSolver01_06::calculateTheoreticalCurve(ModelType type,
                                                           const QMap<QString, double>& params,
                                                           const QVector<double>& providedTime,
                                                           bool highPrecision)
{
    // 1. 准备时间序列
    QVector<double> tPoints = providedTime;
    if (tPoints.isEmpty()) {
        // 如果未提供时间，生成默认的对数时间序列
        tPoints = ModelManager::generateLogTimeSteps(100, -3.0, 3.0);
    }

    // 2. 提取基础参数
    double phi = params.value("phi", 0.05);
    double mu = params.value("mu", 0.5);
    double B = params.value("B", 1.05);
    double Ct = params.value("Ct", 5e-4);
    double q = params.value("q", 5.0);
    double h = params.value("h", 20.0);
    double kf = params.value("kf", 1e-3);
    double L = params.value("L", 1000.0);

    // 3. 计算无因次时间 tD
    QVector<double> tD_vec;
    tD_vec.reserve(tPoints.size());
    for(double t : tPoints) {
        // 无因次时间定义
        double val = 14.4 * kf * t / (phi * mu * Ct * pow(L, 2));
        tD_vec.append(val);
    }

    // 4. 计算无因次压力和导数
    QVector<double> PD_vec, Deriv_vec;
    // 绑定拉普拉斯空间函数，将 ModelType 传递进去
    auto func = std::bind(&ModelSolver01_06::flaplace_composite, std::placeholders::_1, std::placeholders::_2, type);

    calculatePDandDeriv(tD_vec, params, func, PD_vec, Deriv_vec, highPrecision);

    // 5. 转换回实有量纲压力和导数
    // 压力转换系数
    double factor = 1.842e-3 * q * mu * B / (kf * h);
    QVector<double> finalP(tPoints.size()), finalDP(tPoints.size());

    for(int i=0; i<tPoints.size(); ++i) {
        finalP[i] = factor * PD_vec[i];
        finalDP[i] = factor * Deriv_vec[i];
    }

    return std::make_tuple(tPoints, finalP, finalDP);
}

// 通用的 Stehfest 数值反演计算流程
void ModelSolver01_06::calculatePDandDeriv(const QVector<double>& tD,
                                           const QMap<QString, double>& params,
                                           std::function<double(double, const QMap<QString, double>&)> laplaceFunc,
                                           QVector<double>& outPD,
                                           QVector<double>& outDeriv,
                                           bool highPrecision)
{
    int numPoints = tD.size();
    outPD.resize(numPoints);
    outDeriv.resize(numPoints);

    // 确定 Stehfest 参数 N
    int N_param = (int)params.value("N", 4);
    int N = highPrecision ? N_param : 4;
    if (N % 2 != 0) N = 4; // N 必须为偶数
    double ln2 = log(2.0);

    double gamaD = params.value("gamaD", 0.0);

    // 对每个时间点进行数值反演
    for (int k = 0; k < numPoints; ++k) {
        double t = tD[k];
        if (t <= 1e-12) { outPD[k] = 0; continue; }

        double pd_val = 0.0;
        for (int m = 1; m <= N; ++m) {
            double z = m * ln2 / t; // 拉普拉斯变量 s (此处用 z 表示)

            // 调用拉普拉斯空间解函数
            double pf = laplaceFunc(z, params);

            if (std::isnan(pf) || std::isinf(pf)) pf = 0.0;
            pd_val += stefestCoefficient(m, N) * pf;
        }
        outPD[k] = pd_val * ln2 / t;

        // 考虑压敏效应 (gamaD)
        if (std::abs(gamaD) > 1e-9) {
            double arg = 1.0 - gamaD * outPD[k];
            if (arg > 1e-12) {
                outPD[k] = -1.0 / gamaD * std::log(arg);
            }
        }
    }

    // 计算导数 (Bourdet 导数)
    if (numPoints > 2) {
        outDeriv = PressureDerivativeCalculator::calculateBourdetDerivative(tD, outPD, 0.1);
    } else {
        outDeriv.fill(0.0);
    }
}

// 拉普拉斯空间下的复合模型函数实现
double ModelSolver01_06::flaplace_composite(double z, const QMap<QString, double>& p, ModelType type) {
    // 提取模型参数
    double kf = p.value("kf");
    double km = p.value("km");
    double LfD = p.value("LfD");
    double rmD = p.value("rmD");
    double reD = p.value("reD", 0.0);
    double omga1 = p.value("omega1");
    double omga2 = p.value("omega2");
    double remda1 = p.value("lambda1");
    int nf = (int)p.value("nf", 4);
    if(nf < 1) nf = 1;

    double M12 = kf / km; // 渗透率比

    // 计算裂缝位置分布
    QVector<double> xwD;
    if (nf == 1) {
        xwD.append(0.0);
    } else {
        double start = -0.9;
        double end = 0.9;
        double step = (end - start) / (nf - 1);
        for(int i=0; i<nf; ++i) xwD.append(start + i * step);
    }

    // 双重介质参数处理
    double temp = omga2;
    double fs1 = omga1 + remda1 * temp / (remda1 + z * temp);
    double fs2 = M12 * temp;

    // 计算未考虑井储和表皮的压力
    double pf = PWD_composite(z, fs1, fs2, M12, LfD, rmD, reD, nf, xwD, type);

    // 考虑井筒储集系数(C)和表皮系数(S)
    // 仅在模型 1, 3, 5 (变井储) 或其他需要的情况下应用
    // 根据原逻辑：Model_1, Model_3, Model_5 包含 "变井储" 描述，但参数面板中
    // 井储和表皮输入框的显示逻辑是 hasStorage = (Model_1 || Model_3 || Model_5)
    // 所以此处逻辑保持一致
    bool hasStorage = (type == Model_1 || type == Model_3 || type == Model_5);

    if (hasStorage) {
        double CD = p.value("cD", 0.0);
        double S = p.value("S", 0.0);
        if (CD > 1e-12 || std::abs(S) > 1e-12) {
            // Duhamel 原理叠加井储和表皮
            pf = (z * pf + S) / (z + CD * z * z * (z * pf + S));
        }
    }

    return pf;
}

// 核心：计算无限导流裂缝在复合储层中的拉普拉斯解
double ModelSolver01_06::PWD_composite(double z, double fs1, double fs2, double M12,
                                       double LfD, double rmD, double reD, int nf,
                                       const QVector<double>& xwD, ModelType type) {
    using namespace boost::math;
    QVector<double> ywD(nf, 0.0); // 裂缝y坐标假设为0

    double gama1 = sqrt(z * fs1);
    double gama2 = sqrt(z * fs2);
    double arg_g2_rm = gama2 * rmD;
    double arg_g1_rm = gama1 * rmD;

    // 计算贝塞尔函数值
    double k0_g2 = cyl_bessel_k(0, arg_g2_rm);
    double k1_g2 = cyl_bessel_k(1, arg_g2_rm);
    double k1_g1 = cyl_bessel_k(1, arg_g1_rm);

    double term_mAB_i0 = 0.0;
    double term_mAB_i1 = 0.0;

    bool isInfinite = (type == Model_1 || type == Model_2);
    bool isClosed = (type == Model_3 || type == Model_4);
    bool isConstP = (type == Model_5 || type == Model_6);

    // 处理外边界条件
    if (!isInfinite) {
        double arg_re = gama2 * reD;
        double i1_re_s = scaled_besseli(1, arg_re);
        double i0_re_s = scaled_besseli(0, arg_re);
        double k1_re = cyl_bessel_k(1, arg_re);
        double k0_re = cyl_bessel_k(0, arg_re);
        double i0_g2_s = scaled_besseli(0, arg_g2_rm);
        double i1_g2_s = scaled_besseli(1, arg_g2_rm);

        if (isClosed) {
            // 封闭边界
            if (i1_re_s > 1e-100) {
                term_mAB_i0 = (k1_re / i1_re_s) * i0_g2_s * std::exp(arg_g2_rm - arg_re);
                term_mAB_i1 = (k1_re / i1_re_s) * i1_g2_s * std::exp(arg_g2_rm - arg_re);
            }
        } else if (isConstP) {
            // 定压边界
            if (i0_re_s > 1e-100) {
                term_mAB_i0 = -(k0_re / i0_re_s) * i0_g2_s * std::exp(arg_g2_rm - arg_re);
                term_mAB_i1 = -(k0_re / i0_re_s) * i1_g2_s * std::exp(arg_g2_rm - arg_re);
            }
        }
    }

    double term1 = term_mAB_i0 + k0_g2;
    double term2 = term_mAB_i1 - k1_g2;

    double Acup = M12 * gama1 * k1_g1 * term1 + gama2 * cyl_bessel_k(0, arg_g1_rm) * term2;

    double i1_g1_s = scaled_besseli(1, arg_g1_rm);
    double i0_g1_s = scaled_besseli(0, arg_g1_rm);

    double Acdown_scaled = M12 * gama1 * i1_g1_s * term1 - gama2 * i0_g1_s * term2;

    if (std::abs(Acdown_scaled) < 1e-100) Acdown_scaled = 1e-100;

    double Ac_prefactor = Acup / Acdown_scaled;

    // 构建线性方程组求解裂缝流量分布
    int size = nf + 1;
    Eigen::MatrixXd A_mat(size, size);
    Eigen::VectorXd b_vec(size);
    b_vec.setZero();
    b_vec(nf) = 1.0; // 定压条件

    for (int i = 0; i < nf; ++i) {
        for (int j = 0; j < nf; ++j) {
            // 定义积分核函数
            auto integrand = [&](double a) -> double {
                double dist = std::sqrt(std::pow(xwD[i] - xwD[j] - a, 2) + std::pow(ywD[i] - ywD[j], 2));
                double arg_dist = gama1 * dist;
                if (arg_dist < 1e-10) arg_dist = 1e-10;

                double term2_val = 0.0;
                double exponent = arg_dist - arg_g1_rm;
                if (exponent > -700.0) {
                    term2_val = Ac_prefactor * scaled_besseli(0, arg_dist) * std::exp(exponent);
                }
                return cyl_bessel_k(0, arg_dist) + term2_val;
            };
            // 积分计算矩阵元素
            double val = adaptiveGauss(integrand, -LfD, LfD, 1e-5, 0, 10);
            A_mat(i, j) = z * val / (M12 * z * 2 * LfD);
        }
    }
    // 添加定流量约束
    for (int i = 0; i < nf; ++i) {
        A_mat(i, nf) = -1.0;
        A_mat(nf, i) = z;
    }
    A_mat(nf, nf) = 0.0;

    // 求解方程组，返回井底压力
    return A_mat.fullPivLu().solve(b_vec)(nf);
}

// 标度 Bessel I 函数: e^{-x} * I_v(x)
double ModelSolver01_06::scaled_besseli(int v, double x) {
    if (x < 0) x = -x;
    if (x > 600.0) return 1.0 / std::sqrt(2.0 * M_PI * x);
    return boost::math::cyl_bessel_i(v, x) * std::exp(-x);
}

// 高斯-勒让德积分 (15点)
double ModelSolver01_06::gauss15(std::function<double(double)> f, double a, double b) {
    static const double X[] = { 0.0, 0.201194, 0.394151, 0.570972, 0.724418, 0.848207, 0.937299, 0.987993 };
    static const double W[] = { 0.202578, 0.198431, 0.186161, 0.166269, 0.139571, 0.107159, 0.070366, 0.030753 };
    double h = 0.5 * (b - a);
    double c = 0.5 * (a + b);
    double s = W[0] * f(c);
    for (int i = 1; i < 8; ++i) {
        double dx = h * X[i];
        s += W[i] * (f(c - dx) + f(c + dx));
    }
    return s * h;
}

// 自适应高斯积分
double ModelSolver01_06::adaptiveGauss(std::function<double(double)> f, double a, double b, double eps, int depth, int maxDepth) {
    double c = (a + b) / 2.0;
    double v1 = gauss15(f, a, b);
    double v2 = gauss15(f, a, c) + gauss15(f, c, b);
    if (depth >= maxDepth || std::abs(v1 - v2) < 1e-10 * std::abs(v2) + eps) return v2;
    return adaptiveGauss(f, a, c, eps/2, depth+1, maxDepth) + adaptiveGauss(f, c, b, eps/2, depth+1, maxDepth);
}

// Stehfest 算法系数
double ModelSolver01_06::stefestCoefficient(int i, int N) {
    double s = 0.0;
    int k1 = (i + 1) / 2;
    int k2 = std::min(i, N / 2);
    for (int k = k1; k <= k2; ++k) {
        double num = pow(k, N / 2.0) * factorial(2 * k);
        double den = factorial(N / 2 - k) * factorial(k) * factorial(k - 1) * factorial(i - k) * factorial(2 * k - i);
        if(den!=0) s += num/den;
    }
    return ((i + N / 2) % 2 == 0 ? 1.0 : -1.0) * s;
}

// 阶乘函数
double ModelSolver01_06::factorial(int n) {
    if(n<=1) return 1;
    double r=1;
    for(int i=2;i<=n;++i) r*=i;
    return r;
}
