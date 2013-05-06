<?php
/**
 * @file
 * Utilities used in the MIRC@SC module.
 */

/**
 * Retrieves the image markup for an object.
 *
 * @param string $pid
 *   The PID of the object.
 *
 * @param string $alt
 *   Text to be displayed as an alt.
 *
 * @param string $title
 *   Title of the image.
 *
 * @param string $imagecache_preset
 *   The Imagecache External preset we are to use.
 *
 * @return string
 *   Markup for the generated image.
 */
function usc_mirc_get_image_markup($pid, $alt = '', $title = '', $imagecache_preset = NULL) {
  $object = islandora_object_load($pid);
  $imagecache_preset = variable_get('usc_mirc_imagecache_thumbnail_preset', 'usc_mirc_thumbnail');

  $dsid = '';
  if ($object['IMG']) {
    $dsid = 'IMG';
  }
  else {
    $dsid = 'TN';
  }
  $image = "islandora/object/$pid/datastream/$dsid/view";

  if (module_exists('imagecache_external') && is_callable('theme_imagecache_external')) {
    return theme('imagecache_external', array(
      'path' => url($image, array('absolute' => TRUE)),
      'style_name' => $imagecache_preset,
      'alt' => 'Collection image')
    );
  }
  else {
    return theme('image', array(
      'path' => $image,
      'alt' => $alt,
      'title' => $title,
      'attributes' => array(),
      )
    );
  }
}

/**
 * Builds and returns a query for VRO objects based upon parameters.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @param bool $count
 *   Whether this is a count query or not.
 *
 * @param int $page
 *   The page number we are to start at.
 *
 * @param int $number
 *   Number of items per page.
 *
 * @return array
 *   Results from the collection set query.
 */
function usc_mirc_get_vro_query($object, $count = FALSE, $page = NULL, $number = NULL) {
  $collection_relation = variable_get('usc_mirc_collection_relationship', 'isMemberOfCollection');
  $vro_cmodel = variable_get('usc_mirc_cmodel_vro', 'usc:vroCModel');

  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  PREFIX dc: <http://purl.org/dc/elements/1.1/>
  PREFIX is: <http://islandora.ca/ontology/relsext#>
  SELECT ?vro ?vro_title
  WHERE {
  ?vro fre:$collection_relation <info:fedora/$object->id> ;
       fm:hasModel <info:fedora/$vro_cmodel> ;
       fm:state fm:Active ;
       dc:title ?vro_title .
   OPTIONAL {
      ?vro is:isViewableByUser ?usr .
      ?vro is:isViewableByRole ?role .
    }";
  $query .= usc_mirc_append_xacml_filter();
  $query .= "ORDER BY ?vro_title";

  if (!$count) {
    $offset = $page * $number;
    $query .= "
    LIMIT $number
    OFFSET $offset";
  }
  return $query;
}

/**
 * Performs a SPARQL query to go out and get the VRO set.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @param int $page
 *   The page to start on.
 *
 * @param int $number
 *   The number of items per page.
 *
 * @return array
 *   An array containing the results of the VRO query.
 */
function usc_mirc_get_vro_set($object, $page = 0, $number = 20) {
  $query = usc_mirc_get_vro_query($object, FALSE, $page, $number);
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
    'page_size' => $page,
    'page_number' => $number,
  );

  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  return $results;
}

/**
 * Performs a SPARQL query to retrieve the number of children for VROs.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @return int
 *   The number of VRO children.
 */
function usc_mirc_get_vro_children($object) {
  $query = usc_mirc_get_vro_query($object, TRUE);
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  return count($results);
}

/**
 * Builds and returns a query for mezzanine objects based upon parameters.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @param bool $count
 *   Whether this is a count query or not.
 *
 * @param int $page
 *   The page number we are to start at.
 *
 * @param int $number
 *   Number of items per page.
 *
 * @return array
 *   Results from the collection set query.
 */
function usc_mirc_get_mezzanine_query($object, $count = FALSE, $page = NULL, $number = NULL) {
  $collection_relation = variable_get('usc_mirc_collection_relationship', 'isMemberOfCollection');
  $derivative_relation = variable_get('usc_mirc_relationship', 'isDerivativeOf');
  $vro_cmodel = variable_get('usc_mirc_cmodel_vro', 'usc:vroCModel');
  $mezzanine_cmodel = variable_get('usc_mirc_cmodel_mezzanine', 'usc:mezzanineCModel');

  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  PREFIX dc: <http://purl.org/dc/elements/1.1/>
  PREFIX is: <http://islandora.ca/ontology/relsext#>
  SELECT ?mezzanine ?mezzanine_title
  WHERE {
  ?vro fre:$collection_relation <info:fedora/$object->id> ;
       fm:hasModel <info:fedora/$vro_cmodel> ;
       fm:state fm:Active .

  ?mezzanine fre:$derivative_relation ?vro ;
             fm:hasModel <info:fedora/$mezzanine_cmodel> ;
             fm:state fm:Active ;
             dc:title ?mezzanine_title .
    OPTIONAL {
      ?mezzanine is:isViewableByUser ?usr .
      ?mezzanine is:isViewableByRole ?role .
    }";
  $query .= usc_mirc_append_xacml_filter();
  $query .= "ORDER BY ?mezzanine_title";

  if (!$count) {
    $offset = $page * $number;
    $query .= "
    LIMIT $number
    OFFSET $offset";
  }
  return $query;
}

/**
 * Performs a SPARQL query to go out and get the mezzanine set.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @param int $page
 *   The page to start on.
 *
 * @param int $number
 *   The number of items per page.
 *
 * @return array
 *   An array containing the results of the VRO query.
 */
function usc_mirc_get_mezzanine_set($object, $page = 0, $number = 20) {
  $query = usc_mirc_get_mezzanine_query($object, FALSE, $page, $number);
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
    'page_size' => $page,
    'page_number' => $number,
  );

  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  return $results;
}

