<?php
/**
 * Lando support.
 */
$lando_info = json_decode(getenv('LANDO_INFO'), TRUE);
$settings['trusted_host_patterns'] = ['.*'];
$settings['hash_salt'] = md5(getenv('LANDO_HOST_IP'));
$databases['default']['default'] = [
  'driver' => 'mysql',
  'database' => 'site1',
  'username' => $lando_info['database']['creds']['user'],
  'password' => $lando_info['database']['creds']['password'],
  'host' => $lando_info['database']['internal_connection']['host'],
  'port' => $lando_info['database']['internal_connection']['port'],
];

