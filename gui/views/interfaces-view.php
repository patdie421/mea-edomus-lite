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

<script type="text/javascript" src="controllers/interfaces-ctrl.js"></script>

<script>
function select_error_in()
{
   $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC("types list unavailable"),'error');
}


function updateComboBoxsData_in()
{
  $('#typeSelection_in').combobox('reload');
}

<?php
if($isadmin!=0) : ?>
function viewonly_in()
{
  $(".editable_in").textbox({editable: false, disabled:true});
  $(".readonly_in").textbox({readonly: true, disabled:true});
}
<?php
else : ?>
// validation du formulaire avant soumission
function formValidation_in() {
   var name_exist=-1;
   var id=$("#database_in_id").val();
   var name=$("#interface_name").val();

   // vérification controle de surface
   if(!$('#fm_in').form('validate'))
   {
      $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC('one or more mandatory fields are empty'),'error');

      return false;
   }

   // vérification unicité (nom de l'interface)
   return ctrlr_in.nameExist(name,id);

   $("#todisplay").css("display","block");
}
<?php
endif;
?>

jQuery(document).ready(function(){
   // Création du controleur
   ctrlr_in = new InterfacesController({
      // pour la connexion à l'ihm
      grid_id:'table_in',
      dialogBox_id:'dlg_in',
      form_id:'fm_in',
      submitButton_id:'submit_button_in',

      // pour la connexion aux models
      editDataSource:'models/update_interfaces.php', // à compléter
      addDataSource: 'models/new_interfaces.php',
      delDataSource: 'models/delete_interfaces.php'
   });
   
   // lien avec d'autres objets (délégations)
   ctrlr_in.linkToTranslationController(translationController);
   ctrlr_in.linkToCredentialController(credentialController);
   
   // lien spécifique avec la vue (paramètre optionnel du controleur
   ctrlr_in.setButtonsAnimation('b_edit_in', 'b_del_in');

<?php
if($isadmin==0) : ?>
   ctrlr_in.setValidate(formValidation_in);
<?php
else : ?>
   viewonly_in();
<?php
endif; ?>  

  // activation du controleur
   ctrlr_in.start();

   var pg = $("#table_in").datagrid('getPager');
   var options = pg.pagination('options');
   options.afterPageText=ctrlr_in._toLocal("of {pages}");
   options.displayMsg=ctrlr_in._toLocalC("Displaying {from} to {to} of {total} items");
});
</script>

<style>
#fm_in {
   margin:0;
   padding:10px 30px;
}

.editable_in {
}

.readonly_in {
}
</style>

<div style="width:100%;height:100%;">
   <div style="width:100%;height:100%;margin: 0 auto;">
      <table id="table_in"
             title=""
             style="width:100%;height:100%"
             border="false"
             pagination="true"
             pageSize=20
             pageList=[20,50,100]
             idField="id"
             fitColumns="true"
             singleSelect="true"
             toolbar="#toolbar_in"
             url="models/get_interfaces.php">
         <thead>
            <tr>
               <th data-options="field:'id',width:10,hidden:true"><?php mea_toLocalC('id'); ?></th>
               <th data-options="field:'id_interface',width:10,hidden:true"><?php mea_toLocalC('sensor/actuator id'); ?></th>
               <th data-options="field:'name',width:100,fixed:true,sortable:true,align:'left'"><?php mea_toLocalC('name'); ?></th>
               <th data-options="field:'tname',width:75,fixed:true,align:'left'"><?php mea_toLocalC('type'); ?></th>
               <th data-options="field:'dev',width:270, fixed:true,align:'left'"><?php mea_toLocalC('device'); ?></th>
               <th data-options="field:'description',width:200,align:'left'"><?php mea_toLocalC('description'); ?></th>
<!--
               <th data-options="field:'parameters',width:200,align:'left'"><?php mea_toLocalC('parameters'); ?></th>
