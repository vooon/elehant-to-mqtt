#!/bin/bash

set -x

upload() {
	local env=$1
	local file=$2

	rsync -P $PWD/.pio/build/$env/firmware.bin root@esp.vehq.ru:/data/fd-ota/$env/$file
}

upload elehant fw.bin
