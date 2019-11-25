#!/usr/bin/env bash
if [ $# -lt 1 ]
then
	echo "Usage: $0 <project>"
	exit 1
fi
grep -hE "^#define\s+APP_VER" $1/*.* | awk '{print $3}' | sed -e 's/"//g'
