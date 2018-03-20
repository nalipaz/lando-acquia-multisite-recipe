<?php
// $Id: sql.drush.inc,v 1.47 2010/04/19 05:21:17 greg1anderson Exp $

/**
 * @file AH drush commands
 */

/**
 * Error status indicating that an AH drush command failed due to information
 * it was given by the user (e.g. an import file), as opposed to invalid
 * command line arguments, system errors, etc. In other words, the command
 * worked and failure was the correct action.
 *
 * This is important for drush jobs run by tasks. When a failure occurs due to
 * user input, this exit status tells the task system to tell the user it is
 * their fault (and show them the command output) instead of telling them the
 * task system failed.
 */
define('AH_USER_ERROR_STATUS', 130);

/**
 * Cause Drush to exit with a specific exit status.
 */
function ah_drush_exit_with_status($status) {
  $verbose = drush_get_context('DRUSH_VERBOSE');
  if ($verbose) {
    drush_log(dt('Exiting with status @status', array('@status' => $status), 'ok'));
  }

  // This kludge is based on Drupal's kludge which is based on PHP's kludge.
  // Drush registers drush_shutdown which does some standard cleanup and then
  // registers drush_return_status() which calls exit() with status 1 if
  // drush_set_error() has been called. PHP's behavior on exit(non-zero)
  // outside of a shutdown function is that shutdown functions are run and
  // then the program exits with the non-zero status originally provided, even
  // if a shutdown function also provides a non-zero exit status.
  drush_set_context("DRUSH_EXECUTION_COMPLETED", TRUE);
  exit($status);
}

/**
 * Implementation of hook_drush_help().
 */
function ah_drush_help($section) {
  switch ($section) {
    case 'ah:ah-db-backup':
      return dt('Back up an Acquia Cloud database.');
    case 'ah:ah-site-archive-import':
      return dt('Import a Drupal Site Archive into Acquia Cloud.');
  }
}

/**
 * Implementation of hook_drush_command().
 */
