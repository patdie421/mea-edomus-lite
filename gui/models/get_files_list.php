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

if(!isset($_GET['type'])){
    echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>2, "errMsg"=>"parameters error" ));
    exit(1);
}else{
    $type=$_GET['type'];
}

$param=false;
$path=false;
$excludes=false;
$subpath=false;
$prefixes=false;
$extentions=false;

function validPrefixes($dirEntry, $prefixes)
{
   if($prefixes == False)
      return True;

   foreach ($prefixes as $prefix){
      if(startsWith($dirEntry, $prefix))
         return True;
   }
   return False;
}

function validExts($dirEntry, $exts)
{
   foreach ($exts as $ext){
      if(endsWith($dirEntry, "." . $ext))
         return True;
   }
   return False;
}


if($type=='srules')
{
   $param="RULESFILESPATH";
   $extentions=array("srules");
}
else if($type=='rset')
{
   $param="RULESFILESPATH";
   $extentions=array("rset");
}
else if($type=='rules')
{
   $param="RULESFILESPATH";
   $extentions=array("rules");
   $automator=getParamVal($PARAMS_DB_PATH, "RULESFILE");
   $automator=$automator[0]{'value'};
   preg_replace('#/+#','/',$automator);
   $excludes=array($automator);
}
else if($type=='map')
{
   $param="GUIPATH";
   $subpath="maps";
   $extentions=array("map");
}
else if($type=='menu')
{
   $param="GUIPATH";
   $subpath="maps";
   $extentions=array("menu");
}
else if($type=='img')
{
   $param="GUIPATH";
   $subpath="images";
   $extentions=array("jpg","png");
}
else if($type=='widget')
{
   $param="GUIPATH";
   $subpath="widgets";
   $extentions=array("js");
   $prefixes=array("meawidget_");
   $excludes=array("meawidgets.js");
}
else
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>3, "errMsg"=>"type unknown" ));
   exit(1);
}

if($param!=false)
{
   $res=getParamVal($PARAMS_DB_PATH, $param);
   if($res!=false)
   {
      $res=$res[0];
      $path=$res{'value'};
      if($subpath!=false)
         $path=$path."/".$subpath;
   }
}
else
{
   echo json_encode(array('iserror'=>true, "result"=>"KO", "errno"=>4, "errMsg"=>"can't get parameter" ));
   exit(1);
}

$values=array();

if(is_dir($path))
{
   $dirContents = scandir($path);
   if( count($dirContents) > 2 ) // si plus que . et ..
   {
      natsort($dirContents);

      foreach( $dirContents as $dirEntry ) {
         $dirEntryFullPath=$path . "/" . $dirEntry;
         preg_replace('#/+#','/', $dirEntryFullPath);
         
         if($excludes <> false && in_array($dirEntryFullPath, $excludes))
            continue;
         
         if( substr($dirEntry, 0, 1)!="." && !is_dir($dirEntryFullPath) && validExts($dirEntry, $extentions) && validPrefixes($dirEntry, $prefixes))
         {
            array_push($values, $dirEntry);
         }
      }
   }
}

echo json_encode(array('iserror'=>false, "result"=>"OK", "values"=>$values, "errno"=>0));
