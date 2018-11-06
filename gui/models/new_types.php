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
        error_log("not administrator");
        echo json_encode(array('iserror' => true, 'errno' => 98, 'errorMsg' => 'not administrator'));
        exit(1);
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

$id_type = $_REQUEST['id_type'];
$name = $_REQUEST['name'];
$description = $_REQUEST['description'];
$parameters = $_REQUEST['parameters'];
$typeoftype = $_REQUEST['typeoftype'];
//$flag = $_REQUEST['flag'];
$flag = '2';

// connexion à la db
try {
    $file_db = new PDO($PARAMS_DB_PATH);
}
catch(PDOException $e) {

    $error_msg=$e->getMessage();
    error_log($error_msg);
    echo json_encode(array('iserror' => true, 'errorMsg' => $error_msg));
    exit(1);
}
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="INSERT INTO types (id_type, name, description, parameters, typeoftype, flag)
      VALUES ('$id_type','$name', '$description','$parameters','$typeoftype','$flag')";
try {
   $request = $file_db->query($SQL);
}
catch (PDOException $e) {
   $error_msg=$e->getMessage();
   error_log($error_msg);
   echo json_encode(array('iserror' => true, 'errorMsg' => $error_msg));
   $file_db=null;
   exit(1);
}

echo json_encode(array('iserror' => false));

$file_db=null;