function ah_drush_command() {
  $items['ah-db-backup'] = array(
    'description' => 'Back up an Acquia Hosting database.',
    'arguments' => array(
      'site' => dt('Back up database from this AH site.'),
      'roles' => dt('The comma-separated list of database roles to back up. Defaults to all database roles for the site.'),
    ),
    'options' => array(
      'retain' => dt('Number of daily backups to retain.'),
      // TODO: This clearly belongs somewhere other than this
      // command. Move it.
      'ah-syslog' => dt('Log output via syslog.'),
      'structure-tables-list' => dt('Comma separated list of tables to backup only the structure (no data, only the CREATE TABLE command)'),
      'skip-tables-list' => dt('Comma separated list of tables to skip in the backup'),
      'structure-tables-key' => dt('A key in the $structure_tables array. See example.drushrc.php. Optional.'),
      'skip-tables-key' => dt('A key in the $skip_tables array. @see example.drushrc.php. Optional.'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
  $items['ah-site-archive-import'] = array(
    'description' => 'Import a Drupal Site Archive into Acquia Hosting. This command requires a Drush alias for the Acquia Cloud site environment into which to import; e.g.: "drush @<sitename>.<env> ah-sari ./archive.tgz".',
    'arguments' => array(
      'filename' => dt('The archive file to import.'),
    ),
    'options' => array(
      'extract-into' => dt('The temporary directory into which to extract the archive.'),
    ),
    'aliases' => array('ah-sar-import', 'ah-sari'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
  $items['ah-sql-connect'] = array(
    'description' => 'Print a mysql command for connecting to an Acquia Cloud database.',
    'options' => array(
      'site' => 'The site name into which to import. REQUIRED, but use the drush @sitename.env alias instead of specifying this option directly.',
      'env' => 'The site environment into which to import. REQUIRED, but use the drush @sitename.env alias instead of specifying this option directly.',
      'db' => 'The environment-agnostic database name; this is the name shown on the Workflow page of the Cloud UI. Defaults to the --site value, which is the default primary Drupal database for the site.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    'callback' => 'drush_ah_sql_connect_callback',
  );
  $items['ah-sql-cli'] = array(
    'description' => "Open a SQL command-line interface using Drupal's credentials.",
    'options' => array(
      'site' => 'The site name into which to import. REQUIRED, but use the drush @sitename.env alias instead of specifying this option directly.',
      'env' => 'The site environment into which to import. REQUIRED, but use the drush @sitename.env alias instead of specifying this option directly.',
      'db' => 'The environment-agnostic database name; this is the name shown on the Workflow page of the Cloud UI. Defaults to the --site value, which is the default primary Drupal database for the site.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
    'aliases' => array('ah-sqlc'),
    'callback' => 'drush_ah_sql_cli_callback',
  );
  $items['ah-db-import'] = array(
    'description' => 'Imports the Drupal DB as SQL using mysql converting tables to InnoDB in the process.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'examples' => array(
      'drush @site.env ah-db-import' => 'Import the SQL dump to the SQL database of the same name.',
    ),
   'arguments' => array(
      'file' => 'Import SQL file. To file should be relative to Drupal root. If file name ends in .gz, the file will first be extracted to $TMP',
    ),
    'options' => array(
      'site' => 'The site name into which to import. REQUIRED, but use the drush @site.env alias instead of specifying this option directly.',
      'env' => 'The site environment into which to import. REQUIRED, but use the drush @site.env alias instead of specifying this option directly.',
      'drop' => 'Drop all existing tables prior to import. To drop all tables and not replace them with anything, use --drop /dev/null.',
      'force' => 'Import even if the database is not empty.',
      'myisam' => 'List of tables (separated by commas) to be excluded from conversion to InnoDB.',
      'db' => 'Specify database to import if multiple dbs are defined in settings.php',
    ),
  );
  if (DRUSH_VERSION < 5) {
    // Drush before version 5 worked with '--' prepended, later versions don't.
    foreach ($items as $name => $item) {
      if (!empty($items[$name]['options'])) {
        foreach ($items[$name]['options'] as $option => $description) {
          $items[$name]['options']["--{$option}"] = $description;
          unset($items[$name]['options']);
        }
      }
    }
  }
  return $items;
}

/**
 * Command callback: Acquia Hosting SQL Connect
 *
 * Display a MySQL connection string for an Acquia Hosting database based on
 * sitegroup and environment. This does NOT require a valid Drupal docroot.
 */
function _drush_ah_sql_connect_callback() {
  $verbose = drush_get_context('DRUSH_VERBOSE');
  $sitegroup = drush_get_option('site', '');
  $env = drush_get_option('env', '');
  if (!preg_match('@^[-_a-z0-9]+$@', $sitegroup) || !preg_match('@^[-_a-z0-9]+$@', $env)) {
    drush_set_error('AH_SQL_CONNECT_BAD_ARGS', dt('--site and --env options are required; use the drush @sitename.env alias to specify them.'));
    return;
  }
  $db = drush_get_option('db', 'default');

  $site = file_get_contents("/var/www/site-php/$sitegroup/.$env");
  if (empty($site)) {
      drush_set_error('AH_SQL_CONNECT_SITE', dt('cannot determine internal site name for @sitegroup.@env', array('@sitegroup' => $sitegroup, '@env' => $env)));
    return;
  }

  $conf['acquia_use_early_cache'] = TRUE;
  $primary_role = _drush_ah_get_primary_role_name($sitegroup, $env);
  require("/var/www/site-php/{$site}/D6-{$env}-{$primary_role}-settings.inc");

  $info = parse_url($db_url[$db]);
  $info['path'] = substr($info['path'], 1);
  if ($verbose) {
    drush_print_r($db_url);
  }
  return "mysql -h {$info['host']} -u {$info['user']} -p{$info['pass']} {$info['path']}";
}

/**
 * Command callback: Acquia Hosting SQL Connect
 *
 * Same as sql-connect but does not require a valid Drupal docroot.
 */
function drush_ah_sql_connect_callback() {
  drush_print(_drush_ah_sql_connect_callback());
}

/**
 * Command callback: Acquia Hosting SQL CLI.
 *
 * Same as sql-connect but does not require a valid Drupal docroot.
 */
function drush_ah_sql_cli_callback() {
  $status = proc_close(proc_open(_drush_ah_sql_connect_callback(), array(0 => STDIN, 1 => STDOUT, 2 => STDERR), $pipes));
  if ($status != 0) {
    $sitegroup = drush_get_option('site', '');
    $env = drush_get_option('env', '');
    drush_set_error('AH_SQL_CLI_STATUS', dt('@site.@env: mysql exited with status @status', array('@site' => $sitegroup, '@env' => $env, '@status' => $status)));
  }
}

/**
 * Wait for a docroot to be updated.
 */
function _drush_ah_wait_site_update($site_disp, $site_name, $pre_deploy_time, $timeout, $interval) {
  $dt_args = array('@site' => $site_disp);
  $deploy_flag_file = "/var/www/site-php/$site_name/.latest_code_deploy";
  $site_update = @file_get_contents($deploy_flag_file);
  $start = time();
  while (intval($site_update) < $pre_deploy_time) {
    if (time() - $start > $timeout) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: timeout waiting for site update', $dt_args));
      return FALSE;
    }
    drush_log(dt('@site: waiting for site update', $dt_args), 'ok');
    sleep($interval);
    $site_update = @file_get_contents($deploy_flag_file);
  }
  return TRUE;
}

function drush_ah_empty_settings_content() {
  return <<<EOF
<?php
/**
 * This settings.php file was created by the Acquia Cloud ah-site-archive-import
 * Drush command. The imported archive did not contain a settings.php file, so
 * the import process created this file by default. You can replace this file
 * with the standard default settings.php file for your version of Drupal.
 * However, be sure to keep the last line that loads the "Acquia Cloud settings
 * include file", which provides the correct database credentials for your site.
 */
\$update_free_access = FALSE;
\$drupal_hash_salt = '';
ini_set('arg_separator.output',     '&amp;');
ini_set('magic_quotes_runtime',     0);
ini_set('magic_quotes_sybase',      0);
ini_set('session.cache_expire',     200000);
ini_set('session.cache_limiter',    'none');
ini_set('session.cookie_lifetime',  2000000);
ini_set('session.gc_divisor',       100);
ini_set('session.gc_maxlifetime',   200000);
ini_set('session.gc_probability',   1);
ini_set('session.save_handler',     'user');
ini_set('session.use_cookies',      1);
ini_set('session.use_only_cookies', 1);
ini_set('session.use_trans_sid',    0);
ini_set('url_rewriter.tags',        '');

EOF;
}

/**
 * Determine if a Git tag exists in the given remote repository.
 *
 * @param $git_cmd
 *   The git command to operate against $url, e.g. pointing to the wrapper
 *   using the correct key.
 * @param $url
 *   The git repo URL to test.
 * @param $tag
 *   The tag to test.
 * @return
 *   TRUE if $tag exists in $url, FALSE otherwise.
 */
function _ah_git_tag_exists($git_cmd, $url, $tag) {
  // ls-remote --heads only lists heads of branches, so if
  // $git_current_branch is a tag, it will fail.
  return drush_shell_exec("$git_cmd ls-remote --tags $url $tag | grep 'refs/tags/$tag\$'");
}

/**
 * Command callback: Acquia Hosting Site Archive Import.
 *
 * This currently understands Gardens exports and the common Drupal
 * install profile distribution format. There being no standard yet,
 * it is impossible to know what else it might support.
 */
function drush_ah_site_archive_import($orig_path = '') {
  if (empty($orig_path)) {
    return drush_set_error('AH_SITE_ERROR', dt('No archive file specified.'));
  }
  $archive_path = realpath($orig_path);
  if ($archive_path == FALSE) {
    drush_set_error('AH_SITE_ERROR', dt('File not found or accessible: @orig_path', array('@orig_path' => $orig_path)));
  }

  // Determine the target site. If we are not told, do not guess, because if
  // we guess wrong we can blow away an unintended site.
  $sitegroup = drush_get_option('site', '');
  $env = drush_get_option('env', '');
  if (!preg_match('@^[-_a-z0-9]+$@', $sitegroup) || !preg_match('@^[-_a-z0-9]+$@', $env)) {
    if (isset($_SERVER['USER']) && is_dir("/var/www/site-php/{$_SERVER['USER']}")) {
      $site = $_SERVER['USER'];
      $env = '<env>';
      foreach (array('dev', 'test', 'prod') as $envp) {
        if (file_exists("/var/www/site-php/$site/.$envp")) {
          $env = $envp;
          break;
        }
      }
    }
    else {
      $site = '<sitename>';
      $env = '<env>';
    }
    drush_set_error('AH_SITE_ARCHIVE', dt('Specify the Drush alias for the Acquia Cloud site and environment you want to import into; e.g. "drush @@site.@env @archive_path".', array('@archive_path' => $orig_path, '@site' => $site, '@env' => $env)));
    return;
  }

  $site = file_get_contents("/var/www/site-php/$sitegroup/.$env");
  if (empty($site)) {
      drush_set_error('AH_SITE_ARCHIVE', dt('cannot determine internal site name for @sitegroup.@env', array('@sitegroup' => $sitegroup, '@env' => $env)));
    return;
  }

  // Collect re-usable log arguments in $dt_args.
  $extractdir = drush_get_option('extract-into', "/mnt/tmp/{$site}/site-import");
  $dt_args = array('@site' => "$sitegroup.$env", '@archive_path' => $archive_path, '@extractdir' => $extractdir);

  // Determine if the site is using SVN or Git.
  $vcs_type = trim(@file_get_contents("/var/www/site-php/{$site}/vcs-type"));
  if (empty($vcs_type)) {
    drush_set_error('AH_SITE_ARCHIVE', dt('@site: Cannot find version control system.', $dt_args));
    return;
  }
  $dt_args['@vcs_type'] = $vcs_type;

  // Refuse to import on top of a tag, because tags are (should be) write-once
  // and immutable.
  if ($vcs_type == 'svn') {
    $svn_path = file_get_contents("/var/www/site-php/{$site}/svn-path");
    $dt_args['@svnpath'] = $svn_path;
    if (preg_match('@^tags/@', $svn_path)) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: Cannot overwrite an SVN tag (@svnpath) with site-import. Please deploy trunk or a named branch and try again.', $dt_args));
      ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
    }
    drush_log(dt('@site: Importing to SVN path @svnpath', $dt_args));
  }
  elseif ($vcs_type == 'git') {
    $git_ssh_url = file_get_contents("/var/www/site-php/{$site}/git-ssh-url");
    $git_current_branch = file_get_contents("/var/www/site-php/{$site}/git-object");
    $git_wrapper = "/var/www/site-php/{$site}/git-ssh-wrapper";
    $git_rem_cmd = "GIT_SSH={$git_wrapper} git";

    // ls-remote --heads only lists heads of branches, so if
    // $git_current_branch is a tag, it will fail.
    system("{$git_rem_cmd} ls-remote -q --heads {$git_ssh_url} {$git_current_branch}", $ret);
    // Apparently ls-remote does not ALWAYS fail on a tag, so be paranoid.
    if ($ret != 0 || preg_match('@^tags/@', $git_current_branch)) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: Cannot overwrite a Git tag (@gitbranch) with site-import. Please deploy master or another named branch and try again.', $dt_args + array('@gitbranch' => $git_current_branch)));
      ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
      return;
    }
  }

  // Extract the archive.
  drush_log(dt('@site: Importing @archive_path', $dt_args), 'ok');
  system("chmod -R u+w $extractdir 2>/dev/null && rm -rf $extractdir");
  system("mkdir $extractdir", $ret);
  if ($ret != 0) {
    drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot prepare @extractdir: status @ret', $dt_args + array('@ret' => $ret)));
    return;
  }
  drush_log(dt('@site: Extracting archive', $dt_args), 'ok');
  system("tar -C $extractdir -xzf {$archive_path} && chmod -R u+w $extractdir", $ret);
  if ($ret != 0) {
    drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot extract @archive_path (not a gzip-compressed tar file?): status @ret', $dt_args + array('@ret' => $ret)));
    ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
    return;
  }

  // Try to find the docroot, fail without one.
  if (file_exists("{$extractdir}/index.php")) {
    $docroot = $extractdir;
  }
  else {
    $docroots = glob("{$extractdir}/*/index.php");
    if (count($docroots) > 0) {
      $docroot = dirname($docroots[0]);
    }
    else {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot find a Drupal docroot in @archive_path', $dt_args));
      ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
      return;
    }
  }

  // Find the sites directory. If there is one, use it. If there is more than
  // one, fail. If there is not one, we'll create a new sites/default
  // directory and settings.php.
  $search_files = array('settings.php', 'files', 'default.settings.php');
  foreach ($search_files as $file) {
    $import_site_dirs = glob("$docroot/sites/*/$file");
    if (!empty($import_site_dirs)) {
      break;
    }
  }
  if (count($import_site_dirs) > 1) {
    drush_set_error('AH_SITE_ARCHIVE', dt('@site: @count sites directories found in @archive_path; only one is currently allowed', $dt_args + array('@count' => count($import_site_dirs))));
    ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
    return;
  }
  else if (count($import_site_dirs) == 0) {
    // Leave $import_site_dir == NULL since there is no one.
    unset($import_site_dir);
    drush_log(dt('@site: No sites dir found in archive', $dt_args), 'warning');
  }
  else {
    $import_site_dir = dirname($import_site_dirs[0]);
    drush_log(dt('@site: Importing from site directory @dir.', $dt_args + array('@dir' => $import_site_dir)), 'notice');
  }

  // Always import to sites/default regardless of the sites dir name in the
  // archive. Make sure sites/default/settings.php exists.
  $site_dir = "$docroot/sites/default";
  if (!file_exists($site_dir)) {
    if (!drush_op('mkdir', $site_dir, 0777, TRUE)) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot mkdir @dir', $dt_args + array('@dir' => $site_dir)));
      return;
    }
  }
  else if (!is_dir($site_dir)) {
    drush_set_error('AH_SITE_ARCHIVE', dt('@site: @dir exists but is not a directory', $dt_args + array('@dir' => $site_dir)));
    return;
  }

  // Use $import_site_dir settings.php and services.yml, if they exists.
  if (!empty($import_site_dir)) {
    $config_files = array('settings.php', 'services.yml');
    foreach ($config_files as $config_file) {
      $target_path = "$site_dir/$config_file";
      $import_path = "$import_site_dir/$config_file";
      if (!file_exists($import_path)) {
        $import_path = "$import_site_dir/default.$config_file";
      }
      if (file_exists($import_path) && $import_path != $target_path) {
        drush_log(dt('@site: importing @target from @source', $dt_args + array('@target' => $target_path, '@source' => $import_path)), 'ok');
        if (!copy($import_path, $target_path)) {
          drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot copy @src to @dst', $dt_args + array('@src' => $import_path, '@dst' => $site_dir)));
          return;
        }
      }
    }
  }

  // If we do not have a settings.php, create one.
  $settings_path = "$site_dir/settings.php";
  if (!file_exists($settings_path)) {
    drush_log(dt('@site: creating default @path', $dt_args + array('@path' => $settings_path)), 'notice');
    if (!@file_put_contents($settings_path, drush_ah_empty_settings_content())) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: @path does not exist and directory is not writeable', $dt_args + array('@path' => $settings_path)));
      return;
    }
  }

  // TODO. Dev Desktop sets $base_url in settings.php which breaks everything
  // when it is exported. For now, just wipe it.
  drush_shell_exec("sed -i -e '/<@@ADCP_CONF@@>/,/<.@@ADCP_CONF@@>/d' " . escapeshellarg($settings_path));

  // This is needed for D8 Lightning support
  $distro = exec("/usr/local/bin/ah-detect-drupal-distro {$docroot} 2>&1", $distro_out, $ret);
  if ($ret != 0) {
    drush_log(dt('@site: ah-detect-drupal-distro failed with error code @exit_code; output: @output', $dt_args + array('@exit_code' => $ret, '@output' => print_r($distro_out))), 'warn');
  } else if (isset($distro) && $distro != '') {
    drush_log(dt('@site: ah-detect-drupal-distro detected @distro distribution', $dt_args + array('@distro' => $distro)), 'info');
    $settings_include = "
// Initialize install_profile to the Drupal distribution name detected by the Acquia Cloud installation process.
if (class_exists('Drupal') && defined('Drupal::CORE_COMPATIBILITY') && Drupal::CORE_COMPATIBILITY == '8.x') {
 if (!isset(\$settings['install_profile'])) {
    \$settings['install_profile'] = '{$distro}';
  }
}
";
  }

  // We have a settings.php now. Strip out any previous include file (in case
  // this archive was exported from AH), and add our include file.
  drush_log(dt('@site: Updating settings.php', $dt_args), 'ok');
  system("sed -i -e '/^\\s*require\\s*(\\s*.\\/var\\/www\\/site-php\\//d' " . escapeshellarg($settings_path), $ret);
  if ($ret != 0) {
    drush_log(dt('@site: cannot run sed -i on @settings_path: status @ret', $dt_args + array('@ret' => $ret, '@settings_path' => $settings_path)), 'warn');
  }
  $sitegroup = file_get_contents("/var/www/site-php/$site/ah-site-group");
  $primary_role = _drush_ah_get_primary_role_name($sitegroup, $env);
  // Be careful writing anything after this line. It will re-initialize
  // $settings and leave you scratching your head.
  $settings_include .= "

// On Acquia Cloud, this include file configures Drupal to use the correct
// database in each site environment (Dev, Stage, or Prod). To use this
// settings.php for development on your local workstation, set \$db_url
// (Drupal 5 or 6) or \$databases (Drupal 7 or 8) as described in comments above.
if (file_exists('/var/www/site-php')) {
  require('/var/www/site-php/{$sitegroup}/{$primary_role}-settings.inc');
}
";



  if (file_put_contents($settings_path, $settings_include, FILE_APPEND) === FALSE) {
    drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot append to settings file @settings_path', $dt_args + array('@settings_path' => $settings_path)));
    return;
  }

  if ($vcs_type == 'svn') {
    // Remove the old docroot directory.
    drush_log(dt('@site: Removing old docroot', $dt_args), 'ok');
    $svn_url = file_get_contents("/var/www/site-php/{$site}/svn-url");
    $svn_creds = file_get_contents("/var/www/site-php/{$site}/svn-creds");
    system("svn {$svn_creds} ls {$svn_url}/docroot > /dev/null 2>1", $ret);
    // Continue with the remove only if the docroot exists
    if ($ret == 0) {
      system("svn {$svn_creds} remove {$svn_url}/docroot -m 'Preparing for site import.'", $ret);
      if ($ret != 0) {
        drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot svn remove @svn_url/docroot: status @ret', $dt_args + array('@ret' => $ret, '@svn_url' => $svn_url)));
        return;
      }
    }
    else {
      drush_log(dt('@site: svn path @svn_url/docroot does not exist.', $dt_args + array('@svn_url' => $svn_url)), 'ok');
    }
  }
  elseif ($vcs_type == 'git') {
    // make a temporary directory to import the code into
    // TODO: check errors
    $git_wd = "/mnt/tmp/{$site}/site-import-git-wd";
    system("rm -rf $git_wd && mkdir -p $git_wd && chmod 700 $git_wd", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot prepare git import directory @dir: status @ret', $dt_args + array('@ret' => $ret, '@dir' => $git_wd)));
      return;
    }

    drush_log(dt('@site: Cloning existing repo', $dt_args), 'ok');
    $ret = drush_shell_exec("$git_rem_cmd clone $git_ssh_url $git_wd -b $git_current_branch");
    if (!$ret) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot git clone @url', array('@url' => $git_ssh_url) + $dt_args));
      return;
    }
    drush_shell_cd_and_exec($git_wd, "git config user.email 'support@acquia.com'");
    drush_shell_cd_and_exec($git_wd, "git config user.name 'Acquia Cloud'");

    // We're going to git rm the head of a branch. Unlike with svn, git might
    // eventually GC some/all of those files, removing them forever. To
    // prevent that, make a uniquely-named tag that points to the end of the
    // current branch.
    drush_log(dt('@site: Finding an available pre-import tag name', $dt_args), 'ok');
    $git_preserving_tag_base = 'pre-import-' . strftime('%Y-%m-%d');
    $git_preserving_tag = $git_preserving_tag_base;
    $i = 0;
    while (_ah_git_tag_exists($git_rem_cmd, $git_ssh_url, $git_preserving_tag)) {
      $git_preserving_tag = $git_preserving_tag_base . ".$i";
      $i++;
    }
    drush_log(dt('@site: Creating pre-import tag @tag', $dt_args + array('@tag' => $git_preserving_tag)), 'ok');
    $ret = drush_shell_cd_and_exec($git_wd, "git tag -a -m 'Acquia Cloud auto-generated tag to preserve contents from before an import.' $git_preserving_tag $git_current_branch");
    if (!$ret) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot git tag @tag @branch', array('@tag' => $git_preserving_tag, '@branch' => $git_current_branch) + $dt_args));
      return;
    }

    if (file_exists("$git_wd/docroot")) {
      drush_log(dt('@site: Removing docroot', $dt_args), 'ok');
      $ret = drush_shell_cd_and_exec($git_wd, "git rm -r --force --quiet $git_wd/docroot");
      if (!$ret) {
        drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot git rm @wd/docroot', array('@wd' => $git_wd) + $dt_args));
        return;
      }
    }
    else {
      drush_log(dt('@site: @wd/docroot does not exist, skipping remove', array('@wd' => $git_wd) + $dt_args), 'warning');
    }

    system("mkdir -p {$git_wd}/docroot", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: could not mkdir new docroot dir @git_wd/docroot in @git_url checkout: status @ret', $dt_args + array('@ret' => $ret, '@git_url' => $git_ssh_url, '@git_wd' => $git_wd)));
      return;
    }
  }

  // Find the database import file. If there isn't one, just skip it.
  $db_paths = glob("$extractdir/*.sql");
  if (realpath($extractdir) != realpath($docroot)) {
    $db_paths = array_merge($db_paths, glob("$docroot/*.sql"));
  }
  if (count($db_paths) > 1) {
    drush_set_error('AH_SITE_ARCHIVE', dt('@site: @count database .sql files found in @archive_path (@list); only one is currently supported', $dt_args + array('@count' => count($db_paths), '@list' => implode(', ', $db_paths))));
    ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
    return;
  }
  else if (count($db_paths) == 1) {
    $db_path = $db_paths[0];
    drush_log(dt('@site: Importing SQL dump file @file', array('@file' => $db_path) + $dt_args), 'ok');
    drush_set_context('DRUSH_AFFIRMATIVE', TRUE);
    drush_set_option('drop', TRUE);
    drush_invoke_process("@{$sitegroup}.{$env}", 'ah-db-import', array($db_path, '--drop'));
    if (drush_get_error()) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot import @db_path: status @ret', $dt_args + array('@db_path' => $db_path, '@ret' => $ret)));
      ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
      return;
    }
    unlink($db_path);
  }
  else {
    drush_log(dt('@site: No SQL dump file found; run install.php when import is finished.', $dt_args), 'ok');
    drush_set_context('DRUSH_AFFIRMATIVE', TRUE);
    drush_set_option('drop', TRUE);
    drush_invoke_process("@{$sitegroup}.{$env}", 'ah-db-import', array('/dev/null', '--drop'));
    if (drush_get_error()) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot empty database: status @ret', $dt_args + array('@ret' => $ret)));
      return;
    }
  }

  // If we found an import sites directory and it has a files dir, import it
  // into $site_dir/files (note that $site_dir is ALWAYS sites/default, but is
  // the full local path, not just the relative path).
  if (!empty($import_site_dir) && is_dir("$import_site_dir/files")) {
    $import_files_path = "$import_site_dir/files";
    drush_log(dt('@site: Importing files from @dir', $dt_args + array('@dir' => $import_files_path)), 'ok');
    $local_files_path = "/mnt/gfs/{$site}/sites/default/files";
    if (! is_dir($local_files_path)) {
      system("mkdir -p $local_files_path", $ret);
      if ($ret != 0) {
        drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot create local files path @local_files_path: status @ret', $dt_args + array('@local_files_path' => $local_files_path, '@ret' => $ret)));
        return;
      }
    }
    system("rsync -a --no-perms --no-group --chmod=ugo=rwX " . escapeshellarg($import_files_path) . "/ $local_files_path", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot import @files_path: status @ret', $dt_args + array('@files_path' => $import_files_path, '@ret' => $ret)));
      return;
    }
    exec("rm -rf " . escapeshellarg($import_files_path));
  }
  else {
    drush_log(dt('@site: no files imported', $dt_args), 'ok');
  }

  drush_log(dt('@site: Importing new docroot', $dt_args), 'ok');
  if ($vcs_type == 'svn') {
    # Store the time immediately before we push to the repo for use in determining
    # when a new code deploy has taken place.
    $pre_deploy_time = time();

    system("svn {$svn_creds} import --non-interactive -q {$docroot} {$svn_url}/docroot -m 'Site import.'", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot svn import @svn_url/docroot: status @ret', $dt_args + array('@ret' => $ret, '@svn_url' => $svn_url)));
      return;
    }
  }
  elseif ($vcs_type == 'git') {
    // Drupal 7 has a .gitignore file that includes sites/*/settings*.php. In
    // most environments that might be right, but in AH with the settings
    // include file we need it committed.
    // We edit the .gitignore file before it is committed via git
    $git_ignore = glob("$docroot/.gitignore");
    if (count($git_ignore) == 1) {
      drush_log(dt('@site: stripping the settings.php line from the docroot/.gitnore file', $dt_args), 'warning');
      system("sed -i -e '/settings[\*]*.php/d' " . escapeshellarg($docroot) . "/.gitignore", $ret);
      if ($ret != 0) {
        drush_set_error('AH_SITE_ARCHIVE', dt('@site: failed to edit @docroot/.gitignore: status @ret', $dt_args + array('@ret' => $ret, '@docroot' => $docroot)));
        return;
      }
    }

    // We cannot import directly into /docroot in a git repo, so copy the
    // extracted docroot into the Git repo that we have already checked out.
    system("rsync -a --exclude=.git --exclude=.svn " . escapeshellarg($docroot) . "/ {$git_wd}/docroot", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: failed to copy extracted docroot into @git_wd/docroot: status @ret', $dt_args + array('@ret' => $ret, '@git_wd' => $git_wd)));
      return;
    }

    system("cd {$git_wd} && git add --force docroot", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: failed to git-add new docroot @git_wd/docroot: status @ret', $dt_args + array('@ret' => $ret, '@git_wd' => $git_wd)));
      return;
    }

    system("cd $git_wd && git commit --allow-empty -a -m 'Importing site archive'", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: git commit of site import failed: status @ret', $dt_args + array('@ret' => $ret)));
      return;
    }

    # Store the time immediately before we push to the repo for use in determining
    # when a new code deploy has taken place.
    $pre_deploy_time = time();

    system("cd $git_wd && $git_rem_cmd push", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: git push failed to URL @git_url: status @ret', $dt_args + array('@ret' => $ret, '@git_url' => $git_ssh_url)));
      return;
    }
    system("cd $git_wd && $git_rem_cmd push --tags", $ret);
    if ($ret != 0) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: git push --tags failed to URL @git_url: status @ret', $dt_args + array('@ret' => $ret, '@git_url' => $git_ssh_url)));
      return;
    }
    // direct the caller to the newly imported branch
    drush_log(dt('@site: Code imported into the @branchname branch of the Git repository.', $dt_args + array('@branchname' => $git_current_branch)), 'ok');
  }

  // Wait for new site docroot to be deployed.
  if (!_drush_ah_wait_site_update("$sitegroup.$env", $site, $pre_deploy_time, 600, 5)) {
    return;
  }

  // Set file_public_path correctly.
  //
  // TODO: We should probably always set this to the path where we
  // actually installed the files since that is where we know they
  // are. Not sure yet, though. Also, file_public_path is the D7 name,
  // do the D6 name and private path too.
  //
  // For now, Gardens exports are always wrong so we have to do something.
  //
  // Even with the wait for code deploy, it seems possible for this to
  // run before the site is ready. My guess is that fields-config-web
  // can be in the middle of a run when svn changes so it touches
  // site_update after the commit. To handle this, retry a few times.
  if (isset($db_path)) {

    for ($i = 0; $i < 6; $i++) {
      $major_version = drush_drupal_major_version("/var/www/html/$site/docroot");
      if ($major_version != FALSE) {
        break;
      }
      drush_log(dt('@site: Waiting for site to come online', $dt_args), 'ok');
      sleep(30);
    }
    if ($major_version == FALSE) {
      drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot determine drupal major version', $dt_args));
      ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
      return;
    }

    // Only set file_public_path for D6/D7 sites
    if ($major_version < 8) {
      for ($i = 0; $i < 6; $i++) {
        system("drush -q -r /var/www/html/$site/docroot vset --always-set file_public_path sites/default/files", $ret);
        if ($ret == 0) {
          break;
        }
        drush_log(dt('@site: Waiting for site to come online', $dt_args), 'ok');
        sleep(10);
      }
      if ($ret != 0) {
        drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot drush vset files_public_path: status @ret', $dt_args + array('@ret' => $ret)));
        ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
        return;
      }
    }

    // Clear/rebuild Drupal cache.
    if ($major_version >= 8) {
      system("/usr/local/sbin/timelimit.sh 60 drush8 -q -r /var/www/html/$site/docroot cr", $ret);
    }
    else {
      system("/usr/local/sbin/timelimit.sh 60 drush6 -q -r /var/www/html/$site/docroot cc all", $ret);
    }

    if ($ret != 0) {
      if ($ret == 137) {
        drush_set_error('AH_SITE_ARCHIVE', dt('@site: Timed out clearing cache. This usually indicates that importing the archive put you over quota', $dt_args + array('@ret' => $ret)));
      }
      else {
        drush_set_error('AH_SITE_ARCHIVE', dt('@site: cannot clear cache: status @ret', $dt_args + array('@ret' => $ret)));
      }
      ah_drush_exit_with_status(AH_USER_ERROR_STATUS);
      return;
    }
  }

  drush_log(dt('@site: Imported @archive_path', $dt_args), 'ok');
}

