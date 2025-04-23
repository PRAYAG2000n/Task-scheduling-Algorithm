#include <iostream>
#include <vector>
#include <limits>
#include <chrono>
#include <ctime>
#include <climits>
#include <algorithm> 
#include <iterator>

using namespace std;

class JobUnit {
public:
    int identifier;
    bool executeOnCloud;
    double priorityMetric;
    int finishLocal;
    int finishSending;
    int finishCloud;
    int finishReceiving;
    int readyLocal;
    int readySending;
    int readyCloud;
    int readyReceiving;
    int beginTime;
    int assignedResource;
    bool terminalJob;
    bool initialJob;
    int predCountReady;
    int sameChannelReady;
};

// Utility max function
int maxOfTwo(int a, int b) {
    return (a >= b) ? a : b;
}

// Phase A: Initial Job Assignment
void initialJobAssignment(vector<JobUnit>& jobPool, const vector<vector<int>>& execMatrix, int limitThreshold)
{
    for (size_t i = 0; i < execMatrix.size(); i++) {
        jobPool[i].identifier = (int)i + 1;
        int minimalTime = execMatrix[i][0];
        for (size_t j = 0; j < execMatrix[i].size(); j++)
            if (execMatrix[i][j] < minimalTime)
                minimalTime = execMatrix[i][j];

        jobPool[i].executeOnCloud = (minimalTime > limitThreshold);
    }
}

// Phase B: Calculate Job Priorities
void computePriorities(vector<JobUnit>& jobPool, const vector<vector<int>>& execMatrix, const vector<vector<int>>& dependencyGraph, int limitThreshold)
{
    size_t endIndex = jobPool.size() - 1;
    for (size_t idx = 0; idx < jobPool.size(); idx++) {
        int countSucc = 0;
        // Check if terminal job
        for (size_t j = 0; j < dependencyGraph[endIndex - idx].size(); j++)
            if (dependencyGraph[endIndex - idx][j] == 1)
                countSucc++;
        if (countSucc == 0)
            jobPool[endIndex - idx].terminalJob = true;

        countSucc = 0;
        // Check if initial job
        for (size_t j = 0; j < dependencyGraph.size(); j++)
            if (dependencyGraph[j][endIndex - idx] == 1)
                countSucc++;
        if (countSucc == 0)
            jobPool[endIndex - idx].initialJob = true;

        double maxPrio = 0;
        double avgTime;
        if (!jobPool[endIndex - idx].executeOnCloud) {
            double sumT = 0;
            for (size_t j = 0; j < execMatrix[endIndex - idx].size(); j++)
                sumT += execMatrix[endIndex - idx][j];
            avgTime = sumT / 3.0;
        }
        else {
            avgTime = limitThreshold;
        }

        for (size_t j = 0; j < dependencyGraph[endIndex - idx].size(); j++)
            if ((dependencyGraph[endIndex - idx][j] == 1) && (maxPrio < jobPool[j].priorityMetric))
                maxPrio = jobPool[j].priorityMetric;

        jobPool[endIndex - idx].priorityMetric = avgTime + maxPrio;
    }
}

int findMaxPriorityIndex(const vector<JobUnit>& units)
{
    int maxIdx = 0;
    for (size_t i = 0; i < units.size(); i++)
        if (units[i].priorityMetric > units[maxIdx].priorityMetric)
            maxIdx = (int)i;
    return maxIdx;
}

// Compute the earliest local ready time
int localReadyCalc(JobUnit& unit, const vector<JobUnit>& scheduled, const vector<vector<int>>& graph)
{
    int val = 0;
    if (!scheduled.empty()) {
        for (size_t i = 0; i < graph.size(); i++)
            if (graph[i][unit.identifier - 1] == 1)
                for (size_t j = 0; j < scheduled.size(); j++)
                    if ((scheduled[j].identifier == (int)i + 1) &&
                        (val < maxOfTwo(scheduled[j].finishLocal, scheduled[j].finishReceiving)))
                        val = maxOfTwo(scheduled[j].finishLocal, scheduled[j].finishReceiving);
    }
    return val;
}

// Earliest sending ready time if cloud execution
int cloudSendReadyCalc(JobUnit& unit, const vector<JobUnit>& scheduled, const vector<vector<int>>& graph)
{
    int val = 0;
    if (!scheduled.empty()) {
        for (size_t i = 0; i < graph.size(); i++)
            if (graph[i][unit.identifier - 1] == 1)
                for (size_t j = 0; j < scheduled.size(); j++)
                    if ((scheduled[j].identifier == (int)i + 1) &&
                        (val < maxOfTwo(scheduled[j].finishLocal, scheduled[j].finishSending)))
                        val = maxOfTwo(scheduled[j].finishLocal, scheduled[j].finishSending);
    }
    return val;
}

