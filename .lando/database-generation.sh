#!/bin/bash

set -e

# Creates the extra databases as root since the app user does not have those privileges
# Name of db must match that of the acquia db.

for site in ${MULTI_SITES}; do
  mysql -uroot -e "CREATE DATABASE IF NOT EXISTS ${site}; GRANT ALL PRIVILEGES ON ${site}.* TO 'acquia'@'%' IDENTIFIED by 'acquia';"
done

