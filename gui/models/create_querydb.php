<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

switch(check_admin()){
    case 98:
        echo json_encode(array("result"=>"KO","error"=>98,"error_msg"=>"pas habilité" ));
        exit(1);
    case 99:
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO","error"=>1,"error_msg"=>"erreur inconnue" ));
        exit(1);
}

header('Content-type: application/json; charset=utf-8' );

if(!isset($_GET['querydb'])) {
    echo json_encode(array("result"=>"KO", "error"=>2,"error_msg"=>"parameters error" ));
    exit(1);
}else{
    $querydb=$_GET['querydb'];
}

try{
    $file_db = new PDO("sqlite:".$querydb);
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$filename = $QUERYDB_SQL;
if(!$file = fopen($filename, "r")) {
    $err=error_get_last();
    echo json_encode(array("result"=>"KO", "error"=>4,"error_msg"=>utf8_encode($err["message"]) )) ;
    exit(1);
}
if(!$sql = fread($file, filesize($filename))) {
    echo json_encode(array("result"=>"KO", "error"=>4,"error_msg"=>utf8_encode($err["message"]) )) ;
    exit(1);
}
fclose($file);

$inst = explode(";", $sql);

$n = count($inst);
for($i=0;$i<$n;$i++){
	if(trim($inst[$i])!=""){
		error_log("Instruction $i : ".$inst[$i]);
        try{
            $stmt = $file_db->prepare($inst[$i]);
            $stmt->execute();
            $result = $stmt->fetchAll();
        }catch(PDOException $e){
            error_log( $e->getMessage() );
            echo json_encode(array("result"=>"KO", "error"=>6,"error_msg"=>$e->getMessage() ));
            $file_db=null;
            exit(1);
        }
    }
}
    
echo json_encode(array("result"=>"OK"));

$file_db=null;
?>
