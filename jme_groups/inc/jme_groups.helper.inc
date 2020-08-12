<?php

/*
 * helper functions for managing permissions
 */

/**
 * function content_parent_nodes
 * Get the group nodes the parent groups of the provided $node
 * @param object $node
 * @return array
 */
function content_parent_nodes($node) {
  $groups = $node->og_group_ref ['und'];
  $parnodes = array ();
  if (count ( $groups )) {
    foreach ( $groups as $target => $nid ) {
      $target_id = $nid ['target_id'];
      $parnodes [$target_id] = node_load ( $target_id );
      // Get only parent group(S) that are project types
    }
  }
  return $parnodes;
}

/**
 * function content_parent_types
 * Get the content types of the parent groups of the provided $nodes
 * @param array $nodes
 * @param string $limit_type
 * @return void
 */
function content_parent_types(&$nodes, $limit_type = 'project') {
  $parent_types = array ();
  foreach ( $nodes as $nid => $node ) {
    if ($node->type == 'project' && $limit_type == 'project') {
      $parent_types [$node->nid] = $node->type;
    } else {
      $parent_types [$node->nid] = $node->type;
    }
  }
  return $parent_types;
}

// Check if user has permissions to delete
/**
 * function check_delete_perms
 *
 * @param int $nid
 * @param object $user
 * @return void
 */
function check_delete_perms($nid, $user) {
  $node = node_load ( $nid );
  if ($user->uid == $node->uid) {
    return true;
  } else {
    $reqd_roles = array (
        'administrator member',
        'manager'
    );
    // get user role in the group
    // see if we need to get the parent of $nid based on it's type
    switch ($node->type) {
      case 'project' :

        // if manager or admin, then return false
        $current_roles = og_get_user_roles ( 'node', $nid, $user->uid, FALSE );
        // Loop thru the roles and add them to the total list (dupes will be overwritten)
        foreach ( $current_roles as $rid => $role ) {
          $user_pr_roles [$rid] = $role;
        }
        // if user is NOT og manager or og admin AND not in current project, set limits
        if (array_intersect ( $user_pr_roles, $reqd_roles )) {
          return true;
        }
        break;
      default :

        // get parent node(s)
        $parent_nodes = content_parent_nodes ( $node );
        foreach ( $parent_nodes as $itemnid => $itemnode ) {
          // Get the user's role(s) in the next parent project
          $current_roles = og_get_user_roles ( 'node', $itemnid, $user->uid, FALSE );
          if (array_intersect ( $current_roles, $reqd_roles )) {
            return true;
          }
        }
        break;
    }
  }
  return false;
}


/**
 * function check_membership
 * Cehck user's membership in a given group
 * @param int $nid
 * @param int $uid
 * @return Bool
 */
function check_membership($gid, $uid) {
  $user = user_load($uid);
  if (og_is_member(4, $gid, $user)) {
    return TRUE;
  }

  return FALSE;
}

/**
 * function check_member_perms
 * return the current user's perms
 * @param int $nid
 * @param int $uid
 * @return Bool
 */
function check_member_perms($nid, $uid) {
  global $user;
  $itemnode = node_load ( $nid );
  // Make sure the EVENTROOMID is set, if not, deny permissions
  if (! defined ( 'EVENTROOMID' ) || EVENTROOMID == '') {
    // ATTEMPT to set 'EVENTROOMID' from $nid grand-parent
    return FALSE;
  }

  // If deleting a project file, make sure we have a valid project ID
  if ($action == 'DELETE_PROJECT_FILE') {
    if (! defined ( 'PROJECTID' ) || PROJECTID == '') {
      // ATTEMPT to set 'PROJECTID' from $nid parent
      $parent_nodes = content_parent_nodes ( $itemnode );
      // $nid_parent_types = _content_parent_types($parent_nodes);
      // 1 or more Parent Groups is a project
      foreach ( $parent_nodes as $nid => $node ) {
        // remove non-projects
        if ($node->type != 'jme_projects') {
          unset ( $node );
        }
      }

      // QUESTION: which project in $parent_nodes gets
    }

    return FALSE;
  }
  // ================
  $user_jme_roles = og_get_user_roles ( 'node', EVENTROOMID, $user->uid, FALSE );

  // Need to limit delete rights only for members with less than manager/admin role
  if (in_array ( 'administrator member, manager', $user_jme_roles )) {
    return TRUE;
  }

  // see if this node has a parent group that is a project (jme_projects)

  $parent_nodes = content_parent_nodes ( $itemnode );
  $nid_parent_types = content_parent_types ( $parent_nodes );
  if (in_array ( 'jme_projects', $nid_parent_types )) {
    $user_pr_roles = array ();
    // 1 or more Parent Groups is a project, so get UserRoles (for each) to see if user is a NOT a manager

    foreach ( $parent_nodes as $nid => $node ) {
      // Get the user's role(s) in the next parent project
      $current_roles = og_get_user_roles ( 'node', $nid, $user->uid, FALSE );
      // Loop thru the roles and add them to the total list (dupes will be overwritten)
      foreach ( $current_roles as $rid => $role ) {
        $user_pr_roles [$rid] = $role;
      }
    }

    // If $user_pr_roles is not at least a manager, they can only delete files within a project they are part of,
    // Get user group gids
    // $usergroups = og_get_groups_by_user($user);

    // if user is NOT og manager or og admin AND not in current project, set limits
    $reqd_roles = array (
        'administrator member',
        'manager'
    );
    if (! array_intersect ( $user_pr_roles, $reqd_roles )) {
      if (! (array_intersect ( array (
          'staffer',
          'vendor'
      ), $user_pr_roles ) && $itemnode->uid = $user->uid)) {
        return FALSE;
      }
    }
  }
  return TRUE;
}