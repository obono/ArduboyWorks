#!/usr/bin/env bash

BIN_DIR='.circleci'

projects=`cat ${BIN_DIR}/projects.txt`
repo='{
	"repository": "OBONO\u0027s Arduboy Works",
	"api-version": "1.0",
	"maintainer": "OBONO",
	"website": "https://github.com/obono/ArduboyWorks",
	"items": []
}'

for project in ${projects}
do
	version=`${BIN_DIR}/get_version.sh ${project}`
	if [ -z ${version} ]
	then
		echo "Failed to get version of \"${project}\"" >&2
		exit 1
	fi
	ITEM=`${BIN_DIR}/jq_info.sh ${project} ${version} | sed -e 's/"/\\"/g'` || exit 1
	repo=`echo ${repo} | jq ".items |= .+[${ITEM}]"` || exit 1
done
echo ${repo} | jq --tab
