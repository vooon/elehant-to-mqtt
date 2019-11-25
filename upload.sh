#!/bin/bash

set -x

upload() {
	local env=$1
	local file=$2

	rsync $PWD/.pio/build/$env/firmware.bin root@multivax.lan:/raid/srv/fd-ota/$env/$file
}

upload elehant fw.bin
