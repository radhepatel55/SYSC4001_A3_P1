/**
 * @file interrupts.cpp
 * @author Radhe Patel & Avnita Ala
 * @brief main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_101262847_101301514.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}


std::tuple<std::string, std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;
    std::ostringstream memorystream;
    const unsigned int quantum = 100; // 100 ms time slice

    //make the output table (the header row)
    execution_status = print_exec_header();

    // Initialize IO tracking 
    const int MAX_PID = 100;
    unsigned int time_to_next_io[MAX_PID];
    unsigned int io_done_time[MAX_PID];
    bool has_io[MAX_PID];

    for (int i = 0; i < MAX_PID; ++i) {
        time_to_next_io[i] = 0;
        io_done_time[i] = 0;
        has_io[i] = false;
    }

    for (auto &p : list_processes) {
        if (p.PID < MAX_PID && p.io_freq > 0) {
            time_to_next_io[p.PID] = p.io_freq;
            has_io[p.PID] = true;
        }
    }

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {
                if (assign_memory(process)){
                    process.state = READY;
                    ready_queue.push_back(process);
                    job_list.push_back(process);
                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                    logMemoryStatus(memorystream);
                }
            }
        }

        // 2) Manage the wait queue

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        for (int i = 0; i < (int)wait_queue.size(); ++i) {
            PCB &w = wait_queue[i];
            if (w.PID < MAX_PID && io_done_time[w.PID] <= current_time) {
                states old_s = w.state;
                w.state = READY;
                execution_status += print_exec_status(current_time, w.PID, old_s, READY);
                ready_queue.push_back(w);

                for (auto &j : job_list) {
                    if (j.PID == w.PID) {
                        j.state = READY;
                        break;
                    }
                }

                wait_queue.erase(wait_queue.begin() + i);
                --i;
            }
        }
        /////////////////////////////////////////////////////////////////

        // 3) Schedule processes from the ready queue

        //////////////////////////SCHEDULER//////////////////////////////
        if (running.PID == -1 && !ready_queue.empty()) {
            ExternalPriority(ready_queue); 
            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());

            states old_state = running.state;
            running.state = RUNNING;
            if (running.start_time == 0) {
                running.start_time = current_time;
            }
            execution_status += print_exec_status(current_time, running.PID, old_state, RUNNING);

            if (running.PID < MAX_PID && has_io[running.PID] && time_to_next_io[running.PID] == 0) {
                time_to_next_io[running.PID] = running.io_freq;
            }
        }

        // 4) Execute one quantum (or less) of CPU if something is running
        if (running.PID != -1) {
            unsigned int run_time = std::min(running.remaining_time, quantum);

            if (running.PID < MAX_PID && has_io[running.PID]) {
                unsigned int &tio = time_to_next_io[running.PID];
                if (tio < run_time) {
                    run_time = tio;  
                }
            }

            running.remaining_time -= run_time;
            if (running.PID < MAX_PID && has_io[running.PID]) {
                unsigned int &tio = time_to_next_io[running.PID];
                if (run_time >= tio) {
                    tio = 0;
                } else {
                    tio -= run_time;
                }
            }

            current_time += run_time;

            // a) Finished CPU
            if (running.remaining_time == 0) {
                states old_s = running.state;
                running.state = TERMINATED;
                execution_status += print_exec_status(current_time, running.PID, old_s, TERMINATED);
                free_memory(running);
                logMemoryStatus(memorystream);

                for (auto &p : job_list) {
                    if (p.PID == running.PID) {
                        p.state = TERMINATED;
                        break;
                    }
                }

                idle_CPU(running);
            }

            // b) Need I/O now
            else if (running.PID < MAX_PID && has_io[running.PID] && time_to_next_io[running.PID] == 0) {
                states old_s = running.state;
                running.state = WAITING;
                execution_status += print_exec_status(current_time, running.PID, old_s, WAITING);

                io_done_time[running.PID] = current_time + running.io_duration;
                time_to_next_io[running.PID] = running.io_freq;

                wait_queue.push_back(running);

                for (auto &p : job_list) {
                    if (p.PID == running.PID) {
                        p.state = WAITING;
                        break;
                    }
                }

                idle_CPU(running);
            }

            // c) Quantum expired, still CPU left, no I/O
            else {
                states old_s = running.state;
                running.state = READY;
                execution_status += print_exec_status(current_time, running.PID, old_s, READY);

                ready_queue.push_back(running);

                for (auto &p : job_list) {
                    if (p.PID == running.PID) {
                        p.state = READY;
                        break;
                    }
                }

                idle_CPU(running);
            }
        } else {
            current_time++;
        }
    }
    /////////////////////////////////////////////////////////////////
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status, memorystream.str());
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec, memorystatus] = run_simulation(list_process);

    write_output(exec, "execution.txt");
    write_output(memorystatus, "memorylog.txt");

    return 0;
}