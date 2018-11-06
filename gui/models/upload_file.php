<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
include_once('../lib/php/tools.php');
session_start();

$DEBUG_ON=1;

if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   error_log_REQUEST();
}

$path=false;
$param="GUIPATH";
$res=getParamVal($PARAMS_DB_PATH, $param);
if($res!=false)
{
   $res=$res[0];
   $path=$res{'value'};
}
else
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>4, "errMsg"=>"can't get parameter" ));
   exit(1);
}

$filename = basename($_FILES['file']['name']);
$patterns = array();
$patterns[0] = "/[^a-zA-Z0-9\.]/";
$replacements = array();
$replacements[0] = '_';
$filename = preg_replace($patterns, $replacements, $filename);

header('Content-type: text/html');

inform_and_exit_if_not_admin();

$ret=getimagesize($_FILES['file']['tmp_name']);
if($ret==false)
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>6, "errMsg"=>"not an image" ));
   exit(1);
}

set_error_handler(
    create_function(
        '$severity, $message, $file, $line',
        'throw new ErrorException($message, $severity, $severity, $file, $line);'
    )
);
try {
   move_uploaded_file($_FILES['file']['tmp_name'], $path. "/images/" . $filename);
}
catch(Exception $e) {
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>5, "errMsg"=>"can't put file - " . $e->getMessage() ));
   exit(1);
}
restore_error_handler();

echo json_encode(array('iserror'=>false, "result"=>"OK", "filename" => $filename, "errno"=>0));
