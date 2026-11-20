#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <limits>
#include <algorithm>

#include "../include/Task.h"
#include "../include/StudentProfile.h"
#include "../include/StudySession.h"
#include "../include/TimePredictor.h"
#include "../include/DelayPredictor.h"
#include "../include/Scheduler.h"
#include "../include/RecoveryEngine.h"
#include "../include/ReportGenerator.h"

using namespace std;

// ─────────────────────────────────────────────
//  State
// ─────────────────────────────────────────────
static StudentProfile   student;
static vector<Task>     tasks;

static vector<vector<double>> timeTrainingData;
static vector<double>         realTimes;
static vector<vector<double>> delayTrainingData;
static vector<double>         delayResults;

static TimePredictor   timePredictor(0.000001, 5000, 7);
static DelayPredictor  delayPredictor(0.0001,  5000);
static Scheduler       scheduler;
static RecoveryEngine  recoveryEngine;

static bool studentReady  = false;
static bool scheduleReady = false;

static const int    MIN_TRAIN   = 3;
static const string SAVE_FILE   = "recoverplan_save.dat";

// ─────────────────────────────────────────────
//  Input helpers
// ─────────────────────────────────────────────
static bool readLine(const string& prompt, string& out)
{
    while (true)
    {
        cout << prompt << " (or 'back'): ";
        getline(cin, out);

        if (out == "back" || out == "Back" || out == "BACK")
            return false;

        size_t s = out.find_first_not_of(" \t");
        size_t e = out.find_last_not_of(" \t");

        if (s == string::npos) { cout << "  Cannot be empty.\n"; continue; }

        out = out.substr(s, e - s + 1);
        return true;
    }
}

static bool readInt(const string& prompt, int minV, int maxV, int& out)
{
    string line;
    while (true)
    {
        cout << prompt << " [" << minV << "-" << maxV << "] (or 'back'): ";
        getline(cin, line);

        if (line == "back" || line == "Back") return false;

        stringstream ss(line);
        int v;
        if (ss >> v && v >= minV && v <= maxV) { out = v; return true; }
        cout << "  Invalid. Enter a number between " << minV << " and " << maxV << ".\n";
    }
}

static bool readDouble(const string& prompt, double minV, double maxV, double& out)
{
    string line;
    while (true)
    {
        cout << prompt << " [" << minV << "-" << maxV << "] (or 'back'): ";
        getline(cin, line);

        if (line == "back" || line == "Back") return false;

        stringstream ss(line);
        double v;
        if (ss >> v && v >= minV && v <= maxV) { out = v; return true; }
        cout << "  Invalid. Enter a number between " << minV << " and " << maxV << ".\n";
    }
}

// ─────────────────────────────────────────────
//  Serialization helpers (plain-text, pipe-delimited)
// ─────────────────────────────────────────────

// Escape pipes in strings so they don't break the format
static string esc(const string& s)
{
    string out;
    for (char c : s)
    {
        if (c == '|')  out += "\\p";
        else if (c == '\n') out += "\\n";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

static string unesc(const string& s)
{
    string out;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            if      (s[i+1] == 'p')  { out += '|';  i++; }
            else if (s[i+1] == 'n')  { out += '\n'; i++; }
            else if (s[i+1] == '\\') { out += '\\'; i++; }
            else out += s[i];
        }
        else out += s[i];
    }
    return out;
}

static string taskTypeStr(TaskType t)
{
    switch (t)
    {
        case Study:    return "Study";
        case Homework: return "Homework";
        case Project:  return "Project";
        case Revision: return "Revision";
        default:       return "Exam";
    }
}

static TaskType taskTypeFromString(const string& t)
{
    string low = t;
    transform(low.begin(), low.end(), low.begin(), ::tolower);
    if (low == "study")    return Study;
    if (low == "homework") return Homework;
    if (low == "project")  return Project;
    if (low == "revision") return Revision;
    return Exam;
}

