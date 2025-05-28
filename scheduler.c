#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PROCESSES 100
#define TIME_QUANTUM   4
#define MAX_EVENTS   1000

typedef struct {
    int pid, arrival, burst, remaining, priority;
    int start_time, completion, waiting, turnaround;
    int finished;
    long long vruntime; 
    int weight;      
} PCB;

typedef struct {
    int pid, start, end;
} Event;

PCB proc_orig[MAX_PROCESSES];
int N;

void clone(PCB dst[], PCB src[]) { memcpy(dst, src, sizeof(PCB)*N); }

void reset_metrics(PCB p[]) {
    for(int i=0; i<N; i++) {
        p[i].remaining = p[i].burst;
        p[i].start_time = -1;
        p[i].completion = p[i].waiting = p[i].turnaround = 0;
        p[i].finished = 0;
        p[i].vruntime = 0;
        p[i].weight = (10 - p[i].priority) * 100; 
    }
}

void print_table(const char *title, PCB p[]) {
    printf("\n--------------------------------------------------------------\n");
    printf("%s\n", title);
    puts("PID\tArr\tBurst\tPri\tStart\tCompl\tWait\tTurn");
    for(int i=0; i<N; i++)
        printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
            p[i].pid, p[i].arrival, p[i].burst, p[i].priority,
            p[i].start_time, p[i].completion, p[i].waiting, p[i].turnaround);
    double aw=0, at=0;
    for(int i=0; i<N; i++) { aw += p[i].waiting; at += p[i].turnaround; }
    printf("Average Waiting Time    : %.2f\n", aw/N);
    printf("Average Turnaround Time : %.2f\n", at/N);
}

void print_gantt(const char *algo_name, Event ev[], int cnt) {
    printf("\n>>> Gantt Chart – %s\n", algo_name);

    Event merged[MAX_EVENTS];
    int merged_cnt = 0;
    
    if (cnt > 0) {
        merged[0] = ev[0];
        for (int i = 1; i < cnt; i++) {
            if (ev[i].pid == merged[merged_cnt].pid && 
                ev[i].start == merged[merged_cnt].end) {
                // 연속된 동일한 PID 이벤트면 end 시간만 업데이트
                merged[merged_cnt].end = ev[i].end;
            } else {
                // 다른 PID나 IDLE이면 새로운 이벤트 추가
                merged[++merged_cnt] = ev[i];
            }
        }
        merged_cnt++;
    }

    for(int i=0; i<merged_cnt; i++) {
        if (merged[i].pid == -1)
            printf("|  IDLE ");
        else
            printf("|  P%-2d  ", merged[i].pid);
    }
    printf("|\n");
    printf("%-7d", merged[0].start);
    for(int i=0; i<merged_cnt; i++) printf("%-7d", merged[i].end);
    printf("\n");
}

void create_processes_random() {
    printf("Number of processes: ");
    scanf("%d", &N);
    srand((unsigned)time(NULL));
    for(int i=0; i<N; i++) {
        proc_orig[i].pid = i+1;
        proc_orig[i].arrival = rand()%10;
        proc_orig[i].burst = (rand()%10)+5;
        proc_orig[i].priority = rand()%10;
    }
    reset_metrics(proc_orig);
}

void fcfs() {
    PCB P[MAX_PROCESSES]; clone(P, proc_orig); reset_metrics(P);
    Event ev[MAX_EVENTS]; int ec=0;
    int time=0, completed=0;

    while(completed < N) {
        int idx = -1, min_arr = 1e9;
        for(int i=0; i<N; i++)
            if(!P[i].finished && P[i].arrival <= time && P[i].arrival < min_arr) {
                idx = i; min_arr = P[i].arrival;
            }
        if(idx == -1) { 
            ev[ec++] = (Event){-1, time, time + 1};
            time++; 
            continue; 
        }

        if(P[idx].start_time == -1) P[idx].start_time = time;
        int start = time;
        time += P[idx].burst;
        int end = time;
        ev[ec++] = (Event){P[idx].pid, start, end};

        P[idx].completion = time;
        P[idx].turnaround = time - P[idx].arrival;
        P[idx].waiting = P[idx].turnaround - P[idx].burst;
        P[idx].finished = 1; completed++;
    }
    print_table("[FCFS]", P);
    print_gantt("FCFS", ev, ec);
}