// Cloud compute ready time
int cloudComputeReadyCalc(JobUnit& unit, const vector<JobUnit>& scheduled, const vector<vector<int>>& graph)
{
    int val = unit.finishSending;
    if (!scheduled.empty()) {
        for (size_t i = 0; i < graph.size(); i++)
            if (graph[i][unit.identifier - 1] == 1)
                for (size_t j = 0; j < scheduled.size(); j++)
                    if ((scheduled[j].identifier == (int)i + 1) &&
                        (val < maxOfTwo(unit.finishSending, scheduled[j].finishCloud)))
                        val = maxOfTwo(unit.finishSending, scheduled[j].finishCloud);
    }
    return val;
}

// Cloud receive ready time
int cloudReceiveReadyCalc(JobUnit& unit)
{
    return unit.finishCloud;
}

// Schedule job locally
int localScheduling(JobUnit& unit, const vector<JobUnit>& S, const vector<vector<int>>& G, const vector<vector<int>>& timeMatrix)
{
    unit.readyLocal = localReadyCalc(unit, S, G);
    int minimalFinish = INT_MAX;
    int fTime;
    if (S.empty()) {
        for (size_t i = 0; i < timeMatrix[unit.identifier - 1].size(); i++) {
            fTime = unit.readyLocal + timeMatrix[unit.identifier - 1][i];
            if (fTime < minimalFinish) {
                minimalFinish = fTime;
                unit.assignedResource = (int)i;
            }
        }
        return minimalFinish;
    }

    for (size_t i = 0; i < timeMatrix[unit.identifier - 1].size(); i++) {
        fTime = unit.readyLocal + timeMatrix[unit.identifier - 1][i];
        int maxLocalF = 0;
        for (size_t j = 0; j < S.size(); j++)
            if ((S[j].assignedResource == (int)i) && (maxLocalF < S[j].finishLocal))
                maxLocalF = S[j].finishLocal;
        if (maxLocalF > unit.readyLocal)
            fTime = maxLocalF + timeMatrix[unit.identifier - 1][i];
        if (fTime < minimalFinish) {
            minimalFinish = fTime;
            unit.assignedResource = (int)i;
        }
    }
    return minimalFinish;
}

// Schedule job on cloud
int cloudScheduling(JobUnit& unit, const vector<JobUnit>& S, const vector<vector<int>>& G, int sendTime, int cloudTime, int recvTime)
{
    unit.readySending = cloudSendReadyCalc(unit, S, G);
    int totalDelay = sendTime + cloudTime + recvTime;
    int maxS = 0, maxC = 0, maxR = 0, finalF;

    if (S.empty()) {
        unit.finishSending = sendTime;
        unit.readyCloud = sendTime;
        unit.finishCloud = sendTime + cloudTime;
        unit.readyReceiving = sendTime + cloudTime;
        return totalDelay;
    }

    for (size_t i = 0; i < S.size(); i++)
        if (S[i].assignedResource == 3 && maxS < S[i].finishSending)
            maxS = S[i].finishSending;

    if (maxS > unit.readySending)
        unit.finishSending = maxS + sendTime;
    else
        unit.finishSending = unit.readySending + sendTime;

    unit.readyCloud = cloudComputeReadyCalc(unit, S, G);

    for (size_t i = 0; i < S.size(); i++)
        if (S[i].assignedResource == 3 && maxC < S[i].finishCloud)
            maxC = S[i].finishCloud;

    if (maxC > unit.readyCloud)
        unit.finishCloud = maxC + cloudTime;
    else
        unit.finishCloud = unit.readyCloud + cloudTime;

    unit.readyReceiving = cloudReceiveReadyCalc(unit);

    for (size_t i = 0; i < S.size(); i++)
        if (S[i].assignedResource == 3 && maxR < S[i].finishReceiving)
            maxR = S[i].finishReceiving;

    if (maxR > unit.readyReceiving)
        finalF = maxR + recvTime;
    else
        finalF = unit.readyReceiving + recvTime;

    return finalF;
}

// Find finishing time of a job
int jobFinishTime(JobUnit& unit)
{
    return maxOfTwo(unit.finishLocal, unit.finishReceiving);
}

