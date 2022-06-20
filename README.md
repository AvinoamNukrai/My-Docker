# My-Docker
A special Docker/Container

Introduction:
In this project I developed a container that run a (somewhat) isolated program inside it in a certain conditions, such as limiting the number of processes inside it. It helps me a lot to get a better understanding of how containers such as docker work under the hood, and what levels of isolation such containers can provide.

The flow of the program: 
1. Create a new process with flags to separate its namespaces from the parents. Allocate a stack for this new process, of size 8192 bytes.
2. From within the new process:
    1. Change the hostname and root directory
    2. Limit the number of processes that can run within the container by generating the       appropriate cgroups
    3. Change the working directory into the new root directory
    4. Mount the new procfs
    5. Run the terminal/new program
3. When shutting down the container - unmount the container’s filesystem from the host.
4. Delete the files directories that was created when I defined the cgroup for the container.


The following diagram shows where the docker is stands in the overall picture:

![docker](https://user-images.githubusercontent.com/64755588/174562432-d5edc1f9-53ba-4de0-b7bc-fe35074de266.png)

For running the docker you'll need some VM machine to work with (I used the universty one). 
Enjoy ;)
