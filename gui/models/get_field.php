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
        echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>99, "errMsg"=>"not connected" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>1, "errMsg"=>"unknown error" ));
        exit(1);
}

if(!isset($_GET['table']) || !isset($_GET['field']) || !isset($_GET['where'])){
    echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>2, "errMsg"=>"parameters error" ));
    exit(1);
}else{
    $table=$_GET['table'];
    $field=$_GET['field'];
    $where=$_GET['where'];
}

$id=0;
if(isset($_GET['id'])){
    $id=$_GET['id'];
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array('iserror'=>true, "result"=>"KO","errno"=>3,"errMsg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="SELECT $field FROM $table WHERE $where";
try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>4, "errMsg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$values=array();
foreach ($result as $elem){
    $values[]=$elem[$field];
}

header('Content-type: application/json');

echo json_encode(array('iserror'=>false, "result"=>"OK","values"=>$values,"errno"=>0));

$file_db = null;
