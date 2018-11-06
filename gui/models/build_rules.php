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

header('Content-type: application/json');

inform_and_exit_if_not_admin();

if(!isset($_GET['name']) || !isset($_GET['files'])){
    echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>2, "errMsg"=>"parameters error ..." ));
    exit(1);
}else{
    $name=$_GET['name'];
     $files=$_GET['files'];
    if(!is_array($files) || count($files) < 1)
    {
       echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>4, "errMsg"=>"no file" ));
       exit(1);
    }
}

$output = $name . ".rules";

$rulespath = getParamVal($PARAMS_DB_PATH, "RULESFILESPATH");
if ($rulespath == false)
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>3, "errMsg"=>"can't get application parameter" ));
   exit(1);
}
$rulespath= $rulespath[0]{'value'};

$files_str=" ";
foreach($files as $file) {
   $files_str=$files_str . $file . ".srules ";
}

$comp = $BASEPATH . '/bin/mea-compilr'. $files_str . '-p ' . $rulespath . ' -i -d -j -o ' . $output;

ob_start();
passthru($comp);
$res = ob_get_clean(); 
echo $res;
