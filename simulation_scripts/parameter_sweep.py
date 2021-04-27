#Run using python3

import sys
import subprocess

coyote_bin_path="../SpikeModel/build/spike_model"
apps_path="../apps/"
apps=["mt-matmul-vec/matmul"]

if len(sys.argv)<5 and len(sys.argv)<6:
    print(len(sys.argv))
    print("Usage: python3 parameter_sweep.py config_file parameter_name value_list(brace-enclosed, comma-separated) [(Optional)enable_trace]")
    sys.exit()

save_trace=False
if len(sys.argv)==6:
    if sys.argv[5]!="enable_trace":
        print("Unknown parameter "+sys.argv[4])
        sys.exit()
    else:
        save_trace=True

config_file=sys.argv[2]
param=sys.argv[3]
values=sys.argv[4][1:-1].split(',')

out_path="./results_"+param.split(".")[-1]+"/"

subprocess.run(["mkdir", out_path])

for app in apps:
    app_name=app.split("/")[-1]
    subprocess.run(["mkdir", out_path+app_name])
    for v in values:
        command=[coyote_bin_path, "-c",  config_file, "-p", "meta.params.cmd", apps_path+app, "-p", param, v]
        if save_trace:
            command.append("-p")
            command.append("meta.params.trace")
            command.append("true")
        out = open(out_path+app_name+"/out_"+v, "w")
        subprocess.run(command, stdout=out)
        if save_trace:
            subprocess.run(["mv", "trace", out_path+app_name+"/trace_"+v])

        out.close()