/**
 * Performs a SPARQL query to retrieve the number of children for mezzaniness.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @return int
 *   The number of VRO children.
 */
function usc_mirc_get_mezzanine_children($object) {
  $query = usc_mirc_get_mezzanine_query($object, TRUE);
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  return count($results);
}

/**
 * Performs a SPARQL query to retrieve the parent of a mezzanine object.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @return mixed
 *   A bool if it has no parent, or the parent pid otherwise.
 */
function usc_mirc_get_mezzanine_parent($object) {
  global $user;
  $vro_cmodel = 'usc:vroCModel';
  $def_rel = variable_get('usc_mirc_relationship', 'isDerivativeOf');
  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  PREFIX is: <http://islandora.ca/ontology/relsext#>
  SELECT ?object
  WHERE {
    ?subject fre:$def_rel ?object ;
             fm:state fm:Active .
    ?object fm:state fm:Active ;
            fm:hasModel <info:fedora/$vro_cmodel> .
    OPTIONAL {
      ?object is:isViewableByUser ?usr .
      ?object is:isViewableByRole ?role .
    }
    FILTER(sameTerm(?subject, <info:fedora/$object->id>)) . ";
  $query .= usc_mirc_append_xacml_filter();
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);

  if (count($results) > 0) {
    return $results[0]['object']['value'];
  }
  else {
    return FALSE;
  }
}

/**
 * Adds the flowplayer markup to the current page.
 *
 * @global type $base_url
 *
 * @param array $videos
 *   An array of videos that are returned via query. We will only markup the
 *   first one.
 *
 * @return string
 *   Flowplayer markup.
 */
function usc_mirc_mezzanine_player_markup($videos) {
  global $base_url;
  $mod_path = drupal_get_path('module', 'usc_mirc');
  drupal_add_js("$mod_path/lib/flowplayer/flowplayer-3.2.8.min.js", array('group' => JS_LIBRARY));

  $player_class = 'usc-flowplayer';
  $flowplayer_js = <<<EOJS
  jQuery(function() {
    jQuery('.$player_class').flowplayer("$base_url/$mod_path/lib/flowplayer/flowplayer.commercial-3.2.8.swf", {

      //Product key (for sc.edu)
      key: "#\$60fe88f4c446bc34a1c",

      clip: {
        provider: 'lighttpd', // configure clip to use "lighthttpd" plugin for providing video data
        scaling: 'fit', //Maintain aspect ratio when scaling
      },
      canvas: { //Use a full-black background
        background: '#000000',
        backgroundGradient: 'none',
      },

      // streaming plugins are configured normally under the plugins node
      plugins: {
        lighttpd: {
          url: 'flowplayer.pseudostreaming-3.2.8.swf'
        }
      }
    })
  });
EOJS;
  drupal_add_js($flowplayer_js, array('type' => 'inline'));

  if (count($videos) > 0) {
    $output = array(
      '#markup' => l(theme('image', array(
        "path" => "$mod_path/images/playbuttonblack.jpg",
        'alt' => 'Video placeholder',
        'attributes' => array(),
        )), $videos[0],
        array(
          'html' => TRUE,
          'attributes' => array(
            'class' => array($player_class),
          ),
        )),
    );
  }
  else {
    $output = array(
      '#markup' => theme('image', array("path" => "$mod_path/images/NoVideoAvailable.jpg", 'attributes' => array())),
    );
  }
  return drupal_render($output);
}

/**
 * Performs a SPARQL query to retrieve derivatives of an object.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @param string $subject
 *   The subject we are searching for in SPARQL.
 *
 * @return array
 *   The result set.
 */
function usc_mirc_get_derivatives($object, $subject = NULL) {
  global $user;
  if ($subject === NULL) {
    $def_rel = variable_get('usc_mirc_relationship', 'isDerivativeOf');
    $subject = "?subject fre:$def_rel <info:fedora/$object->id>";
  }
  // Perform query.
  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  PREFIX is: <http://islandora.ca/ontology/relsext#>
  SELECT ?subject ?label
  WHERE {
  $subject ;
         fm:label ?label ;
         fm:state fm:Active .";
  $query .= "OPTIONAL {
      ?subject is:isViewableByUser ?usr .
      ?subject is:isViewableByRole ?role .
    }";
  $query .= usc_mirc_append_xacml_filter();
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  return $results;
}

