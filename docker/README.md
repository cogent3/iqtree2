# IQ-TREE Development Docker Container

This folder contains the Docker configuration files for setting up a development environment for the IQ-TREE project.

## Prerequisites

Docker must be installed on your system. You can download Docker Desktop from [here](https://www.docker.com/products/docker-desktop).  Alternatively, on macOS, you can use Colima. See the section below for instructions on installing Colima.

## Installing Colima on macOS

Colima is a tool that provides a lightweight Docker-compatible environment on macOS without the need for a hypervisor. It is an alternative to Docker Desktop.

1. Install Colima using Homebrew:

    ```sh
    brew install colima
    ```

2. Initialize Colima with the Docker runtime:

    ```sh
    colima start --runtime docker
    ```

3. Verify that Colima is running:

    ```sh
    colima status
    ```

Now you can use Docker commands as you normally would, and Colima will handle the container runtime on macOS.

## Docker Image Contents

This container downloads and installs the following dependencies:

- Eigen library: Used for linear algebra.
- TERRAPHAST library: Used for computing the likelihood of a tree.
- Boost libraries: Used for various utility functions and data structures.
- zlib: Compression library.
- libbz2: Library for high-quality data compression.
- liblzma: Compression library.
- TBB (Threading Building Blocks): Used for parallel programming.
- Google Performance Tools: Contains TCMalloc, heap-checker, heap-profiler, and cpu-profiler.
- OpenMPI: Message Passing Interface library for parallel programming.
- libpll: Library for phylogenetic analysis.
- Clang: C language family frontend for LLVM.
- CMake: Cross-platform build system.
- Git: Distributed version control system.
- wget: Network utility to retrieve files from the web.
- plf_nanotimer: A high-resolution, cross-platform timer library for precise time measurement in nanoseconds.
- plf_colony: A container library optimized for frequent insertions and erasures while maintaining cache-friendliness and iterator stability.
- LSD2 (Least Squares Dating 2) is a phylogenetic dating library and tool that estimates divergence times and substitution rates on a phylogenetic tree. 


## Building the Docker Image

To build the Docker image, navigate to the root of the repository and run the following command to build a docker image named `iqtree-dev` using the Dockerfile in the `docker` directory:

`docker build --tag iqtree2-dev -f docker/Dockerfile .`

## Running the Docker Container

To start a Docker container using the image you just built and run iqtree interactively in a terminal session, run the following command:

`docker run -it --rm -v ${PWD}:/iqtree2 iqtree2-dev`

This command does the following:

- `run`: Runs a command in a new container.
- `-v ${PWD}:/iqtree2`: Mounts the current directory on the host to `/iqtree2` in the container.
- `iqtree-dev`: The name of the image to use.

## Running the container in terminal mode

To run the container in terminal mode, add the following argument to the `docker run` command:

`-it -rm`

- `-it`: Allocates a pseudo-TTY connected to the container’s stdin and stdout.
- `--rm`: Automatically removes the container when it exits.

## Running the container in detached mode

Most developers will likely run a container persistently (in detached mode) across multiple terminal sessions. To run the container in detached mode, add the following argument to the `docker run` command:

`-d`

NB: If you have the flag -rm in your docker run command, the container will be removed when it exits.  This is not what you want if you want the container to persist.  So, if you are running the container in detached mode, you should remove the -rm flag from the docker run command.

You will need to find the container ID to subsequently attach to the container. To do this, run the following command:

`docker ps -all`

This will list all running/stopped containers.  Find the container ID for the container you want to attach to.  

If it is stopped, you will need to start it using the following command:

`docker start <container_id>`

Then, run the following command to attach to the container:

`docker attach <container_id>`

## Stopping the Docker Container

In interactive mode simply enter `exit`.

In detached mode use the following command:

`docker stop <container_id>`

## Naming continers

By default containers are given random names, like `flamboyant_badger`.  To explicitly name a container, add the following argument to the `docker run` command:

`--name <container_name>`

when referring to a container using docker command, you can use either the container ID or the container name.

## How to configure your SSH keys into the Docker container

If you intend to contribute to a private fork of the iqtree2 repository, and you have an SSH private key set up on your host machine, and you have added your public SSH key to your GitHub account (https://github.com/settings/keys), then you can mount your SSH private key into the Docker container so that you can push and pull from your fork from inside the container. To do this, add the following argument to the `docker run` command:

### for macOS/Linux

`-v ~/.ssh/PRIVATE_KEY:/root/.ssh/id_rsa`

replace `PRIVATE_KEY` with the name of your private key file.

### for Windows (powershell)

`-v $env:USERPROFILE/.ssh/PRIVATE_KEY:/root/.ssh/id_rsa`

replace `PRIVATE_KEY` with the name of your private key file.

### Checking that you can authenticate with Github.com from inside the container

Run the following command from an interactive terminal session inside the container:

```sh
ssh -T git@github.com
```

This will prompt you to add github.com to your list of known hosts.  Type `yes` to add github.com to your list of known hosts.  You should see the following message:

`Hi USER_NAME! You've successfully authenticated, but GitHub does not provide shell access.` 


## Configuring the container for debugging using Xcode

To configure the container for debugging using Xcode, add the following arguments to the `docker run` command:

`-p 1234:1234 -e DISPLAY=host.docker.internal:0`

- `-p 1234:1234`: Maps port 1234 on the host to port 1234 in the container to allow Xcode debuugging.
- `-e DISPLAY=host.docker.internal:0`: Specifies the X11 display to use will be on the host machine.

## Configuring the container for debugging using VScode

To configure the container for debugging using VScode, add the following arguments to the `docker run` command:

`-p 3000:3000`

- `-p 3000:3000`: Maps port 3000 on the host to port 3000 in the container to allow VS Code debugging.

## Running a file in the container

To run a file in the container, add the command and it's arguments to the end of the `docker run` command:

`docker run iqtree2-dev <command> <arguments>`

eg: To run the iqtree executable in the container:

`docker run iqtree2-dev iqtree -s example.phy`

or to run pytest in the \iqtest2\test directory:

`docker run iqtree2-dev cd \iqtree2\test && pytest`

## Sample docker run command

A docker run command for developing in detached mode from VS Code with the ability to check in code to a private fork of the iqtree2 repository on GitHub would look like this:

`docker run -it -d -v ${PWD}:/iqtree2 -v $env:USERPROFILE/.ssh/github:/root/.ssh/id_rsa -p 3000:3000 --name iqtree2-dev iqtree2-dev /bin/bash`

## Inside the Docker Container

Once inside the Docker container, you will be in the `/iqtree2` directory where you can find the IQ-TREE 2 project files. You can perform git operations, build the project, and run tests as you would in a regular development environment.

To build the project 

```sh
cd /iqtree2
rm -rf build
mkdir -p build
cd build
cmake ..
make
```

## Cleaning Up

To remove the Docker image you created, first find the image ID using:

`docker images`

Then, remove the image using:

`docker rmi <image_id>`

Replace `<IMAGE_ID>` with the ID of the image you want to remove.

## References

- [Docker Documentation](https://docs.docker.com/)
- [Dockerfile Reference](https://docs.docker.com/engine/reference/builder/)
- [Dockerfile Best Practices](https://docs.docker.com/develop/develop-images/dockerfile_best-practices/)