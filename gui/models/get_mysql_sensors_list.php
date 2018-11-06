<?php
/**
 * @file   check_mysql_connexion.php
 * @date   2016/03/15
 * @author Patrice Dietsch <patrice.dietsch@gmail.com>
 * @brief  récupération des données d'historique dans mysql.
 */
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

$server="127.0.0.1";
$port="3306";
$base="domotique";
$user="domotique";
$password="maison";


header('Content-type: application/json');

$check=check_admin();
$check=98;
switch($check)
{
   case 98:
      break;
   case 99:
      exit(1);
   case 0:
      break;
   default:
      exit(1);
}

$sql = "SELECT 
           sensors_names.sensor_id AS sensor_id,
           sensors_names.name AS sensor_name,
           sensors_names.description AS sensor_description,
           collectors_names.collector_id AS collector_id,
           collectors_names.name AS collector_name
       FROM
           sensors_names
       INNER JOIN collectors_names
       ON sensors_names.collector_id = collectors_names.collector_id;";

$dns = 'mysql:host='.$server.';dbname='.$base.";port=".$port;
try
{
   $db = new PDO(
      $dns,
      $user,
      $password,
      array(
         PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
         PDO::ATTR_TIMEOUT => 3
      )
   );
} catch(PDOException $e)
{
   die($e->getMessage());
}

$rows = array();

$request = $db->query($sql);

$id = "{"."\"sensor_id\":\"-1\", \"text\":\"\"}";
 $val = [
         "id" => $id,
         "text" => "NONE",
      ];
      $rows[]=$val; 

try
{
   while($row = $request->fetch(PDO::FETCH_OBJ))
   {
      $description = $row->sensor_description;
      if(strlen($description)>0)
      {
         $text =  $row->sensor_name." - ".$description;
      }
      else
      {
         $text = $row->sensor_name;
      }
      $text2 = $row->sensor_name." (".$row->collector_name.")";
      
      $id = "{"."\"sensor_id\":\"".$row->sensor_id."\", \"collector_id\":\"". $row->collector_id."\", \"text\":\"".$text."\", \"text2\":\"".$text2."\"}";

      $val = [
         "id" => $id,
         "text" => $text,
         "group" => $row->collector_name 
      ];

      $rows[]=$val; 
   }

   print json_encode($rows);
}
catch(PDOException $e)
{
   $error_msg=$e->getMessage();
   $db=null;
   die($error_msg);
   echo "console.log(' error  = $error_msg');\n";
}

$db=null;
?>
