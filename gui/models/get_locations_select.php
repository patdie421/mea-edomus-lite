<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

$debug=0;

switch(check_admin()){
    case 98:
        break;
    case 99:
        error_log("not connected");
        echo json_encode(array('iserror' => true, 'errno' => 99, 'errorMsg' => 'not connected'));
        exit(1);
    case 0:
        break;
    default:
        error_log("unknown error");
        echo json_encode(array('iserror' => true, 'errno' => 100, 'errorMsg' => 'unknown'));
        exit(1);
}

// connexion à la db
try {
    $file_db = new PDO($PARAMS_DB_PATH);
}
catch(PDOException $e) {

    $error_msg=$e->getMessage(); 
    error_log($error_msg);
    echo json_encode(array('iserror' => true, 'errno' => 1, 'errorMsg' => $error_msg));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

// requete des données
$SQL="SELECT id_location,name FROM locations WHERE deleted_flag <> 1";

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
    echo json_encode(array('iserror' => true, 'errno' => 2, 'errorMsg' => $error_msg));
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
   error_log($debug_msg);
}

// emission des données
header('Content-type: application/json');
echo json_encode($rows);

$file_db = null;