// ─────────────────────────────────────────────
//  Save
// ─────────────────────────────────────────────
static void saveState()
{
    ofstream f(SAVE_FILE.c_str());
    if (!f) { cout << "  [Warning] Could not save state.\n"; return; }

    // ── Student ──
    f << "STUDENT\n";
    f << esc(student.getName())            << "|"
      << student.getAvailableHours()       << "|"
      << student.getCompletionRate()       << "|"
      << student.getNumberOfMissedSessions() << "|"
      << student.getFocusScore()           << "|"
      << student.getSessionsPerDay()       << "\n";

    // Past records
    const vector<SubjectRecord>& recs = student.getPastRecord();
    f << "RECORDS " << recs.size() << "\n";
    for (size_t i = 0; i < recs.size(); i++)
        f << esc(recs[i].subjectName) << "|" << recs[i].grade << "\n";

    // Session log
    const vector<string>& logs = student.getSessionLog();
    f << "LOGS " << logs.size() << "\n";
    for (size_t i = 0; i < logs.size(); i++)
        f << esc(logs[i]) << "\n";

    // ── Tasks ──
    f << "TASKS " << tasks.size() << "\n";
    for (size_t i = 0; i < tasks.size(); i++)
    {
        f << tasks[i].getTaskId()                      << "|"
          << esc(tasks[i].getSubjectName())             << "|"
          << esc(tasks[i].getTaskName())                << "|"
          << taskTypeStr(tasks[i].getTaskType())        << "|"
          << tasks[i].getDifficulty()                   << "|"
          << tasks[i].getImportance()                   << "|"
          << tasks[i].getStudentEstimatedTime()         << "|"
          << tasks[i].getPagesOrQuestions()             << "|"
          << tasks[i].getDaysRemaining()                << "|"
          << tasks[i].getPredictedDuration()            << "|"
          << tasks[i].getDelayProbability()             << "|"
          << (tasks[i].getIsCompleted() ? 1 : 0)       << "\n";
    }

    // ── Sessions ──
    const vector<StudySession>& sessions = scheduler.getScheduledSessions();
    f << "SESSIONS " << sessions.size() << "\n";
    for (size_t i = 0; i < sessions.size(); i++)
    {
        f << sessions[i].getSessionId()               << "|"
          << sessions[i].getTaskId()                  << "|"
          << esc(sessions[i].getCourseName())          << "|"
          << esc(sessions[i].getTaskName())            << "|"
          << sessions[i].getDayNumber()               << "|"
          << sessions[i].getPlannedHours()            << "|"
          << sessions[i].getActualHours()             << "|"
          << (sessions[i].isCompleted() ? 1 : 0)     << "|"
          << (sessions[i].isMissed()    ? 1 : 0)     << "\n";
    }

    // Overflow tasks
    const vector<Task>& ov = scheduler.getOverflowTasks();
    f << "OVERFLOW " << ov.size() << "\n";
    for (size_t i = 0; i < ov.size(); i++)
    {
        f << ov[i].getTaskId()                     << "|"
          << esc(ov[i].getSubjectName())            << "|"
          << esc(ov[i].getTaskName())               << "|"
          << taskTypeStr(ov[i].getTaskType())       << "|"
          << ov[i].getDifficulty()                  << "|"
          << ov[i].getImportance()                  << "|"
          << ov[i].getStudentEstimatedTime()        << "|"
          << ov[i].getPagesOrQuestions()            << "|"
          << ov[i].getDaysRemaining()               << "|"
          << ov[i].getPredictedDuration()           << "|"
          << ov[i].getDelayProbability()            << "|"
          << (ov[i].getIsCompleted() ? 1 : 0)      << "\n";
    }

    // ── Training data ──
    f << "TIME_TRAIN " << realTimes.size() << "\n";
    for (size_t i = 0; i < realTimes.size(); i++)
    {
        f << realTimes[i];
        for (size_t j = 0; j < timeTrainingData[i].size(); j++)
            f << "|" << timeTrainingData[i][j];
        f << "\n";
    }

    f << "DELAY_TRAIN " << delayResults.size() << "\n";
    for (size_t i = 0; i < delayResults.size(); i++)
    {
        f << delayResults[i];
        for (size_t j = 0; j < delayTrainingData[i].size(); j++)
            f << "|" << delayTrainingData[i][j];
        f << "\n";
    }

    f << "SCHEDULE_READY " << (scheduleReady ? 1 : 0) << "\n";
    f << "END\n";
}

