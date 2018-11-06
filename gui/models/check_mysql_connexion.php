<?php
/**
 * @file   check_mysql_connexion.php
 * @date   2014/11/17
 * @author Patrice Dietsch <patrice.dietsch@gmail.com>
 * @brief  test la connexion à une base mysql.
 * @detail ceci est un cgi de type GET. le va tenter une connexion à une base mysql
 *         avec les parametres server, port, base, user et password. Si la connexion
  *        peut être établie il retourne "OK" sinon "KO" dans un json :  
 *         { result=>["OK" ou "KO"] : OK = connexion possible, KO une erreur
 *           error=>[numéro d'erreur] : les numéros d'erreur possible sont :
 *              1 : erreur inconnue ou erreur de parametre (voir error_msg)
 *             98 : demandeur pas habilité
 *             99 : demandeur pas connecté
 *              2 : erreur de connexion à la base (message db dans error_msg)
 *           errmsg=>[un message],
 *           debug=>[requete sql executée] }
 
 * @param  server  nom ou adresse du serveur mysql
 * @param  port    port d'ecoute du server
 * @param  base    nom de la base à tester
 * @param  user    compte d'accès
 * @param  passord mot de passe du compte
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

$check=check_admin();
error_log($check);
switch($check){
    case 98:
        break;
    case 99:
        echo json_encode(array('iserror' => true, 'errno'=> 98, 'errorMsg' => 'not connected', "result"=>"KO", "error"=>98, "error_msg"=>"pas habilité" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array('iserror' => true, 'errno'=> 100, 'errorMsg' => 'unknow error', "result"=>"KO", "error"=>100, "error_msg"=>"erreur inconnue (".$check.")." ));
        exit(1);
}

header('Content-type: application/json; charset=utf-8' );

if(!isset($_REQUEST['server']) ||
   !isset($_REQUEST['port']) ||
   !isset($_REQUEST['base']) ||
   !isset($_REQUEST['user']) ||
   !isset($_REQUEST['password'])){
    echo json_encode(array('iserror' => true, 'errno'=> 1, 'errorMsg' => 'parameters error', "result"=>"KO", "error"=>1, "error_msg"=>"parameters error" ));
 
    exit(1);
}else{
    $server=$_REQUEST['server'];
    $port=$_REQUEST['port'];
    $base=$_REQUEST['base'];
    $user=$_REQUEST['user'];
    $password=$_REQUEST['password'];
}

error_log($server." ".$port." ".$base." ".$password);
$dns = 'mysql:host='.$server.';dbname='.$base.";port=".$port;
try{
    $connection = new PDO(
        $dns,
        $user,
        $password, 
        array(
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_TIMEOUT => 3
        )
    );
}catch(PDOException $e){
    echo json_encode(array('iserror' => true, 'errno'=> 2, 'errorMsg' => $e->getMessage(), "result"=>"KO", "error"=>2, "error_msg"=>$e->getMessage() ));

    exit(1);
}
echo json_encode(array('iserror' => false, "result"=>"OK"));

$connection=null;
?>
