#!/usr/bin/env bash

if [ $# -lt 1 ]
then
	echo "Usage: $0 <repo json file>"
	exit 1
fi

TEMPLATE_FILE='.circleci/README.md.template'
PLACEHOLDER='{{ProductsInfo}}'

repo_file=$1
items=`cat ${repo_file} | jq .items`
len=`echo ${items} | jq length`

sed -n "/^${PLACEHOLDER}\$/,\$!p" ${TEMPLATE_FILE}

for i in `seq 0 $(($len - 1))`
do
	item=`echo ${items} | jq .[${i}]`
	app_code=`printf OBN-Y%02d $(($i + 1))`
	title=`echo ${item} | jq -r .title`
	note=`echo ${item} | jq -r .extraNote`
	desc=`echo ${item} | jq -r .description`
	url=`echo ${item} | jq -r .url`
	source_url=`echo ${item} | jq -r .sourceUrl`
	project=${source_url##*/}

	if [ "${url}" != 'null' ]
	then
		title="[${title}](${url})"
	fi
	if [ "${note}" != 'null' ]
	then
		title="${title} (${note})"
	fi

	echo '*' ${app_code} ${title}
	echo '  *' ${desc}
	echo '  * Depend on Arduboy Library 1.1.1\'
	echo "    ![title](docs/img/${project}1.gif) ![playing](docs/img/${project}2.gif)"
done

sed -n "1,/^${PLACEHOLDER}\$/!p" ${TEMPLATE_FILE}
