#include "../include/ReportGenerator.h"

#include <iomanip>

using namespace std;

string ReportGenerator::getRiskLevel(double probability)
{
    if (probability < 0)
    {
        return "N/A";
    }

    if (probability < 0.40)
    {
        return "Low";
    }

    if (probability < 0.70)
    {
        return "Medium";
    }

    return "High";
}

string ReportGenerator::getRecommendedAction(const Task& task)
{
    double probability = task.getDelayProbability();

    if (probability < 0)
    {
        return "Predict delay first";
    }

    if (probability >= 0.70)
    {
        return "Schedule earlier and split into smaller sessions";
    }

    if (probability >= 0.40)
    {
        return "Keep monitored and avoid postponing";
    }

    return "Normal scheduling";
}

void ReportGenerator::printStudentProfile(const StudentProfile& student)
{
    cout << "\n\n";
    cout << "============================================================" << endl;
    cout << "                    STUDENT PROFILE                         " << endl;
    cout << "============================================================" << endl;

    student.printProfile();
}

void ReportGenerator::printTaskList(const vector<Task>& tasks)
{
    cout << "\n\n";
    cout << "============================================================" << endl;
    cout << "                       TASK LIST                            " << endl;
    cout << "============================================================" << endl;

    if (tasks.empty())
    {
        cout << "No tasks available." << endl;
        return;
    }

    Task::displayHeader();

    for (size_t i = 0; i < tasks.size(); i++)
    {
        tasks[i].displayTaskRow();
    }
}

void ReportGenerator::printTaskRiskReport(const vector<Task>& tasks)
{
    cout << "\n\n";
    cout << "============================================================" << endl;
    cout << "                    TASK RISK REPORT                        " << endl;
    cout << "============================================================" << endl;

    if (tasks.empty())
    {
        cout << "No tasks available." << endl;
        return;
    }

    cout << left
         << setw(5)  << "ID"
         << setw(15) << "Subject"
         << setw(22) << "Task"
         << setw(14) << "PredHours"
         << setw(14) << "DelayProb"
         << setw(12) << "Risk"
         << "Recommended Action"
         << endl;

    cout << "---------------------------------------------------------------------------------------------" << endl;

    for (size_t i = 0; i < tasks.size(); i++)
    {
        cout << left
             << setw(5)  << tasks[i].getTaskId()
             << setw(15) << tasks[i].getSubjectName()
             << setw(22) << tasks[i].getTaskName();

        if (tasks[i].getPredictedDuration() <= 0)
        {
            cout << setw(14) << "N/A";
        }
        else
        {
            cout << fixed << setprecision(2)
                 << setw(14) << tasks[i].getPredictedDuration();
        }

        if (tasks[i].getDelayProbability() < 0)
        {
            cout << setw(14) << "N/A";
        }
        else
        {
            cout << fixed << setprecision(2)
                 << setw(14) << tasks[i].getDelayProbability();
        }

        cout << setw(12) << getRiskLevel(tasks[i].getDelayProbability())
             << getRecommendedAction(tasks[i])
             << endl;
    }
}

void ReportGenerator::printSchedule(const vector<StudySession>& sessions)
{
    cout << "\n\n";
    cout << "============================================================" << endl;
    cout << "                    GENERATED SCHEDULE                      " << endl;
    cout << "============================================================" << endl;

    if (sessions.empty())
    {
        cout << "No study sessions scheduled." << endl;
        return;
    }

    cout << left
         << setw(8)  << "Day"
         << setw(10) << "Session"
         << setw(15) << "Subject"
         << setw(22) << "Task"
         << setw(12) << "Hours"
         << "Status"
         << endl;

    cout << "---------------------------------------------------------------------------------------------" << endl;

    for (size_t i = 0; i < sessions.size(); i++)
    {
        string status = "Planned";

        if (sessions[i].isMissed())
        {
            status = "Missed";
        }
        else if (sessions[i].isCompleted())
        {
            if (sessions[i].isDelayed())
            {
                status = "Completed with delay";
            }
            else
            {
                status = "Completed";
            }
        }

        cout << left
             << "Day " << setw(4) << sessions[i].getDayNumber()
             << setw(10) << sessions[i].getSessionId()
             << setw(15) << sessions[i].getCourseName()
             << setw(22) << sessions[i].getTaskName()
             << fixed << setprecision(2)
             << setw(12) << sessions[i].getPlannedHours()
             << status
             << endl;
    }
}

void ReportGenerator::printOverflowTasks(const vector<Task>& overflowTasks)
{
    cout << "\n\n";
    cout << "============================================================" << endl;
    cout << "                       OVERFLOW TASKS                       " << endl;
    cout << "============================================================" << endl;

    if (overflowTasks.empty())
    {
        cout << "No overflow tasks. All tasks fit in the schedule." << endl;
        return;
    }

    cout << left
         << setw(5)  << "ID"
         << setw(15) << "Subject"
         << setw(22) << "Task"
         << setw(12) << "Hours"
         << "Reason"
         << endl;

    cout << "---------------------------------------------------------------------------------------------" << endl;

    for (size_t i = 0; i < overflowTasks.size(); i++)
    {
        cout << left
             << setw(5)  << overflowTasks[i].getTaskId()
             << setw(15) << overflowTasks[i].getSubjectName()
             << setw(22) << overflowTasks[i].getTaskName()
             << fixed << setprecision(2)
             << setw(12) << overflowTasks[i].getStudentEstimatedTime()
             << "Not enough available time before deadline"
             << endl;
    }
}

void ReportGenerator::printFinalSummary(
    const vector<Task>& tasks,
    const vector<StudySession>& sessions
)
{
    int completedTasks = 0;
    int pendingTasks = 0;
    int highRiskTasks = 0;

    int missedSessions = 0;
    int delayedSessions = 0;

    for (size_t i = 0; i < tasks.size(); i++)
    {
        if (tasks[i].getIsCompleted())
        {
            completedTasks++;
        }
        else
        {
            pendingTasks++;
        }

        if (tasks[i].getDelayProbability() >= 0.70)
        {
            highRiskTasks++;
        }
    }

    for (size_t i = 0; i < sessions.size(); i++)
    {
        if (sessions[i].isMissed())
        {
            missedSessions++;
        }

        if (sessions[i].isDelayed())
        {
            delayedSessions++;
        }
    }

    cout << "\n\n";
    cout << "============================================================" << endl;
    cout << "                       FINAL SUMMARY                        " << endl;
    cout << "============================================================" << endl;

    cout << "Total Tasks: " << tasks.size() << endl;
    cout << "Completed Tasks: " << completedTasks << endl;
    cout << "Pending Tasks: " << pendingTasks << endl;
    cout << "High Risk Tasks: " << highRiskTasks << endl;

    cout << "Total Sessions: " << sessions.size() << endl;
    cout << "Missed Sessions: " << missedSessions << endl;
    cout << "Delayed Sessions: " << delayedSessions << endl;

    cout << "============================================================" << endl;
}