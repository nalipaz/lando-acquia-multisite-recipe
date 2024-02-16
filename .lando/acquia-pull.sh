#!/bin/bash

set -e

# Get the lando logger
. /helpers/log.sh

# Set the module
LANDO_MODULE="acquia"

# Set some acli variables
ACLI_DB_HOST=database
ACLI_DB_NAME=acquia
ACLI_DB_USER=acquia
ACLI_DB_PASSWORD=acquia

# Set option defaults
CODE='dev';
DATABASE='dev';
FILES='dev';
KEY='none'
SECRET='none'

# PARSE THE ARGZZ
while (( "$#" )); do
  case "$1" in
    -k|--key|--key=*)
      echo '--'
      if [ "${1##--key=}" != "$1" ]; then
        KEY="${1##--key=}"
        shift
      else
        KEY=$2
        shift 2
      fi
      ;;
    -s|--secret|--secret=*)
      if [ "${1##--secret=}" != "$1" ]; then
        SECRET="${1##--secret=}"
        shift
      else
        SECRET=$2
        shift 2
      fi
      ;;
    -c|--code|--code=*)
      CODEFLAG=true
      if [ "${1##--code=}" != "$1" ]; then
        CODE="${1##--code=}"
        shift
      else
        CODE=$2
        shift 2
      fi
      ;;
    -d|--database|--database=*)
      DATABASEFLAG=true
      if [ "${1##--database=}" != "$1" ]; then
        DATABASE="${1##--database=}";
        shift
      else
        DATABASE=$2
        shift 2
      fi
      ;;
    -s|--site|--site=*)
      SITEFLAG=true
      if [ "${1##--site=}" != "$1" ]; then
        SITE="${1##--site=}";
        shift
      else
        SITE=$2
        shift 2
      fi
      ;;
    -f|--files|--files=*)
      FILEFLAG=true
      if [ "${1##--files=}" != "$1" ]; then
        FILES="${1##--files=}"
        shift
      else
        FILES=$2
        shift 2
      fi
      ;;
    --rsync|--rsync=*)
        RSYNC=$1
        shift
      ;;
    --no-auth)
        NO_AUTH=true
        shift
      ;;
    --on-demand)
        ON_DEMAND=" --on-demand"
        shift
      ;;
    --)
      shift
      break
      ;;
    -*|--*=)
      shift
      ;;
    *)
      shift
      ;;
  esac
done

# Generate token with the key passed from tooling
if [ "$KEY" != "none" ]; then
  /usr/local/bin/acli auth:login -k "$KEY" -s "$SECRET" -n
fi

# @TODO: if lando has not already generated and exchanged a key with acquia cloud let us do that here

# We accept either all (the default) which will sync all sites within a multi-site
#   or we accept one site name. If a name is provided here we use only that.
if [ "$SITE" != "all" ]; then
  MULTI_SITES="${SITE}"
fi

# Get the codez
if [ "$CODE" != "none" ]; then
  if [ $CODEFLAG ]; then
    echo -n "    "
    lando_check "Using the code flag supplied to pull code from $CODE."
  fi
  acli -n pull:code "$AH_SITE_GROUP.$CODE"
fi

# Get the database
if [ "$DATABASE" != "none" ]; then
  if [ $DATABASEFLAG ]; then
    echo -n "    "
    lando_check "Using the database flag supplied to pull the database from $DATABASE."
  fi

  # Destroy existing tables
  # NOTE: We do this so the source DB **EXACTLY MATCHES** the target DB
  for site in ${MULTI_SITES}; do
    echo -n ""
    lando_check "Pulling database for the ${site} multisite."

    TABLES[${site}]=$(mysql --user=${ACLI_DB_USER} --password=${ACLI_DB_PASSWORD} --database=${site} --host=${ACLI_DB_HOST} --port=3306 -e 'SHOW TABLES' | awk '{ print $1}' | grep -v '^Tables' ) || true
    echo -n "    "
    lando_check "Destroying all current tables in local database named ${site} if needed... "

    for t in ${TABLES[${site}]}; do
      echo -n "    "
      lando_check "Dropping $t from local database named ${site}..."
      mysql --user=${ACLI_DB_USER} --password=${ACLI_DB_PASSWORD} --database=${site} --host=${ACLI_DB_HOST} --port=3306 <<-EOF
        SET FOREIGN_KEY_CHECKS=0;
        DROP VIEW IF EXISTS \`$t\`;
        DROP TABLE IF EXISTS \`$t\`;
EOF
    done

    # Importing the database.
    echo -n "    "
    lando_check "Running the command: acli pull:db ${AH_SITE_GROUP}.${DATABASE} ${site}${ON_DEMAND}"
    acli pull:db "${AH_SITE_GROUP}.${DATABASE}" ${site}${ON_DEMAND}

    # Weak check that we got tables
    PULL_DB_CHECK_TABLE=${LANDO_DB_USER_TABLE:-users}
    echo -n "    "
    lando_check "Checking db pull for expected tables..."

    if ! mysql --user=${ACLI_DB_USER} --password=${ACLI_DB_PASSWORD} --database=${site} --host=${ACLI_DB_HOST} --port=3306 -e "SHOW TABLES;" | grep $PULL_DB_CHECK_TABLE >/dev/null; then
      lando_red "Database pull failed... "
      exit 1
    fi
  done
fi

# Get the files
if [ "$FILES" != "none" ]; then
  if [ $FILESFLAG ]; then
    echo -n "    "
    lando_check "Using the files flag supplied to pull files from $FILES."
  fi

  for site in ${MULTI_SITES}; do
    echo -n ""
    lando_check "Pulling files for the ${site} multisite."
    # acli -n pull:files "$FILES" -> non interactive causes a broken pipe error right now
    lando_check "Running the command: acli -n pull:files ${AH_SITE_GROUP}.${FILES} ${site}"
    acli -n pull:files "$AH_SITE_GROUP.$FILES" ${site}
  done
fi

# Finish up!
echo -n "    "
lando_check "Pull completed successfully!"

