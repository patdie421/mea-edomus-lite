<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   ob_start();
   print_r($_REQUEST);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log($debug_msg);
}

$response = array();

switch(check_admin()){
   case 98: // pas admin, mais on laisse consulter
      break;
   case 99:
      $response["iserror"]=true;
      $response["errno"]=99;
      $response["errorMsg"]='not logged in';
      $response["rows"]=array();
      $response["total"]=0;
      echo json_encode($response);
      exit(1);
   case 0:
      break;
   default:
      $response["iserror"]=true;
      $response["errno"]=100;
      $response["errorMsg"]='unknown error';
      $response["rows"]=array();
      $response["total"]=0;
      echo json_encode($response);
      exit(1);
}

//parametres passés si pagination sur la datagrid
$page = isset($_POST['page']) ? intval($_POST['page']) : 1;
$rows = isset($_POST['rows']) ? intval($_POST['rows']) : 20;

// parametres passés si tri sur la datagrid
$sort = isset($_POST['sort'])   ? strval($_POST['sort']) : 'id';
$order = isset($_POST['order']) ? strval($_POST['order']) : 'asc';
$offset = ($page-1)*$rows;

// connexion à la db
try {
   $file_db = new PDO($PARAMS_DB_PATH);
}
catch(PDOException $e) {
   $error_msg=$e->getMessage();
   $response["iserror"]=true;
   $response["errno"]=1;
   $response["errorMsg"]=$error_msg;
   $response["rows"]=array();
   $response["total"]=0;
   echo json_encode($response);
   exit(1);
}

// parametrage du requetage
// $file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
// $file_db->setAttribute(PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

// récupération du nombre total de ligne

try {
   $request = $file_db->query("SELECT count(*) FROM interfaces");
   $row = $request->fetch(PDO::FETCH_NUM);
   $total=$row[0];
}
catch (PDOException $e) {
   $error_msg=$e->getMessage();
   $response["iserror"]=true;
   $response["errno"]=2;
   $response["errorMsg"]=$error_msg;
   $response["rows"]=array();
   $response["total"]=0;
   echo json_encode($response);
   $file_db=null;
   exit(1);
}

// requete des données
$SQL="SELECT interfaces.id as id,
             interfaces.id_interface as id_interface,
             interfaces.id_type as id_type,
             interfaces.name as name,
             interfaces.description as description,
             interfaces.dev as dev,
             interfaces.parameters as parameters,
             interfaces.state as state,
             types.name as tname
      FROM interfaces
      INNER JOIN types
         ON interfaces.id_type = types.id_type
      ORDER BY $sort $order
      LIMIT $offset, $rows";
error_log($SQL);
try {
   $request = $file_db->query($SQL);
   $rows = array();
   while($row = $request->fetch(PDO::FETCH_OBJ))
   {
      array_push($rows, $row);
   }
}
catch(PDOException $e)
{
   $error_msg=$e->getMessage();
   $response["iserror"]=true;
   $response["errno"]=3;
   $response["errorMsg"]=$error_msg;
   $response["rows"]=array();
   $response["total"]=0;
   echo json_encode($response);   
   $file_db=null;
   exit(1);
}

$response["iserror"]=false;
$response["total"]=$total;
$response["rows"]=$rows;

// emission des données
header('Content-type: application/json');
echo json_encode($response);

$file_db = null;

