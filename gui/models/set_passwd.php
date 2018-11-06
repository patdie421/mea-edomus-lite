<?php
include_once('../lib/configs.php');
session_start();

header('Content-type: application/json');

if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   ob_start();
   print_r($_REQUEST);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log($debug_msg);
}

if($_SESSION['flag']==2) {
    echo json_encode(array("iserror" => true, "errno"=>1,"errorMsg"=>"password modification not allowed" ));
    exit(1);
}


if(isset($_REQUEST['new_password'])){
    $new_password=$_REQUEST['new_password'];
}
else
{
   echo json_encode(array("iserror" => true, "errno"=>2,"errorMsg"=>"parameter error" ));
   exit(1);
}

$old_password="";
if(!isset($_SESSION['change_passwd_flag']))
{
   if(isset($_REQUEST['old_password'])){
      $old_password=$_GET['old_password'];
   }
   else
   {
      echo json_encode(array("iserror" => true, "errno"=>3,"errorMsg"=>"parameter error" ));
      exit(1);
   }
}

$user_name=$_SESSION['userid'];


if(isset($_SESSION['logged_in']))
{
   $SQL= "UPDATE users SET "
           ."password=:new_password "
        ."WHERE name=:name "
        ."AND password=:old_password";
//    $SQL_UPDATE="UPDATE users SET password=\"$new_password\" WHERE name=\"$user_name\" AND password=\"$old_password\"";
}else{
   $SQL= "UPDATE users SET "
           ."password=:new_password,"
           ."flag=0 "
        ."WHERE name=:name";
//    $SQL_UPDATE="UPDATE users SET password=\"$new_password\", flag=0 WHERE name=\"$user_name\"";
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("iserror" => true, "errno"=>4,"errorMsg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

try{
   $stmt = $file_db->prepare($SQL);
   $stmt->execute(
      array(
         ":name"        => $user_name,
         ":new_password"=> $new_password,
//         ":old_password"=> $old_password,
      )
   );
}
catch (PDOException $e) {
   $error_msg=$e->getMessage();
   error_log($error_msg);
   echo json_encode(array('iserror' => true, 'errno' => 5, 'errorMsg' => $error_msg));
   $file_db=null;
   exit(1);
}


/*
try{
    $stmt = $file_db->prepare($SQL_UPDATE);
    $stmt->execute();
}catch(PDOException $e){
    echo json_encode(array("iserror" => true, "errno"=>3,"erroMsg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}
*/
  
if($stmt->rowCount()>0){
    echo json_encode(array("retour"=>0, 'iserror' => false));
    $_SESSION['logged_in']=1;
}else{
    echo json_encode(array("retour"=>99, 'iserror' => false));
}

$file_db = null;
