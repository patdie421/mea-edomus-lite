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
      destview=translationController.toLocalC('sensors/actuators');
   else
      destview=destview.mea_hexDecode();

   // un peu de ménage pour éviter les doublons ...
   // les boites de dialogue et objets associés (forme, boutons, données de combobox, ...) reste présents lorsqu'on
   // recharge ce fichier une seconde fois (ils sont rattachés à <body> pars jquery easyui). On nettoie donc avant 
   // de recharger le premier onglet de cette page. On ne peut pas nétoyer avant de sortir de la page, alors on fait
   // le ménage avant ...
   $("#dlg_sa").parent().remove();
   $("#dlg_in").parent().remove();
   $("#dlg_ty").parent().remove();
   $("#dlg_lo").parent().remove();

   $("body").find(".combo-p").remove(); // données "temporaires" des combobox
   $("body").find(".window-shadow").remove(); // reste des boites de dialogue
   $("body").find(".window-mask").remove(); // reste des boites de dialogue
  
   page2_ctrlr = new TabsPageController("page2_tab");
   page2_ctrlr.linkToTranslationController(translationController);
   page2_ctrlr.linkToCredentialController(credentialController); // pour la gestion des habilitations
   page2_ctrlr.start(destview);
});
</script>

<div id="page2_tab" class="easyui-tabs" border=false fit=true>
    <div title="<?php mea_toLocalC('sensors/actuators'); ?>" href="views/capteurs-actionneurs-view.php"></div>
    <div title="<?php mea_toLocalC('interfaces'); ?>" href="views/interfaces-view.php"></div>
    <div title="<?php mea_toLocalC('types'); ?>" href="views/types-view.php"></div>
    <div title="<?php mea_toLocalC('Locations'); ?>" href="views/localisations-view.php"></div>
</div>
