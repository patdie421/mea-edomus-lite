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


header('Content-Type: text/javascript');


$check=check_admin();

switch($check)
{
   case 98:
      break;
   case 99:
      echo "console.log('pas habilité');\n";
      echo $callback."([]);";
      exit(1);
   case 0:
      break;
   default:
      echo "console.log('erreur inconnue');\n";
      echo $callback."([]);";
      exit(1);
}


if(!isset($_REQUEST['sensor_id']))
{
   echo "console.log('sensor_id is mandarory');\n";
   echo $callback."([]);";
   exit(1);
}
else
{
   $sensor_id=$_REQUEST['sensor_id'];
}

if(!isset($_REQUEST['collector_id']))
{
   echo "console.log('collector_id is mandarory');\n";
   echo $callback."([]);";
   exit(1);
}
else
{
   $collector_id=$_REQUEST['collector_id'];
}

if(!isset($_REQUEST['id']))
{
   echo "console.log('id is mandarory');\n";
   echo $callback."([]);";
   exit(1);
}
else
{
   $id=$_REQUEST['id'];
}

$callback = $_GET['callback']; 
if (!preg_match('/^[a-zA-Z0-9_]+$/', $callback))
{
   echo "console.log('Invalid callback name: ".$start."');\n";
   echo $callback."([]);";
   exit(1);
} 
 
$start = @$_GET['start']; 
if ($start && !preg_match('/^[0-9]+$/', $start))
{ 
   echo "console.log('Invalid start parameter: ".$start."');\n";
   echo $callback."([]);";
   exit(1);
} 
 
$end = @$_GET['end']; 
if ($end && !preg_match('/^[0-9]+$/', $end))
{ 
   echo "console.log('Invalid end parameter: ".$end."');\n";
   echo $callback."([]);";
   exit(1);
} 


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

if (!$end)
   $end = (time())*1000; 
$end = $end + (3600*1000); // on va un peu plus loin pour être sûr de tout avoir

// construction des requetes sur les différentes tables
$min=0;
$max=0;
$sql = "SELECT UNIX_TIMESTAMP(min(date))*1000 AS min, UNIX_TIMESTAMP(max(date))*1000 AS max FROM sensors_values";
$request = $db->query($sql);
$row = $request->fetch(PDO::FETCH_OBJ);
if($row)
{
   $min = $row->min;
   $max = $row->max;
}
$request = NULL;

$min_c=0;
$max_c=0;
$sql = "SELECT UNIX_TIMESTAMP(min(date))*1000 AS min, UNIX_TIMESTAMP(max(date))*1000 AS max FROM sensors_values_c";
$request = $db->query($sql);
$row = $request->fetch(PDO::FETCH_OBJ);
if($row)
{
   $min_c = $row->min;
   $max_c = $row->max;
}
$request = NULL;

if(!$start || $start == 0)
   $start=$min_c;

$startTime = False;
$endTime = False;
$sartTime_c = False;
$endTime_c = False;
if($end < $max_c)
{
   $endTime_c = $end;
   $startTime_c = $start;
}
else if($start >= $max_c)
{
   $startTime = $start;
   $endTime = $end;
}
else
{
   $startTime_c = $start;
   $endTime_c = $max_c;
   $startTime = $max_c;
   $endTime = $end;
}

$sql=False;
$sql_c=False;

error_log("sensor_id: ".$sensor_id);

if(($startTime_c != False) && ($endTime_c != False))
{
   $s = gmstrftime('%Y-%m-%d %H:%M:%S', $startTime_c / 1000);
   $e = gmstrftime('%Y-%m-%d %H:%M:%S', $endTime_c / 1000);

   error_log($s." ".$e);
//   echo "console.log(' start_c = $s, end_c = $e ');\n";
   $sql_c="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg1 AS avg,
         min1 AS min,
         max1 AS max,
         min(date) AS date,
         (UNIX_TIMESTAMP(date) DIV 3600 * 3600 + 1800) *1000 AS tms
      FROM sensors_values_c
      WHERE sensor_id=".$sensor_id." AND collector_id=".$collector_id." AND (date between '$s' AND '$e')
      GROUP BY sensor_id, DATE_FORMAT(date, \"%Y-%m-%d %H\")
      ORDER BY tms
      LIMIT 0, 5000;";
}

//         UNIX_TIMESTAMP(max(date))*1000 AS tms
//      ORDER BY tms
if(($startTime != False) && ($endTime != False))
{
   $range = $end - $start; 

   $s = gmstrftime('%Y-%m-%d %H:%M:%S', $startTime / 1000);
   $e = gmstrftime('%Y-%m-%d %H:%M:%S', $endTime / 1000);
   error_log($s." ".$e);

//   echo "console.log(' start = $s, end = $e, range = $range');\n";
   if($range < 3 * 24 * 3600 * 1000)
   {
      $sql="SELECT
         sensor_id AS id,
         1 as nb,
         value1 AS avg,
         value1 AS min,
         value1 AS max,
         date AS date,
         (UNIX_TIMESTAMP(date))*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND collector_id=".$collector_id." AND (date between '$s' AND '$e')
      ORDER BY date
      LIMIT 0, 5000;";
   }
   else
   {
      $sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         (UNIX_TIMESTAMP(date) DIV 3600 * 3600 + 1800) *1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND collector_id=".$collector_id." AND (date between '$s' AND '$e')
      GROUP BY sensor_id, DATE_FORMAT(date, \"%Y-%m-%d %H\")
      ORDER BY date
      LIMIT 0, 5000;";
   }
}

/*
   $sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         UNIX_TIMESTAMP( max(date) )*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND date between '$startTime' AND '$endTime'
      GROUP BY sensor_id, UNIX_TIMESTAMP(date) DIV 900
      ORDER BY tms
      LIMIT 0, 5000;";

   $sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         (UNIX_TIMESTAMP(max(date)))*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND date between '$startTime' AND '$endTime'
      GROUP BY sensor_id, DATE_FORMAT(date, \"%Y-%m-%d %H\")
      ORDER BY tms
      LIMIT 0, 5000;";

   $sql="SELECT
         sensor_id AS id,
         count(sensor_id) AS nb,
         avg(value1) AS avg,
         min(value1) AS min,
         max(value1) AS max,
         min(date) AS date,
         UNIX_TIMESTAMP(max(date))*1000 AS tms
      FROM sensors_values
      WHERE sensor_id=".$sensor_id." AND date between '$startTime' AND '$endTime'
      GROUP BY sensor_id, date(date)
      ORDER BY tms
      LIMIT 0, 5000;";
}
*/

$rows = array();
//error_log($sql);
//error_log($sql_c);
try
{
   if($sql_c != False)
   {
      $request = $db->query($sql_c);
      while($row = $request->fetch(PDO::FETCH_OBJ))
      {
         $rows[]="[".$row->tms.",".number_format($row->avg,2,'.','')."]"; 
      }
   }

   if($sql != False)
   {
      $request = $db->query($sql);
      while($row = $request->fetch(PDO::FETCH_OBJ))
      {
         $rows[]="[".$row->tms.",".number_format($row->avg,2,'.','')."]";
      }
   }

   $rows[]="[".(time()*1000).",null]";

   echo $callback ."({id: $id, data: [\n" . join(",\n", $rows) ."\n]});"; 
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
