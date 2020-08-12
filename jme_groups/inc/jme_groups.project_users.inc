<?php
/**
 * function jme_groups_project_user_mgmt_form
 * custom form to manage group membership
 * @param $form
 * @param $form_state
 * @return $form object
 */
function jme_groups_project_user_mgmt_form($form, &$form_state) {
  // Prepare gid info
  $gids = _getProjectJmeGids ();

  $jme_gid = $gids [0];
  $pr_gid = $gids [1];

  $jme_members = _get_users_in_group ( $jme_gid );
  $pr_members = array_keys ( _get_users_in_group ( $pr_gid ) );

  $form ['title'] = array (
      '#type' => 'markup',
      '#markup' => t( '<h2 class="project-users-header pane-title">Add / Remove Project Users</h2>' )
  );

  $form ['jmegid'] = array (
      '#type' => 'hidden',
      '#value' => $jme_gid
  );

  $form ['prgid'] = array (
      '#type' => 'hidden',
      '#value' => $pr_gid
  );

  $form ['memberselector'] = array (
      '#type' => 'select',
      '#options' => $jme_members,
      '#default_value' => $pr_members,
      '#multiple' => TRUE,
      '#size' => 25,
      '#attributes' => array (
          'class' => array (
              'jme-multiselect'
          )
      ),
      '#required' => FALSE
  );
  $form ['actions'] = array (
      '#type' => 'actions'
  );
  $form ['actions'] ['reset'] = array (
      '#type' => 'button',
      '#button_type' => 'button',
      '#value' => 'Cancel'
  );
  $form ['actions'] ['submit'] = array (
      '#type' => 'submit',
      '#value' => 'Save Changes'
  );

  return $form;
}

/**
 * function jme_groups_project_user_mgmt_form_submit
 * Custom submit function for jme_groups_project_user_mgmt_form
 * @param $form
 * @param $form_state
 * @return void
 */
function jme_groups_project_user_mgmt_form_submit($form, &$form_state) {
  $redirectto = request_uri ();
  $redirectto = substr ( $redirectto, 1 );
  $jme_gid = $form_state ['values'] ['jmegid'];
  $pr_gid = $form_state ['values'] ['prgid'];
  $all_opts = array_keys ( $form_state ['complete form'] ['memberselector'] ['#options'] );

  $selected_opts = array_keys ( $form_state ['complete form'] ['memberselector'] ['#value'] );

  // Add new members from og_membership based on $selected_opts
  $users = user_load_multiple ( $all_opts );

  foreach ( $users as $user ) {
    if (in_array ( $user->uid, $selected_opts )) {
      $actiontype = 'add';
    } else {
      $actiontype = 'remove';
    }
    _members_update ( $actiontype, $pr_gid, $jme_gid, $user );
  }
  drupal_goto ( $redirectto );
}

/**
 * function _members_update
 * Update membership per jme_groups_project_user_mgmt_form settings
 * @param string $type
 * @param int $pr_gid
 * @param int $jme_gid
 * @param object $user
 * @return void
 */
function _members_update($type, $pr_gid, $jme_gid, $user) {
  $group_type = 'node'; // All our/my groups are of type 'node'
  $entity_type = 'user'; // All our/my members are type 'user'
  $values = array (
      'entity_type' => 'user',
      'entity' => $user,
      'field_name' => FALSE,
      'state' => 1
  );
  $return = FALSE;
  switch ($type) {
    case 'add' :
      $values ['state'] = OG_STATE_ACTIVE;
      og_group ( $group_type, $pr_gid, $values );
      grant_default_og_roles ( $jme_gid, $pr_gid, 'jme_project', $user->uid );
      break;
    case 'approve' :
      $values ['state'] = 1;
      og_group ( $group_type, $pr_gid, $values );
      break;
    case 'remove' :
      og_ungroup ( $group_type, $pr_gid, $entity_type, $user->uid );
      break;
    default :

      // $ret/urn = 'FALSE TYPE';
      break;
  }
  return $return;
}

/**
 * function grant_default_og_roles
 * Grant default roles to member in the selected group
 * @param  $jme_gid
 * @param [type] $pr_gid
 * @param [type] $group_node_type
 * @param [type] $uid
 * @return void
 */
