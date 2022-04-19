#!/bin/bash
echo "Profiling all metrics from the code" 
nvprof --log-file ./logs/all.log  -m all ./AES.x e 128 in.dat key_128_0 
echo "Profiling shared_load_transactions metric and shared_store_transactions metric from the code"
nvprof --log-file ./logs/shared_load_transactions.log  -m shared_load_transactions,shared_store_transactions ./AES.x e 128 in.dat key_128_0