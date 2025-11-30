/**
 * @file interrupts.cpp
 * @author Radhe Patel & Avnita Ala
 * @brief main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_student1_student2.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

void logMemoryStatus(std::ostream& os) {
    os << "Memory Partition Status:\n";
    os << "Partition Number | Size | Occupied By (PID)\n";
    os << "-------------------------------------------\n";
    for (const auto& partition : memory_paritions) {
        os << std::setw(16) << partition.partition_number << " | "
           << std::setw(4) << partition.size << " | "
           << std::setw(16) << (partition.occupied == -1 ? "Free" : std::to_string(partition.occupied)) << "\n";
    }
    os << "-------------------------------------------\n\n";
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
    unsigned int quantum = 100; // 100 ms time slice

    //make the output table (the header row)
    execution_status = print_exec_header();

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
        // not required because there is no I/O in Round Robin Scheduling
        /////////////////////////////////////////////////////////////////

        // 3) Schedule processes from the ready queue
        //////////////////////////SCHEDULER//////////////////////////////
        if (!ready_queue.empty()) {
            PCB& current = ready_queue.front();
            current.state = RUNNING;
            execution_status += print_exec_status(current_time, current.PID, READY, RUNNING);

            // Run up to quantum (100), or until process completion
            unsigned int run_time = std::min(current.remaining_time, quantum);
            current.remaining_time -= run_time;
            current_time += run_time;

            if (current.remaining_time == 0) {
                current.state = TERMINATED;
                execution_status += print_exec_status(current_time, current.PID, RUNNING, TERMINATED);
                free_memory(current);
                logMemoryStatus(memorystream);
                
                for (auto &p : job_list) {
                    if (p.PID == current.PID) {
                        p.state = TERMINATED;
                        break;
                    }
                }

                ready_queue.erase(ready_queue.begin());
            } 
            
            else {
                current.state = READY;
                execution_status += print_exec_status(current_time, current.PID, RUNNING, READY);
                PCB next_process = current;
                ready_queue.erase(ready_queue.begin());
                ready_queue.push_back(next_process);
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