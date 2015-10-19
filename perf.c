/*
libbp.so,
a library to count accesses to a variable in Score-P and VampirTrace
Copyright (C) 2015 TU Dresden, ZIH
 */

#define PACKAGE 1
#define PACKAGE_VERSION 1

#ifdef VT
#include <vampirtrace/vt_plugin_cntr.h>
#else
#ifdef SCOREP
#include <scorep/SCOREP_MetricPlugins.h>
#else
#error "You need Score-P or VampirTrace to compile this plugin"
#endif /* SCOREP*/
#endif /* VT */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <bfd.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <dlfcn.h>


#include <linux/perf_event.h>

#include <linux/hw_breakpoint.h>


/* init and fini do not do anything */
/* This is intended! */
int32_t init() {
    return 0;
}
void fini() {
    /* we do not close perfs file descriptors */
    /* as we do not store this information */
}

/* This function writes the attr definitions for a given event name
 * If the event has not been found, attr->type is PERF_TYPE_MAX
 * */
void build_perf_attr(struct perf_event_attr * attr, const char * complex_name) {

    int  i;
    void * address;
    memset( attr, 0, sizeof( struct perf_event_attr ) );

    // split complex_name into action (read, write, execute) and name
    // encoding is <action>_<name>
    // <action> can be r, rw, w, and x
    attr->type    = PERF_TYPE_MAX;
    char *action,*name;
    action = strtok((char*)complex_name,"_");
    name = strtok(NULL,"\0");

    // set attr->bp_type = action (e.g., HW_BREAKPOINT_R);
    switch(action[0]) {
    case 'r':
        switch(action[1]) {
        case 'w':
            attr->bp_type = HW_BREAKPOINT_RW;
        default:
            attr->bp_type = HW_BREAKPOINT_R;
        }
    case 'w':
        attr->bp_type = HW_BREAKPOINT_W;
    case 'x':
        attr->bp_type = HW_BREAKPOINT_X;
    case ' ':
        attr->bp_type = HW_BREAKPOINT_EMPTY;
    default:
        attr->bp_type = HW_BREAKPOINT_INVALID;
    }


    //try to find variable via dlsym */
    address = dlsym("RTLD_DEFAULT", name);
    if (address != NULL) {
        attr->type = PERF_TYPE_BREAKPOINT;
        attr->bp_addr = (uint64_t)address;
        attr->bp_len = HW_BREAKPOINT_LEN_8;

        //if type is known:
        //           sizeof(type), HW_BREAKPOINT_LEN_8;
    }
    else {
// when name found via dlsym or bfd:

        /* try to find variable via bfd */
        unsigned int storage_needed, number;
        bfd *bfd_variable;
    bfd_init(); //Initialize Magic
        bfd_variable = bfd_openr("/proc/self/exe",NULL);
        bfd_get_arch_size(bfd_variable);
        bfd_find_target(name,bfd_variable);
        bfd_check_format(bfd_variable,bfd_object);
        storage_needed = bfd_get_symtab_upper_bound(bfd_variable);
        asymbol ** table;
        table=malloc(storage_needed);
        number = bfd_canonicalize_symtab(bfd_variable,table);
        for (i=0; i<number; i++) {
            if (strstr(table[i]->name,name)!=0) {
                attr->type = PERF_TYPE_BREAKPOINT;
                attr->bp_addr = (uint64_t)bfd_asymbol_base(table[i]);
                attr->bp_len = HW_BREAKPOINT_LEN_8;
            }
        }
        free(table);
        storage_needed = bfd_get_dynamic_symtab_upper_bound(bfd_variable);
        table= malloc(storage_needed);
        number = bfd_canonicalize_dynamic_symtab(bfd_variable,table);
        for (i=0; i<number; i++) {
            if (strstr(table[i]->name,name)!=0) {
                attr->type = PERF_TYPE_BREAKPOINT;
                attr->bp_addr = (uint64_t)bfd_asymbol_base(table[i]);
                attr->bp_len = HW_BREAKPOINT_LEN_8;
            }
        }
        bfd_close(bfd_variable);
        free(table);
        attr->type = PERF_TYPE_BREAKPOINT;
        // if still not found dont change attr
    }
}
/* registers perf event */
int32_t add_counter(char * event_name) {
    int fd;
    struct perf_event_attr attr;

    build_perf_attr(&attr, event_name);
    /* wrong metric */
    if (attr.type==PERF_TYPE_MAX) {
        fprintf(stderr, "PERF metric not recognized: %s", event_name );
        return -1;
    }
    fd = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
    if (fd<=0) {
        fprintf(stderr, "Unable to open counter \"%s\". Aborting.\n",event_name);
        return -1;
    }
    return fd;
}

