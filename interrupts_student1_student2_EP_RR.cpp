/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_student1_student2.hpp>

//sorts the PID
void priority_sort_first(std::vector<PCB> &queue) {
    std::sort( 
                queue.begin(), queue.end(), 
                []( const PCB &first, const PCB &second ){
                    return (first.PID < second.PID);
                }
            );
    
}

//sorts the PID in reverse
void priority_sort_last(std::vector<PCB> &queue) {
    std::sort( 
                queue.begin(), queue.end(), 
                []( const PCB &first, const PCB &second ){
                    return (first.PID > second.PID);
                }
            );
    
}


std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    const unsigned int TIME_QUANT = 100;
    unsigned int quantum_remaining = 0;
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //Sorting processes by PID 
    priority_sort_first(list_processes);

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

 

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time <= current_time && process.state == NOT_ASSIGNED) {
                //update the state to a new state because it has entered the queue
                process.state = NEW;
                
                if(assign_memory(process)) {

                    process.state = READY;  //Set the process state to READY
                    ready_queue.push_back(process); //Add the process to the ready queue

                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                }
                job_list.push_back(process); //Add it to the list of processes
                
            } else if(process.arrival_time <= current_time && process.state == NEW) {
                //try to assign NEW state memeory
                if(assign_memory(process)) {
                    process.state = READY;  //Set the process state to READY
                    ready_queue.push_back(process); //Add the process to the ready queue
                    sync_queue(job_list,process); // updates the process in job list
                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);

                }
            }
        }


        //========================DEBUG PRINTS============================//
        // debug prints
        std::cout << "current Time:" << current_time << std::endl;
        //std::cout << "list_processes:\n";
        //std::cout << print_PCB(list_processes) << std::endl;
        //std::cout << "ready_queue:\n";
        //std::cout << print_PCB(ready_queue) << std::endl;
        std::cout << "wait_queue:\n";
        std::cout << print_PCB(wait_queue) << std::endl;
        std::cout << "job_list:\n";
        std::cout << print_PCB(job_list) << std::endl;
        //=====================END DEBUG PRINTS============================//


        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
       
        
        
        for (auto iter = wait_queue.begin(); iter != wait_queue.end(); ) {
            iter->io_remaining--;
            if(iter->io_remaining < 1) {
                //change the state thats has no more time to wait to READY
                iter->state = READY;
                ready_queue.push_back(*iter);
                sync_queue(job_list,*iter);
                execution_status += print_exec_status(current_time, iter->PID, WAITING, READY);
                
                //Erase the state from the wait queue after you do everything you need to with it. 
                iter = wait_queue.erase(iter);
            } else {
                ++iter;
            }
        }
        /////////////////////////////////////////////////////////////////


        //////////////////////////SCHEDULER//////////////////////////////
         if(running.state == RUNNING) {
            running.remaining_time--;
            quantum_remaining--; //decrese quantum time every millisecond 

            // check if the process is finished so you reset
            if(running.remaining_time == 0){
                //set a process that has no more time left to TERMINATED
                terminate_process(running,job_list);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
                idle_CPU(running);
                quantum_remaining = 0; //reset the quantum_remaining
            } else if(quantum_remaining == 0) { //check if the quantum is expired
            
                //move the state from RUNNING to READY beacuse its run out of its quantumn time 
                running.state = READY;
                ready_queue.push_back(running);
                sync_queue(job_list,running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                idle_CPU(running);
                quantum_remaining = 0; //reset the quantum_remaining

            }

            if(running.state == RUNNING && !ready_queue.empty()){
                //sort the ready queue by the largest PID to smallest PID
                priority_sort_last(ready_queue);

                //if the prority of the queues smallest > current prority
                if(ready_queue.back().PID < running.PID) {
                    
                    //change the current state from running to ready and add it to the ready_queue
                    running.state = READY;
                    ready_queue.push_back(running);
                    sync_queue(job_list, running);
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                    idle_CPU(running);
                    quantum_remaining = TIME_QUANT; //reset quantum time

                    
                }
            }
            
            //if the time passed % I/O Frequency, then you know that an I/O occurs
            if(running.io_freq > 0 && (running.processing_time - running.remaining_time) > 0 && ((running.processing_time -running.remaining_time) % running.io_freq) == 0) {
                //std::cout << "I/O has occured " << running.PID << "\nCurrent Time: " << current_time << std::endl;
                
                //change current program to the WAITING and add it to the wait_queue
                running.state = WAITING;
                running.io_remaining = running.io_duration;
                wait_queue.push_back(running);
                sync_queue(job_list,running);
                execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
                idle_CPU(running);
            }
        } 

        if(running.state == NOT_ASSIGNED && !ready_queue.empty()) {

            //get the item that has the smalles PID and set that as running
            priority_sort_last(ready_queue);
            run_process(running,job_list,ready_queue,current_time);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
            quantum_remaining = TIME_QUANT;
        }

        /////////////////////////////////////////////////////////////////

        current_time++;
    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
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
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}