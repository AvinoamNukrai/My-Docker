// Written by Avinoam Nukrai - EX5 OS course Hebrew U 2022 spring

// Imports
#include <bits/stdc++.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <iostream>
#include <sys/stat.h>
#include "sched.h"
#include "unistd.h"
#include "string.h"
using namespace std;

// Constants
#define FLAGS (CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD)
#define SYSTEM_ERROR(info) "system error: " << (info) << endl;
#define ERROR_SETHOSTNAME "sethostname failed"
#define ERROR_MKDIR "mkdir failed"
#define ERROR_CHROOT "chroot failed"
#define ERROR_MOUNT "mount failed"
#define ERROR_EXECVP "execvp failed"
#define CGROUP_PROCS_PATH "/sys/fs/cgroup/pids/cgroup.procs"
#define PIDSMAX_PATH "/sys/fs/cgroup/pids/pids.max"
#define NOTI_PATH "/sys/fs/cgroup/pids/notify_on_release"
#define INVALID_ARGS_NUM "invalid number of arguments"
#define ALLOC_FAILED "allocation failed!"
#define NUM_OF_FOLDS 3
#define STACK_SIZE 8192


/**
 * This struct represent the args of the child process
 */
typedef struct childProcArgs {
    char** argv;
    size_t progArgsNum; // args number of programCGROUP_PROCS_PATH
} childProcArgs ;


/**
 * This function print error msg to the cerr and exit with 1
 * @param msg - string to print to the cerr
 */
void error_print_exit(char* msg){
    cerr << SYSTEM_ERROR(msg);
    exit(1);
}


/**
 * Thiis function opens a file (and creates it if not exists) and write into it the info
 * @param path - path to the file
 * @param info - number to write to the file
 */
void open_write_to_file(const char *path, int info) {
    ofstream file(path); // open file if exists, else create it
    if (file.is_open()){
        file << info;
        file.close();
    }
}

/**
 * This function checks if some folder exists some given path
 * @param path - path to check
 * 
 * @return int - indicates if found or not
 */
int check_if_folder_exist(const char* path){
    struct stat sf;
    if (stat(path, &sf) != 0){
        return 1;
    }
    return 0;
}

/**
 * This function is the entry_point of the child process that responsible to everything that happens inside the container
 * @param arg - arg of the entry_point
 * @return some int
 */
int child_entry_point(void* arg){
    // first - pars the args from the arg
    auto *procArgs = (childProcArgs*)arg;
    char* newHostname = (char*)procArgs->argv[1];
    char* newRootDir = (char*)procArgs->argv[2];
    const string maxProcNum = procArgs->argv[3];
    char* pathToRun = (char*)procArgs->argv[4];
    size_t argsNum = procArgs->progArgsNum;
    // init the args array for the program we want to run inside the container
    char** argsForRun = (char**)malloc(sizeof(char*) * argsNum);
    argsForRun[0] = pathToRun;
    for (size_t i = 1; i < argsNum - 1; i++){
        argsForRun[i] = (char*)procArgs->argv[4 + i];
    }
    argsForRun[argsNum - 1] = nullptr;
    // 1. Change the hostname and root directory
    if (sethostname(newHostname, strlen(newHostname)) == -1) error_print_exit((char *) ERROR_SETHOSTNAME);
    if (chroot(newRootDir) == -1) error_print_exit((char*) ERROR_CHROOT);
    chdir(newRootDir);
    // 2. Limit the number of processes that can run within the container by generating the appropriate cgroups
    char* paths[NUM_OF_FOLDS] = {(char*)"/sys/fs", (char*)"/sys/fs/cgroup", (char*)"/sys/fs/cgroup/pids"};
    for (int i = 0; i < NUM_OF_FOLDS; i++){
        if (check_if_folder_exist(paths[i])){ // means that the folder is not exists
            if (mkdir(paths[i], 0755) == -1) error_print_exit((char*) ERROR_MKDIR);
        }
    }
    open_write_to_file(CGROUP_PROCS_PATH, getpid());
    open_write_to_file(PIDSMAX_PATH, stoi(maxProcNum));
    open_write_to_file(NOTI_PATH, 1);
    // 4. Mount the new procfs
    if (mount("proc", "/proc", "proc", 0, 0) == -1) error_print_exit((char*) ERROR_MOUNT);
    // 5. Run the terminal/new program
    if (execvp(pathToRun, argsForRun) == -1) error_print_exit((char*) ERROR_EXECVP);
    // free alloc
    for (size_t i = 0; i < argsNum; ++i) {  // TODO: to free all allocations!
        free(&argsForRun[i]);
        argsForRun[i] = nullptr;
    }
    free(argsForRun);
    return 0;
}


/**
 * This func delete all the files and folders we've created during the running of the container
 * @param argv - the arguments of the program for extract the
 */
void delete_all_files_folders(char* originalRoot) {
    char* newRoot = (char*)malloc(sizeof(char) * (strlen(originalRoot) + strlen(CGROUP_PROCS_PATH)));
    strcpy(newRoot, originalRoot);
    char* paths_files[NUM_OF_FOLDS] = {(char*)"/sys/fs/cgroup/pids/cgroup.procs", (char*)"/sys/fs/cgroup/pids/pids.max", (char*)"/sys/fs/cgroup/pids/notify_on_release"};
    char* paths_folders[NUM_OF_FOLDS] = {(char*)"/sys/fs/cgroup/pids", (char*)"/sys/fs/cgroup", (char*)"/sys/fs"};
    for (int i = 0; i < NUM_OF_FOLDS; ++i) {
        remove(strcat(newRoot, paths_files[i])); // for files
        strcpy(newRoot, originalRoot);
        rmdir(strcat(newRoot, paths_folders[i])); // for folders
        strcpy(newRoot, originalRoot);
    }
    free(newRoot);
}


/**
 * This is the main func of the program, acting as the main/father process
 * @param argc - arguments
 * @param argv - num of args in argv
 * @return int
 */
int main(int argc, char* argv[]){
    char* newRoot = (char*)argv[2];
    if (argc < 5){
        cerr << SYSTEM_ERROR(INVALID_ARGS_NUM)
        return EXIT_FAILURE;
    }
    // creates the child process
    void* stack = malloc(STACK_SIZE);
    if (!stack){
        cerr << SYSTEM_ERROR(ALLOC_FAILED)
        return EXIT_FAILURE;
    }
    auto* childProArgs =(childProcArgs*) malloc(sizeof(childProcArgs));
    childProArgs->argv = argv;
    childProArgs->progArgsNum = argc - 3;
    clone(child_entry_point,(char*)stack + STACK_SIZE,FLAGS, (void *)childProArgs);
    wait(NULL);
    umount(strcat(newRoot, "/proc"));
    delete_all_files_folders(argv[2]);
    free(childProArgs);
    free(stack);
    return 0;
}