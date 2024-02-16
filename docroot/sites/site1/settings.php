<?php

// Additions needed at very bottom in settings.php to make multisite and lando work.

$settings['config_sync_directory'] = '../config/site1';

/**
 * Settings need to connect to the non-default database on Acquia.
 * This code can be obtained from the Acquia interface, but you should add the lando bit.
 */
if (file_exists('/var/www/site-php') && getenv('LANDO') !== 'ON') {
  require '/var/www/site-php/multisiteacquiaapp/site1-settings.inc';
}

/**
 * Lando support.
 */
if (getenv('LANDO') === 'ON') {
  if (file_exists($app_root . '/' . $site_path . '/settings.lando.php')) {
    include $app_root . '/' . $site_path . '/settings.lando.php';
  }
}

