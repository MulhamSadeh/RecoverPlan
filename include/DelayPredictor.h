#ifndef DELAYPREDICTOR_H
#define DELAYPREDICTOR_H

#include <iostream>
#include <cmath>
#include <vector>

#include "TimePredictor.h"
#include "Task.h"
#include "StudentProfile.h"

using namespace std;

class DelayPredictor : public TimePredictor
{
private:
    double sigmoid(double z);

public:
    DelayPredictor(double learingRate, int graditrate);

    vector<double> getFeatures(Task& task, StudentProfile& student);

    double predict(vector<double>& features) override;

    void predictDelay(Task& task, StudentProfile& student);
};

#endif