// Synthetic-only holdout evaluation of the project's actual C++ predictors.
// This is a training sanity check, not evidence of accuracy on real students.
#include "TimePredictor.h"
#include "DelayPredictor.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

using Vec=std::vector<double>;
using Matrix=std::vector<Vec>;
struct Example { Vec x; double y; };

static double logistic(double z) {
    return z>=0 ? 1.0/(1.0+std::exp(-z)) : std::exp(z)/(1.0+std::exp(z));
}
static double mae(const Vec& y,const Vec& p) {
    double s=0; for(size_t i=0;i<y.size();++i) s+=std::abs(y[i]-p[i]);
    return s/y.size();
}
static double rmse(const Vec& y,const Vec& p) {
    double s=0;for(size_t i=0;i<y.size();++i) s+=std::pow(y[i]-p[i],2);
    return std::sqrt(s/y.size());
}
static double brier(const Vec& y,const Vec& p) {
    double s=0;for(size_t i=0;i<y.size();++i) s+=std::pow(y[i]-p[i],2);
    return s/y.size();
}
static double auc(const Vec& y,const Vec& p) {
    size_t positive=0,negative=0; double score=0;
    for(size_t i=0;i<y.size();++i) {
        if(y[i]>0.5) ++positive; else ++negative;
        for(size_t j=0;j<i;++j) {
            if(y[i]==y[j]) continue;
            double pos=y[i]>0.5?p[i]:p[j];
            double neg=y[i]>0.5?p[j]:p[i];
            score+=pos>neg?1.0:(pos==neg?0.5:0.0);
        }
    }
    if(!positive||!negative) throw std::runtime_error("Both classes required for AUC");
    return score/(positive*negative);
}
int main() {
    std::mt19937 gen(20261120);
    std::uniform_real_distribution<double> unit(0,1), est(0.5,5.0),
        pages(0,25), ptime(0.5,6.0);
    std::normal_distribution<double> noise(0,0.3);
    std::vector<Example> reg;
    for(int i=0;i<750;++i) {
        Vec x={double(1+gen()%10),double(1+gen()%10),est(gen),
            pages(gen),double(1+gen()%5),double(1+gen()%10),unit(gen)};
        double duration=std::max(0.1,0.4+0.10*x[0]+0.04*x[1]+1.45*x[2]+
            0.028*x[3]+0.08*x[4]-0.07*x[5]-0.3*x[6]+noise(gen));
        reg.push_back({x,duration});
    }
    Matrix X;Vec y;
    for(size_t i=0;i<550;++i) {X.push_back(reg[i].x);y.push_back(reg[i].y);}
    TimePredictor duration(0.0005,15000,7);
    duration.train(X,y);
    Vec real,pred,baseline;
    for(size_t i=550;i<reg.size();++i) {
        real.push_back(reg[i].y);
        auto x=reg[i].x;
        pred.push_back(duration.predict(x));
        baseline.push_back(x[2]);
    }
    double baseline_mae=mae(real,baseline), model_mae=mae(real,pred);
    double baseline_rmse=rmse(real,baseline), model_rmse=rmse(real,pred);

    std::vector<Example> cls;
    for(int i=0;i<1500;++i) {
        Vec x={ptime(gen),double(1+gen()%10),double(1+gen()%10),
            double(1+gen()%10),double(1+gen()%8),double(gen()%5),
            unit(gen),double(1+gen()%10)};
        double z=-0.7+0.40*(x[0]-3)+0.23*(x[1]-5)-0.13*(x[3]-5)
            -0.16*(x[4]-4)+0.21*x[5]-0.60*x[6]-0.13*(x[7]-5);
        cls.push_back({x,unit(gen)<logistic(z)?1.0:0.0});
    }
    X.clear();y.clear();
    for(size_t i=0;i<1100;++i) {X.push_back(cls[i].x);y.push_back(cls[i].y);}
    double prevalence=std::accumulate(y.begin(),y.end(),0.0)/y.size();
    DelayPredictor delay(0.006,9000);
    delay.train(X,y);
    real.clear();pred.clear();baseline.clear();
    for(size_t i=1100;i<cls.size();++i) {
        real.push_back(cls[i].y);
        auto x=cls[i].x;
        pred.push_back(delay.predict(x));
        baseline.push_back(prevalence);
    }
    double baseline_brier=brier(real,baseline),model_brier=brier(real,pred);
    double model_auc=auc(real,pred);
    std::cout<<std::fixed<<std::setprecision(4)
        <<"SYNTHETIC DATA ONLY (seed=20261120)\n"
        <<"duration train=550 test=200 | baseline MAE="<<baseline_mae
        <<" RMSE="<<baseline_rmse<<" | linear MAE="<<model_mae
        <<" RMSE="<<model_rmse<<"\n"
        <<"delay train=1100 test=400 | constant baseline Brier="<<baseline_brier
        <<" | logistic Brier="<<model_brier<<" AUC="<<model_auc<<"\n";
    if(!(std::isfinite(model_mae)&&std::isfinite(model_brier)&&std::isfinite(model_auc))
        ||model_mae>=baseline_mae||model_rmse>=baseline_rmse||
        model_brier>=baseline_brier||model_auc<=0.55)
        throw std::runtime_error("Synthetic learning sanity check failed");
}
