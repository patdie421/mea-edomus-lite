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
   case 98: // pas admin, mais on laisse consulter
      break;
   case 99:
      $response["iserror"]=true;
      $response["errno"]=99;
      $response["errorMsg"]='not logged in';

      $response["result"]="KO"; // pour la compatibilité
      $response["error"]=$response["errno"]; // pour la compatibilité      
      $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité

      echo json_encode($response);
      exit(1);
   case 0:
      break;
   default:
      $response["iserror"]=true;
      $response["errno"]=100;
      $response["errorMsg"]='unknown error';
      
      $response["result"]="KO"; // pour la compatibilité
      $response["error"]=$response["errno"]; // pour la compatibilité      
      $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité

      echo json_encode($response);
      exit(1);
}


try {
    $file_db = new PDO($PARAMS_DB_PATH);
}
catch (PDOException $e)
{
   $error_msg=$e->getMessage();
   $response["iserror"]=true;
   $response["errno"]=2;
   $response["errorMsg"]=$error_msg;
   
   $response["result"]="KO"; // pour la compatibilité
   $response["error"]=$response["errno"]; // pour la compatibilité      
   $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité

   echo json_encode($response);

   exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="SELECT * FROM application_parameters";

try {    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}
catch(PDOException $e)
{
    $error_msg=$e->getMessage();
    $response["iserror"]=true;
    $response["errno"]=3;
    $response["errorMsg"]=$error_msg;
    
    $response["result"]="KO"; // pour la compatibilité
    $response["error"]=$response["errno"]; // pour la compatibilité      
    $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité

    echo json_encode($response);

    $file_db=null;
    exit(1);
}
$result["iserror"]=false;

echo json_encode($result);

$file_db = null;
