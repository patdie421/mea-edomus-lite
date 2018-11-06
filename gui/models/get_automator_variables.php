<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
include_once('../lib/php/tools.php');
session_start();

$DEBUG_ON=1;
if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   error_log_REQUEST();
}

header('Content-type: application/json');


inform_and_exit_if_not_connected();

function validExts($dirEntry, $exts)
{
   foreach ($exts as $ext){
      if(endsWith($dirEntry, "." . $ext))
         return True;
   }
   return False;
}


$res=getParamVal($PARAMS_DB_PATH, "RULESFILESPATH");
if($res!=false)
{
   $res=$res[0];
   $path=$res{'value'};
}

$res=getParamVal($PARAMS_DB_PATH, "RULESFILE");
if($res!=false)
{
   $res=$res[0];
   $automator_file=$res{'value'};
}


function cmp($a, $b)
{
   if ($a["group"].$a["name"] == $b["group"].$b["name"]) {
       return 0;
   }
   return ($a["group"].$a["name"] < $b["group"].$b["name"]) ? -1 : 1;
}


$extentions=array("rules");
$values=array();
$automator_current_vars=array();
$automator_other_vars=array();
$rows=array();

if(is_dir($path))
{
   $dirContents = scandir($path);
   if( count($dirContents) > 2 ) // si plus que . et ..
   {
      natsort($dirContents);

      foreach( $dirContents as $dirEntry ) {
         $dirEntryFullPath=$path . "/" . $dirEntry;
         preg_replace('#/+#','/', $dirEntryFullPath);

         if($dirEntry <> "." && $dirEntry <> ".." &&  validExts($dirEntry, $extentions))
         {         
            $rulesJSON = file_get_contents($dirEntryFullPath);
            $rules = json_decode($rulesJSON, true);

            foreach($rules["inputs"] as $rule)
            {
               if($rule["value"] <> "<NOP>" && $rule["value"]<>"<LABEL>")
               {
                  if($dirEntryFullPath == $automator_file)
                  {
                     $automator_vars[$rule["name"]]="current";
                  }
                  else
                  {
                     if(!array_key_exists($rule["name"], $automator_vars))
                     {
                        $automator_vars[$rule["name"]]="other";
                     }
                  }
               }
            } 
         }
      }

      foreach($automator_vars as $key => $value)
      {
         $val = [
            "name" => $key,
            "text" => $key,
            "group" => $value
         ];
         $rows[]=$val;
      }
   }

   usort($rows, "cmp");
}

echo json_encode($rows);
