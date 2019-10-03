#!/usr/bin/env bash
git config --global user.email "obono@users.noreply.github.com"
git config --global user.name "circleci"
git add _hexs/*.hex docs/repo.json README.md
git commit -m ":game_die: Automatic deployment [ci skip]"
if [ $? = 0 ]
then
	git push origin || exit 1
fi
