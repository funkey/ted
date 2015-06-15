Tolerant Edit Distance
======================

Dependencies:
-------------

  * Gurobi

    Get the gurobi solver from http://www.gurobi.com. Academic licenses are free.

  * CMake, Git, GCC, boost

    On Ubuntu 14.04, get the build tools via:

      `sudo apt-get install cmake git build-essential libboost-dev`

Get Source:
-----------

  ```
  git clone https://github.com/funkey/ted
  cd ted
  git submodule update --init
  ```

Configure:
----------

  ```
  mkdir build
  cd build
  cmake ..
  ```

  Set cmake variable Gurobi_ROOT_DIR (or the environment variable
  GUROBI_ROOT_DIR before you run cmake) to the Gurobi subdirectory containing
  the /lib and /bin directories.

Compile:
--------

  ```
  make
  ```