/**
 * Performs a SPARQL query to build a breadcrumb link back to the parent object.
 *
 * @param FedoraObject $object
 *   A FedoraObject.
 *
 * @return mixed
 *   Markup to the of the object or FALSE if no parents are visible to the user.
 */
function usc_mirc_build_breadcrumb($object) {
  $vro_cmodel = variable_get('usc_mirc_cmodel_vro', 'usc:vroCModel');
  $mezz_cmodel = variable_get('usc_mirc_cmodel_mezzanine', 'usc:mezzanineCModel');
  $pid = $object->id;
  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  PREFIX is: <http://islandora.ca/ontology/relsext#>
  SELECT ?parent
  WHERE {
    {
      ?this fre:isDerivativeOf ?vro ;
            fm:hasModel <info:fedora/$mezz_cmodel> ;
            fm:state fm:Active .
      ?vro fre:isMemberOfCollection ?parent ;
           fm:hasModel <info:fedora/$vro_cmodel> ;
           fm:state fm:Active .
      OPTIONAL {
        ?parent is:isViewableByUser ?usr .
        ?parent is:isViewableByRole ?role .
      }
    }
    UNION
    {
      ?this fre:isMemberOfCollection ?parent ;
            fm:hasModel ?model ;
            fm:state fm:Active .
      FILTER(!sameterm(?model, <info:fedora/$vro_cmodel>)) .
      OPTIONAL {
        ?parent is:isViewableByUser ?usr .
        ?parent is:isViewableByRole ?role .
      }
    }
    FILTER(sameterm(?this, <info:fedora/$pid>))";
  $query .= usc_mirc_append_xacml_filter();

  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  if ($results) {
    $parent_object = islandora_object_load($results[0]['parent']['value']);
    $title = $parent_object->label;
    return l($title, "islandora/object/$parent_object->id");
  }
  return FALSE;
}

/**
 * Retrieves a list of mezzanine formats from the taxonomy.
 *
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   An array containing the options retrieved from the taxonomy.
 */
function usc_mirc_get_mezzanine_formats(&$form_state) {
  $vocabs = taxonomy_get_vocabularies();
  foreach ($vocabs as $id => $voc) {
    if ($voc->machine_name == variable_get('usc_mirc_mezzanine_format_vocab', 'usc_mirc_mezzanine_format_vocab')) {
      $format_options = array();
      $format_tree = taxonomy_get_tree($voc->vid);
      foreach ($format_tree as $term) {
        $format_options[$term->name] = $term->name;
      }
      return array(
        '#type' => 'select',
        '#title' => t('Derivative format'),
        '#description' => t('Whether this mezzanine is edge-to-edge, cropped, or so on. (This list can be edited/expanded on via <a href=@link>derivative formats</a>)', array('@link' => url("admin/structure/taxonomy/$voc->machine_name/add"))),
        '#options' => $format_options,
      );
    }
  }
}

/**
 * Retrieves the types of mezzanines from the taxonomy.
 *
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   An array containing the options retrieved from the taxonomy.
 */
function usc_mirc_get_mezzanine_types(&$form_state) {
  $vocabs = taxonomy_get_vocabularies();
  foreach ($vocabs as $id => $voc) {
    if ($voc->machine_name == variable_get('usc_mirc_mezzanine_type_vocab', 'usc_mirc_mezzanine_type_vocab')) {
      $type_options = array();
      $type_tree = taxonomy_get_tree($voc->vid);
      foreach ($type_tree as $term) {
        $type_options[$term->name] = $term->name;
      }
      return array(
        '#type' => 'select',
        '#title' => t('Derivative type'),
        '#description' => t('Whether this mezzanine is edited, or so on. (This list can be edited/expanded on via <a href=@link>derivative types</a>)', array('@link' => url("admin/structure/taxonomy/$voc->machine_name/add"))),
        '#options' => $type_options,
      );
    }
  }
}

/**
 * Constructs and returns the XACML filter for the current user.
 *
 * @return string
 *   The XACML filter.
 */
function usc_mirc_append_xacml_filter() {
  global $user;
  $filter_string = '';
  $role_count = 0;
  foreach ($user->roles as $role) {
    if ($role_count === 0) {
      $filter_string .= "FILTER(!bound(?role) || (bound(?role) && (sameTerm(?role, '$role')";
    }
    else {
      $filter_string .= " || sameTerm(?role, '$role')";
    }
    $role_count++;
  }
  $filter_string .= "))";
  if ($user->uid != 0) {
    $user_name = $user->name;
  }
  else {
    $user_name = 'anonymous';
  }
  $filter_string .= " || (!bound(?usr) || (bound(?usr) && sameTerm(?usr, '$user_name'))))\n}";
  return $filter_string;
}
