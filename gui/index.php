<?php
//
//  PAGE PRINCIPALE (VIEW) : home page
//
include_once('lib/configs.php');
include_once('lib/php/auth_utils.php');

session_start();
if(isset($_SESSION['language']))
{
   $LANG=$_SESSION['language'];
}
include_once('lib/php/translation.php');
include_once('lib/php/'.$LANG.'/translation.php');
mea_loadTranslationData($LANG,'');
?>

<!DOCTYPE html>

<?php
// contrôle et redirections
if(!isset($_SESSION['logged_in']))
{
   $dest=$_SERVER['PHP_SELF'];
   echo "<script>window.location = \"login.php?dest=$dest\";</script>";
   exit();
}

if(check_map_only() == 0)
{
   echo "<script>window.location = \"maps.php\";</script>";
}

$isadmin = check_admin();
?>


<html>
<head>
   <title>
   <?php echo $TITRE_APPLICATION;?>
   </title>
   <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
   <meta name="viewport" content="width=device-width, initial-scale=1, minimun-scale=1.0, maximun-scale=1.0, user-scalable=no">
   <meta name="apple-mobile-web-app-capable" content="yes" />
   <meta name="description" content="domotique DIY !">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/default/easyui.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/icon.css">
   <link rel="stylesheet" type="text/css" href="lib/jquery-easyui-1.4.4/themes/color.css">
   <link rel="stylesheet" type="text/css" href="lib/mea-edomus.css">
<!--
   <link rel='stylesheet' href='lib/bgrins-spectrum/spectrum.css' />
-->
</head>
   
<script src="lib/ace/src-noconflict/ace.js" type="text/javascript"></script>

<script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.min.js"></script>

<script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.easyui.min.js"></script>
<?php
echo '<script type="text/javascript" src="lib/jquery-easyui-1.4.4/locale/easyui-lang-'.$LANG.'.js"></script>'
?>
<script type="text/javascript" src="lib/jquery-easyui-datagridview/datagrid-groupview.js"></script>

<!--
<script type="text/javascript" src="lib/noty-2.2.10/js/noty/packaged/jquery.noty.packaged.min.js"></script>
<script src="lib/bgrins-spectrum/spectrum.js" type="text/javascript"></script>
-->

<!-- Chargement des modules et objets communs -->
<script type="text/javascript" src="models/common/models-utils.js"></script>

<script type="text/javascript" src="controllers/common/meaobject.js"></script>
<script type="text/javascript" src="controllers/common/commoncontroller.js"></script>
<script type="text/javascript" src="controllers/common/gridcontroller.js"></script>
<script type="text/javascript" src="controllers/common/translationcontroller.js"></script>
<script type="text/javascript" src="controllers/common/credentialcontroller.js"></script>
<script type="text/javascript" src="controllers/common/viewscontroller.js"></script>
<script type="text/javascript" src="controllers/common/tabspagecontroller.js"></script>
<script type="text/javascript" src="controllers/common/filechoosercontroller.js"></script>
<script type="text/javascript" src="controllers/common/filechooseruploadercontroller.js"></script>
<script type="text/javascript" src="controllers/common/permmemcontroller.js"></script>
<script type="text/javascript" src="controllers/common/orderedFileslistselector.js"></script>

<script type="text/javascript" src="widgets/meawidget.js"></script>

<!-- surcharge des méthodes du controleur de traduction spécifiques à une langue donnée -->
<?php
   echo "<script type='text/javascript' src='lib/js/".$LANG."/mea-translation.js'></script>"; // extension
?>

<script>
// fonctions "hors" controleur
function logout()
{
   $.get('models/destroy_php_session.php',
      {},
      function(data){
      // controler le retour du serveur ...
      },
      "json"
   )
   .always(function(){ window.location = "login.php"; });
}


