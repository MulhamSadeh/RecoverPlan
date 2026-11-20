#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <iostream>
#include <vector>
#include <string>

#include "Task.h"
#include "StudySession.h"
#include "StudentProfile.h"

using namespace std;

class ReportGenerator
{
private:
    static string getRiskLevel(double probability);
    static string getRecommendedAction(const Task& task);

public:
    static void printStudentProfile(const StudentProfile& student);

    static void printTaskList(const vector<Task>& tasks);

    static void printTaskRiskReport(const vector<Task>& tasks);

    static void printSchedule(const vector<StudySession>& sessions);

    static void printOverflowTasks(const vector<Task>& overflowTasks);

    static void printFinalSummary(
        const vector<Task>& tasks,
        const vector<StudySession>& sessions
    );
};

#endif