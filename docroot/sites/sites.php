<?php

$lando_sites_file = dirname(__FILE__) . "/sites.lando.php";
if (file_exists($lando_sites_file)) {
  include $lando_sites_file;
}
$local_sites_file = dirname(__FILE__) . "/sites.local.php";
if (file_exists($local_sites_file)) {
  include $local_sites_file;
}
