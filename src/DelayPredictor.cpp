#include "../include/DelayPredictor.h"
#include <vector>
#include <cmath>

using namespace std;

DelayPredictor::DelayPredictor(double learingRate, int graditrate)
    : TimePredictor(learingRate, graditrate, 8)
{
}

vector<double> DelayPredictor::getFeatures(Task& task, StudentProfile& student)
{
    vector<double> features;

    double predictedDuration = task.getPredictedDuration();

    if (predictedDuration <= 0)
    {
        predictedDuration = task.getStudentEstimatedTime();
    }

    features.push_back(predictedDuration);
    features.push_back(task.getDifficulty());
    features.push_back(task.getImportance());
    features.push_back(task.getDaysRemaining());
    features.push_back(student.getAvailableHours());
    features.push_back(student.getNumberOfMissedSessions());
    features.push_back(student.getCompletionRate());
    features.push_back(student.getFocusScore());

    return features;
}

double DelayPredictor::sigmoid(double z)
{
    return 1.0 / (1.0 + exp(-z));
}

double DelayPredictor::predict(vector<double>& features)
{
    double z = dotProduct(weights, features) + bias;
    return sigmoid(z);
}

void DelayPredictor::predictDelay(Task& task, StudentProfile& student)
{
    vector<double> features = getFeatures(task, student);

    double delayProbability = predict(features);

    task.setDelayProbability(delayProbability);
}