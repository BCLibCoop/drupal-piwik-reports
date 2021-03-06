<?php

/**
 * @file
 * Reports callback for Piwik Reports
 */

/**
 * Report menu links
 */
function piwik_reports_menu_block_page() {
  $item = menu_get_item();
  if ($content = system_admin_menu_block($item)) {
    $output = theme('admin_block_content', array('content' => $content));
  }
  else {
    $output = '<p>' . t('You do not have any reports.') . '</p>';
  }

  return $output;
}

/**
 * Return the form where to select the report period.
 */
function piwik_reports_dateselect_form() {

  $period = array(
    0 => t('Today'),
    1 => t('Yesterday'),
    2 => t('Last week'),
    3 => t('Last month'),
    4 => t('Last year'),
    5 => t('From beginning'),
    6 => t('Last quarter')
  );

  $form['#attributes'] = array('class' => array('search-form'));
  $form['basic'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select site and time period'),
  );
  $form['basic']['inline'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $form['basic']['inline']['period'] = array(
    '#type' => 'select',
    '#title' => '',
    '#default_value' => isset($_SESSION['piwik_reports_period']) ? $_SESSION['piwik_reports_period'] : 0,
    '#options' => $period,
  );
  $form['basic']['inline']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#weight' => 15,
  );
  return $form;
}

/**
 * Return the form where to select the site.
 */
function piwik_reports_siteselect_form($form = array(), $sites = array()) {
  $options = array();
  $allowed_keys = explode(',', variable_get('piwik_reports_allowed_sites', ''));
  $session_site_exists = false;
  foreach ($sites as $site) {
    if (empty($allowed_keys[0]) || in_array($site['idsite'], $allowed_keys)) {
      $options[$site['idsite']] = $site['name'];
    }
    if (isset($_SESSION['piwik_reports_site']) && $_SESSION['piwik_reports_site'] == $site['idsite']) $session_site_exists = true;
  } 
  if (!isset($_SESSION['piwik_reports_site']) || $_SESSION['piwik_reports_site'] == '' || !$session_site_exists) $_SESSION['piwik_reports_site'] = $sites[0]['idsite'];
  if (count($options) > 1) {
    $form['basic']['inline']['site'] = array(
      '#type' => 'select',
      '#title' => t('Site: '),
      '#weight' => -5,
      '#default_value' => $_SESSION['piwik_reports_site'],
      '#options' => $options,
    );
  }
  elseif (count($options) == 1) {
    $form['basic']['inline']['site'] = array(
      '#type' => 'hidden',
      '#value' => $_SESSION['piwik_reports_site'],
    );
    $form['basic']['inline']['sitename'] = array(
      '#type' => 'textfield',
      '#title' => t('Site: '),
      '#weight' => -5,
      '#size' => 25,
      '#value' => $sites[0]['name'],
      '#disabled' => true,
    );
  }
  return $form;
}


/**
 * Return the $form that handles piwik reports
 *
 * @param $form
 * @param $form_state
 *   see form api.
 * @param $report
 *   The report name.
 *
 * @return
 */
