<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

session_start();

header('Content-type: application/json');

switch(check_admin()){
    case 99:
        error_log("not connected");
        echo json_encode(array('iserror' => true, 'errno' => 99, 'errorMsg' => 'not connected'));
        exit(1);
    default:
        if (isset($_SESSION['userid']))
        {
           $_SESSION['userid'] = $_SESSION['userid'];
           echo json_encode(array('isError' => false));
        } 
        else
        {
           echo json_encode(array('iserror' => true, 'errno' => 98, 'errorMsg' => 'unknown error'));
        }
        exit(1);
}
?>
