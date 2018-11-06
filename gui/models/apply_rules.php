<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

header('Content-type: application/json');

$DEBUG_ON=1;

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
        echo json_encode(array('iserror' => true, "result"=>"KO", 'errno' => 98, 'errorMsg' => 'not administrator'));
        exit(1);
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

if(!isset($_GET['name'])){
    echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>2, "errMsg"=>"parameters error" ));
    exit(1);
}else{
    $name=$_GET['name'];
}



function endsWith($haystack, $needle) {
    // search forward starting from end minus needle length characters
    return $needle === "" || (($temp = strlen($haystack) - strlen($needle)) >= 0 && strpos($haystack, $needle, $temp) !== FALSE);
}


function getParamVal($db, $param)
{
   $response=[];

   try {
      $file_db = new PDO($db);
   }
   catch (PDOException $e) {
      return false;
   }

   $file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
   $file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

   $SQL='SELECT * FROM application_parameters WHERE key = "'.$param.'"';
   try {
      $stmt = $file_db->prepare($SQL);
      $stmt->execute();
      $result = $stmt->fetchAll();
   }
   catch(PDOException $e) {
      $result = false;
   }
   $file_db=null;
   return $result;
}

$rulespath = getParamVal($PARAMS_DB_PATH, "RULESFILESPATH");
if ($rulespath == false)
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>3, "errMsg"=>"can't get application parameter (RULESFILESPATH)" ));
   exit(1);
}
$rulesfile = getParamVal($PARAMS_DB_PATH, "RULESFILE");
if ($rulesfile == false)
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>4, "errMsg"=>"can't get application parameter (RULESFILE)" ));
   exit(1);
}


$rulespath= $rulespath[0]{'value'};
$rulesfile= $rulesfile[0]{'value'};
$source = $rulespath . '/' . $name . ".rules";

set_error_handler(
    create_function(
        '$severity, $message, $file, $line',
        'throw new ErrorException($message, $severity, $severity, $file, $line);'
    )
);
try {
   $ret=copy ($source, $rulesfile);
}
catch(Exception $e) {
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>5, "errMsg"=>"can't copy file - " . $e->getMessage() ));
   exit(1);
}
restore_error_handler();

echo json_encode(array('iserror'=>false, "result"=>"OK", "errno"=>0, "errMsg"=>"succes" ));
