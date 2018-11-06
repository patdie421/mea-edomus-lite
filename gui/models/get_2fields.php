<?php
/**
 * @file   get_2fields-jqg_grid.php
 * @date   2014/11/17
 * @author Patrice Dietsch <patrice.dietsch@gmail.com>
 * @brief  retourne dans une même reponse les valeurs de deux champs distincts
 *         se trouvant dans deux tables distinctes
 * @detail ceci est un cgi de type GET. Il va retourner le résultat de deux requêtes
 *         SQL du type SELECT [fieldx] FROM [tablex] WHERE [wherex] avec x=1 ou 2
 *         Le resultat est retourné dans un json avec les éléments suivants :
 * @return { result=>["OK" ou "KO"] : OK = pas d'erreur, KO une erreur
 *           error=>[numéro d'erreur] : les numéros d'erreur possible sont :
 *              1 : erreur inconnue
 *             99 : demandeur pas connecté
 *              2 : parametres incorrects table, field ou value absent
 *              3 : échec d'ouverture base de données (message db dans errmsg)
 *              4 : echec de la première requete (message db dans errmsg)
 *              5 : echec de la deuxième requete (message db dans errmsg)
 *              0 : pas d'erreur (result = "OK")
 *           errmsg=>[un message],
 *           value1=>resultat de la première requete (dans un tableau json),
 *           value2=>resultat de la seconde requete (dans un tableau json)}
 * @param  table1 première table
 * @param  field1 premier champ
 * @param  where1 premier clause where
 * @param  table2 deuxième table
 * @param  field2 deuxième champ
 * @param  where2 deuxième clause where
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

switch(check_admin()){
    case 98:
        break;
    case 99:
        echo json_encode(array('iserror'=>true,"result"=>"KO","errno"=>99,"erroMsg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array('iserror'=>true,"result"=>"KO","errno"=>1,"erroMsg"=>"erreur inconnue" ));
        exit(1);
}

include_once('../lib/configs.php');

if(!isset($_GET['table1']) ||
   !isset($_GET['field1']) ||
   !isset($_GET['where1']) ||
   !isset($_GET['table2']) ||
   !isset($_GET['field2']) ||
   !isset($_GET['where2'])){
    echo json_encode(array('iserror'=>true,"result"=>"KO","errno"=>2,"errMsg"=>"parameters error 2" ));
    exit(1);
}else{
    $table1=$_GET['table1'];
    $field1=$_GET['field1'];
    $where1=$_GET['where1'];
    $table2=$_GET['table2'];
    $field2=$_GET['field2'];
    $where2=$_GET['where2'];
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array('iserror'=>true,"result"=>"KO","errno"=>3,"erroMsg"=>$e->getMessage() ));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

$SQL="SELECT $field1 FROM $table1 WHERE $where1";
try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result1 = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array('iserror'=>true,"result"=>"KO","errno"=>4,"errMsg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$SQL="SELECT $field2 FROM $table2 WHERE $where2";
try{    
    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result2 = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array('iserror'=>true,"result"=>"KO","errno"=>5,"erroMsg"=>$e->getMessage() ));
    $file_db=null;
    exit(1);
}

$values1=array();
foreach ($result1 as $elem){
    $values1[]=$elem[$field1];
}

$values2=array();
foreach ($result2 as $elem){
    $values2[]=$elem[$field2];
}

header('Content-type: application/json');

echo json_encode(array('iserror'=>false, "result" => "OK", "values1"=>$values1, "values2"=>$values2));

$file_db = null;