// ─────────────────────────────────────────────
//  Load
// ─────────────────────────────────────────────
static vector<string> splitPipe(const string& line)
{
    vector<string> parts;
    string cur;
    for (size_t i = 0; i < line.size(); i++)
    {
        if (line[i] == '|') { parts.push_back(cur); cur.clear(); }
        else cur += line[i];
    }
    parts.push_back(cur);
    return parts;
}

static bool loadState()
{
    ifstream f(SAVE_FILE.c_str());
    if (!f) return false;

    string line;

    // STUDENT
    getline(f, line); // "STUDENT"
    if (line != "STUDENT") return false;

    getline(f, line);
    {
        vector<string> p = splitPipe(line);
        if (p.size() < 5) return false;

        student.setSessionsPerDay(1); // Backward-compatible with five-field saves.
        if (p.size() >= 6)
        {
            int sessions;
            if (stringstream(p[5]) >> sessions) student.setSessionsPerDay(sessions);
        }
        student.setName(unesc(p[0]));
        double hours; stringstream(p[1]) >> hours; student.setAvailableHours(hours);
        double cr;    stringstream(p[2]) >> cr;    student.setCompletionRate(cr);
        int missed;   stringstream(p[3]) >> missed; student.setNumberOfMissedSessions(missed);
        int focus;    stringstream(p[4]) >> focus;  student.setFocusScore(focus);
        studentReady = true;
    }

    // RECORDS
    getline(f, line);
    int recCount = 0;
    sscanf(line.c_str(), "RECORDS %d", &recCount);
    for (int i = 0; i < recCount; i++)
    {
        getline(f, line);
        vector<string> p = splitPipe(line);
        if (p.size() >= 2)
        {
            double g; stringstream(p[1]) >> g;
            student.addSubjectRecord(unesc(p[0]), g);
        }
    }

    // LOGS
    getline(f, line);
    int logCount = 0;
    sscanf(line.c_str(), "LOGS %d", &logCount);
    for (int i = 0; i < logCount; i++)
    {
        getline(f, line);
        student.addSessionLog(unesc(line));
    }

    // TASKS
    getline(f, line);
    int taskCount = 0;
    sscanf(line.c_str(), "TASKS %d", &taskCount);
    tasks.clear();
    for (int i = 0; i < taskCount; i++)
    {
        getline(f, line);
        vector<string> p = splitPipe(line);
        if (p.size() < 12) continue;

        int id, diff, imp, pages, days, completed;
        double est, pred, delayP;
        stringstream(p[0]) >> id;
        stringstream(p[4]) >> diff;
        stringstream(p[5]) >> imp;
        stringstream(p[6]) >> est;
        stringstream(p[7]) >> pages;
        stringstream(p[8]) >> days;
        stringstream(p[9]) >> pred;
        stringstream(p[10]) >> delayP;
        stringstream(p[11]) >> completed;

        Task t(id, unesc(p[1]), unesc(p[2]),
               taskTypeFromString(p[3]),
               diff, imp, est, pages, days);
        t.setPredictedDuration(pred);
        t.setDelayProbability(delayP);
        if (completed) t.markAsCompleted();
        tasks.push_back(t);
    }

    // SESSIONS
    getline(f, line);
    int sessionCount = 0;
    sscanf(line.c_str(), "SESSIONS %d", &sessionCount);

    vector<StudySession> loadedSessions;
    for (int i = 0; i < sessionCount; i++)
    {
        getline(f, line);
        vector<string> p = splitPipe(line);
        if (p.size() < 9) continue;

        int sid, tid, day, comp, miss;
        double planned, actual;
        stringstream(p[0]) >> sid;
        stringstream(p[1]) >> tid;
        stringstream(p[4]) >> day;
        stringstream(p[5]) >> planned;
        stringstream(p[6]) >> actual;
        stringstream(p[7]) >> comp;
        stringstream(p[8]) >> miss;

        StudySession s(sid, tid, unesc(p[2]), unesc(p[3]), day, planned);
        s.setActualHours(actual);
        if (comp) s.setCompleted(true);
        if (miss) s.setMissed(true);
        loadedSessions.push_back(s);
    }

    // OVERFLOW
    getline(f, line);
    int ovCount = 0;
    sscanf(line.c_str(), "OVERFLOW %d", &ovCount);

    vector<Task> loadedOverflow;
    for (int i = 0; i < ovCount; i++)
    {
        getline(f, line);
        vector<string> p = splitPipe(line);
        if (p.size() < 12) continue;

        int id, diff, imp, pages, days, completed;
        double est, pred, delayP;
        stringstream(p[0]) >> id;
        stringstream(p[4]) >> diff;
        stringstream(p[5]) >> imp;
        stringstream(p[6]) >> est;
        stringstream(p[7]) >> pages;
        stringstream(p[8]) >> days;
        stringstream(p[9]) >> pred;
        stringstream(p[10]) >> delayP;
        stringstream(p[11]) >> completed;

        Task t(id, unesc(p[1]), unesc(p[2]),
               taskTypeFromString(p[3]),
               diff, imp, est, pages, days);
        t.setPredictedDuration(pred);
        t.setDelayProbability(delayP);
        if (completed) t.markAsCompleted();
        loadedOverflow.push_back(t);
    }

    scheduler.loadSessions(loadedSessions, loadedOverflow);

    // TIME_TRAIN
    getline(f, line);
    int timeSamples = 0;
    sscanf(line.c_str(), "TIME_TRAIN %d", &timeSamples);
    timeTrainingData.clear(); realTimes.clear();
    for (int i = 0; i < timeSamples; i++)
    {
        getline(f, line);
        vector<string> p = splitPipe(line);
        if (p.empty()) continue;
        double rv; stringstream(p[0]) >> rv;
        realTimes.push_back(rv);
        vector<double> row;
        for (size_t j = 1; j < p.size(); j++)
        { double v; stringstream(p[j]) >> v; row.push_back(v); }
        timeTrainingData.push_back(row);
    }

    // DELAY_TRAIN
    getline(f, line);
    int delaySamples = 0;
    sscanf(line.c_str(), "DELAY_TRAIN %d", &delaySamples);
    delayTrainingData.clear(); delayResults.clear();
    for (int i = 0; i < delaySamples; i++)
    {
        getline(f, line);
        vector<string> p = splitPipe(line);
        if (p.empty()) continue;
        double rv; stringstream(p[0]) >> rv;
        delayResults.push_back(rv);
        vector<double> row;
        for (size_t j = 1; j < p.size(); j++)
        { double v; stringstream(p[j]) >> v; row.push_back(v); }
        delayTrainingData.push_back(row);
    }

    // SCHEDULE_READY
    getline(f, line);
    int sr = 0;
    sscanf(line.c_str(), "SCHEDULE_READY %d", &sr);
    scheduleReady = (sr == 1);

    return true;
}