/**
 * Consult the specified options and return the list of tables
 * specified.
 *
 * Copied from Drush 7 because this does not exist in Drush 3/4/5.
 * https://github.com/drush-ops/drush/blob/4ea18c76eab3d05e4df537dd0164766c6911a89f/commands/sql/sql.drush.inc#L444-L481
 *
 * @param option_name
 *   The option name to check: skip-tables, structure-tables
 *   or tables.  This function will check both *-key and *-list,
 *   and, in the case of sql-sync, will also check target-*
 *   and source-*, to see if an alias set one of these options.
 * @returns array
 *   Returns an array of tables based on the first option
 *   found, or an empty array if there were no matches.
 */
function _drush_ah_sql_get_raw_table_list($option_name) {
  foreach(array('' => 'cli', 'target-,,source-' => NULL) as $prefix_list => $context) {
    foreach(explode(',',$prefix_list) as $prefix) {
      $key_list = drush_get_option($prefix . $option_name . '-key', NULL, $context);
      drush_log($key_list);
      foreach(explode(',', $key_list) as $key) {
        $all_tables = drush_get_option($option_name, array());
        if (array_key_exists($key, $all_tables)) {
          return $all_tables[$key];
        }
        if ($option_name != 'tables') {
          $all_tables = drush_get_option('tables', array());
          if (array_key_exists($key, $all_tables)) {
            return $all_tables[$key];
          }
        }
      }
      $table_list = drush_get_option($prefix . $option_name . '-list', NULL, $context);
      if (isset($table_list)) {
        return empty($table_list) ? array() : explode(',', $table_list);
      }
    }
  }

  return array();
}