void sjf_np() {
    PCB P[MAX_PROCESSES]; clone(P, proc_orig); reset_metrics(P);
    Event ev[MAX_EVENTS]; int ec=0;
    int time=0, completed=0;

    while(completed < N) {
        int idx = -1, min_burst = 1e9;
        for(int i=0; i<N; i++)
            if(!P[i].finished && P[i].arrival <= time && P[i].burst < min_burst) {
                idx = i; min_burst = P[i].burst;
            }
        if(idx == -1) { 
            ev[ec++] = (Event){-1, time, time + 1};
            time++; 
            continue; 
        }

        if(P[idx].start_time == -1) P[idx].start_time = time;
        int start = time;
        time += P[idx].burst;
        int end = time;
        ev[ec++] = (Event){P[idx].pid, start, end};

        P[idx].completion = time;
        P[idx].turnaround = time - P[idx].arrival;
        P[idx].waiting = P[idx].turnaround - P[idx].burst;
        P[idx].finished = 1; completed++;
    }
    print_table("[SJF – NP]", P);
    print_gantt("SJF – NP", ev, ec);
}

void sjf_p() {
    PCB P[MAX_PROCESSES]; clone(P, proc_orig); reset_metrics(P);
    Event ev[MAX_EVENTS]; int ec=0;
    int time=0, completed=0;
    int last = -1;

    while(completed < N) {
        int idx = -1, min_rem = 1e9;
        if(last != -1) {
            idx = last;
            min_rem = P[idx].remaining;
        }
        for(int i=0; i<N; i++)
            if(!P[i].finished && P[i].arrival <= time && P[i].remaining < min_rem) {
                idx = i; min_rem = P[i].remaining;
            }

        if(idx != -1) {
            if(P[idx].start_time == -1) P[idx].start_time = time;
            int start = time;
            P[idx].remaining--;
            time++;
            int end = time;

            if(idx != last)
                ev[ec++] = (Event){P[idx].pid, start, end};
            else
                ev[ec-1].end = end;

            last = idx;

            if(P[idx].remaining == 0) {
                P[idx].completion = time;
                P[idx].turnaround = time - P[idx].arrival;
                P[idx].waiting = P[idx].turnaround - P[idx].burst;
                P[idx].finished = 1; completed++;
                last = -1;
            }
        } else { 
            ev[ec++] = (Event){-1, time, time + 1};
            last = -1;
            time++; 
            continue; 
        }
    }
    print_table("[SJF – P]", P);
    print_gantt("SJF – P", ev, ec);
}

void priority_np() {
    PCB P[MAX_PROCESSES]; clone(P, proc_orig); reset_metrics(P);
    Event ev[MAX_EVENTS]; int ec=0;
    int time=0, completed=0;

    while(completed < N) {
        int idx = -1, min_pri = 1e9;
        for(int i=0; i<N; i++)
            if(!P[i].finished && P[i].arrival <= time && P[i].priority < min_pri) {
                idx = i; min_pri = P[i].priority;
            }
        if(idx == -1) { 
            ev[ec++] = (Event){-1, time, time + 1};
            time++; 
            continue; 
        }

        if(P[idx].start_time == -1) P[idx].start_time = time;
        int start = time;
        time += P[idx].burst;
        int end = time;
        ev[ec++] = (Event){P[idx].pid, start, end};

        P[idx].completion = time;
        P[idx].turnaround = time - P[idx].arrival;
        P[idx].waiting = P[idx].turnaround - P[idx].burst;
        P[idx].finished = 1; completed++;
    }
    print_table("[Priority – NP]", P);
    print_gantt("Priority – NP", ev, ec);
}

