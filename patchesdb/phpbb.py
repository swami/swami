#!/usr/bin/python
#
# phpbb.py - Functions for integration with phpBB forums database.
#
# PatchesDB
# Copyright (C) 2005-2007 Josh Green <jgreen@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License only.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA or point your web browser to http://www.gnu.org.

import subprocess
import InstDB

def escape_phpstr (str):
  """Escape PHP chars for single quoted strings"""

  str = str.replace ("\\", "\\\\")
  str = str.replace ("'", "\\'")

  return str


def add_user (login, password, email):
  """Add user to PHPBB forums database"""

  # PHP code to add a user to PHPBB
  script = """
define('IN_PHPBB', true);
$phpEx = "php";

$phpbb_root_path = '""" + escape_phpstr (InstDB.PHPBBPath) + """/';
include($phpbb_root_path . 'common.php');

// Start session management
$user->session_begin();   
$auth->acl($user->data);
$user->setup();

require($phpbb_root_path .'includes/functions_user.php');

$user_row = array(
  'username'              => '""" + escape_phpstr (login) + """',
  'user_password'         => phpbb_hash ('""" + escape_phpstr (password) + """'),
  'user_email'            => '""" + escape_phpstr (email) + """',
  'group_id'              => 2,
  'user_timezone'         => -8.0,
  'user_dst'              => 0,
  'user_lang'             => 'en',
  'user_type'             => USER_INACTIVE,
  'user_actkey'           => '',
  'user_regdate'          => time()
);

// Add the user and return status code as appropriate
if (user_add($user_row)) exit (0);
else exit (1);
"""

  return subprocess.call ([InstDB.PHP_CMD, '-r', script])


def activate_user (login):
  """Activate user in PHPBB forums database"""

  # PHP code to activate a PHPBB user account
  script = """
define('IN_PHPBB', true);
$phpEx = "php";

$phpbb_root_path = '""" + escape_phpstr (InstDB.PHPBBPath) + """/';
include($phpbb_root_path . 'common.php');

// Start session management
$user->session_begin();   
$auth->acl($user->data);
$user->setup();

require($phpbb_root_path .'includes/functions_user.php');

$user_id_ary = false;
$username = '""" + escape_phpstr (login) + """';

// Get user ID by username
$result = user_get_id_name ($user_id_ary, $username);
if ($result != false) exit (1);

// Activate the user
user_active_flip ('activate', $user_id_ary);
exit (0);
"""

  return subprocess.call ([InstDB.PHP_CMD, '-r', script])
