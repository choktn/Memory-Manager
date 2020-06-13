#include <iostream>
#include <fstream>
#include <algorithm>
#include <math.h>
#include <iomanip>
#include <vector>

using namespace std;

struct ProcessInfo
{
    int pid, memarrivaltime, arrivaltime, lifetime, addrspace = 0, memorypieces;
    bool inmemory = false, completed = false;
};

struct MemoryInfo
{
    int start, end, pid, pagenum, size;
    bool free, deleteblock = false;
};

bool getSmallestLifeTime(const ProcessInfo &a, const ProcessInfo &b)
{
    return a.lifetime < b.lifetime;
}

bool getSmallestStartTime(const MemoryInfo &a, const MemoryInfo &b)
{
    return a.start < b.start;
}

void outputMemoryMap(vector<MemoryInfo> &memorymap)
{
    sort(memorymap.begin(), memorymap.end(), getSmallestStartTime);
    cout << setw(19) << "Memory Map: ";
    for (int i = 0; i < memorymap.size(); i++)
    {

        cout << memorymap[i].start << "-" << memorymap[i].end << ": ";
        if (!memorymap[i].free)
        {
            cout << "Process " << memorymap[i].pid << ", Page " << memorymap[i].pagenum << endl;
        }
        else
        {
            cout << "Free frame(s)" << endl;
        }
        cout << setw(19);
    }
}

void outputInputQueue(vector<ProcessInfo> inputqueue)
{
    cout << setw(21) << "Input Queue: [";
    for (int i = 0; i < inputqueue.size(); i++)
    {
        if (!inputqueue[i].inmemory)
        {
            cout << " " << inputqueue[i].pid;
        }
    }
    cout << " ]" << endl;
}

bool allocateMemory(ProcessInfo &process, vector<MemoryInfo> &memorymap, vector<ProcessInfo> &inputqueue, int memorysize, int pagesize)
{
    bool allocated = false;
    MemoryInfo tempblock;
    int pagenum = ceil(float(process.addrspace) / float(pagesize));
    int freespace = 0, currpagenum = 1, deletecount = 0;
    for (int i = 0; i < memorymap.size(); i++)
    {
        if (memorymap[i].free)
        {
            freespace += memorymap[i].size;
        }
    }

    if (freespace >= process.addrspace)
    {
        for (int i = 0; i < memorymap.size(); i++)
        {
            if (allocated || currpagenum == pagenum + 1)
            {
                allocated = true;
                break;
            }
            if (memorymap[i].free)
            {
                if (memorymap[i].size >= process.addrspace && currpagenum == 1) //allocate the current block if it can hold the entire address space of the current process
                {

                    for (int j = 0; j < pagenum; j++)
                    {
                        if (j == 0)
                        {
                            tempblock.start = memorymap[i].start;
                        }
                        else
                        {
                            tempblock.start = tempblock.end + 1;
                        }
                        tempblock.end = tempblock.start + (pagesize - 1);
                        tempblock.pid = process.pid;
                        tempblock.pagenum = j + 1;
                        tempblock.size = pagesize;
                        tempblock.free = false;
                        memorymap.push_back(tempblock);

                        memorymap[i].size -= pagesize;
                        if (memorymap[i].size > 0)
                        {
                            memorymap[i].start = tempblock.end + 1;
                        }
                        else
                        {
                            memorymap[i].deleteblock = true;
                            deletecount++;
                        }
                    }
                    currpagenum = pagenum + 1;
                    allocated = true;
                    break;
                }
                else if (memorymap[i].size > pagesize) //allocate the current block if it is partially the size of or it can fill the rest of the address space of the current process
                {
                    for (int j = 0; j < memorymap[i].size / pagesize; j++)
                    {
                        if (currpagenum == pagenum + 1)
                        {
                            allocated = true;
                            break;
                        }
                        if (j == 0)
                        {
                            tempblock.start = memorymap[i].start;
                        }
                        else
                        {
                            tempblock.start = tempblock.end + 1;
                        }
                        tempblock.end = tempblock.start + (pagesize - 1);
                        tempblock.pid = process.pid;
                        tempblock.pagenum = currpagenum;
                        tempblock.size = pagesize;
                        tempblock.free = false;
                        memorymap.push_back(tempblock);

                        memorymap[i].size -= pagesize;
                        if (memorymap[i].size > 0)
                        {
                            memorymap[i].start = tempblock.end + 1;
                        }
                        else
                        {
                            memorymap[i].deleteblock = true;
                            deletecount++;
                        }
                        currpagenum++;
                    }
                }
                else if (memorymap[i].size == pagesize) //allocate the current block if it is only one page size
                {
                    memorymap[i].pid = process.pid;
                    memorymap[i].pagenum = currpagenum;
                    memorymap[i].free = false;

                    currpagenum++;
                }
            }
        }
        if (deletecount > 0)
        {
            for (int i = 0; i < deletecount; i++)
            {
                for (int j = 0; j < memorymap.size(); j++)
                {
                    if (memorymap[j].deleteblock)
                    {
                        memorymap.erase(memorymap.begin() + j);
                    }
                }
            }
        }
    }

    if (allocated)
    {
        cout << setw(24) << "MM moved Process " << process.pid << " to memory" << endl;
        process.inmemory = true;
        for (int i = 0; i < inputqueue.size(); i++)
        {
            if (inputqueue[i].pid == process.pid)
            {
                inputqueue[i] = process;
            }
        }
        outputInputQueue(inputqueue);
    }

    return allocated;
}

