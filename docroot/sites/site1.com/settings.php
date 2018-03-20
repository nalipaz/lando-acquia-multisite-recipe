<?php

// All your normal settings stuff.

// ... followed by ...

// The following variable is Acquia-centric but could easily be swapped out for some other environment variable.
// Include environment specific settings files if they exist.
if (key_exists('AH_SITE_ENVIRONMENT', $_ENV)) {
  $environment_settings = $site_directory . '/settings.' . $_ENV['AH_SITE_ENVIRONMENT'] . '.php';
  
  if (file_exists($environment_settings)) {
    include $environment_settings;
  }
}