// lancement du controleur sans les communications "live" (réduction des fonctions disponibles)
function liveComUnavailable(destview)
{
   $('#aa').accordion('remove',translationController.toLocalC('supervision')); // pas menu nécessitant des communications lives
   $('#mea-layout').layout('collapse','south'); // console masquée
   $('#console').text(translationController.toLocalC('No live information available !')); // un peu d'info quand même
   
viewsController.addView(translationController.toLocalC('sensors/actuators'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('interfaces'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('types'),'page2.php','page2_tab');
   viewsController.addView(translationController.toLocalC('locations'),'page2.php','page2_tab');

   viewsController.addView(translationController.toLocalC('application'),'page3.php','page3_tab');
   viewsController.addView(translationController.toLocalC('users'),'page3.php','page3_tab');

<?php
if($isadmin==0) :?>
   viewsController.addView(translationController.toLocalC('rules editor'),'page4.php','page4_tab');
   viewsController.addView(translationController.toLocalC('rules manager'),'page4.php','page4_tab');
<?else : ?>
   viewsController.addView(translationController.toLocalC('rules viewer'),'page4.php','page4_tab');
<?php
endif?>

   if(destview=="" || typeof(viewController.views[destview])=="undefined")
      destview=translationController.toLocalC('sensors/actuators');
   
   viewsController.displayView(destview);
}


function resizeDiv()
{
   vpw = $(window).width();
   vph = $(window).height();
   vph = vph - 130;
   if(vph < 670)
      vph=670;
   $('#mea-layout').layout('resize', { width:'100%', height:vph });
}


jQuery(document).ready(function() {
   $.ajaxSetup({ cache: false });

   resizeDiv();
   $(window).resize(function() {
      resizeDiv();
   });

   //
   // récupération des variables depuis PHP
   //
<?php
   echo "LANG='$LANG';";

   if(isset($_REQUEST['view'])) {
      echo "var destview=\""; echo $_REQUEST['view']; echo "\";\n";
   }
   else {
      echo "var destview=\"\";\n";
   }
?>
   //
   // initialisation des controleurs
   //
   // pour la traduction
   translationController = new TranslationController();
   translationController.loadDictionaryFromJSON("lib/translation_"+LANG+".json");
   extend_translation(translationController);

   $.fn.panel.defaults.loadingMessage=translationController.toLocalC("Loading...");

   // controleur d'habilitation
   credentialController = new CredentialController("models/get_auth.php");

   // controleur memoire
   permMemController = new PermMemController();

   // initialisation du contrôle de vues
   viewsController = new ViewsController();   

   liveComUnavailable(destview);

   // propagation d'évenement sur le paneau central
   $('#mea-layout').layout('panel','center').panel({
      onResize: function() {
         $(document).trigger( "MeaCenterResize", [ "", "" ] );
      },
      onBeforeLoad: function() {
         $(document).trigger( "MeaCleanView", [ "", "" ] );
      }
   });

});
</script>

<style>
a.meamenu {
   color:black;
   text-decoration:none;
}

a.meamenu:visited {
}

a.meamenu:hover {
   font-weight:bold;
}

.console {
   border: dotted 1px gray;
   font-family:"Lucida Console", Monaco, monospace;
   font-size:12px;
   overflow-y:scroll;
}
</style>

<body style="margin:0;padding:0;height:100px;width:100%;overflow:auto">
   <div style="min-width:950px;">
      <div id='logo'  style="float:left; width:250px; height:50px; text-align:left;">LOGO</div>
      <div id='pub' style="width:250px; float:right; height:50px; text-align:right;">INFORMATION</div>
      <div id='titre-page' style="height:50px; text-align:center;">CENTER</div>
   </div>
   <div id='meamain' style="min-width:950px;min-height:650px;margin:10px;">
<!--
      <div id='mea-layout' class="easyui-layout" fit=true>
-->
      <div id='mea-layout' class="easyui-layout">

         <div region="west" split="true" collapsible="false" title="" style="width:200px;">
            <div id="aa" class="easyui-accordion" data-options="animate:false,border:false" style="width:100%;">
            
               <div title="<?php mea_toLocalC('automator'); ?>" style="overflow:auto;padding:10px;">
<?php
if($isadmin==0) :?>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('rules editor'),'page4.php','page4_tab')"><?php mea_toLocalC('rules editor'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('rules manager'),'page4.php','page4_tab')"><?php mea_toLocalC('rules manager'); ?></a></div>
<?php
else : ?>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('rules viewer'),'page4.php','page4_tab')"><?php mea_toLocalC('rules viewer'); ?></a></div>
<?php
endif?>
               </div>

               <div title="<?php mea_toLocalC('I/O configuration'); ?>" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('sensors/actuators'),'page2.php','page2_tab')"><?php mea_toLocalC('sensors/actuators'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('interfaces'),'page2.php','page2_tab')"><?php mea_toLocalC('interfaces'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('types'),'page2.php','page2_tab')"><?php mea_toLocalC('types'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('locations'),'page2.php','page2_tab')"><?php mea_toLocalC('locations'); ?></a></div>
               </div>

               <div id='forlivecom' title="<?php mea_toLocalC('supervision'); ?>" data-options="selected:true" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('indicators'),'page1.php','page1_tab')"><?php mea_toLocalC('indicators'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('services'),'page1.php','page1_tab')"><?php mea_toLocalC('services'); ?></a></div>
               </div>
               
               <div title="<?php mea_toLocalC('preferences'); ?>" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('application'),'page3.php','page3_tab')"><?php mea_toLocalC('application'); ?></a></div>
                  <div><a href="#" class="meamenu" onclick="javascript:viewsController.displayView(translationController.toLocalC('users'),'page3.php','page3_tab')"><?php mea_toLocalC('users'); ?></a></div>

               </div>
               <div title="<?php mea_toLocalC('session'); ?>" style="overflow:auto;padding:10px;">
                  <div><a href="#" class="meamenu" onclick="javascript:logout()"><?php mea_toLocalC('logout'); ?></a></div>
               </div>
            </div>
         </div>

         <div id="content" region="center" title=""></div>
         <div id="livelog" region="south" split="true" collapsible="true" title="<?php mea_toLocalC('live log'); ?>" style="height:150px;position:relative;overflow:hidden">
            <div id="console" class="console" style="background:lightgray;width:auto;height:100%"></div>
         </div>
      </div>
   </div>
   <div style="font-size: 10px; text-align:center;">
   (c) 2014 Mea soft
   </div>
</body>
</html>
