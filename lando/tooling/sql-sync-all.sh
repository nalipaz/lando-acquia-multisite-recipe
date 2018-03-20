#!/usr/bin/env bash

drush cc drush --quiet
# Ascertain our appname from one of the prod aliases.
appname=`lando drush sa|grep -Po '@[a-z]+.prod\r'|sed -E 's/@([a-z]+).prod/\1/'`

for site in $(lando drush sa|grep -Po '@[a-z]+\r'|grep -v '@none'); do
  echo "[${site}] Synchronizing SQL from ${1} to lando..."
  drush sql-sync @${appname}.${1}.${site} @${site} -y
done
