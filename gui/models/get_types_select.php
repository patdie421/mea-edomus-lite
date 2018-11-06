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

switch(check_admin()){
    case 98:
        break;
    case 99:
        error_log("not connected");
        echo json_encode(array('iserror' => true, 'msg' => 'not connected'));
        exit(1);
    case 0:
        break;
    default:
        error_log("unknown error");
        echo json_encode(array('iserror' => true, 'msg' => 'unknown'));
        exit(1);
}

// connexion à la db
try {
    $file_db = new PDO($PARAMS_DB_PATH);
}
catch(PDOException $e) {

    $error_msg=$e->getMessage(); 
    error_log($error_msg);
    echo json_encode(array('iserror' => true, 'msg' => $error_msg));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$page = isset($_POST['page']) ? intval($_POST['page']) : 1;


// requete des données
$where="";
if(isset($_REQUEST['nature']))
{
   if($_REQUEST['nature']=='interface')
   {
      $where=" WHERE typeoftype=='10'";
   }
   else if($_REQUEST['nature']=='io')
   {
      $where=" WHERE typeoftype<>'10'";
   }
}
$SQL="SELECT id_type,name,typeoftype FROM types".$where." ORDER BY typeoftype,name";

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
    error_log($error_msg);
    echo json_encode(array('iserror' => true, 'msg' => $error_msg));
    $file_db=null;
    exit(1);
}

// pour debug
if ($debug==1)
{
   ob_start();
   print_r($rows);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log("Resultat de la requetes :");
   error_log($debug_msg);
}

// emission des données
header('Content-type: application/json');
echo json_encode($rows);

$file_db = null;
