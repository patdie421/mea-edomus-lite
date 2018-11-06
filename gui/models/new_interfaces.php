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
    case 98:
        error_log("not administrator");
        echo json_encode(array('iserror' => true, 'errno' => 98, 'errorMsg' => 'not administrator'));
        exit(1);
        break;
    case 99:
        error_log("not connected");
        echo json_encode(array('iserror' => true, 'errno' => 99, 'errorMsg' => 'not connected'));
        exit(1);
    case 0:
        break;
    default:
        error_log("unknown error");
        echo json_encode(array('iserror' => true, 'errno' => 100, 'errorMsg' => 'unknown'));
        exit(1);
}


$id_interface = $_REQUEST['id_interface'];
$name = $_REQUEST['name'];
$id_type = $_REQUEST['id_type'];
$description = $_REQUEST['description'];
$dev = $_REQUEST['dev'];
$parameters = $_REQUEST['parameters'];
$state = $_REQUEST['state'];


// connexion à la db
try {
    $file_db = new PDO($PARAMS_DB_PATH);
}
catch(PDOException $e) {

    $error_msg=$e->getMessage(); 
    error_log($error_msg);
    echo json_encode(array('iserror' => true, 'errorMsg' => $error_msg));
    exit(1);
}
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$next_id=0;
$SQL = "SELECT MAX(id_interface) AS max_id FROM interfaces";
try {
   $request = $file_db->query($SQL);
   $row = $request->fetch(PDO::FETCH_NUM);
   $next_id=$row[0];
}
catch (PDOException $e) {
   $error_msg=$e->getMessage(); 
   error_log($error_msg);
   echo json_encode(array('iserror' => true, 'errorMsg' => $error_msg));
   $file_db=null;
   exit(1);
}
$next_id=$next_id+1;

$SQL= "INSERT INTO interfaces (id_interface, name, id_type, description, dev, parameters, state)
          VALUES (:id_interface, :name, :id_type, :description, :dev, :parameters, :state)";
try{
   $stmt = $file_db->prepare($SQL);
   $stmt->execute(
       array(
         ":id_interface" => $next_id,
         ":name"         => $name,
         ":id_type"      => $id_type,
         ":description"  => $description,
         ":dev"          => $dev,
         ":parameters"   => $parameters,
         ":state"        => $state,
      )
   );
}
catch (PDOException $e) {
   $error_msg=$e->getMessage();
   error_log($error_msg);
   echo json_encode(array('iserror' => true, 'errno' => '3', 'errorMsg' => $error_msg));
   $file_db=null;
   exit(1);
}

/*
$SQL="INSERT INTO interfaces (id_interface, name, id_type, description, dev, state)
      VALUES ('$next_id','$name', '$id_type', '$description','$dev','$state')";
try {
   $request = $file_db->query($SQL);
}
catch (PDOException $e) {
   $error_msg=$e->getMessage(); 
   error_log($error_msg);
   echo json_encode(array('iserror' => true, 'errorMsg' => $error_msg));
   $file_db=null;
   exit(1);
}
*/
  
echo json_encode(array('iserror' => false, 'errorMsg' => 'done'));

$file_db=null;
