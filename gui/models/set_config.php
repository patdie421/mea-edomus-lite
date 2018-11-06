<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

$check=check_admin();
switch($check){
    case 98:
        $response["iserror"]=true;
        $response["errno"]=98;
        $response["errorMsg"]='not administrator';
        // pour compatibilité
        $response["result"]="KO"; // pour la compatibilité
        $response["error"]=$response["errno"]; // pour la compatibilité      
        $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité
        // fin pour compatibilité
        echo json_encode($response);
        exit(1);
        
    case 99:
        $response["iserror"]=true;
        $response["errno"]=99;
        $response["errorMsg"]='not logged in';
        // pour compatibilité
        $response["result"]="KO"; // pour la compatibilité
        $response["error"]=$response["errno"]; // pour la compatibilité      
        $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité
        // fin pour compatibilité
        echo json_encode($response);
        exit(1);
    case 0:
        break;
        
    default:
        $response["iserror"]=true;
        $response["errno"]=100;
        $response["errorMsg"]='unknown error ('.$scheck.')';
        // pour compatibilité
        $response["result"]="KO"; // pour la compatibilité
        $response["error"]=1; // pour la compatibilité      
        $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité
        // fin pour compatibilité
        echo json_encode($response);
        exit(1);
}

//$fields=[];
$fields[0]="VENDORID";
$fields[1]="DEVICEID";
$fields[2]="INSTANCEID";
$fields[3]="PLUGINPATH";
$fields[4]="DBSERVER";
$fields[5]="DBPORT";
$fields[6]="DATABASE";
$fields[7]="USER";
$fields[8]="PASSWORD";
$fields[9]="BUFFERDB";

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}
catch (PDOException $e)
{
    $error_msg=$e->getMessage();
    $response["iserror"]=true;
    $response["errno"]=2;
    $response["errorMsg"]=$error_msg;
    // pour compatibilité
    $response["result"]="KO"; // pour la compatibilité
    $response["error"]=1; // pour la compatibilité      
    $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité
    // fin pour compatibilité
    echo json_encode($response);
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="UPDATE application_parameters set value=:value WHERE key=:key";
try{
   $stmt = $file_db->prepare($SQL);
}
catch (PDOException $e) {
   $error_msg=$e->getMessage();
   $response["iserror"]=true;
   $response["errno"]=3;
   $response["errorMsg"]=$error_msg;
   // pour compatibilité
   $response["result"]="KO"; // pour la compatibilité
   $response["error"]=2; // pour la compatibilité      
   $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité
   // fin pour compatibilité
   echo json_encode($response);
   $file_db=null;
   exit(1);
}

try{
   for($i=0;$i<10;$i++) {
      $stmt->execute(
         array(
            ":key"   => $fields[$i],
            ":value" => $_GET[$fields[$i]]
         )
      );
   }
}
catch(PDOException $e)
{
   $error_msg=$e->getMessage();
   $response["iserror"]=true;
   $response["errno"]=3;
   $response["errorMsg"]=$error_msg;
   // pour compatibilité
   $response["result"]="KO"; // pour la compatibilité
   $response["error"]=2; // pour la compatibilité      
   $response["error_msg"]=$response["errorMsg"]; // pour la compatibilité
   // fin pour compatibilité
   echo json_encode($response);
   $file_db=null;
   exit(1);
}


$response["iserror"]=false;
// pour compatibilité
$response["retour"]="OK"; // pour la compatibilité
// fin pour compatibilité
echo json_encode($response);


$file_db = null;