// ─────────────────────────────────────────────
//  Utilities
// ─────────────────────────────────────────────
static int nextTaskId()
{
    int maxId = 0;
    for (size_t i = 0; i < tasks.size(); i++)
        if (tasks[i].getTaskId() > maxId) maxId = tasks[i].getTaskId();
    return maxId + 1;
}

static Task* findTask(int id)
{
    for (size_t i = 0; i < tasks.size(); i++)
        if (tasks[i].getTaskId() == id) return &tasks[i];
    return nullptr;
}

static bool recoveryNeeded()
{
    const vector<StudySession>& s = scheduler.getScheduledSessions();
    for (size_t i = 0; i < s.size(); i++)
        if (s[i].isMissed() || s[i].isDelayed()) return true;
    return false;
}

static void updateCompletionRate()
{
    const vector<StudySession>& s = scheduler.getScheduledSessions();
    int total = 0, done = 0;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i].isCompleted() || s[i].isMissed())
        {
            total++;
            if (s[i].isCompleted()) done++;
        }
    }
    if (total > 0)
        student.setCompletionRate(static_cast<double>(done) / total);
}

static bool readTaskType(TaskType& out)
{
    cout << "\n  Task type:\n";
    cout << "    1. Study\n";
    cout << "    2. Homework\n";
    cout << "    3. Project\n";
    cout << "    4. Revision\n";
    cout << "    5. Exam\n";

    int ch;
    if (!readInt("  Choice", 0, 5, ch)) return false;
    if (ch == 0) return false;

    const TaskType types[] = { Study, Homework, Project, Revision, Exam };
    out = types[ch - 1];
    return true;
}

static int stringToInt(const string& t)   { int v=0;    stringstream(t)>>v; return v; }
static double stringToDouble(const string& t) { double v=0; stringstream(t)>>v; return v; }

