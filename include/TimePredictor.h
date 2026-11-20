#ifndef TIME_H
#define TIME_H

#include "Task.h"
#include "StudentProfile.h"
#include <vector>

using namespace std;

class TimePredictor
{
protected:
    int featureCount;
    double bias;
    vector<double> weights;
    double learingRate;
    int graditrate;

    double dotProduct(vector<double>& weights, vector<double>& features);

public:
    TimePredictor(double learingRate, int graditrate, int featureCount);

    vector<double> getFeatures(Task& task, StudentProfile& student);

    void predictTime(Task& task, StudentProfile& student);

    virtual double predict(vector<double>& features);

    void train(vector<vector<double>>& X_train, vector<double>& y_train);
};

#endif