// Print the final schedule
void printFinalSchedule(const vector<JobUnit>& S)
{
    for (size_t i = 0; i < S.size(); i++) {
        int doneT = jobFinishTime((JobUnit&)S[i]);
        cout << "Job" << S[i].identifier << ": ";
        switch (S[i].assignedResource) {
        case 0:
            cout << "Local Resource " << S[i].assignedResource + 1 << ", ";
            break;
        case 1:
            cout << "Local Resource " << S[i].assignedResource + 1 << ", ";
            break;
        case 2:
            cout << "Local Resource " << S[i].assignedResource + 1 << ", ";
            break;
        case 3:
            cout << "Cloud Resource, ";
            break;
        default:
            break;
        }
        cout << "start: " << S[i].beginTime << ", finish: " << doneT << endl;
    }
}

// Determine overall completion time
int findCompletionTime(const vector<JobUnit>& S)
{
    int mTime = 0;
    for (size_t i = 0; i < S.size(); i++)
        if ((S[i].terminalJob) && (mTime < jobFinishTime((JobUnit&)S[i])))
            mTime = jobFinishTime((JobUnit&)S[i]);
    return mTime;
}

// Calculate total energy usage
double computeEnergy(const vector<JobUnit>& S, int p1, int p2, int p3, double ps)
{
    double energy = 0;
    for (size_t i = 0; i < S.size(); i++) {
        int ft = jobFinishTime((JobUnit&)S[i]);
        switch (S[i].assignedResource) {
        case 0:
            energy += p1 * (ft - S[i].beginTime);
            break;
        case 1:
            energy += p2 * (ft - S[i].beginTime);
            break;
        case 2:
            energy += p3 * (ft - S[i].beginTime);
            break;
        case 3:
            energy += ps * (S[i].finishSending - S[i].beginTime);
            break;
        default:
            break;
        }
    }
    return energy;
}

// Compute ready counts
void updateReadyCount1(vector<JobUnit>& S, const vector<vector<int>>& G)
{
    for (size_t i = 0; i < S.size(); i++) {
        int c = 0;
        for (size_t j = 0; j < G.size(); j++)
            if (G[j][S[i].identifier - 1] == 1)
                for (size_t k = 0; k < S.size(); k++)
                    if (S[k].identifier == (int)j + 1)
                        c++;
        S[i].predCountReady = c;
    }
}

void updateReadyCount2(vector<JobUnit>& S)
{
    for (size_t i = 0; i < S.size(); i++) {
        int c = 0;
        for (size_t j = 0; j < S.size(); j++)
            if ((S[i].assignedResource == S[j].assignedResource) && (S[j].beginTime < S[i].beginTime))
                c++;
        S[i].sameChannelReady = c;
    }
}

// Local scheduling for known channel
int finalizeLocal(JobUnit& v, const vector<JobUnit>& SN, const vector<vector<int>>& G, const vector<vector<int>>& execTimes)
{
    v.readyLocal = localReadyCalc(v, SN, G);
    int fTime;
    int maxLoc = 0;
    if (SN.empty())
        fTime = v.readyLocal + execTimes[v.identifier - 1][v.assignedResource];
    else {
        for (size_t i = 0; i < SN.size(); i++)
            if ((SN[i].assignedResource == v.assignedResource) && (maxLoc < SN[i].finishLocal))
                maxLoc = SN[i].finishLocal;
        if (maxLoc > v.readyLocal)
            fTime = maxLoc + execTimes[v.identifier - 1][v.assignedResource];
        else
            fTime = v.readyLocal + execTimes[v.identifier - 1][v.assignedResource];
    }
    return fTime;
}

void reScheduling(const vector<JobUnit>& S, vector<JobUnit>& SN, int newRes, JobUnit target, const vector<vector<int>>& G, const vector<vector<int>>& times, int ts, int tc, int tr)
{
    vector<JobUnit> temp = S;
    int fullDelay = ts + tc + tr;

    for (size_t i = 0; i < temp.size(); i++)
        if (target.identifier == temp[i].identifier) {
            temp[i].assignedResource = newRes;
            if (newRes == 3) {
                temp[i].finishLocal = 0;
                temp[i].readyLocal = 0;
            }
        }

    while (!temp.empty()) {
        updateReadyCount1(temp, G);
        updateReadyCount2(temp);
        size_t m = 0;
        while (m < temp.size() && (temp[m].predCountReady != 0) && (temp[m].sameChannelReady != 0))
            m++;
        if (temp[m].assignedResource == 3) {
            temp[m].finishReceiving = cloudScheduling(temp[m], SN, G, ts, tc, tr);
            temp[m].beginTime = temp[m].finishReceiving - fullDelay;
        }
        else {
            temp[m].finishLocal = finalizeLocal(temp[m], SN, G, times);
            temp[m].beginTime = temp[m].finishLocal - times[temp[m].identifier - 1][temp[m].assignedResource];
        }
        SN.push_back(temp[m]);
        temp.erase(temp.begin() + m);
    }
}