// ─────────────────────────────────────────────
//  Menu
// ─────────────────────────────────────────────
static void showMenu()
{
    cout << "\n";
    cout << "══════════════════════════════════════\n";
    cout << "           RecoverPlan\n";
    cout << "══════════════════════════════════════\n";

    if (studentReady)
        cout << "  Student : " << student.getName() << " | "
             << student.getAvailableHours() << "h/day | "
             << student.getSessionsPerDay() << " sessions/day | "
             << tasks.size() << " task(s)\n";
    else
        cout << "  [No student set — do step 1 first]\n";

    cout << "--------------------------------------\n";
    cout << "  SETUP\n";
    cout << "    1. Set student info\n";
    cout << "    2. Add subject record\n";
    cout << "\n";
    cout << "  TASKS\n";
    cout << "    3. Add task manually\n";
    cout << "    4. Import tasks from CSV\n";
    cout << "    5. Export sample CSV\n";
    cout << "    6. Show tasks\n";
    cout << "    7. Mark task completed\n";
    cout << "\n";
    cout << "  SCHEDULE\n";
    cout << "    8. Build schedule\n";
    cout << "    9. Show risk report\n";
    cout << "   10. Show schedule\n";
    cout << "   11. Record session results\n";
    cout << "   12. Run recovery plan\n";
    cout << "\n";
    cout << "  REPORTS\n";
    cout << "   13. Final report\n";
    cout << "\n";
    cout << "   14. Clear all data\n";
    cout << "    0. Exit\n";
    cout << "══════════════════════════════════════\n";
    cout << "Choice: ";
}

// ─────────────────────────────────────────────
//  Actions
// ─────────────────────────────────────────────
static void doSetStudent()
{
    cout << "\n── Set Student ──\n";

    string name;
    if (!readLine("Name", name)) return;

    double hours;
    if (!readDouble("Available study hours per day", 0, 24, hours)) return;

    int sessions;
    if (!readInt("Study sessions per day", 1, 24, sessions)) return;

    int focus;
    if (!readInt("Focus score", 1, 10, focus)) return;

    student.setName(name);
    student.setAvailableHours(hours);
    student.setSessionsPerDay(sessions);
    student.setFocusScore(focus);
    student.setCompletionRate(0);
    student.setNumberOfMissedSessions(0);

    studentReady  = true;
    scheduleReady = false;

    saveState();
    cout << "  ✓ Student saved.\n";
}

static void doAddRecord()
{
    if (!studentReady) { cout << "  Set student first (option 1).\n"; return; }

    cout << "\n── Add Subject Record ──\n";

    string subject;
    if (!readLine("Subject name", subject)) return;

    double grade;
    if (!readDouble("Grade", 0, 100, grade)) return;

    student.addSubjectRecord(subject, grade);
    saveState();
    cout << "  ✓ Record added.\n";
}

static void doAddTask()
{
    if (!studentReady) { cout << "  Set student first (option 1).\n"; return; }

    cout << "\n── Add Task ──\n";

    string subject, name;
    if (!readLine("Subject name", subject)) return;
    if (!readLine("Task name",    name))    return;

    TaskType type;
    if (!readTaskType(type)) return;

    int diff, imp, pages, days;
    double estTime;

    if (!readInt("Difficulty",                   1,     10,    diff))    return;
    if (!readInt("Importance",                   1,     10,    imp))     return;
    if (!readDouble("Estimated time (hrs)",      0.1, 500.0,  estTime)) return;
    if (!readInt("Pages / questions (0 if N/A)", 0,  10000,   pages))   return;
    if (!readInt("Days remaining",               0,    365,    days))    return;

    tasks.push_back(Task(nextTaskId(), subject, name, type,
                         diff, imp, estTime, pages, days));
    scheduleReady = false;

    saveState();
    cout << "  ✓ Task added (ID " << tasks.back().getTaskId() << ").\n";
}