function piwik_reports_reports($form, &$form_state, $report) {
  global $user;

  $piwik_auth = (isset($user->data['piwik_reports_token_auth']) && !empty($user->data['piwik_reports_token_auth'])? $user->data['piwik_reports_token_auth'] : variable_get('piwik_reports_token_auth', ''));
  $piwik_url = piwik_reports_get_url();
  $sites = piwik_reports_get_sites($piwik_auth);
  if (empty($sites)) {
    $_SESSION['piwik_reports_site'] == '';
    drupal_set_message(t('You can not access any data on the selected Piwik server. Please check authentication string and permissions with your Piwik server administrator.'), 'warning');
    return;
  }
  $form = piwik_reports_dateselect_form();
  $form = piwik_reports_siteselect_form($form, $sites);
  $piwik_site_id = $_SESSION['piwik_reports_site'];
  $period = isset($_SESSION['piwik_reports_period']) ? $_SESSION['piwik_reports_period'] : 0;

  if ($period == 1) {
    // Special handling for "yesterday" = 1.
    // The yesterday date value is required.
    $date = _piwik_reports_select_period($period);
  }
  //Custom periods
  elseif ($period >= 5) $date = _piwik_reports_select_period($period);
  else {
    // Otherwise it returns the today date value.
    $date = _piwik_reports_select_period(0);
  }
  $period_name = _piwik_reports_get_period_name($period);

  // Create an array of URL parameters for easier maintenance.
  $data_params[0] = array();
  $data_params[0]['idSite'] = $piwik_site_id;
  $data_params[0]['date'] = $date;
  $data_params[0]['period'] = $period_name;
  $data_params[0]['disableLink'] = 1;
  $data_params[0]['module'] = 'Widgetize';
  $data_params[0]['action'] = 'iframe';
  $data_params[0]['disableLink'] = 1;
  $data_params[0]['widget'] = 1;
 // $data_params[0]['loading'] = t('Loading data...');
  if (!empty($piwik_auth)) {
    $data_params[0]['token_auth'] = $piwik_auth;
  }

  switch ($report) {
    case 'visitors_overview':
      $title[0] = t('Visits trend');
      $data_params[0]['columns[]'] = 'nb_visits';
      $data_params[0]['moduleToWidgetize'] = 'VisitsSummary';
      $data_params[0]['actionToWidgetize'] = 'getEvolutionGraph';
    break;
    case 'visitors_times':
      $title[0] = t('Visits by local time');
      $data_params[0]['moduleToWidgetize'] = 'VisitTime';
      $data_params[0]['actionToWidgetize'] = 'getVisitInformationPerLocalTime';
    break;
    case 'visitors_settings':
      $data_params[0]['filter_limit'] = 6;

      $data_params[1] = $data_params[0];
      $data_params[2] = $data_params[0];
      $data_params[3] = $data_params[0];
      // Browser families
      $title[0] = t('Browser families');
      $data_params[0]['moduleToWidgetize'] = 'UserSettings';
      $data_params[0]['actionToWidgetize'] = 'getBrowserType';
      // Screen resolutions
      $title[1] = t('Screen resolution');
      $data_params[1]['moduleToWidgetize'] = 'UserSettings';
      $data_params[1]['actionToWidgetize'] = 'getConfiguration';
      // Operating systems
      $title[2] = t('Operating system');
      $data_params[2]['moduleToWidgetize'] = 'UserSettings';
      $data_params[2]['actionToWidgetize'] = 'getOS';
      // Client configurations
      $title[3] = t('Client configuration');
      $data_params[3]['moduleToWidgetize'] = 'UserSettings';
      $data_params[3]['actionToWidgetize'] = 'getResolution';
    break;
    case 'visitors_locations':
      $title[0] = t('Visitors countries');
      $data_params[0]['moduleToWidgetize'] = 'UserCountry';
      $data_params[0]['actionToWidgetize'] = 'getCountry';
      $data_params[0]['filter_limit'] = 15;
    break;
    case 'custom_variables':
      $title[0] = t('Custom variables');
      $iframe_height[0] = 1000;
      $data_params[0]['moduleToWidgetize'] = 'CustomVariables';
      $data_params[0]['actionToWidgetize'] = 'getCustomVariables';
      $data_params[0]['filter_limit'] = 15;
    break;
    case 'actions_pageurls':
      $title[0] = t('Page visits');
      $data_params[0]['moduleToWidgetize'] = 'Actions';
      $data_params[0]['actionToWidgetize'] = 'getPageUrls';
      $data_params[0]['filter_limit'] = 15;
    break;
    case 'actions_entryurls':
      $title[0] = t('Page visits');
      $data_params[0]['moduleToWidgetize'] = 'Actions';
      $data_params[0]['actionToWidgetize'] = 'getEntryPageUrls';
      $data_params[0]['filter_limit'] = 15;
    break;
    case 'actions_exiturls':
      $title[0] = t('Page visits');
      $data_params[0]['moduleToWidgetize'] = 'Actions';
      $data_params[0]['actionToWidgetize'] = 'getExitPageUrls';
      $data_params[0]['filter_limit'] = 15;
    break;
    case 'actions_outlinks':
      $title[0] = '';
      $data_params[0]['moduleToWidgetize'] = 'Actions';
      $data_params[0]['actionToWidgetize'] = 'getOutlinks';
      $data_params[0]['filter_limit'] = 15;
    break;
    case 'actions_downloads':
      $title[0] = '';
      $data_params[0]['moduleToWidgetize'] = 'Actions';
      $data_params[0]['actionToWidgetize'] = 'getDownloads';
      $data_params[0]['filter_limit'] = 15;
    break;
    case 'referers_evolution':
      $title[0] = '';
      $data_params[0]['moduleToWidgetize'] = 'Referers';
      $data_params[0]['actionToWidgetize'] = 'getRefererType';
    break;
    case 'referers_search':
      $data_params[1] = $data_params[0];

      $title[0] = t('Search engines');
      $data_params[0]['moduleToWidgetize'] = 'Referers';
      $data_params[0]['actionToWidgetize'] = 'getSearchEngines';

      $title[1] = t('Keywords');
      $data_params[1]['moduleToWidgetize'] = 'Referers';
      $data_params[1]['actionToWidgetize'] = 'getKeywords';
    break;
    case 'referers_websites':
      $title[0] = '';
      $data_params[0]['moduleToWidgetize'] = 'Referers';
      $data_params[0]['actionToWidgetize'] = 'getWebsites';
    break;
    case 'goals_evolution':
      $goals = piwik_reports_get_goals($piwik_auth, $_SESSION['piwik_reports_site']);
      if (count($goals) == 0) {
        $empty_text = t('No goals have been set. Check with your Piwik server administrator if you desire some.');
        $title[0] = NULL;
        break;
      }
      $common_data_params = $data_params[0];
      $i = 0;
      foreach ($goals as $goal) {
      $title[$i] = $goal['name'];
      $iframe_height[$i] = 370;
      $data_params[$i] = $common_data_params;
      $data_params[$i]['moduleToWidgetize'] = 'Goals';
      $data_params[$i]['actionToWidgetize'] = 'widgetGoalReport';
      $data_params[$i]['idGoal'] = $goal['idgoal'];
      $i++;
      }
    break;
  }
  // Build the data URL with all params and urlencode it.
  foreach ($data_params as $key => $data) {
    $theme_args['data_url'][] = array('url' => $piwik_url . 'index.php?' . drupal_http_build_query($data), 
    'title' => $title[$key], 
    'iframe_height' => ( isset($iframe_height[$key]) && $iframe_height[$key] > 0 ? $iframe_height[$key] : '100%'),
    'empty_text' => ( isset($empty_text) ? $empty_text : NULL),
    );
  }
  // Render HTML code.
  $output = theme('piwik_reports', $theme_args);
  $form['content'] = array(
    '#type' => 'item',
    '#markup' => $output,
  );
  return $form;
}

