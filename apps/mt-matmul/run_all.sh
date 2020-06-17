#/bin/bash!

if [ "$#" -gt 2 ] || [ "$#" -lt 1 ]; then
    echo "Usage: #cores [opt]Mat_dim(power of 2)"
    exit 2
fi

if [ "$#" -eq 2 ]; then
    perl matmul_gendata.pl --size $2 > dataset.h
fi

sed -i '/li a7/c li a7, '$1 ./my_crt.S
sh compile.sh
/home/bscuser/local/riscvv08/gnu/bin/spike --ic=1:1:8 --dc=8:8:64 --isa=RV64IMAFDCV -p$1 matmul
