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

<script type="text/javascript" src="controllers/rulesmanagercontroller.js"></script>

<script>

jQuery(document).ready(function() {
/*
   $("#panel_rm").height($("#rulemgrzone").height()-35);
   $(document).on( "CenterResize", function( event, arg1, arg2 ) {
      setTimeout( function() {
         $("#panel_rm").panel('resize',{
            height: $("#rulemgrzone").height()-35,
            width: $("#rulemgrzone").width() });
      },
      5);
   });
*/
   // liaison view/controller
   ctrlr_rulesManager = new RulesManagerController(
      "rulemgrzone",
      "panel_rm",
      "files_to_sel_rm",
      "files_sel_rm",
      "builded_rules_rm",
      "button_in_rm",
      "button_out_rm",
      "button_up_rm",
      "button_down_rm",
      "button_build_rm",
      "button_apply_rm",
      "button_delete_rm",
      "rulemgrmenu");
   ctrlr_rulesManager.linkToTranslationController(translationController); 
   ctrlr_rulesManager.linkToCredentialController(credentialController); 

   page4_ctrlr.addLeaveViewsCallbacks(function() { 
      $('#mn1_rm').remove();
      $('#mn2_rm').remove();
   });

   domenu_rm = ctrlr_rulesManager.domenu.bind(ctrlr_rulesManager);

   ctrlr_rulesManager.start();
});
</script>

<style>
.editable_ap {
}
</style>

<div id="rulemgrzone" style="width:auto;height:100%;">
   <div id="rulemgrmenu" style="height:28px; width:100%;padding-top:6px;border-bottom:1px solid #95B8E7;display:none">
      <a href="#" class="easyui-menubutton" data-options="menu:'#mn1_rm'"><?php mea_toLocalC('rules set'); ?></a>
      <a href="#" class="easyui-menubutton" data-options="menu:'#mn2_rm'"><?php mea_toLocalC('builded rules'); ?></a>

      <div id="mn1_rm" style="width:150px; display:none">
         <div onclick="javascript:domenu_rm('new')"><?php mea_toLocalC('New'); ?></div>
         <div onclick="javascript:domenu_rm('load')"><?php mea_toLocalC('Load'); ?></div>
         <div onclick="javascript:domenu_rm('save')"><?php mea_toLocalC('Save'); ?></div>
         <div onclick="javascript:domenu_rm('saveas')"><?php mea_toLocalC('Save as'); ?></div>
         <div class="menu-sep"></div>
         <div onclick="javascript:domenu_rm('delete')"><?php mea_toLocalC('Delete'); ?></div>
<!--
         <div class="menu-sep"></div>
         <div onclick="javascript:domenu_rm('build')"><?php mea_toLocalC('build'); ?></div>
-->
      </div>
      <div id="mn2_rm" style="width:150px; display:none">
         <div onclick="javascript:domenu_rm('delete_builded')"><?php mea_toLocalC('delete'); ?></div>
         <div class="menu-sep"></div>
         <div onclick="javascript:domenu_rm('apply')"><?php mea_toLocalC('apply'); ?></div>
      </div>
   </div>

<!--   <div class="easyui-panel" style="height:100%;margin-top:30px">
-->
   <div id="panel_rm" class="easyui-panel" data-options="border:false">
      <form id="fm_rm" method="post" novalidate>
         <div style="width:100%;text-align:center;padding-top:20px;padding-bottom:20px">
            <div id="rm" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC_2d('choose rules to build'); ?>" style="width:700px;padding:10px;">
               <table width="600px" align="center" style="padding-bottom: 12px;">
                  <col width="40%">
                  <col width="10%">
                  <col width="40%">
                  <col width="10%">

                  <tr>
                     <td align="center"><B><?php mea_toLocalC('available'); ?></B></td>
                     <td></td>
                     <td align="center"><B><?php mea_toLocalC('selected'); ?></B></td>
                     <td></td>
                  </tr>

                  <tr>
                     <td align="center">
<!--
                        <select multiple name="files_to_sel_rm" id="files_to_sel_rm" size="12" style="width:100%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
                        </select>
-->
                        <div id="files_to_sel_rm" style="width:100%;height:200px;">
                        </div>
                     </td>
                     <td align="center">
                        <table>
                           <tr><td>
                              <a id="button_in_rm" href="javascript:void(0)" data-options="iconCls:'icon-mearightarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                           </td></tr>
                           <tr><td>
                              <a id="button_out_rm" href="javascript:void(0)" data-options="iconCls:'icon-mealeftarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                           </td></tr>
                        </table>
                     </td>

                     <td align="center">
<!--
                        <select name="files_sel_rm" id="files_sel_rm" size="12" style="width:100%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
                        </select>
-->
                        <div id="files_sel_rm" style="width:100%;height:200px;">
                     </td>
                     <td align="center">
                        <table>
                           <tr><td>
                              <a id="button_up_rm" href="javascript:void(0)" data-options="iconCls:'icon-meauparrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                           </td></tr>
                           <tr><td>
                              <a id="button_down_rm" href="javascript:void(0)" data-options="iconCls:'icon-meadownarrow'" class="easyui-linkbutton" style="width:30px;height:30px"></a>
                           </td></tr>
                        </table>
                     </td>
                  </tr>
               </table>

               <div style="margin:0 auto; text-align:center;">
                  <a id="button_build_rm" href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-reload'"><?php mea_toLocalC('build'); ?></a>
               </div>
            </div>

            <p></p>

            <div id="rm" class="easyui-panel" data-options="style:{margin:'0 auto'}" title="<?php mea_toLocalC_2d('choose builded rules to apply'); ?>" style="width:700px;padding:10px;">
               <table width="600px" align="center" style="padding-bottom: 12px;">
                  <col width="100%">
                  <tr>
                     <td align="center"><B><?php mea_toLocalC('available builded rules'); ?></B></td>
                  </tr>
                  <tr>
                     <td align="center">
<!--
                        <select name="builded_rules_rm" id="builded_rules_rm" size="12" style="width:40%;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;">
                        </select>
-->
                        <div id="builded_rules_rm" style="width:40%;height:200px;">
                     </td>
                  </tr>
               </table>

               <div style="margin:0 auto; text-align:center;">
                  <a id="button_apply_rm" href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-meacompiler'"><?php mea_toLocalC('apply'); ?></a>
                  <a id="button_delete_rm", href="javascript:void(0)" class="easyui-linkbutton" style="width=50px;" data-options="iconCls:'icon-delele'"><?php mea_toLocalC('delete'); ?></a>
               </div>
            </div>

            <p></p>

         </div>
      </form>
   </div>
</div>