/**
 * Command callback.
 */
function drush_ah_db_backup($site, $roles = '*') {

  // Setup DNS resolver.
  require_once("/usr/share/php/Net/DNS2_wrapper.php");
  $resolver = new Net_DNS2_Resolver(array('nameservers' => array('127.0.0.1', 'dns-master')));

  global $conf;
  $conf['acquia_use_early_cache'] = TRUE;
  $date = strftime('%Y-%m-%d');
  $vars = array('@site' => $site);

  $retain = drush_get_option('retain');
  if (empty($retain) || !is_numeric($retain) || $retain < 1) {
    $retain = 3;
  }

  $structure_tables = _drush_ah_sql_get_raw_table_list('structure-tables');
  $skip_tables = _drush_ah_sql_get_raw_table_list('skip-tables');
  $skip_tables = array_merge($structure_tables, $skip_tables);

  // TODO: This clearly belongs somewhere other than this
  // command. Move it.
  $use_syslog = drush_get_option('ah-syslog') || !posix_isatty(STDERR);
  if ($use_syslog) {
    if (posix_isatty(STDERR)) {
      $opts = LOG_PERROR|LOG_PID;
    }
    else {
      $opts = LOG_PID;
    }
    openlog('drush::ah-db-backup', $opts, LOG_DAEMON);
    drush_set_context('DRUSH_LOG_CALLBACK', '_ah_syslog');
  }

  // Each site is/has exactly one stage (stored in the ah-site-stage
  // file), and each site dir only stores include files for one stage.
  $stage = file_get_contents("/var/www/site-php/{$site}/ah-site-stage");

  if ($roles == '*') {
    $roles = array();
    // The old include files still exist. If we glob for
    // D6-*-settings.inc, we'll make two backups: <dbname>.sql.gz and
    // <stage>-<dbname>.sql.gz. So, only glob for the new-style files,
    // ensuring we only get one per database.
    $paths = glob("/var/www/site-php/{$site}/D6-{$stage}-*-settings.inc");
    foreach ($paths as $path) {
      if (preg_match('@D6-'.$stage.'-([^-]+)-settings.inc$@', $path, $m)) {
        $roles[] = $m[1];
      }
    }
    $roles = implode(',', $roles);
  }

  foreach (explode(',', $roles) as $role) {
    # Determine if database dump was successful.
    $success = TRUE;
    unset($db_passive);

    require("/var/www/site-php/{$site}/D6-{$stage}-{$role}-settings.inc");
    $db = $conf['acquia_hosting_site_info']['db']['name'];
    $db_urls = $conf['acquia_hosting_site_info']['db']['db_url_ha'];

    // If there is only one db server, use that as the "passive".
    if (count($db_urls) == 1) {
      $db_passive = reset($db_urls);
    }
    else {
      try {
        $response = $resolver->query("cluster-{$conf['acquia_hosting_site_info']['db']['db_cluster_id']}.mysql.", 'CNAME');
        $db_active = empty($response->answer[0]->cname) ? reset($db_urls) : $response->answer[0]->cname;
        foreach ($db_urls as $db_url) {
          if (strpos($db_url, $db_active) === FALSE) {
            $db_passive = $db_url;
            break;
          }
        }
      }
      catch (Net_DNS2_Exception $e) {
      }
      if (!isset($db_passive)) {
        // Fall back, if there are two $db_urls, pick the second one.
        $db_active = reset($db_urls);
        $db_passive = next($db_urls);
        // This might not be a good backup. Mark as unsuccessful.
        drush_log(dt('No passive database server found!'), 'warning');
        $success = FALSE;
      }
    }

    $db_info = drush_convert_db_from_db_url($db_passive);

    $vars = array('@site' => $site, '@role' => $role, '@db' => $db, '@server' => $db_info['host']);
    drush_log(dt('@site:@role:@db: backing up from @server', $vars), 'ok');

    if (count($db_urls) > 1) {
      try {
        // Find the slave lag for this host.
        $response = $resolver->query("lag.{$db_info['host']}.cluster-{$conf['acquia_hosting_site_info']['db']['db_cluster_id']}.mysql.", 'TXT');
        $slave_lag_value = isset($response->answer[0]->text[0]) ? $response->answer[0]->text[0] : "";

        // Determine metadata for the db dump
        if (is_numeric($slave_lag_value)) {
          $est_slave_timestamp = time() - $slave_lag_value;
          $est_slave_time = date('Y-m-d H:i:s', $est_slave_timestamp) + ' UTC';
        }
        else {
          $est_slave_timestamp = 'Unknown';
          $est_slave_time = 'Unknown UTC';
        }

        // If slave lag is NULL or greater than 24hrs, log an error and mark
        // the backup as FAILED because this cannot be considered a "daily"
        // backup.
        if ($slave_lag_value == "NULL" || (is_numeric($slave_lag_value) && (intval($slave_lag_value) > 60 * 60 * 24))) {
          drush_log(dt('Bad slave lag: {@response_val}', array('@response_val' => $slave_lag_value)), 'error');
          $success = FALSE;
        }
        // If slave lag is known and greater than 15m (but less than or equal
        // to 24hrs), log a warning and continue as this can be considered a
        // "daily" backup.
        elseif (is_numeric($slave_lag_value) && (intval($slave_lag_value) > 60 * 15)) {
          // Slave lag was too high or NULL. Mark this as potentially bad.
          drush_log(dt('High slave lag: {@response_val}', array('@response_val' => $slave_lag_value)), 'warning');
        }
      }
      catch (Net_DNS2_Exception $e) {
        // DNS failed to resolve.  Mark this as potentially bad.
        drush_log(dt('DNS failed to resolve! - ' . $e->getMessage()), 'warning');
        $success = FALSE;
      }
    }

    // Set the path for success and failure.
    if ($success) {
      $dumppath = "/mnt/gfs/{$site}/backups/{$stage}-{$role}-{$db}-{$date}.sql.gz";
    }
    else {
      // Probably want a better name than FAILED?
      $dumppath = "/mnt/gfs/{$site}/backups/FAILED-{$stage}-{$role}-{$db}-{$date}.sql.gz";
      drush_set_error('AH_BACKUP', dt('AH_SERIOUS_DB_BACKUP: @site:@role:@db: Possible inconsistent backup taken.', $vars));
    }

    $dotdumppath = "/mnt/gfs/{$site}/backups/.{$stage}-{$role}-{$db}-{$date}.sql.gz";
    $defaults_file = _ah_create_mysql_defaults_file($db_info);
    $opts = "--defaults-extra-file={$defaults_file} --single-transaction --ignore-create-error";
    $slave_lag_metadata = "-- Reported slave lag at backup: {$slave_lag_value}\n-- Estimated db timestamp: {$est_slave_timestamp} (that is {$est_slave_time}, for Pim)\n";

    try {
      $mysql_dump_cmd = "/bin/bash -c 'set -o pipefail; (echo \"${slave_lag_metadata}\"; mysqldump $opts {$db_info['database']}";
      // Append the ignore-table options.
      foreach ($skip_tables as $table) {
        $mysql_dump_cmd .= " --ignore-table={$db_info['database']}.$table";
      }
      // Run mysqldump again if we need some structure-only tables.
      if (!empty($structure_tables)) {
        $mysql_dump_cmd .= "; mysqldump $opts --no-data {$db_info['database']} " . implode(' ', $structure_tables);
      }
      $mysql_dump_cmd .= ") | gzip > {$dotdumppath}'";
      _ah_exec_db_backup_cmd($mysql_dump_cmd, $vars);

      $mv_cmd = "mv $dotdumppath $dumppath && chmod 440 $dumppath";
      _ah_exec_db_backup_cmd($mv_cmd, $vars);

      // Tail outputs lines starting with the Nth so add another 1 to retain.
      $tail = $retain + 1;
      $out = _ah_exec_db_backup_cmd("/bin/bash -c 'set -o pipefail; if [ `find /mnt/gfs/{$site}/backups/ -iname \*-{$db}-\*.sql.gz | grep -v 'FAILED-' | wc -l` -ne 0 ]; then ls -r /mnt/gfs/{$site}/backups/*-{$db}-*.sql.gz | grep -v 'FAILED' | tail -n +{$tail} | xargs --no-run-if-empty rm -v; fi'", $vars);
      // Log the output of the last command (cleaning old backups).
      drush_log(dt('@site:@role:@db: @out', $vars + array('@out' => implode("\n", $out))), 'ok');
      $out = _ah_exec_db_backup_cmd("/bin/bash -c 'ls -r /mnt/gfs/{$site}/backups/ | grep '.*-{$db}-.*.sql.gz' | grep -v 'FAILED-' | wc -l'", $vars);
      $failed_to_keep = $tail - intval($out[0]);
      // This should never happen.
      if ($failed_to_keep < 1) {
        $failed_to_keep = 1;
      }
      // Always keep one failed backup if the latest dump failed.
      if ((!$success) && $failed_to_keep == 1) {
        $failed_to_keep++;
      }
      // Clean up failed backups.  Only keep the latest failure or as many
      // failures as nessessary based on the retain value (if we don't have
      // that many successful dumps).
      _ah_exec_db_backup_cmd("bash -c 'set -o pipefail; if [ `find /mnt/gfs/{$site}/backups/ -iname FAILED-\*\-{$db}\-* | wc -l` -ne 0 ]; then ls -r /mnt/gfs/{$site}/backups/FAILED-*-{$db}-*.sql.gz | tail -n +{$failed_to_keep} | xargs --no-run-if-empty rm -v; fi;'", $vars);
    }
    catch (Exception $e) {
      drush_set_error('AH_BACKUP', $e->getMessage());
    }
    unlink($defaults_file);
  }

  $out = array();
  exec("bash -c 'ls -l /mnt/gfs/{$site}/backups/*-{$db}-*.sql.gz' 2>&1", $out, $ret);
  if ($ret != 0) {
    drush_set_error('AH_BACKUP', dt('AH_SERIOUS_DB_BACKUP: @site: failed to list new backups: @ret, @out', $vars + array('@ret' => $ret, '@out' => implode("\n", $out))));
  }
  else {
    drush_log(dt('@site: backup files: @out', $vars + array('@ret' => $ret, '@out' => implode("\n", $out))), 'ok');
  }

  // No matter what, always clean up extraneous dot-dumpfiles.
  exec("rm -f /mnt/gfs/{$site}/backups/.{$db}-*.sql.gz", $out, $ret);
}

