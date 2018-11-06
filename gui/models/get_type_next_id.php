<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

switch(check_admin()){
    case 98:
        break;
    case 99:
        echo json_encode(array('iserror' => true, 'errno' => 99, 'errorMsg' => 'not connected'));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array('iserror' => true, 'errno' => 100, 'errorMsg' => 'unknown'));
        exit(1);
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    $error_msg=$e->getMessage();
    echo json_encode(array('iserror' => true, 'errno' => 1, 'errorMsg' => $error_msg));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

try{      
    $stmt = $file_db->prepare("SELECT MAX(id_type) AS max_id FROM types");
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array('iserror' => true, 'errno' => 2, 'errorMsg' => $error_msg));
    $file_db=null;
    exit(1);
}
$next_id=$result[0]['max_id']+1;

header('Content-type: application/json');

echo json_encode(array('iserror'=>false,"next_id"=>$next_id));

$file_db = null;