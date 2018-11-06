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

<!DOCTYPE html>

<html>
<head>
   <style>
      html,body{ margin:0px;padding:0px;height:100%;width:100%; }
   </style>
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
   
   <script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.min.js"></script>
   <script type="text/javascript" src="lib/jquery-easyui-1.4.4/jquery.easyui.min.js"></script>
</head>

<script type="text/javascript" src="models/common/models-utils.js"></script>

<script type="text/javascript" src="controllers/common/meaobject.js"></script>
<script type="text/javascript" src="controllers/common/commoncontroller.js"></script>
<script type="text/javascript" src="controllers/common/credentialcontroller.js"></script>
<?php if(!isset($_REQUEST['autologin'])) :?>
<script type="text/javascript" src="controllers/common/translationcontroller.js"></script>
<?php endif ?>
<!-- surcharge des méthodes du controleur de traduction spécifiques à une langue donnée -->
<?php if(!isset($_REQUEST['autologin']))
   echo "<script type='text/javascript' src='lib/js/".$LANG."/mea-translation.js'></script>"; // extension
?>

<script type="text/javascript" src="controllers/password-ctrl.js"></script>

<script type="text/javascript">
<?php echo "LANG='$LANG';"; ?>
var chgPasswordDest="";
var destination="";
var autologin=false;

function login()
{
   passwordController.login($("#userid").val(),$("#passwd").val(),destination,chgPasswordDest);
}


jQuery(document).ready(function(){
   $.ajaxSetup({ cache: false });

<?php
if(!isset($_REQUEST['autologin'])) :?>
   translationController = new TranslationController();
   translationController.loadDictionaryFromJSON("lib/translation_"+LANG+".json");
   extend_translation(translationController);
<?php
endif ?>

   // controleur d'habilitation
   credentialController = new CredentialController("models/get_auth.php");

   passwordController = new PasswordController('models/login.php','models/set_password.php');
   passwordController.linkToCredentialController(credentialController);

<?php
if(!isset($_REQUEST['autologin'])) :?>
   passwordController.linkToTranslationController(translationController);
<?php
endif ?>

<?php
   if(isset($_REQUEST['autologin']))
   {
      echo "autologin = \"".($_REQUEST['autologin'])."\";\n"; 
   }
?>

<?php
if(isset($_REQUEST['autologin'])) :?>
   passwordController.login(autologin, autologin, destination, false);
<?php
else: ?>
   $("#loginpanel").show();
   userid = $("#userid");
   passwd = $("#passwd");

   userid.textbox('textbox').on('keydown', function(e) {
      console.log("ICI");
      if(e.keyCode == 13 || e.keyCode == 9)
      {
         e.preventDefault();
         passwd.textbox('clear').textbox('textbox').focus();
      }
   });

   passwd.textbox('textbox').on('keydown', function(e) {
      if(e.keyCode == 13)
         login();
   });

   <?php
      echo "chgPasswordDest = \"change_password2.php";
      if(isset($_REQUEST['view'])) {
         $view=$_REQUEST['view'];
         echo "?view=$view";
      }
      echo "\";\n";

      if(isset($_REQUEST['dest'])) {
         echo "destination = \"".($_REQUEST['dest']);
      }
      else
      {     
         echo "destination = \"index.php";
         if(isset($_REQUEST['view']))
         {
            echo "?view="; echo $_REQUEST['view'];
         }
      }
      echo "\";\n";
   ?>
   $('#login').click(login);
<?php
endif
?>
});
</script>

<body style="margin:0;padding:0;width:100%;">
<?php
if(!isset($_REQUEST['autologin'])) :?>
   <div id="loginpanel" style="position:absolute;width:100%;height:100%;display:hidden">
   <div style="postition:absolute;display:table;width:100%;height:100%">
      <div style="display:table-cell;text-align:center;vertical-align:middle;">
         <div>
            <div style="display:inline-block">
               <img src="lib/logo-mea-eDomus.png" border="0" align="center" width=360px/>
            </div>
         </div>
         <div style="display:inline-block;margin-top:10px">
            <div class="easyui-panel" title="<?php mea_toLocalC('Login to system'); ?>" style="width:360px;padding:20px 20px 20px 20px;margin:0 auto">
               <div style="margin-bottom:10px">
                  <input id="userid" class="easyui-textbox" autocapitalize="off" autocorrect="off" style="width:100%;height:35px;padding:12px;" data-options="prompt:'<?php mea_toLocalC('user id'); ?>',iconCls:'icon-man',iconWidth:38">
               </div>
               <div style="margin-bottom:20px">
                  <input id="passwd" class="easyui-textbox" type="password" style="width:100%;height:35px;padding:12px" data-options="prompt:'<?php mea_toLocalC('password'); ?>',iconCls:'icon-lock',iconWidth:38">
               </div>
               <a href="#" id="login" class="easyui-linkbutton" data-options="iconCls:'icon-ok'" style="padding:5px 0px;width:100%;">
                  <span style="font-size:14px;"><?php mea_toLocalC('login'); ?></span>
               </a>
            </div>
         </div>
      </div>
   </div>
   </div>
<?php
endif
?>
</body>
</html>