void deallocateMemory(ProcessInfo &process, vector<MemoryInfo> &memorymap)
{
    for (int i = 0; i < memorymap.size(); i++) //free frames associated with the current process
    {
        if (memorymap[i].pid == process.pid)
        {
            memorymap[i].free = true;
            memorymap[i].pid = -1;
            memorymap[i].pagenum = -1;
        }
    }

    int deletecount = 0;
    //could not get free block combining to work completely, just wanted to show what we tried to do
    /*sort(memorymap.begin(), memorymap.end(), getSmallestStartTime);
    for (int i = 0; i < memorymap.size(); i++) //combine any closeby free blocks i.e. 0-99, 100-199, 200-299 --> 0-299
    {
        if (memorymap[i].free)
        {
            for (int j = i + 1; j < memorymap.size(); j++)
            {
                if (memorymap[j].free)
                {
                    if (memorymap[i].end + 1 == memorymap[j].start)
                    {
                        memorymap[i].end = memorymap[j].end;
                        memorymap[i].size += memorymap[j].size;
                        memorymap[i].pid = -1;
                        memorymap[i].pagenum = -1;
                        memorymap[i].free = true;

                        memorymap[j].deleteblock = true;
                        memorymap[j].free = false;
                        deletecount++;
                    }
                }
                else
                {
                    j = memorymap.size();
                }
            }
        }
    }
    if (deletecount > 0)
    {
        for (int i = 0; i < deletecount; i++)
        {
            for (int j = 0; j < memorymap.size(); j++)
            {
                if (memorymap[j].deleteblock)
                {
                    memorymap.erase(memorymap.begin() + j);
                }
            }
        }
    }*/
}
int main()
{
    ifstream file;
    int memorysize, pagesize, tempmemsize, processcount, t = 0, processcompletions = 0;
    float turnaroundtime = 0;
    bool newevent = false, firstarrival = false;
    string filename;
    vector<int> deletequeue;
    vector<ProcessInfo> processlist, inputqueue;
    vector<MemoryInfo> memorymap;
    ProcessInfo tempprocess;
    MemoryInfo initmap;

    cout << "Enter memory size: ";
    cin >> memorysize;
    cout << "Enter page size (1:100, 2:200, 3:400): ";
    cin >> pagesize;
    cout << "Enter the name of the workload file (just the name): ";
    cin >> filename;

    if (pagesize == 1)
    {
        pagesize = 100;
    }
    else if (pagesize == 2)
    {
        pagesize = 200;
    }
    else if (pagesize == 3)
    {
        pagesize = 400;
    }

    file.open(filename + ".txt");

    if (file.is_open())
    {
        file >> processcount;
        for (int i = 0; i < processcount; i++)
        {
            file >> tempprocess.pid;
            file >> tempprocess.arrivaltime;
            file >> tempprocess.lifetime;
            file >> tempprocess.memorypieces;
            for (int j = 0; j < tempprocess.memorypieces; j++)
            {
                file >> tempmemsize;
                tempprocess.addrspace += tempmemsize;
            }

            processlist.push_back(tempprocess);
            tempprocess.addrspace = 0;
        }
        file.close();
    }
    else
    {
        cout << "Error. Could not open file " + filename + ".txt" << endl;
    }

    initmap.start = 0;
    initmap.end = memorysize - 1;
    initmap.size = memorysize;
    initmap.free = true;

    memorymap.push_back(initmap);

    do
    {
        if (processlist.size() > 0)
        {
            for (int i = 0; i < processlist.size(); i++) //add processes to input queue when their arrival time is reached
            {
                if (processlist[i].arrivaltime == t && !newevent)
                {
                    cout << "\nt = " << t;
                    newevent = true;
                    firstarrival = true;
                }
                if (processlist[i].arrivaltime == t && newevent)
                {
                    if (!firstarrival)
                    {
                        cout << setw(15) << "Process " << processlist[i].pid << " arrives" << endl;
                    }
                    else
                    {
                        cout << ": Process " << processlist[i].pid << " arrives" << endl;
                        firstarrival = false;
                    }
                    inputqueue.push_back(processlist[i]);

                    outputInputQueue(inputqueue);
                    outputMemoryMap(memorymap);
                }
            }
        }

        if (processlist.size() > 0) //deallocate processes from memory when they reach their lifetime
        {
            sort(processlist.begin(), processlist.end(), getSmallestLifeTime);
            for (int i = 0; i < processlist.size(); i++)
            {
                if (processlist[i].inmemory && t - processlist[i].memarrivaltime == processlist[i].lifetime)
                {
                    if (!newevent)
                    {
                        cout << "\nt = " << t << ": Process " << processlist[i].pid << " completes" << endl;
                        newevent = true;
                    }
                    else
                    {
                        cout << setw(15) << "Process " << processlist[i].pid << " completes" << endl;
                    }
                    processlist[i].completed = true;
                    turnaroundtime += t - processlist[i].arrivaltime;

                    deallocateMemory(processlist[i], memorymap);
                    deletequeue.push_back(processlist[i].pid);
                    
                    outputMemoryMap(memorymap);
                    
                    processcompletions++;
                }
            }
            if (deletequeue.size() > 0)
            {
                for (int i = 0; i < deletequeue.size(); i++)
                {
                    for (int j = 0; j < processlist.size(); j++)
                    {
                        if (processlist[j].pid == deletequeue[i])
                        {
                            processlist.erase(processlist.begin() + j);
                        }
                    }
                }
                deletequeue.clear();
            }
        }

        if (inputqueue.size() > 0) //try to send processes in queue to memory
        {
            for (int i = 0; i < inputqueue.size(); i++)
            {
                if (allocateMemory(inputqueue[i], memorymap, inputqueue, memorysize, pagesize))
                {
                    inputqueue[i].memarrivaltime = t;
                    outputMemoryMap(memorymap);

                    for (int j = 0; j < processlist.size(); j++)
                    {
                        if (processlist[j].pid == inputqueue[i].pid)
                        {
                            processlist[j] = inputqueue[i];
                        }
                    }
                    deletequeue.push_back(inputqueue[i].pid);
                }
            }
            if (deletequeue.size() > 0)
            {
                for (int i = 0; i < deletequeue.size(); i++)
                {
                    for (int j = 0; j < inputqueue.size(); j++)
                    {
                        if (inputqueue[j].pid == deletequeue[i])
                        {
                            inputqueue.erase(inputqueue.begin() + j);
                        }
                    }
                }
                deletequeue.clear();
            }
        }

        newevent = false;
        firstarrival = false;
        t++;
    } while (t != 100000 && processcompletions != processcount);

    cout << "\nAverage turnaround: " << turnaroundtime / processcount << "(" << turnaroundtime << "/" << processcount << ")" << endl;
    return 0;
}
