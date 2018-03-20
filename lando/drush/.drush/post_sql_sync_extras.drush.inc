<?php


/**
 * Implement hook help alter.
 *
 * When a hook extends a command with additional options, it must
 * implement help alter and declare the option(s).  Doing so will add
 * the option to the help text for the modified command, and will also
 * allow the new option to be specified on the command line.  Without
 * this, Drush will fail with an error when a user attempts to use
 * the option.
 */
function post_sql_sync_extras_drush_help_alter(&$command) {
  if ($command['command'] == 'sql-sync') {
    $command['options']['rr']  = "Rebuild the registry on the target site after the sync operation has completed.";
    $command['options']['fra']  = "Revert all features on the target site after the sync operation has completed.";
    $command['options']['enable']  = "Enable the specified modules in the target database after the sync operation has completed.";
    $command['options']['disable'] = "Disable the specified modules in the target database after the sync operation has completed.";
    $command['options']['permission'] = "Add or remove permissions from a role in the target database after the sync operation has completed. The value of this option must be an array, so it may only be specified in a site alias record or drush configuration file.  See `drush topic docs-example-sync-extension`.";
  }
}

/**
 * Implement hook post sql sync.
 *
 * The post hook is only called if the sql-sync operation completes
 * without an error.  When called, we check to see if the user specified
 * any modules to enable/disable.  If so, we will call pm-enable/pm-disable on each module.
 */
function drush_post_sql_sync_extras_post_sql_sync($source = NULL, $destination = NULL) {
  $registry_rebuild = drush_get_option('rr');
  if ($registry_rebuild) {
    drush_log(dt("Rebuild the registry post-sql-sync"), 'ok');
    drush_invoke_process($destination, 'rr');
  }
  $features_revert = drush_get_option('fra');
  if ($features_revert) {
    drush_log(dt("Revert all features post-sql-sync"), 'ok');
    drush_invoke_process($destination, 'fra', array(), array('yes' => TRUE));
  }
  $modules_to_enable = drush_get_option_list('enable');
  if (!empty($modules_to_enable)) {
    drush_log(dt("Enable !modules post-sql-sync", array('!modules' => implode(',', $modules_to_enable))), 'ok');
    drush_invoke_process($destination, 'pm-enable', $modules_to_enable, array('yes' => TRUE));
  }
  $modules_to_disable = drush_get_option_list('disable');
  if (!empty($modules_to_disable)) {
    drush_log(dt("Disable !modules post-sql-sync", array('!modules' => implode(',', $modules_to_disable))), 'ok');
    drush_invoke_process($destination, 'pm-disable', $modules_to_disable, array('yes' => TRUE));
  }
  $permissions_table = drush_get_option('permission');
  if (!empty($permissions_table)) {
    foreach ($permissions_table as $role_name => $actions) {
      if (array_key_exists('add', $actions)) {
        $permissions_to_add = is_array($actions['add']) ? $actions['add'] : explode(', ', $actions['add']);
        foreach ($permissions_to_add as $permission) {
          $values = drush_invoke_process($destination, 'role-add-perm', array($role_name, $permission), array(), array('integrate' => TRUE));
        }
      }
      if (array_key_exists('remove', $actions)) {
        $permissions_to_remove = is_array($actions['remove']) ? $actions['remove'] : explode(', ', $actions['remove']);
        foreach ($permissions_to_remove as $permission) {
          $values = drush_invoke_process($destination, 'role-remove-perm', array($role_name, $permission), array(), array('integrate' => TRUE));
        }
      }
    }
  }
  if ($registry_rebuild) {
    drush_log(dt("Rebuild the registry post-sql-sync"), 'ok');
    drush_invoke_process($destination, 'rr');
  }
}
