<?php
/**
 * @file
 * Acquia's commandfiles add 'site' and 'env' options. Declare those as global options to avoid 'unrecognized option' error.
 */

/**
 * Implements hook_drush_help_alter().
 */
function ahoptions_drush_help_alter(&$command) {
  if ($command['command'] == 'global-options') {
    // Do not include these in options in standard help.
    if ($command['#brief'] === FALSE) {
      $command['options']['site'] = array(
        'description' => 'The site to operate on. Sent whenever an Acquia site alias is used',
        'hidden' => TRUE,
      );
      $command['options']['env'] = array(
        'description' => 'The environment to operate on. Sent whenever an Acquia site alias is used',
        'hidden' => TRUE,
      );
    }
  }
}
