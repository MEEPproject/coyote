#/bin/bash!
if [ "$#" -gt 2 ] || [ "$#" -lt 1 ]; then
    echo "Usage: #cores [opt]Mat_dim(power of 2)"
    exit 2
fi
sh compile.sh $1 $2
spike --ic=1:1:8 --dc=8:8:64 --isa=RV64IMAFDCV -p$1 matmul