void migrateJobs(vector<JobUnit>& S, const vector<vector<int>>& G, const vector<vector<int>>& times, int ts, int tc, int tr, int p1, int p2, int p3, double ps, int timeMax)
{
    int origCompletion = findCompletionTime(S);
    double origEnergy = computeEnergy(S, p1, p2, p3, ps);
    double bestGain = 0;

    for (size_t i = 0; i < S.size(); i++) {
        int currRes = S[i].assignedResource;
        if (S[i].assignedResource != 3) {
            for (int newRes = 0; newRes < 4; newRes++) {
                if (newRes != currRes) {
                    vector<JobUnit> newSeq;
                    double oldE = computeEnergy(S, p1, p2, p3, ps);
                    reScheduling(S, newSeq, newRes, S[i], G, times, ts, tc, tr);
                    int newComp = findCompletionTime(newSeq);
                    double newE = computeEnergy(newSeq, p1, p2, p3, ps);
                    if ((newE < oldE) && (origCompletion >= newComp))
                        S = newSeq;
                    else if ((newE < oldE) && (newComp <= timeMax)) {
                        double ratio = (origEnergy - newE) / (newComp - origCompletion);
                        if (ratio > bestGain) {
                            bestGain = ratio;
                            S = newSeq;
                        }
                    }
                }
            }
        }
    }
}

void outerLoopRefine(vector<JobUnit>& S, const vector<vector<int>>& G, const vector<vector<int>>& times, int ts, int tc, int tr, int p1, int p2, int p3, double ps, int timeMax)
{
    double prevEn;
    double nextEn = 0;
    prevEn = computeEnergy(S, p1, p2, p3, ps);
    while (nextEn < prevEn) {
        prevEn = computeEnergy(S, p1, p2, p3, ps);
        migrateJobs(S, G, times, ts, tc, tr, p1, p2, p3, ps, timeMax);
        nextEn = computeEnergy(S, p1, p2, p3, ps);
    }
}

void initialScheduling(vector<JobUnit>& finalSet, vector<JobUnit>& initialJobs, const vector<vector<int>>& times, const vector<vector<int>>& G, int ts, int tc, int tr)
{
    int fullLatency = ts + tc + tr;
    for (size_t i = 0; i < G.size(); i++) {
        int maxPidx = findMaxPriorityIndex(initialJobs);
        if (!initialJobs[maxPidx].executeOnCloud) {
            int localMin = localScheduling(initialJobs[maxPidx], finalSet, G, times);
            int cloudFin = cloudScheduling(initialJobs[maxPidx], finalSet, G, ts, tc, tr);
            if (cloudFin < localMin) {
                initialJobs[maxPidx].readyLocal = 0;
                initialJobs[maxPidx].finishLocal = 0;
                initialJobs[maxPidx].assignedResource = 3;
                initialJobs[maxPidx].finishReceiving = cloudFin;
                initialJobs[maxPidx].beginTime = cloudFin - fullLatency;
            }
            else {
                initialJobs[maxPidx].finishCloud = 0;
                initialJobs[maxPidx].finishSending = 0;
                initialJobs[maxPidx].readySending = 0;
                initialJobs[maxPidx].readyCloud = 0;
                initialJobs[maxPidx].readyReceiving = 0;
                initialJobs[maxPidx].finishReceiving = 0;
                initialJobs[maxPidx].finishLocal = localMin;
                initialJobs[maxPidx].beginTime = localMin - times[initialJobs[maxPidx].identifier - 1][initialJobs[maxPidx].assignedResource];
            }
        }
        else {
            initialJobs[maxPidx].finishLocal = 0;
            initialJobs[maxPidx].readyLocal = 0;
            initialJobs[maxPidx].assignedResource = 3;
            initialJobs[maxPidx].finishReceiving = cloudScheduling(initialJobs[maxPidx], finalSet, G, ts, tc, tr);
            initialJobs[maxPidx].beginTime = initialJobs[maxPidx].finishReceiving - fullLatency;
        }
        finalSet.push_back(initialJobs[maxPidx]);
        initialJobs.erase(initialJobs.begin() + maxPidx);
    }
}

