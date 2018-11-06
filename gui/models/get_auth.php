<?php
include_once('../lib/configs.php');
session_start();

if( !isset($_SESSION['userid']) ||
    !isset($_SESSION['logged_in']) ||
    !isset($_SESSION['profil'])) {
        echo json_encode(array("result"=>"KO", "iserror" => true, "errno"=>99,"errorMsg"=>"non connecté" ));
        exit(1);
}

echo json_encode(array("result"=>"OK", "iserror" => false, "userid"=>$_SESSION['userid'], "logged_in"=>(int)$_SESSION['logged_in'], "profil"=>(int)$_SESSION['profil']));

$file_db = null;
