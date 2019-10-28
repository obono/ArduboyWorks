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
	arduino-cli compile --fqbn arduboy:avr:arduboy ${project} -o ${OUT_DIR}/${project}_v${version}.hex || exit 1
	echo "--- Completed!"
	echo
done
rm -f ${OUT_DIR}/*.elf