int main()
{
    int TASK_COUNT; // JOB_COUNT
    int NATIVE_CORES; // LOCAL_UNITS

    cout << "Enter number of tasks (TASK_COUNT): ";
    cin >> TASK_COUNT;
    cout << "Enter number of cores (NATIVE_CORES): ";
    cin >> NATIVE_CORES;

    int tSend, tCloud, tReceive;
    int tMaximum;
    int POWER_RES_1, POWER_RES_2, POWER_RES_3;
    double POWER_CLOUD;

    cout << "Enter sending time (tSend): ";
    cin >> tSend;
    cout << "Enter cloud computation time (tCloud): ";
    cin >> tCloud;
    cout << "Enter receiving time (tReceive): ";
    cin >> tReceive;
    cout << "Enter maximum allowed completion time (tMaximum): ";
    cin >> tMaximum;

    cout << "Enter power consumption for resource 1 (POWER_RES_1): ";
    cin >> POWER_RES_1;
    cout << "Enter power consumption for resource 2 (POWER_RES_2): ";
    cin >> POWER_RES_2;
    cout << "Enter power consumption for resource 3 (POWER_RES_3): ";
    cin >> POWER_RES_3;
    cout << "Enter cloud power consumption (POWER_CLOUD): ";
    cin >> POWER_CLOUD;

    vector<vector<int>> executionMatrix(TASK_COUNT, vector<int>(NATIVE_CORES, 0)); // JOB_COUNT = TASK_COUNT LOCAL_UNITS = NATIVE_CORES
    cout << "Enter the execution times for " << TASK_COUNT << " jobs on "
        << NATIVE_CORES << " local units (one line per job):" << endl;
    for (int i = 0; i < TASK_COUNT; i++) {
        for (int j = 0; j < NATIVE_CORES; j++) {
            cin >> executionMatrix[i][j];
        }
    }

    vector<vector<int>> dependencyGraph(TASK_COUNT, vector<int>(TASK_COUNT, 0));
    cout << "Enter the dependency matrix (" << TASK_COUNT << "x" << TASK_COUNT << "):" << endl;
    for (int i = 0; i < TASK_COUNT; i++) {
        for (int j = 0; j < TASK_COUNT; j++) {
            cin >> dependencyGraph[i][j];
        }
    }

    vector<JobUnit> initialJobs(TASK_COUNT);
    vector<JobUnit> finalSchedule;

    int limitVal = tSend + tCloud + tReceive;

    initialJobAssignment(initialJobs, executionMatrix, limitVal);
    computePriorities(initialJobs, executionMatrix, dependencyGraph, limitVal);

    clock_t startRun, endRun;
    startRun = clock();
    initialScheduling(finalSchedule, initialJobs, executionMatrix, dependencyGraph, tSend, tCloud, tReceive);
    endRun = clock();

    cout << "Initial schedule:" << endl;
    printFinalSchedule(finalSchedule);
    double runTime = (double)(endRun - startRun) / (double)CLOCKS_PER_SEC * 1000.0;
    cout << "Total energy: " << computeEnergy(finalSchedule, POWER_RES_1, POWER_RES_2, POWER_RES_3, POWER_CLOUD) << endl;
    cout << "Completion time: " << findCompletionTime(finalSchedule) << endl;
    cout << "Initial scheduling time: " << runTime << " ms" << endl;

    startRun = clock();
    outerLoopRefine(finalSchedule, dependencyGraph, executionMatrix, tSend, tCloud, tReceive, POWER_RES_1, POWER_RES_2, POWER_RES_3, POWER_CLOUD, tMaximum);
    endRun = clock();

    cout << "After migrations:" << endl;
    printFinalSchedule(finalSchedule);
    runTime = (double)(endRun - startRun) / (double)CLOCKS_PER_SEC * 1000.0;
    cout << "Total energy: " << computeEnergy(finalSchedule, POWER_RES_1, POWER_RES_2, POWER_RES_3, POWER_CLOUD) << endl;
    cout << "Completion time: " << findCompletionTime(finalSchedule) << endl;
    cout << "Refinement time: " << runTime << " ms" << endl;
    cout << endl;
    cout << "Press Enter to continue..." << endl;
    cin.get(); 
    
    return 0;
}