function _ah_exec_db_backup_cmd($cmd, $vars) {
  $out = array();
  exec($cmd . " 2>&1", $out, $ret);
  if ($ret != 0) {
    throw new Exception(dt('AH_SERIOUS_DB_BACKUP: @site:@role:@db: @cmd failed with status @ret: @out', $vars + array('@cmd' => $cmd, '@ret' => $ret, '@out' => implode("\n", $out))));
  }
  return $out;
}

// TODO: This clearly belongs somewhere other than this
// command. Move it.
function _ah_syslog($entry) {
  $verbose = drush_get_context('DRUSH_VERBOSE');
  $debug = drush_get_context('DRUSH_DEBUG');

  switch ($entry['type']) {
    case 'ok' :
    case 'completed' :
    case 'success' :
      $level = LOG_NOTICE;
      break;
    case 'notice' :
    case 'message' :
    case 'info' :
      if (!$verbose) {
        // print nothing
        return TRUE;
      }
      $level = LOG_NOTICE;
      break;
    case 'warning' :
    case 'failed' :
    case 'error' :
    default :
      $level = LOG_WARNING;
      break;
  }

  if ($debug) {
    $timer = sprintf('[%s sec, %s]', round($entry['timestamp']-DRUSH_REQUEST_TIME, 2), drush_format_size($entry['memory']));
    $entry['message'] = $entry['message'] . ' ' . $timer;
  }

  syslog($level, $entry['message']);
}

