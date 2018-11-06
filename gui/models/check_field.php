<?php
/**
 * @file   check_field.php
 * @date   2014/11/17
 * @author Patrice Dietsch <patrice.dietsch@gmail.com>
 * @brief  contrôle qu'une valeur existe dans le champ d'une table.
 * @detail ceci est un cgi de type GET. Il va retourner le résultat de l'une des
 *         requetes SQL suivantes :
 *         SELECT count(*) FROM [table] WHERE [field] = [value]
 *         ou si <id> est précisé :
 *         SELECT count(*) FROM [table] WHERE [filed] = [value] AND id<>[id]
 *         Le resultat est retourné dans un json avec les éléments suivants :
 * @return { result=>["OK" ou "KO"] : OK = pas d'erreur, KO une erreur
 *           error=>[numéro d'erreur] : les numéros d'erreur possible sont :
 *              1 : erreur inconnue
 *             99 : demandeur pas connecté
 *              2 : parametres incorrects table, field ou value absent
 *              3 : échec d'ouverture base de données (message db dans errmsg)
 *              4 : echec requete (message db dans errmsg)
 *              0 : pas d'erreur (result = "OK")
 *           errmsg=>[un message],
 *           exist=>[nb d'occurence],
 *           debug=>[requete sql executée] }
 * @param  table  nom de la table à interroger
 * @param  field  nom du champ à contrôler
 * @param  value  valeur à rechercher
 * @param  id     id de la table à exclure de la recherche
 */
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

switch(check_admin()){
    case 98:
        break;
    case 99:
        echo json_encode(array("result"=>"KO","error"=>99,"error_msg"=>"non connecté" ));
        exit(1);
    case 0:
        break;
    default:
        echo json_encode(array("result"=>"KO","error"=>1,"error_msg"=>"erreur inconnue" ));
        exit(1);
}

if(!isset($_GET['table']) || !isset($_GET['field']) || !isset($_GET['value'])){
    echo json_encode(array("result"=>"KO","error"=>2,"error_msg"=>"parameters error 1" ));
    exit(1);
}else{
    $table=$_GET['table'];
    $index=$_GET['field'];
    $value=$_GET['value'];
}

$id=0;
if(isset($_GET['id'])){
    $id=$_GET['id'];
}

try {
    $file_db = new PDO($PARAMS_DB_PATH);
}catch (PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>3,"error_msg"=>$e->getMessage()));
    exit(1);
}

$file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
$file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

try {
    if($id){
        $SQL="SELECT count(*) AS exist FROM $table WHERE $index=\"$value\" AND id<>$id";
    }
    else{
        $SQL="SELECT count(*) AS exist FROM $table WHERE $index=\"$value\"";
    }

    $stmt = $file_db->prepare($SQL);
    $stmt->execute();
    $result = $stmt->fetchAll();
}catch(PDOException $e){
    echo json_encode(array("result"=>"KO","error"=>4,"error_msg"=>$e->getMessage(),"debug"=>$SQL ));
    $file_db=null;
    exit(1);
}
$exist=$result[0]['exist'];

header('Content-type: application/json');

echo json_encode(array("result"=>"OK","exist"=>$exist,"error"=>0));

$file_db = null;

