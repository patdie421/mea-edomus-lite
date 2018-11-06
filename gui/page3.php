<?php
include_once('lib/configs.php');
session_start();
if(isset($_SESSION['language']))
{
   $LANG=$_SESSION['language'];
}
include_once('lib/php/translation.php');
include_once('lib/php/'.$LANG.'/translation.php');
mea_loadTranslationData($LANG,'');
?>

<script>
jQuery(document).ready(function(){
<?php
   if(isset($_REQUEST['view'])) {
      echo "var destview=\""; echo $_REQUEST['view']; echo "\";\n";
   }
   else {
      echo "var destview=\"\";\n";
   }
?>
   if(destview=="")
      destview=translationController.toLocalC('application');
   else
      destview=destview.mea_hexDecode();
   
   // un peu de ménage pour éviter les doublons ...
   $("#dlg_us").parent().remove();
   $("body").find(".combo-p").remove();
   $("body").find(".window-shadow").remove();
   $("body").find(".window-mask").remove();

   page3_ctrlr = new TabsPageController("page3_tab");
   page3_ctrlr.linkToTranslationController(translationController);
   page3_ctrlr.linkToCredentialController(credentialController); // pour la gestion des habilitations

   page3_ctrlr.start(destview);
});
</script>

<div id="page3_tab" class="easyui-tabs" border=false fit=true>
    <div title="<?php mea_toLocalC('application'); ?>" href="views/application-view.php" style="padding:20px;"></div>
    <div title="<?php mea_toLocalC('users'); ?>" href="views/utilisateurs-view.php"></div>
</div>
