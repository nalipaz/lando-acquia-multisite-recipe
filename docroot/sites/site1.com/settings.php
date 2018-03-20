#!/usr/bin/env bash

# Ascertain our appname from one of the prod aliases.
appname=$(drush sa|grep -Po '@[a-z]+.prod$'|sed -E 's/@([a-z]+).prod/\1/')

for site in $(drush sa|grep -Po '@[a-z]+[^.]$'|grep -v '@none'|grep -v '@'${appname}|grep -Po '[a-z]+'); do
  echo "[${site}] Synchronizing SQL from ${1} to lando..."
  drush sql-sync @${appname}.${1}.${site} @${site} -y
done
