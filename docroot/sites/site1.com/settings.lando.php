<?php
$databases['default'] = array (
  'default' => array (
    'driver' => 'mysql',
    'database' => 'database',
    'username' => 'mysql',
    'password' => 'password',
    'prefix' => '',
    'port' => 3306,
  )
);

// The only line that really needs to change.
$databases['default']['default']['host'] = 'site1_db';

$conf['file_temporary_path'] = '/tmp/';
$conf['memcache_servers'] = array(
  'localhost:11211' => 'cluster1',
);
$conf['memcache_bins'] = array(
  'bin1' => 'cluster1',
);
$conf['shield_enabled'] = 0;
$conf['plupload_temporary_uri'] = '/tmp';
$conf['is_https'] = (array_key_exists('HTTP_X_FORWARDED_PROTO', $_SERVER) && $_SERVER['HTTP_X_FORWARDED_PROTO'] === 'https');
if ($conf['is_https']) {
  $base_url = 'https://' . $_SERVER['HTTP_HOST'];
}
