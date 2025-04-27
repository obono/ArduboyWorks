#!/usr/bin/env bash

if [ $# -lt 1 ]
then
	echo "Usage: $0 <project> | all"
	exit 1
fi

BIN_DIR='.circleci'
OUT_DIR='_hexs'

if [ $1 = 'all' ]
then
	projects=`cat ${BIN_DIR}/projects.txt`
else
	projects=$1
fi

for project in ${projects}
do
	version=`${BIN_DIR}/get_version.sh ${project}`
	if [ -z ${version} ]
	then
		echo "Failed to get version of \"${project}\"" >&2
		exit 1
	fi
	echo "--- Building \"${project}\"..."
	cd ${project}
	arduino-cli compile --fqbn arduboy-homemade:avr:arduboy --build-properties "core=arduboy-core,boot=org" --output-dir . || exit 1
	mv ${project}.ino.hex ../${OUT_DIR}/${project}_v${version}.hex
	rm *.eep *.elf *.with_bootloader.*
	cd ..
	echo "--- Completed!"
	echo
done