-->
               <th data-options="field:'state',width:50,fixed:true,align:'left',formatter:ctrlr_in.toCheckmark"><?php mea_toLocalC('state'); ?></th>
               <th data-options="field:'id_type',hidden:true,align:'left'"><?php mea_toLocalC('type id'); ?></th>
            </tr>
         </thead>
      </table>
   </div>

   <div id="toolbar_in">
   <?php
      if($isadmin == 0) : ?>
         <a href="#" class="easyui-linkbutton" id="b_add_in" iconCls="icon-add" plain="true" onclick="ctrlr_in.add();"><?php mea_toLocalC('add'); ?></a>
         <a href="#" class="easyui-linkbutton" id="b_edit_in" iconCls="icon-edit" plain="true" onclick="ctrlr_in.edit();"><?php mea_toLocalC('edit/view'); ?></a>
         <a href="#" class="easyui-linkbutton" id="b_del_in" iconCls="icon-remove" plain="true" onclick="ctrlr_in.del();"><?php mea_toLocalC('delete'); ?></a>
   <?php
      else : ?>
         <a href="#" class="easyui-linkbutton" id="b_edit_in" iconCls="icon-edit" plain="true" onclick="ctrlr_in.view();"><?php mea_toLocalC('view'); ?></a><?php
      endif; ?>
   </div>
</div>

<div id="dlg_in" class="easyui-dialog" style="width:500px;height:450px;padding:10px 20px;display=none" modal="true" closed="true" buttons="#dlg_buttons_in" data-options="onOpen: updateComboBoxsData_in">
    <div class="ftitle"><?php mea_toLocalC_2d('interfaces'); ?></div>
    <form id="fm_in" method="post" novalidate>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('name'); ?></label>
            <input id="interface_name" name="name" class="easyui-textbox editable_in" style="width:100px;" data-options="required:true,validType:'device_name_validation',missingMessage:translationController.toLocalC('interface name is mandatory')">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('type'); ?></label>
            <select id="typeSelection_in" name="id_type" class="easyui-combobox readonly_in"  data-options="required:true, editable:false, panelHeight:'auto', valueField:'id_type', textField:'name', url:'models/get_types_select.php', onLoadError:select_error_in, onBeforeLoad:function(param) { param.nature='interface'; }, missingMessage:translationController.toLocalC('type is mandatory')" style="width:120px;">
            </select>
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('description'); ?></label>
            <input name="description" class="easyui-textbox editable_in" data-options="multiline:true" style="width:270px;height:50px">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('device'); ?></label>
            <input name="dev" class="easyui-textbox editable_in" data-options="required:true,missingMessage:translationController.toLocalC('device is mandatory')" style="width:270px;">
        </div>
        <div class="fitem" style="">
            <label><?php mea_toLocalC_2d('parameters'); ?></label>
            <input name="parameters" class="easyui-textbox editable_in" data-options="multiline:true" style="width:270px;height:75px">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('state'); ?></label>
            <select class="easyui-combobox readonly_in" name="state" data-options="required:true,editable:false,panelHeight:'auto'" style="width:120px;">
               <option value=0><?php mea_toLocalC('unactivated'); ?></option>
               <option value=1><?php mea_toLocalC('activated'); ?></option>
               <option value=2><?php mea_toLocalC('delegated'); ?></option>
            </select>
        </div>
        
        <input name="interface_id" type="hidden">
        <input name="id" type="hidden" id="database_in_id" defaultValue='-1' value='-1'>
    </form>
</div>
<div id="dlg_buttons_in">
<?php
   if($isadmin == 0) : ?>
      <a href="javascript:void(0)" id="submit_button_in"  class="easyui-linkbutton" iconCls="icon-ok" style="width:100px"><?php mea_toLocalC('save'); ?></a>
      <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_in').dialog('close')" style="width:100px"><?php mea_toLocalC('cancel'); ?></a>
<?php
   else : ?>
      <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_in').dialog('close')" style="width:100px"><?php mea_toLocalC('close'); ?></a>
<?php
   endif; ?>
</div>
