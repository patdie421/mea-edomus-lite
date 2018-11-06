<?php

function error_log_REQUEST()
{
   ob_start();
   print_r($_REQUEST);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log($debug_msg);
}


function inform_and_exit_if_not_connected()
{
   switch(check_admin()){
      case 98:
         break;
      case 99:
         echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>99, "errMsg"=>"not connected" ));
         exit(1);
      case 0:
         break;
      default:
         echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>1, "errMsg"=>"unknown error" ));
         exit(1);
   }
}


function inform_and_exit_if_not_admin()
{
   switch(check_admin()) {
      case 98:
         echo json_encode(array('iserror' => true, "result"=>"KO", 'errno' => 98, 'errorMsg' => 'not administrator'));
         exit(1);
         break;
      case 99:
         echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>99, "errMsg"=>"not connected" ));
         exit(1);
      case 0:
         break;
      default:
          echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>1, "errMsg"=>"unknown error" ));
          exit(1);
   }
}


function startsWith($haystack, $needle) {
    return $needle === "" || strrpos($haystack, $needle, -strlen($haystack)) !== false;
}


function endsWith($haystack, $needle) {
    return $needle === "" || (($temp = strlen($haystack) - strlen($needle)) >= 0 && strpos($haystack, $needle, $temp) !== FALSE);
}

function getParamVal($db, $param)
{
   $response=array();

   try {
      $file_db = new PDO($db);
   }
   catch (PDOException $e) {
      return false;
   }

   $file_db->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);
   $file_db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION); // ERRMODE_WARNING | ERRMODE_EXCEPTION | ERRMODE_SILENT

   $SQL='SELECT * FROM application_parameters WHERE key = "'.$param.'"';
   try {
      $stmt = $file_db->prepare($SQL);
      $stmt->execute();
      $result = $stmt->fetchAll();
   }
   catch(PDOException $e) {
      $result = false;
   }
   $file_db=null;
   return $result;
}