/* reads value */
uint64_t get_value(int fd) {
    uint64_t count=0;
    size_t res=read(fd, &count, sizeof(uint64_t));
    if (res!=sizeof(uint64_t))
        return !0;
    return count;
}

#ifdef VT

vt_plugin_cntr_metric_info * get_event_info(char * event_name) {
    vt_plugin_cntr_metric_info * return_values;

    struct perf_event_attr attr;
    build_perf_attr(&attr, event_name);
    /* wrong metric */
    if (attr.type==PERF_TYPE_MAX) {
        fprintf(stderr, "PERF metric not recognized: %s", event_name );
        return NULL;
    }
    return_values=
        malloc(2 * sizeof(vt_plugin_cntr_metric_info) );
    if (return_values==NULL) {
        fprintf(stderr, "VampirTrace Perf Plugin: failed to allocate memory for passing information to VT.\n");
        return NULL;
    }
    return_values[0].name=strdup(event_name);
    return_values[0].unit=NULL;
    return_values[0].cntr_property=VT_PLUGIN_CNTR_LAST|VT_PLUGIN_CNTR_START|
                                   VT_PLUGIN_CNTR_UNSIGNED;
    return_values[1].name=NULL;
    return return_values;
}


/**
 * This function get called to give some informations about the plugin to VT
 */
vt_plugin_cntr_info get_info() {
    vt_plugin_cntr_info info;
    memset(&info,0,sizeof(vt_plugin_cntr_info));
    info.init              = init;
    info.add_counter       = add_counter;
    info.vt_plugin_cntr_version = VT_PLUGIN_CNTR_VERSION;
    info.run_per           = VT_PLUGIN_CNTR_PER_THREAD;
    info.synch             = VT_PLUGIN_CNTR_SYNCH;
    info.get_event_info    = get_event_info;
    info.get_current_value = get_value;
    info.finalize          = fini;
    return info;
}
#elif SCOREP

SCOREP_Metric_Plugin_MetricProperties * get_event_info(char * event_name)
{
    SCOREP_Metric_Plugin_MetricProperties * return_values;

    struct perf_event_attr attr;
    build_perf_attr(&attr, event_name);
    /* wrong metric */
    if (attr.type==PERF_TYPE_MAX) {
        fprintf(stderr, "PERF metric not recognized: %s", event_name );
        return NULL;
    }
    return_values=
        malloc(2 * sizeof(SCOREP_Metric_Plugin_MetricProperties) );
    if (return_values==NULL) {
        fprintf(stderr, "Score-P Perf Plugin: failed to allocate memory for passing information to Score-P.\n");
        return NULL;
    }
    return_values[0].name        = strdup(event_name);
    return_values[0].unit        = NULL;
    return_values[0].description = NULL;
    return_values[0].mode        = SCOREP_METRIC_MODE_ACCUMULATED_START;
    return_values[0].value_type  = SCOREP_METRIC_VALUE_UINT64;
    return_values[0].base        = SCOREP_METRIC_BASE_DECIMAL;
    return_values[0].exponent    = 0;
    return_values[1].name=NULL;
    return return_values;
}

bool get_optional_value( int32_t   id,
                         uint64_t* value ) {
    *value=get_value(id);
    return true;
}

/**
 * This function get called to give some informations about the plugin to scorep
 */
SCOREP_METRIC_PLUGIN_ENTRY( PerfScoreP )
{
    /* Initialize info data (with zero) */
    SCOREP_Metric_Plugin_Info info;
    memset( &info, 0, sizeof( SCOREP_Metric_Plugin_Info ) );

    /* Set up the structure */
    info.plugin_version               = SCOREP_METRIC_PLUGIN_VERSION;
    info.run_per                      = SCOREP_METRIC_PER_THREAD;
    info.sync                         = SCOREP_METRIC_SYNC;
    info.initialize                   = init;
    info.finalize                     = fini;
    info.get_event_info               = get_event_info;
    info.add_counter                  = add_counter;
    info.get_current_value            = get_value;
    info.get_optional_value           = get_optional_value;

    return info;
}
#endif /* SCOREP */
