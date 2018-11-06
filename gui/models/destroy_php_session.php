<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

switch(check_admin()){
    case 98:
        if(isset($_POST['session_id'])) {
            echo json_encode(array("result"=>"KO", 'iserror'=>true, "errno"=>98,"errorMsg"=>"pas habilité" ));
            exit(1);
        }
        break;
    case 99:
        echo json_encode(array("result"=>"KO",'iserror'=>true,"errno"=>99,"errorMsg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO",'iserror'=>true,"errno"=>100,"erroMsg"=>"erreur inconnue" ));
        exit(1);
}

if(isset($_POST['session_id'])){
    $session_id = $_POST['session_id'];
} else {
    $session_id = session_id();
}


$my_session_id=session_id();
session_write_close();
session_id($session_id);
session_start();
if (ini_get("session.use_cookies")) {
    $params = session_get_cookie_params();
    setcookie(session_name(), '', time() - 42000,
        $params["path"], $params["domain"],
        $params["secure"], $params["httponly"]
    );
}
session_destroy();


try{
    $file_db = new PDO($PARAMS_DB_PATH);
}catch(PDOException $e){
    echo json_encode(array('iserror'=>true,"result"=>"KO","errno"=>1,"errorMsg"=>$e->getMessage() ));
    exit(1);
}
$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="DELETE FROM sessions WHERE sessionid=\"$session_id\"";
try{
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
}catch(PDOException $e){
    echo json_encode(array('iserror'=>true,"result"=>"KO","errno"=>2,"errorMsg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}
$stmt=null;
$file_db=null;


header('Content-type: application/json');
echo json_encode(array("result"=>"OK", 'iserror'=>false));
