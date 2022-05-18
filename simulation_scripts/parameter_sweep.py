#!/usr/bin/env python3
#Run using python3

import sys
import subprocess
import queue
import time

# Configuration
coyote_bin_path="../SpikeModel/build/spike_model"
apps_path="../apps/"
#apps=["mt-matmul-vec/matmul", "axpy/axpy", "spmv-vec/spmv_consph", "spmv-vec/spmv_cop20k", "spmv-vec/spmv_pdb1HYS", "spmv-vec/spmv_synthetic"]
apps=["mt-matmul-vec/matmul"]
cores=1 # Change to the number of procs to use
polling_time=1 # seconds

# Params validation
if len(sys.argv)<5 or len(sys.argv)>6:
    print(len(sys.argv))
    print("Usage: python3 parameter_sweep.py config_file parameter_name value_list(brace-enclosed, comma-separated. Example '[true,false]') output_path [(Optional)enable_trace]")
    sys.exit()

# Trace?
save_trace=False
if len(sys.argv)==6:
    if sys.argv[5]!="enable_trace":
        print("Unknown parameter "+sys.argv[5])
        sys.exit()
    else:
        save_trace=True
        cores=1

results_path=sys.argv[4]
config_file=sys.argv[1]
param=sys.argv[2]

print("Sweeping param "+param)
print("Apps: "+str(apps)+"\n")

# Get values for sweeping
values=sys.argv[3][1:-1].split(',')
# Generate out path
out_path=results_path+"/results_"+param.split(".")[-1]+"/"
# Create variable results folder
subprocess.run(["mkdir", "-p", out_path]) 

# Enqueue (FIFO) commands and files
commands = queue.Queue()
outs = queue.Queue()
errs = queue.Queue()
for app in apps:
    app_name=app.split("/")[-1]
    # Create app results folder
    subprocess.run(["mkdir", "-p", out_path+app_name])
    # Copy configuration file to app results folder
    subprocess.run(["cp", config_file, out_path+app_name])
    for v in values:
        command=["time", coyote_bin_path, "-c",  config_file, "-p", "meta.params.cmd", apps_path+app, "-p", param, v]
        if save_trace:
            command.append("-p")
            command.append("meta.params.trace")
            command.append("true")
        commands.put(command)
        outs.put(out_path+app_name+"/out_"+v)
        errs.put(out_path+app_name+"/err_"+v)

# Use multiple cores
avail_cores = cores
procs = []
files = []
while not commands.empty() or procs:
    # Execute simulations
    while not commands.empty() and avail_cores > 0:
        avail_cores -= 1
        command = commands.get()
        out_f = outs.get()
        err_f = errs.get()
        out = open(out_f, "w")
        err = open(err_f, "w")
        app_name = command[6].split("/")[-1]
        if save_trace:
            value = command[-4]
        else:
            value = command[-1]
        print("\tExecuting: " + app_name + " with value " + value)
        proc = subprocess.Popen(command, stdout=out, stderr=err, stdin=subprocess.PIPE)
        files.append(out)
        files.append(err)
        procs.append(proc)
    # Polling
    for p in procs:
        if p.poll() is not None:
            procs.remove(p)
            avail_cores += 1
            if save_trace: # Save trace runs only with 1 core in parallel
                subprocess.run(["mv", "trace", out_path+app_name+"/trace_"+value])
        else:
            time.sleep(polling_time)

# Close all opened files
for file in files:
    file.close()