static void doImportCSV()
{
    if (!studentReady) { cout << "  Set student first (option 1).\n"; return; }

    cout << "\n── Import Tasks from CSV ──\n";
    cout << "  Format: subject,task,type,difficulty,importance,estimatedTime,pages,days\n";
    cout << "  Types : Study / Homework / Project / Revision / Exam\n\n";

    string path;
    if (!readLine("CSV file path", path)) return;

    ifstream file(path.c_str());
    if (!file) { cout << "  ✗ Cannot open file: " << path << "\n"; return; }

    string line;
    int added = 0, skipped = 0, lineNum = 0;

    while (getline(file, line))
    {
        lineNum++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        vector<string> f;
        string field;
        stringstream ss(line);
        while (getline(ss, field, ',')) f.push_back(field);

        if (lineNum == 1)
        {
            string h = f[0];
            transform(h.begin(), h.end(), h.begin(), ::tolower);
            if (h == "subject") continue;
        }

        if (f.size() < 8)
        {
            cout << "  ✗ Line " << lineNum << " skipped (only " << f.size() << " columns).\n";
            skipped++; continue;
        }

        int    diff  = stringToInt(f[3]);
        int    imp   = stringToInt(f[4]);
        double est   = stringToDouble(f[5]);
        int    pages = stringToInt(f[6]);
        int    days  = stringToInt(f[7]);

        bool valid = true;
        if (diff < 1 || diff > 10) { cout << "  ✗ Line " << lineNum << ": difficulty out of [1-10].\n"; valid=false; }
        if (imp  < 1 || imp  > 10) { cout << "  ✗ Line " << lineNum << ": importance out of [1-10].\n"; valid=false; }
        if (est  <= 0)             { cout << "  ✗ Line " << lineNum << ": estimatedTime must be > 0.\n"; valid=false; }
        if (days < 0)              { cout << "  ✗ Line " << lineNum << ": days cannot be negative.\n";   valid=false; }
        if (!valid) { skipped++; continue; }

        tasks.push_back(Task(nextTaskId(), f[0], f[1],
                             taskTypeFromString(f[2]),
                             diff, imp, est, pages, days));
        added++;
    }

    file.close();
    scheduleReady = false;
    saveState();

    cout << "  ✓ Imported: " << added << "  |  Skipped: " << skipped << "\n";
}

static void doExportSampleCSV()
{
    const string path = "sample_tasks.csv";
    ofstream file(path.c_str());
    if (!file) { cout << "  ✗ Cannot create file.\n"; return; }

    file << "subject,task,type,difficulty,importance,estimatedTime,pages,days\n";
    file << "OOP,Project Classes,Project,8,9,4,0,3\n";
    file << "Algorithms,Homework 3,Homework,7,8,3,12,2\n";
    file << "Calculus,Exam Review,Revision,6,10,5,40,5\n";
    file << "English,Reading Chapter 4,Study,3,2,2,10,7\n";
    file << "Electronics,Lab Report,Homework,5,7,2.5,8,4\n";

    cout << "  ✓ Created: " << path << "\n";
}

static void doShowTasks()
{
    if (tasks.empty()) { cout << "  No tasks yet.\n"; return; }
    ReportGenerator::printTaskList(tasks);
}

static void doMarkCompleted()
{
    if (tasks.empty()) { cout << "  No tasks yet.\n"; return; }

    int id;
    if (!readInt("Task ID to mark completed", 1, 999999, id)) return;

    Task* t = findTask(id);
    if (!t)                  { cout << "  ✗ Task ID " << id << " not found.\n"; return; }
    if (t->getIsCompleted()) { cout << "  Already completed.\n"; return; }

    t->markAsCompleted();
    scheduleReady = false;
    saveState();
    cout << "  ✓ Task " << id << " marked as completed.\n";
}

