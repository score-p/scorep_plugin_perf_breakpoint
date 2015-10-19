#Perf Breakpoint Plugin

This README topics the installation and usage of the Score-P (and VampirTrace) perf plugin counter.

##Compilation and Installation

###Prerequisites

To compile this plugin, you need:

* C compiler

* CMake

* Score-P (or VampirTrace) installation

* a recent Linux kernel (`2.6.32+`) with activated tracing and the kernel headers

###Building

1. Create a build directory

        mkdir build
        cd build

2. Invoke CMake

    If cmake does not find Score-P, resp. VampirTrace, specify the directory if it is not in the
    default path with `-DSCOREP_DIR=<PATH>`, resp. `-DVT_INC=<path>`.

    The plugin will use alternatively the environment variables `SCOREP_DIR` and `VT_DIR`.

        cmake .. -DSCOREP_DIR=/opt/scorep

3. Invoke make

        make

    The resulting files will be named libbp.so (Score-P) and libbpVt.so (VampirTrace).

4. Copy it to a location listed in `LD_LIBRARY_PATH` or add current path to `LD_LIBRARY_PATH` with

        export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`

##Usage

To add a kernel event counter to your trace, you have to specify the environment variable
`SCOREP_METRIC_PLUGINS`, resp. `VT_PLUGIN_CNTR_METRIC`.

`SCOREP_METRIC_PLUGINS`/`VT_PLUGIN_CNTR_METRIC` specifies the software events that shall be recorded
when tracing an application. You can add the following metrics:

* `r_<NAME>`

* `w_<NAME>`

* `rw_<NAME>`

* `x_<NAME>`

Where `<NAME>` refers to the name of a variable or function, `r` refers to reading acces, `w` to
write, `rw` to read and write, and `x` to execute.

E.g. for the ScoreP plugin:

    export SCOREP_METRIC_PLUGINS="bp"
    export SCOREP_BP_METRICS="x_Gomp_Init"

or for the VampirTrace plugin:

    export VT_PLUGIN_CNTR_METRICS="bpVT_x_Gomp_Init"

###If anything fails

1. Check whether the plugin library can be loaded from the `LD_LIBRARY_PATH`.

2. Write a mail to the author.

##Author

Robert Schoene (robert.schoene at tu-dresden dot de)