void priority_p() {
    PCB P[MAX_PROCESSES]; clone(P, proc_orig); reset_metrics(P);
    Event ev[MAX_EVENTS]; int ec=0;
    int time=0, completed=0;
    int last = -1;

    while(completed < N) {
        int idx = -1, min_pri = 1e9;
        if(last != -1) {
            idx = last;
            min_pri = P[idx].priority;
        }
        for(int i=0; i<N; i++)
            if(!P[i].finished && P[i].arrival <= time && P[i].priority < min_pri) {
                idx = i; min_pri = P[i].priority;
            }

        if(idx != -1) {
            if(P[idx].start_time == -1) P[idx].start_time = time;
            int start = time;
            P[idx].remaining--;
            time++;
            int end = time;

            if(idx != last)
                ev[ec++] = (Event){P[idx].pid, start, end};
            else
                ev[ec-1].end = end;

            last = idx;

            if(P[idx].remaining == 0) {
                P[idx].completion = time;
                P[idx].turnaround = time - P[idx].arrival;
                P[idx].waiting = P[idx].turnaround - P[idx].burst;
                P[idx].finished = 1; completed++;
                last = -1;
            }
        } else { 
            ev[ec++] = (Event){-1, time, time + 1};
            last = -1;
            time++; 
            continue; 
        }
    }
    print_table("[Priority – P]", P);
    print_gantt("Priority – P", ev, ec);
}

void rr() {
    PCB P[MAX_PROCESSES]; clone(P, proc_orig); reset_metrics(P);
    Event ev[MAX_EVENTS]; int ec=0;
    int q[MAX_PROCESSES]; int front=0, rear=0;
    int time=0, completed=0;

    for(int i=0; i<N; i++) if(P[i].arrival == 0) q[rear++] = i;

    while(completed < N) {
        if(front == rear) {
            ev[ec++] = (Event){-1, time, time + 1};
            time++;
            for(int i=0; i<N; i++) 
                if(!P[i].finished && P[i].arrival == time) q[rear++] = i;
            continue;
        }

        int idx = q[front++ % MAX_PROCESSES];
        if(P[idx].start_time == -1) P[idx].start_time = time;

        int start = time;
        int exec = (P[idx].remaining > TIME_QUANTUM) ? TIME_QUANTUM : P[idx].remaining;
        P[idx].remaining -= exec;
        time += exec;
        int end = time;
        ev[ec++] = (Event){P[idx].pid, start, end};

        for(int t=start+1; t<=end; t++)
            for(int i=0; i<N; i++) 
                if(!P[i].finished && P[i].arrival == t) q[rear++] = i;

        if(P[idx].remaining == 0) {
            P[idx].completion = time;
            P[idx].turnaround = time - P[idx].arrival;
            P[idx].waiting = P[idx].turnaround - P[idx].burst;
            P[idx].finished = 1; completed++;
        } else {
            q[rear++ % MAX_PROCESSES] = idx;
        }
    }
    print_table("[Round Robin]", P);
    print_gantt("Round Robin", ev, ec);
}