/**
 * Reports form submit function
 */
function piwik_reports_reports_submit($form, &$form_state) {
  $_SESSION['piwik_reports_period'] = $form_state['values']['period'];
  $_SESSION['piwik_reports_site'] = $form_state['values']['site'];
}

/**
 * Helper function that return the starting and ending dates according to the
 * selected period.
 *
 * @param $period
 *    selected period
 * @return
 *    array with starting and ending date
 */
function _piwik_reports_select_period($period) {
  switch ($period) {
    case 0: //Today
      $date = date("Y-m-d");
      break;

    case 1: //Yesterday day -1
      $d = mktime(0, 0, 0, date("m"), date("d") - 1, date("Y"));
      $date = date("Y-m-d", $d);
      break;

    case 2: //Last week day -7
      $d = mktime(0, 0, 0, date("m"), date("d") - 7, date("Y"));
      $date = date("Y-m-d", $d);
      break;

    case 3: //Last month, month -1
      $d = mktime(0, 0, 0, date("m") - 1, date("d"), date("Y"));
      $date = date("Y-m-d", $d);
      break;

    case 4:  //Last year, year -1
      $d = mktime(0, 0, 0, date("m"), date("d"), date("Y") - 1);
      $date = date("Y-m-d", $d);
      break;

    case 5: //Range launch through today
      $launch = date("Y-m-d", 1430438400);
      $today = date("Y-m-d");
      $date = $launch . ',' . $today;
      break;

    case 6:
      $curMonth = date("m", time());
      $year = date("Y", time());
      $lastMonth = $curMonth-1;
      if($lastMonth == '12') $year = $year -1; //if last month was December,
      // bump year back
      $lastQstartMonth = ($lastMonth - 3) + 1;
      $lastQstartDate = date("Y-m-d", strtotime("$year".'-'."$lastQstartMonth"
        .'-'.'01'));
      $lastQendDate = date("Y-m-t", strtotime("$year".'-'."$lastMonth"));

      $date = $lastQstartDate . ',' . $lastQendDate;
  }
  return $date;
}

/**
 * Helper function that return the name of the selected period.
 *
 * @param $period
 *    selected period
 * @return
 *    name
 */
function _piwik_reports_get_period_name($period) {
  // Possible periods are day, week, month, year.
  switch ($period) {
    case 0:
      $p = "day";
      break;

    case 1:
      $p = "day";
      break;

    case 2:
      $p = "week";
      break;

    case 3:
      $p = "month";
      break;

    case 4:
      $p = "year";
      break;

    case 5:
    case 6:
      $p = "range";
      break;
  }
  return $p;
}
