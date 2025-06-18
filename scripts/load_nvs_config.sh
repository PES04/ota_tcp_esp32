#!/bin/bash

while getopts ":p:s:o:" opt; do
  case $opt in
    p) port="$OPTARG" ;;
    s) size="$OPTARG" ;;
    o) offset="$OPTARG" ;;
    *) exit 1 ;;
  esac
done

if [ -z "$port" ] || [ -z "$size" ] || [ -z "$offset" ]; then
  exit 1
fi

if [ -z "$IDF_PATH" ]; then
  echo "IDF environment is not configured"
  exit 1
fi

nvsConfigDir="./nvs_config"
csv_file="$nvsConfigDir/nvs_config.csv"
bin_file="$nvsConfigDir/nvs.bin"

nvs_gen_script="$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py"
python_cmd="python"

if [ ! -f "$csv_file" ]; then
  echo "Nvs_config not found"
  exit 1
fi

gen_cmd="$python_cmd \"$nvs_gen_script\" generate \"$csv_file\" \"$bin_file\" $size"
eval $gen_cmd
if [ $? -ne 0 ]; then
  exit 1
fi

flash_cmd="$python_cmd -m esptool --port $port write_flash $offset \"$bin_file\""
eval $flash_cmd
