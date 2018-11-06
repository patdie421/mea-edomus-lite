<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');
session_start();

$root="";
// $root="/usr/local";
$exclusions = array("/etc", "/var", "/usr", "/bin", "/usr/bin", "/proc", "/dev", "/System", "/private");

if(isset($DEBUG_ON) && ($DEBUG_ON == 1))
{
   ob_start();
   print_r($_REQUEST);
   $debug_msg = ob_get_contents();
   ob_end_clean();
   error_log($debug_msg);
}

$id = isset($_REQUEST['id']) ? $_REQUEST['id'] : "/";
$type = isset($_REQUEST['type']) ? $_REQUEST['type'] : "directory";


function getDirs($root,$dir,$exclusions,$checkChildsOnly=false)
{
  // si checkChildsOnly = true la fonction regarde uniquement s'il exist au moins 1 sous répertore
   if($checkChildsOnly===true)
      $result=false;
   else
      $result=array();
   
   if(is_dir($root . $dir))
   {
      $dirContents = scandir($root . $dir);
        
      if( count($dirContents) > 2 ) /* si plus que . et .. */
      {
         natsort($dirContents);
         // natcasesort($dirContents);
         
         foreach( $dirContents as $dirEntry ) {
            $dirEntryFullPath=$root . $dir . $dirEntry;
            if(   substr($dirEntry, 0, 1)!="."
               && is_dir($dirEntryFullPath)
               && !in_array($dirEntryFullPath, $exclusions) )
            {
               if($checkChildsOnly)
                  return true;
               else
               {
                  $node = array();
                  $node['id'] = $dirEntryFullPath . "/";
                  $node['text'] = $dirEntry;
                  if(getDirs($root,$node['id'],$exclusions,true)===true)
                  {
                     $node['state'] = 'closed';
                  }
                  else
                  {
                     $node['state'] = 'open';
                     $node['iconCls'] = 'icon-rep';
                  }
                  array_push($result, $node);
               }
            }
         }
      }
   }
   return $result;
}

if($type=="directory")
{
   $dirs=getDirs($root,$id,$exclusions);
   if($dirs === false || $dirs === true)
      $dirs=array();
   echo json_encode($dirs);
   exit(0);
}

if($type=="file")
{
   $files=array(); // à remplacer par getFiles() ...
   echo json_encode($files);
   exit(0);
}

echo json_encode(array());

// http://jsfiddle.net/568Lzoe7/3/