function _ah_create_mysql_defaults_file($db_info) {
  $keymap = array(
    'driver' => NULL,
    'database' => NULL,
    'username' => 'user',
  );
  $defaults_file = tempnam("/mnt/tmp", 'db-defaults');
  chmod($defaults_file, 0600);
  $defaults = array('[client]');
  foreach ($db_info as $k => $v) {
    if (array_key_exists($k, $keymap)) {
      $k = $keymap[$k];
    }
    if (isset($k) && !empty($v)) {
      $defaults[] = "$k=$v";
    }
  }
  file_put_contents($defaults_file, implode("\n", $defaults)."\n");
  return $defaults_file;
}

/**
 * Retrieve the MySQL instance name for a given database role in a specific site.env
 *
 * @param $sitegroup
 *   The sitegroup for which we want to get the database role.
 * @param $env
 *   The name of the environment for the site.
 * @param $role_name
 *   The user specified database name.
 * @return
 *   The Acquia Hosting database name for $role_name
 */
function _drush_ah_get_db_name($sitegroup, $env, $role_name) {
  $site = file_get_contents("/var/www/site-php/$sitegroup/.$env");
  if (empty($site)) {
    drush_set_error('AH_GET_DB_NAME', dt('cannot determine internal site name for @sitegroup.@env', array('@sitegroup' => $sitegroup, '@env' => $env)));
    return NULL;
  }

  $conf['acquia_use_early_cache'] = TRUE;
  require("/var/www/site-php/{$site}/D6-{$env}-{$role_name}-settings.inc");

  return $conf['acquia_hosting_site_info']['db']['name'];
}