void lottery() {
    PCB P[MAX_PROCESSES]; clone(P, proc_orig); reset_metrics(P);
    Event ev[MAX_EVENTS]; int ec=0;
    int time=0, completed=0;
    int last = -1;
    int total_tickets = 0;
    int tickets[MAX_PROCESSES];
    const int BASE_TICKETS = 100;  
    const int LOTTERY_QUANTUM = 4; 

    for(int i=0; i<N; i++) {
        tickets[i] = (10 - P[i].priority) * BASE_TICKETS;
        total_tickets += tickets[i];
    }


    while(completed < N) {
        int current_tickets = 0;
        int available[MAX_PROCESSES], avail_count = 0;
        
        for(int i=0; i<N; i++) {
            if(!P[i].finished && P[i].arrival <= time) {
                current_tickets += tickets[i];
                available[avail_count++] = i;
            }
        }

        if(avail_count > 0) {
            int winning_ticket = rand() % current_tickets;
            int idx = -1;
            int ticket_sum = 0;

            for(int i=0; i<avail_count; i++) {
                ticket_sum += tickets[available[i]];
                if(winning_ticket < ticket_sum) {
                    idx = available[i];
                    break;
                }
            }

            if(P[idx].start_time == -1) P[idx].start_time = time;
            int start = time;

            int exec_time = (P[idx].remaining > LOTTERY_QUANTUM) ? LOTTERY_QUANTUM : P[idx].remaining;
            P[idx].remaining -= exec_time;
            time += exec_time;
            int end = time;

            if(idx != last)
                ev[ec++] = (Event){P[idx].pid, start, end};
            else
                ev[ec-1].end = end;

            last = idx;

            if(P[idx].remaining == 0) {
                P[idx].completion = time;
                P[idx].turnaround = time - P[idx].arrival;
                P[idx].waiting = P[idx].turnaround - P[idx].burst;
                P[idx].finished = 1; completed++;
                last = -1;
            }
        } else { 
            ev[ec++] = (Event){-1, time, time + 1};
            time++; 
            last = -1; 
            continue; 
        }
    }
    print_table("[Lottery]", P);
    print_gantt("Lottery", ev, ec);
}

void cfs() {
    PCB P[MAX_PROCESSES]; clone(P, proc_orig); reset_metrics(P);
    Event ev[MAX_EVENTS]; int ec=0;
    int time=0, completed=0;
    int last = -1;
    const int MIN_GRANULARITY = 1;   
    const int TARGET_LATENCY = 2*N;    
    const int BASE_WEIGHT = 1000;

    while(completed < N) {
        int idx = -1;
        long long min_vruntime = 1e9;  
        int total_weight = 0;
        int available_count = 0;

        for(int i=0; i<N; i++) {
            if(!P[i].finished && P[i].arrival <= time) {
                total_weight += P[i].weight;
                available_count++;
                if(P[i].vruntime < min_vruntime) {
                    min_vruntime = P[i].vruntime;
                    idx = i;
                }
            }
        }

        if(available_count > 0) {
            if(P[idx].start_time == -1) P[idx].start_time = time;

            int time_slice = (TARGET_LATENCY * P[idx].weight) / total_weight;
            if(time_slice < MIN_GRANULARITY) time_slice = MIN_GRANULARITY;
            
            int exec_time = (time_slice > P[idx].remaining ? P[idx].remaining : time_slice);
            
            int start = time;
            P[idx].remaining -= exec_time;
            time += exec_time;
            int end = time;

            P[idx].vruntime += (exec_time * BASE_WEIGHT) / P[idx].weight;

            if(idx != last)
                ev[ec++] = (Event){P[idx].pid, start, end};
            else
                ev[ec-1].end = end;

            last = idx;

            if(P[idx].remaining == 0) {
                P[idx].completion = time;
                P[idx].turnaround = time - P[idx].arrival;
                P[idx].waiting = P[idx].turnaround - P[idx].burst;
                P[idx].finished = 1; completed++;
                last = -1;
            }
        } else {
            ev[ec++] = (Event){-1, time, time + 1};
            time++;
            last = -1;
            continue;
        }
    }
    print_table("[CFS]", P);
    print_gantt("CFS", ev, ec);
}

int main() {
    printf("================  CPU Scheduling Simulator  ================\n");
    create_processes_random();
    print_table("Original Process Table", proc_orig);

    fcfs();
    sjf_np();
    sjf_p();
    priority_np();
    priority_p();
    rr();
    lottery();
    cfs();  

    return 0;
}
