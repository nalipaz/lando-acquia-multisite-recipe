<?php

// phpcs:ignoreFile

$sites = [
  // Site1
  'site1.com' => 'site1',
  'www.site1.com' => 'site1',
  'dev.site1.com' => 'site1',
  'stg.site1.com' => 'site1',
  'prd.site1.com' => 'site1',

  // Site2
  'site2.com' => 'site2',
  'www.site2.com' => 'site2',
  'dev.site2.com' => 'site2',
  'stg.site2.com' => 'site2',
  'prd.site2.com' => 'site2',
];

/**
 * Lando support.
 */
if (getenv('LANDO') === 'ON') {
  if (file_exists($app_root . '/sites/sites.lando.php')) {
    include $app_root . '/sites/sites.lando.php';
  }
}