/**
 * Retrieve the primary database role for an environment. The primary
 * role named used to be the sitegroup, but now it is 'primary'.
 */
function _drush_ah_get_primary_role_name($sitegroup, $env) {
  return file_exists("/var/www/site-php/$sitegroup/primary-settings.inc") ? 'primary' : $sitegroup;
}

/**
 * Command callback
 *
 * This is a drush warpper for the ah-import-db.php script.
 */
function drush_ah_db_import($filepath='' ) {
  $sitegroup = drush_get_option('site', '');
  $env = drush_get_option('env', '');
  $drop = drush_get_option('drop');
  $role_name = drush_get_option('db');
  $force = drush_get_option('force');
  $myisam_tables = drush_get_option('myisam', NULL);
  $myisam_str = "";
  $site = file_get_contents("/var/www/site-php/$sitegroup/.$env");
  $drop_str = "";
  $force_str = "";
  if (empty($sitegroup) || empty($env)) {
    drush_set_error('AH_GET_DB_NAME', dt('--site and --env options are required; use the drush @sitename.env alias to specify them.'));
    return;
  }
  if (empty($role_name)) {
    $role_name = _drush_ah_get_primary_role_name($sitegroup, $env);
    drush_set_option('db', $role_name);
  }

  // If the sitegroup or env is invalid, then get_db_name will throw an error for us
  $db_name = _drush_ah_get_db_name($sitegroup, $env, $role_name);
  if (empty($db_name)) {
    drush_set_error('AH_DB_IMPORT', "Cannot determine database name. Specify the correct one with --db");
    return;
  }

  // Create a seperate --myisam option for every table
  if (isset($myisam_tables)) {
    // Ensure that the --myisam option has some tables listed and is not incorrectly used
    if (!empty($myisam_tables)) {
      foreach (explode(',', $myisam_tables) as $table_name) {
        $myisam_str .= "--myisam " . $table_name . " ";
      }
    }
    else {
      drush_set_error('AH_DB_IMPORT', dt('--myisam needs tables to be speicified.'));
      return;
    }
  }

  if (drush_get_option('simulate')) {
    drush_set_error('AH_DB_IMPORT', "--simulate is not supported.");
    return;
  }

  if (empty($filepath)){
    drush_set_error('AH_DB_IMPORT', "File is a required argument.");
    return;
  }

  if (!file_exists($filepath)) {
    drush_set_error('AH_DB_IMPORT', dt("@site: File @file does not exist.", array('@site' => $sitegroup, '@file' => $filepath)));
    return;
  }

  if (!is_readable($filepath)) {
    drush_set_error('AH_DB_IMPORT', dt("@site: File @file is not readable.", array('@site' => $sitegroup, '@file' => $filepath)));
    return;
  }

  if (!empty($drop)) {
    $drop_str = "--drop";
  }
  if (!empty($force)) {
    $force_str = "--force";
  }
  // We pass control to the ah-import-db script so that it can display it's output.
  system("ah-import-db --site {$site} --db {$db_name} {$drop_str} {$force_str} {$myisam_str} {$filepath}", $ret);

  if ($ret != 0) {
    if (empty($drop) && empty($force)) {
      drush_set_error('AH_DB_IMPORT', dt('@site: cannot import database: status @ret. Try using --drop or --force.', array('@site' => $sitegroup, '@db_path' => $db_path, '@ret' => $ret)));
      return;
    }
    if (!empty($drop) && $filepath == '/dev/null') {
      drush_set_error('AH_DB_IMPORT', dt('@site: cannot empty database: status @ret', array('@site' => $sitegroup, '@ret' => $ret)));
      return;
    }
    drush_set_error('AH_DB_IMPORT', dt('@site: cannot import @db_path: status @ret', array('@site' => $sitegroup, '@db_path' => $filepath, '@ret' => $ret)));
    return;
  }
  drush_log('Imported into database successfully', 'ok');
}
