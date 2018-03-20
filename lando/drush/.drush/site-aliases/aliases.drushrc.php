<?php

$directories = array(
  'site1.com',
  'site2.com',
  'site3.com',
);

foreach ($directories as $dir) {
  $domain = str_replace('www.', '', $dir);
  $alias = str_replace('.com', '', $domain);
  $aliases[$alias] = array(
    'root' => '/app/docroot',
    'uri' => $domain,
    'target-command-specific' => array(
      'sql-sync' => array(
        // Allow enabling/disabling of modules on lando.
//        'enable' => array('stage_file_proxy'),
//        'disable' => array('shield'),
        // Rebuild the registry post-sql-sync.
        'rr' => TRUE,
        // Don't revert all features on post-sql-sync.
        'fra' => FALSE,
      ),
    ),
  );
}
