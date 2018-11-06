<?php
include_once('../lib/configs.php');
include_once('../lib/php/auth_utils.php');

session_start();
if(isset($_SESSION['language']))
{
   $LANG=$_SESSION['language'];
}
include_once('../lib/php/translation.php');
include_once('../lib/php/'.$LANG.'/translation.php');
mea_loadTranslationData($LANG,'../');

$isadmin = check_admin();
if($isadmin !=0 && $isadmin != 98) : ?>
<script>
   window.location = "login.php";
</script>
<?php
   exit(1);
endif;
?>

<script type="text/javascript" src="controllers/application-ctrl.js"></script>
<script type="text/javascript" src="lib/js/fsselector.js"></script>

<script>
<?php
if($isadmin==0) : ?>
function fsPluginPath()
{
   fileselector.directory(translationController.toLocalC('plugin path'), 500, 400, 'PLUGINPATH');
}

function fsBufferPath()
{
   fileselector.directory(translationController.toLocalC('buffer path'), 500, 400, 'BUFFERDB');
}
<?php
endif; ?>

<?php
if($isadmin!=0) : ?>
function viewonly_ap()
{
  $(".editable_ap").textbox({editable: false, disabled:true});
}
<?php
endif; ?>

jQuery(document).ready(function(){
   applicationPrefsController = new ApplicationPrefsController();
   fileselector = new FSSelector("models/get_dir.php");

   applicationPrefsController.linkToTranslationController(translationController);
   console.log("1:{"+applicationPrefsController._localDoubleDot()+"}");

<?php
   if($isadmin!=0) : ?>
      viewonly_ap();
<?php
   endif; ?>
   applicationPrefsController.load();
});
</script>

<style>
.editable_ap {
}
</style>


<form id="fm_ap" method="post" novalidate>
    <div style="width:100%;text-align:center;">
        
        <div style="margin:0 auto; width:700px; padding-bottom:20px; text-align:right;">
           <a href="javascript:void(0)" class="easyui-linkbutton" onclick="javascript:applicationPrefsController.load()" data-options="iconCls:'icon-reload'"><?php mea_toLocalC('reload'); ?></a>
<?php
if($isadmin==0) : ?>
           <a href="javascript:void(0)" class="easyui-linkbutton" onclick="javascript:applicationPrefsController.save()" data-options="iconCls:'icon-save'"><?php mea_toLocalC('save'); ?></a>
<?php
endif; ?>
        </div>
        
        <div id="ap1" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('xPL Address'); ?>" style="width:700px;padding:10px;">
            <table width="500px" align="center" style="padding-bottom: 12px;">
                <col width="100px">
                <col width="30px">
                <col width="100px">
                <col width="30px">
                <col width="100px">
                <tr>
                    <td align="center">
                        <label for="VENDORID" id="vendorid"><?php mea_toLocalC('vendor ID');?></label>
                    </td>
                    <td></td>
                    <td align="center">
                        <label for="DEVICEID" id="deviceid"><?php mea_toLocalC('device ID');?></label>
                    </td>
                    <td></td>
                    <td align="center">
                        <label for="INSTANCEID" id="instanceid"><?php mea_toLocalC('Instance ID');?></label>
                    </td>
                </tr>
                <tr>
                    <td align="center">
                        <input class="easyui-textbox editable_ap" name="ENDORID" id="VENDORID" data-options="required:true, validType:'name_validation', missingMessage:translationController.toLocalC('vendor ID name is mandatory')" style="height:25px; text-align: center;"/>
                    </td>
                    <td align="center">
                        <div style="padding:0.5em;">-</div>
                    </td>
                    <td align="center">
                        <input class="easyui-textbox editable_ap" name="DEVICEID" id="DEVICEID" style="height:25px; text-align: center;" data-options="required:true,validType:'name_validation',missingMessage:translationController.toLocalC('device ID name is mandatory')" />
                    </td>
                    <td align="center">
                        <div style="padding: 0.5em;">.</div>
                    </td>
                    <td align="center">
                        <input class="easyui-textbox editable_ap" name="INSTANCEID" id="INSTANCEID" style="height:25px; text-align: center;" data-options="required:true,validType:'device_name_validation',missingMessage:translationController.toLocalC('instance ID name is mandatory')" />
                    </td>
                </tr>
            </table>
        </div>
        <p></p>
        <div id="ap2" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC('plugin path'); ?>" style="width:700px;padding:10px;">
            <table width="600px" align="center" style="padding-bottom: 12px;">
                <col width="30%">
                <col width="70%">
                <tr>
                    <td align="right">
                        <label for="PLUGINPATH" id="libplugins"><?php mea_toLocalC_2d('plugins library');?></label>
                    </td>
                    <td>
<?php
if($isadmin==0) : ?>
                        <input class="easyui-textbox editable_ap" style="height:25px; width:100%; margin-bottom:0px;" name="PLUGINPATH" id="PLUGINPATH" data-options="buttonText:translationController.toLocalC('select'),prompt:translationController.toLocalC('path...'),onClickButton:fsPluginPath"/>
<?php
else : ?>
                        <input class="easyui-textbox editable_ap" style="height:25px; width:100%; margin-bottom:0px;" name="PLUGINPATH" id="PLUGINPATH"/>
<?php
endif; ?>
                    </td>

                </tr>
            </table>
        </div>
        <p></p>
        <div style="margin:0 auto; width:700px; padding-top:20px; text-align:right;">
           <a href="javascript:void(0)" class="easyui-linkbutton" onclick="javascript:applicationPrefsController.load()" data-options="iconCls:'icon-reload'"><?php mea_toLocalC('reload'); ?></a>
<?php
if($isadmin==0) : ?>
           <a href="javascript:void(0)" class="easyui-linkbutton" onclick="javascript:applicationPrefsController.save()" data-options="iconCls:'icon-save'"><?php mea_toLocalC('save'); ?></a>
<?php
endif; ?>
        </div>
   </div>
</form>
