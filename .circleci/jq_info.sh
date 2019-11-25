#!/usr/bin/env bash

if [ $# -lt 3 ]
then
	echo "Usage: $0 <project> <app_code> <version>"
	exit 1
fi

SRC_URL_BASE='https://github.com/obono/ArduboyWorks/tree/master/'
HEX_URL_BASE='https://raw.githubusercontent.com/obono/ArduboyWorks/master/_hexs/'
IMG_URL_BASE='https://obono.github.io/ArduboyWorks/img/'

project=$1
app_code=$2
version=$3

base_json=`cat ${project}/info.json` || exit 1
title=`echo ${base_json} | jq '.title'`

echo ${base_json} | jq ". | .+
{
	\"version\": \"${version}\",
	\"sourceUrl\": \"${SRC_URL_BASE}${project}\",
	\"author\": \"OBONO\",
	\"binaries\": [
		{
			\"title\": ${title},
			\"filename\": \"${HEX_URL_BASE}${project}_v${version}.hex\",
			\"device\": \"Arduboy\"
		}
	],
	\"screenshots\": [
		{
			\"filename\": \"${IMG_URL_BASE}${project}2.gif\",
			\"title\": \"Game screen\"
		},
		{
			\"filename\": \"${IMG_URL_BASE}${project}1.gif\",
			\"title\": \"Title screen\"
		}
	],
	\"license\": \"MIT\",
	\"arduboy\": \"\",
	\"codeName\": \"${app_code}\",
	\"nickName\": \"${project}\"
}" || exit 1
