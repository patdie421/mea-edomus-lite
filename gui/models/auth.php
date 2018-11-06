<?php
include_once('../lib/configs.php');
session_start();

header('Content-type: application/json');

if(isset($_GET['user_name'])){
    $user_name = trim($_GET['user_name']);
}else{
    echo json_encode(array("retour"=>-1, "error"=>1, "errormsg"=>"user_name absent"));
    exit();
}
if(isset($_GET['user_password'])){
    $user_password = $_GET['user_password'];
}else{
    echo json_encode(array("retour"=>-1, "error"=>2, "errormsg"=>"user_password absent"));
    exit();
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("retour"=>-1, "error"=>3,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

try {
    $SQL="SELECT id, flag, profil FROM users WHERE name=\"$user_name\" AND password=\"$user_password\"";

    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("retour"=>-1,"error"=>4,"error_msg"=>$e->getMessage(),"debug"=>$SQL ));
    exit(1);
}

// stockage de session ID dans la base (ajouter une colonne "session_id");
header('Content-type: application/json');

if($result){
    try {
        $id=$result[0]['id'];
        $session_id=session_id();
        $SQL="INSERT INTO sessions (userid,sessionid) VALUES (\"$user_name\",\"$session_id\")";
        $stmt = $file_db->prepare($SQL);
        $stmt->execute();
    } catch (PDOException $e){
        error_log($e->getMessage());
        $file_db=null;
        exit(1);
    }

    $_SESSION['userid']=$user_name;
    $_SESSION['profil']=(int)$result[0]['profil'];
    if($result[0]['flag']!=1)
        $_SESSION['logged_in']=1;
    else
        $_SESSION['change_passwd_flag']=1;
    $_SESSION['flag']=$result[0]['flag'];
    
    echo json_encode(array("retour"=>1, "flag"=>$result[0]['flag']));
}else{
    session_destroy();
    echo json_encode(array("retour"=>0, "erreur"=>"identifiant et/ou mot de passe incorrect"));
}

$file_db = null;
