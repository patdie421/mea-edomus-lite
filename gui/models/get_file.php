<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
include_once('../lib/php/tools.php');
session_start();

if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   error_log_REQUEST();
}

header('Content-type: application/json');

inform_and_exit_if_not_connected();


if(!isset($_GET['type']) || !isset($_GET['name'])){
    echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>2, "errMsg"=>"parameters error" ));
    exit(1);
}else{
    $type=$_GET['type'];
    $name=$_GET['name'];
}


$param=false;
$path=false;
$extention=false;
if($type=='srules')
{
   $param="RULESFILESPATH";
   $extention="srules";
}
else if($type=='rset')
{
   $param="RULESFILESPATH";
   $extention="rset";
}
else if($type=='map')
{
   $param="GUIPATH";
   $subpath="maps";
   $extention="map";
}
else if($type=='menu')
{
   $param="GUIPATH";
   $subpath="maps";
   $extention="menu";
}
else
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>3, "errMsg"=>"type unknown" ));
   exit(1);
}

if($param!=false)
{
   $res=getParamVal($PARAMS_DB_PATH, $param);
   if($res!=false)
   {
      $res=$res[0];
      $path=$res{'value'};
      if($subpath!=false)
         $path=$path."/".$subpath;
   }
}
else
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>4, "errMsg"=>"can't get parameter" ));
   exit(1);
}


set_error_handler(
    create_function(
        '$severity, $message, $file, $line',
        'throw new ErrorException($message, $severity, $severity, $file, $line);'
    )
);
try {
   $file=file_get_contents($path . "/" . $name . "." . $extention);
}
catch(Exception $e) {
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>5, "errMsg"=>"can't get file - " . $e->getMessage() ));
   exit(1);
}
restore_error_handler();

echo json_encode(array('iserror'=>false, "result"=>"OK","file"=>$file, "errno"=>0));
