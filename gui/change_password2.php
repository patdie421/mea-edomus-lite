<?php
include_once('lib/configs.php');
session_start();

if(isset($_SESSION['language']))
{
   $LANG=$_SESSION['language'];
}
error_log("ICI:[".$LANG."]");
//include_once('lib/php/translation.php');
//include_once('lib/php/'.$LANG.'/translation.php');
//mea_loadTranslationData($LANG,'');
?>

<!DOCTYPE html>
<html>
<head>
   <title>
   <?php echo $TITRE_APPLICATION;?>
   </title>
   <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
   <meta name="viewport" content="width=device-width, initial-scale=0.99">
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
<script type="text/javascript" src="controllers/common/translationcontroller.js"></script>
<script type="text/javascript" src="controllers/common/credentialcontroller.js"></script>

<!-- surcharge des méthodes du controleur de traduction spécifiques à une langue donnée -->
<?php
   echo "<script type='text/javascript' src='lib/js/".$LANG."/mea-translation.js'></script>"; // extension
?>

<script type="text/javascript" src="controllers/password-ctrl.js"></script>

<script>
<?php echo "LANG='$LANG';"; ?>

function cancel()
{
   window.location="index.php";
}

function change()
{
   var password=$("#password").val();
   var password2=$("#password2").val();
   var prevpassword=false;
   
      if( $('#prevpassword').length )
   {
      prevpassword=$('#prevpassword').val();
      // vérifier ici le mot de passe sur le serveur
   }
   
   if( password != password2)
   {
     $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC("password and password validation don't match"),'error');
      return false;
   }
   
   if(!passwordController.validePassword(password))
   {
     $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC('bad password format. Password must have 8 characters, 1 or more numeric character and/or 1 or mor special caracter'),'error');
      return false;
   }
   if(passwordController.chgpasswd(password,prevpassword) == 0)
   {
      window.location="index.php";
      return true;
   }
   
   return false;
}


jQuery(document).ready(function(){
   $.ajaxSetup({ cache: false });

   translationController = new TranslationController();
   translationController.loadDictionaryFromJSON("lib/translation_"+LANG+".json");
   extend_translation(translationController);
   
   // controleur d'habilitation
   credentialController = new CredentialController("models/get_auth.php");

   passwordController = new PasswordController('models/auth.php','models/set_passwd.php');
   // lien avec d'autres objets (délégations)
   passwordController.linkToTranslationController(translationController);
   passwordController.linkToCredentialController(credentialController);
  
   $('#change').click(change);
   $('#cancel').click(cancel);

});

</script>


<body>
   <div style="min-width:950px;min-height:650px;margin:10px;text-align:center;">
      <div>
         <div style="display:inline-block">
            <img src="lib/logo-mea-eDomus.png" border="0" align="center" width=500px/>
         </div>
      </div>
      <div style="display:inline-block;margin-top:75px">
         <div class="easyui-panel" title="Login to system" style="width:400px;padding:30px 70px 20px 70px;margin:0 auto;text-align:right;" >

            <?php
            if(!isset($_SESSION['change_passwd_flag'])){
               echo "<div style='margin-bottom:20px'>";
               echo "<input id='prevpassword' class='easyui-textbox' type='password' style='width:100%;height:40px;padding:12px' data-options=\"prompt:'current password',iconCls:'icon-lock',iconWidth:38\">";
               echo "</div>";
            }
            ?>
            <div style="margin-bottom:20px">
               <input id="password" class="easyui-textbox" type="password" style="width:100%;height:40px;padding:12px" data-options="prompt:'new password',iconCls:'icon-lock',iconWidth:38">
            </div>
            
            <div style="margin-bottom:20px">
               <input id="password2" class="easyui-textbox" type="password" style="width:100%;height:40px;padding:12px" data-options="prompt:'new password configmation',iconCls:'icon-lock',iconWidth:38">
            </div>
            
            <a href="#" id="cancel" class="easyui-linkbutton" data-options="iconCls:'icon-cancel'" style="padding:5px 0px;">
               <span style="font-size:14px;">cancel</span>
            </a>
            <a href="#" id="change" class="easyui-linkbutton" data-options="iconCls:'icon-ok'" style="padding:5px 0px;">
               <span style="font-size:14px;">change</span>
            </a>
         </div>
      </div>
   </div>
</body>
</html>