static void doBuildSchedule()
{
    if (!studentReady) { cout << "  Set student first (option 1).\n"; return; }
    if (tasks.empty()) { cout << "  Add tasks first (option 3 or 4).\n"; return; }

    bool hasTime  = realTimes.size()    >= (size_t)MIN_TRAIN;
    bool hasDelay = delayResults.size() >= (size_t)MIN_TRAIN;

    if (hasTime)
    {
        timePredictor.train(timeTrainingData, realTimes);
        for (size_t i = 0; i < tasks.size(); i++)
        {
            timePredictor.predictTime(tasks[i], student);
            if (tasks[i].getPredictedDuration() <= 0)
                tasks[i].setPredictedDuration(tasks[i].getStudentEstimatedTime());
        }
        cout << "  ✓ Time model trained (" << realTimes.size() << " samples).\n";
    }
    else
    {
        for (size_t i = 0; i < tasks.size(); i++)
            tasks[i].setPredictedDuration(tasks[i].getStudentEstimatedTime());
        cout << "  ℹ Using student estimates (need " << MIN_TRAIN << "+ completed sessions).\n";
    }

    if (hasDelay)
    {
        delayPredictor.train(delayTrainingData, delayResults);
        for (size_t i = 0; i < tasks.size(); i++)
            delayPredictor.predictDelay(tasks[i], student);
        cout << "  ✓ Delay model trained (" << delayResults.size() << " samples).\n";
    }
    else
    {
        for (size_t i = 0; i < tasks.size(); i++)
            tasks[i].setDelayProbability(-1);
        cout << "  ℹ Delay prediction unavailable (need " << MIN_TRAIN << "+ sessions).\n";
    }

    cout << "\n  Scheduling mode:\n";
    cout << "    1. Nearest deadline first\n";
    cout << "    2. Highest importance first\n";
    cout << "    3. Highest difficulty first\n";
    cout << "    4. Shortest task first\n";
    if (hasDelay) cout << "    5. Highest delay risk first\n";

    int mode;
    if (!readInt("  Mode", 1, hasDelay ? 5 : 4, mode)) return;
    if (!hasDelay && mode == 5) mode = 1;

    scheduler.setScheduleMode(mode);
    scheduler.buildSchedule(tasks, student);
    scheduleReady = true;

    saveState();
    cout << "  ✓ Schedule built.\n";
}

static void doShowRisk()
{
    if (tasks.empty()) { cout << "  No tasks yet.\n"; return; }
    ReportGenerator::printTaskRiskReport(tasks);
}

static void doShowSchedule()
{
    if (!scheduleReady) { cout << "  Build schedule first (option 8).\n"; return; }
    ReportGenerator::printSchedule(scheduler.getScheduledSessions());
    ReportGenerator::printOverflowTasks(scheduler.getOverflowTasks());
}

static void doRecordResults()
{
    if (!scheduleReady) { cout << "  Build schedule first (option 8).\n"; return; }

    vector<StudySession>& sessions = scheduler.getScheduledSessions();

    bool anyPending = false;
    for (size_t i = 0; i < sessions.size(); i++)
        if (!sessions[i].isCompleted() && !sessions[i].isMissed())
        { anyPending = true; break; }

    if (!anyPending) { cout << "  All sessions already recorded.\n"; return; }

    cout << "\n── Record Session Results ──\n";
    cout << "  (Type 'back' or choose 0 to stop)\n\n";

    for (size_t i = 0; i < sessions.size(); i++)
    {
        if (sessions[i].isCompleted() || sessions[i].isMissed())
        {
            cout << "  Session " << sessions[i].getSessionId()
                 << " [" << sessions[i].getCourseName() << " / "
                 << sessions[i].getTaskName() << "] — already recorded.\n";
            continue;
        }

        cout << "  ┌─ Session " << sessions[i].getSessionId() << " ──────────\n";
        cout << "  │  Subject : " << sessions[i].getCourseName()    << "\n";
        cout << "  │  Task    : " << sessions[i].getTaskName()      << "\n";
        cout << "  │  Day     : " << sessions[i].getDayNumber()     << "\n";
        cout << "  │  Planned : " << sessions[i].getPlannedHours()  << " hrs\n";
        cout << "  └──────────────────────────\n";
        cout << "    1. Completed on time\n";
        cout << "    2. Completed with delay\n";
        cout << "    3. Missed\n";
        cout << "    4. Skip (record later)\n";
        cout << "    0. Stop\n";

        int result;
        if (!readInt("  Result", 0, 4, result)) break;
        if (result == 0) break;
        if (result == 4) continue;

        Task* task = findTask(sessions[i].getTaskId());
        if (!task) continue;

        if (result == 1)
        {
            sessions[i].setCompleted(true);
            sessions[i].setActualHours(sessions[i].getPlannedHours());

            timeTrainingData.push_back(timePredictor.getFeatures(*task, student));
            realTimes.push_back(sessions[i].getActualHours());
            delayTrainingData.push_back(delayPredictor.getFeatures(*task, student));
            delayResults.push_back(0);

            student.addSessionLog("Completed on time: " + sessions[i].getTaskName());
            cout << "  ✓ Recorded.\n\n";
        }
        else if (result == 2)
        {
            double actual;
            if (!readDouble("  Actual hours spent", 0.01, 500, actual)) continue;

            sessions[i].setCompleted(true);
            sessions[i].setActualHours(actual);

            timeTrainingData.push_back(timePredictor.getFeatures(*task, student));
            realTimes.push_back(actual);
            delayTrainingData.push_back(delayPredictor.getFeatures(*task, student));
            delayResults.push_back(1);

            student.addSessionLog("Delayed: " + sessions[i].getTaskName());
            cout << "  ✓ Recorded as delayed.\n\n";
        }
        else if (result == 3)
        {
            sessions[i].setMissed(true);
            delayTrainingData.push_back(delayPredictor.getFeatures(*task, student));
            delayResults.push_back(1);

            student.addSessionLog("Missed: " + sessions[i].getTaskName());
            cout << "  ✓ Recorded as missed.\n\n";
        }
    }

    updateCompletionRate();
    saveState();

    cout << "\n  ── Training Data ──\n";
    cout << "  Time  : " << realTimes.size()    << " samples (" << MIN_TRAIN << " needed)\n";
    cout << "  Delay : " << delayResults.size() << " samples (" << MIN_TRAIN << " needed)\n";
}

