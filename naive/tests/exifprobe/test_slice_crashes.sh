#!/bin/bash
find_and_run() {
	J="find __work_exifprobe/$1/crashes -type f"
	for S in `$J`
	do
		echo $S
		__work_exifprobe/prep -c $S
		echo "----"
	done	
}

find_and_run "outd1"
find_and_run "outd2"
find_and_run "outd3"
find_and_run "outd4"
find_and_run "outd5"
find_and_run "outd6"
find_and_run "outd7"
find_and_run "outd8"
exit 0
