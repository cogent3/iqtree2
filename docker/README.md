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

`docker build -t iqtree2-dev -f docker/Dockerfile .`

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

## How to configure your SSH keys into the Docker container

If you intend to contribute to a private fork of the iqtree2 repository, and you have an SSH private key set up on your host machine, and you have added your public SSH key to your GitHub account (https://github.com/settings/keys), then you can copy your SSH private key into the Docker container so that you can push and pull from your fork from inside the container. To do this, add the following argument to the `docker run` command:

### for macOS/Linux
`-e SSH_PRIVATE_KEY_FILE_NAME=~/.ssh/id_rsa`

You can change the path to your private key file if you are using a different path.  For example, if your private key is located at ~/.ssh/my_private_key, you can add the following argument to the `docker run` command:

`-e SSH_PRIVATE_KEY_FILE_NAME=~/.ssh/my_private_key`
### for Windows (powershell)

`-e SSH_PRIVATE_KEY_FILE_NAME=$env:USERPROFILE/.ssh/id_rsa`

The file id_rsa will be copied from the path specified by the SSH_PRIVATE_KEY_FILE_NAME environment variable into the Docker container at the path /root/.ssh/id_rsa.  

## Configuring the container for debugging using Xcode

To configure the container for debugging using Xcode, add the following arguments to the `docker run` command:

`-p 1234:1234 -e DISPLAY=host.docker.internal:0`

...
- `-p 1234:1234`: Maps port 1234 on the host to port 1234 in the container to allow Xcode debuugging.
- `-e DISPLAY=host.docker.internal:0`: Specifies the X11 display to use will be on the host machine.

## Configuring the container for debugging using VScode

To configure the container for debugging using VScode, add the following arguments to the `docker run` command:

`-p 3000:3000`

...
- `-p 3000:3000`: Maps port 3000 on the host to port 3000 in the container to allow VS Code debugging.

## Inside the Docker Container

Once inside the Docker container, you will be in the `/iqtree2` directory where you can find the IQ-TREE 2 project files. You can perform git operations, build the project, and run tests as you would in a regular development environment.

The executable built during the construction of the image will be in the /iqtree2/build directory.  Rebuilding that executable can be done by running the following commands:

```sh
cd /iqtree2/build
cmake ..
make
```

## Exiting the Docker Container

To exit the Docker container, simply enter `exit`.

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