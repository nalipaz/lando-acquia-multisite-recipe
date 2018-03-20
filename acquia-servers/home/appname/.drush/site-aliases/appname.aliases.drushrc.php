<?php

$appname = 'appname';
$directories = array(
  'site1.com',
  'site2.com',
  'site3.com',
);
$environments = array('prod', 'test', 'dev', 'proto', 'proto2');

foreach ($directories as $dir) {
  $domain = str_replace('www.', '', $dir);
  $alias = str_replace('.com', '', $domain);
  
  foreach ($environments as $env) {
    $aliases[$env . '.' . $alias] = array(
      'parent' => '@' . $appname . '.' . $env,
    );
    
    // Set the uri for each alias.
    // subdomains get changed as we typically name our sites, production is www.example.com, and test is staging.example.com
    $sub = str_replace('prod', 'www', $env);
    $sub = str_replace('test', 'staging', $sub);
    $aliases[$env . '.' . $alias]['uri'] = $sub . '.' . $domain;
  }
}