static void doRecovery()
{
    if (!scheduleReady) { cout << "  Build schedule first (option 8).\n"; return; }

    if (!recoveryNeeded())
    {
        cout << "  ✓ No missed or delayed sessions — no recovery needed.\n";
        return;
    }

    recoveryEngine.recoverTask(scheduler.getScheduledSessions(), tasks, student);
    scheduleReady = true;
    saveState();
}

static void doFinalReport()
{
    if (!studentReady) { cout << "  Set student first (option 1).\n"; return; }

    ReportGenerator::printStudentProfile(student);
    ReportGenerator::printTaskList(tasks);

    if (scheduleReady)
    {
        ReportGenerator::printTaskRiskReport(tasks);
        ReportGenerator::printSchedule(scheduler.getScheduledSessions());
        ReportGenerator::printOverflowTasks(scheduler.getOverflowTasks());
        ReportGenerator::printFinalSummary(tasks, scheduler.getScheduledSessions());
    }
    else
    {
        cout << "\n  [Schedule not built yet — run option 8 first]\n";
    }
}

static void doClearData()
{
    cout << "\n  ⚠ This will delete ALL data (student, tasks, schedule, training).\n";
    string confirm;
    if (!readLine("Type 'yes' to confirm", confirm)) return;
    if (confirm != "yes" && confirm != "YES") { cout << "  Cancelled.\n"; return; }

    student = StudentProfile();
    tasks.clear();
    timeTrainingData.clear(); realTimes.clear();
    delayTrainingData.clear(); delayResults.clear();
    scheduler = Scheduler();
    studentReady  = false;
    scheduleReady = false;

    remove(SAVE_FILE.c_str());
    cout << "  ✓ All data cleared.\n";
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────
int main()
{
    // Auto-load on startup
    if (loadState())
        cout << "  ✓ Previous session loaded: " << student.getName()
             << " | " << tasks.size() << " task(s)\n";
    else
        cout << "  No saved data found — starting fresh.\n";

    string line;
    int choice = -1;

    do
    {
        showMenu();
        getline(cin, line);

        if (line.empty()) { choice = -1; continue; }
        stringstream ss(line);
        if (!(ss >> choice)) { choice = -1; }

        cout << "\n";

        switch (choice)
        {
            case  1: doSetStudent();      break;
            case  2: doAddRecord();       break;
            case  3: doAddTask();         break;
            case  4: doImportCSV();       break;
            case  5: doExportSampleCSV(); break;
            case  6: doShowTasks();       break;
            case  7: doMarkCompleted();   break;
            case  8: doBuildSchedule();   break;
            case  9: doShowRisk();        break;
            case 10: doShowSchedule();    break;
            case 11: doRecordResults();   break;
            case 12: doRecovery();        break;
            case 13: doFinalReport();     break;
            case 14: doClearData();       break;
            case  0: break;
            default: cout << "  ✗ Invalid choice.\n";
        }

    } while (choice != 0);

    cout << "\n  Goodbye.\n";
    return 0;
}
