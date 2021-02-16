#!/usr/bin/python

import sys

vector_instruction_dict={}

for i in range(1,len(sys.argv)):
    tmp_vector_instruction_set=set()
    with open(sys.argv[i]) as f:
        for line in f:
            tokens=line.split()
            if len(tokens)>=4 and tokens[2][0]=='v':
                tmp_vector_instruction_set.add(tokens[2])
    print("File "+sys.argv[i]+" vector instruction summary:")
    for v in tmp_vector_instruction_set:
        print("\t"+v)
        if v not in vector_instruction_dict.keys():
            vector_instruction_dict[v]=[sys.argv[i]]
        else:
            vector_instruction_dict[v].append(sys.argv[i])

print("Overall vector instruction summary:")
for v in vector_instruction_dict.keys():
    print("\t"+v)

print("Overall vector instruction summary (Wiki table format):")

app_name_pos_dict={}
apps_header=""
for i in range(1,len(sys.argv)):
    app_name_pos_dict[sys.argv[i]]=i
    apps_header+=" !! "+sys.argv[i]

print('{| class="wikitable sortable"')
print('|+ Target vector instructions')
print('|-')
print('! Instruction'+apps_header)
print('|- style="background-color:#E0FFFF" ||')
dark_bg=False
for v in vector_instruction_dict.keys():
    pos_to_tick=[]
    for app in vector_instruction_dict[v]:
        pos_to_tick.append(app_name_pos_dict[app])

    inst_usage_string=" "
    for i in range(1,len(sys.argv)):
        inst_usage_string+='|| '
        if i in pos_to_tick:
            inst_usage_string+='X '

    print('| '+v+inst_usage_string)
    close_row_str='|-'
    if dark_bg:
        close_row_str+=' style="background-color:#E0FFFF" |'
    print(close_row_str)

    dark_bg=not dark_bg

print('|}')
