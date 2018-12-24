#!/bin/bash

set -x

upload() {
	local env=$1
	local file=$2

	rsync $PWD/.pioenvs/$env/firmware.bin root@multivax.lan:/raid/srv/fd-ota/$env/$file
}

upload elehant fw.bin