function grant_default_og_roles($jme_gid, $pr_gid, $group_node_type, $uid) {
  // $uid = $user->uid;
  if ($group_node_type == 'eventroom') {
    // Get user's roles from global roles
    $user_roles = $user->roles;
    $gid = $jme_gid;
  } else {
    // Get user's roles from OG Roles
    $user_roles = og_get_user_roles ( 'node', $jme_gid, $uid, FALSE );
    // $user_roles = array_flip($user_roles);
    $gid = $pr_gid;
  }

  // Get list of available roles for current eventroom
  $group = entity_load_single ( 'node', $gid );
  list ( , , $bundle ) = entity_extract_ids ( 'node', $group );
  $og_roles = og_roles ( 'node', $bundle, $gid, FALSE, FALSE );
  // Flip the array so we can use the name to get the GID for the og_role_grant function below
  $og_roles = array_flip ( $og_roles );
  if ($user_roles) {
    foreach ( $user_roles as $key => $rolename ) {
      $rid = $og_roles [$rolename];
      og_role_grant ( 'node', $gid, $uid, $rid );
    }
  }
}

/**
 * function ajax_project_users_callback
 *
 * @param $form
 * @param $form_state
 * @return array
 */
function ajax_project_users_callback($form, $form_state) {
  $selector = '#append_div';

  $commands = array ();
  $commands [] = ajax_command_append ( $selector, "Stuff..." );
  $commands [] = ajax_command_replace ( "#append_status", "<div id='append_status'>Updated append_command_example " . date ( 'r' ) . "</div>" );
  return array (
      '#type' => 'ajax',
      '#commands' => $commands
  );

  $commands = array ();
  $comment_output = 'Yada-yada!!!';
  $comment_output = '<div id="test-2">' . $comment_output . '</div>';

  $commands [] = ajax_command_replace ( '#testing', $comment_output );

  return array (
      '#type' => 'ajax',
      '#commands' => $commands
  );
}

/**
 * function _getProjectJmeGids
 * Get list of gids for current eventroom and project for further processing
 * @return array
 */
function _getProjectJmeGids() {
  $url = explode ( "/", request_uri () );

  $eventroom = "eventroom1/" . $url [2];
  $gnode = drupal_lookup_path ( 'source', $eventroom );
  $jme_gid = str_replace ( 'node/', '', $gnode );

  $project = 'eventroom-project1/' . $url [4];
  $pnode = drupal_lookup_path ( 'source', $project );
  $pr_gid = str_replace ( 'node/', '', $pnode );
  $gids = array (
      $jme_gid,
      $pr_gid
  );

  return $gids;
}

 /**
  * function _get_users_in_group
  * Get list of users in a group
  * @param int $gid
  * @param boolean $keysonly
  * @return array
  */
function _get_users_in_group($gid, $keysonly = FALSE) {
  $query = db_select ( 'users', 'u' );
  $query->condition ( 'u.uid', 0, '<>' )->condition ( 'u.uid', 1, '<>' )->condition ( 'u.status', 1, '=' )->fields ( 'u', array (
      'uid',
      'name'
  ) )->join ( 'og_membership', 'ogm', "ogm.gid = :gid AND u.uid = ogm.etid AND ogm.entity_type = 'user'", array (
      ':gid' => $gid
  ) );

  $group_members = $query->execute ();
  $user_uids = array ();
  // // Loop through and get each user->uid
  foreach ( $group_members as $member ) {
    $user_uids [] = $member->uid;
  }
  if (count ( $user_uids ) == 0)
    return array ();
    // Load array of users objects
  $users = user_load_multiple ( $user_uids );

  // Prepare a list of members, including uid, firstname and lastname
  $memslist = array ();
  foreach ( $users as $user ) {
    $roles = $user->roles;
    $isadmin = in_array ( 'administrator', $roles );
    if (! $isadmin) {
      if ($keysonly == FALSE) {
        $memslist [$user->uid] = $user->field_name_first ['und'] [0] ['value'] . ' ' . $user->field_name_last ['und'] [0] ['value'];
      } else {
        $memslist [] = $user->uid;
      }
    }
  }
  // Return list of users
  return $memslist;
}