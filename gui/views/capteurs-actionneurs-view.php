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

<script type="text/javascript" src="controllers/capteurs-actionneurs-ctrl.js"></script>
<script>
function select_error()
{
   alert("Accès aux données impossible");
}


function strTypeOfType_sa(group)
{
   switch(group)
   {
     case '0':  return "Entrée";
     case '1':  return "Sortie";
     case '10': return "Interface";
     default: return "Autre";
   }
}


function select_error_sa_type()
{
   $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC("types list unavailable"),'error');
}


function select_error_sa_interface()
{
   $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC("interfaces list unavailable"),'error');
}


function select_error_sa_location()
{
   $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC("locations list unavailable"),'error');
}


function updateComboBoxsData_sa()
{
  $('#typeSelection_sa').combobox('reload');
  $('#interfaceSelection_sa').combobox('reload');
  $('#locationSelection_sa').combobox('reload');
}


<?php
if($isadmin!=0) : ?>
function viewonly_sa()
{
  $(".editable_sa").textbox({editable: false, disabled:true});
  $(".readonly_sa").textbox({readonly: true, disabled:true});
  $(".checkbox_sa").attr("disabled", true);
}
<?php
else : ?>
// validation du formulaire avant soumission
function formValidation_sa() {
   var name_exist=-1;
   var id=$("#database_sa_id").val();
   var name=$("#sa_name").val();

   // vérification controle de surface
   if(!$('#fm_sa').form('validate'))
   {
      $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC('one or more mandatory fields are empty'),'error');

      return false;
   }

   // vérification unicité (nom du capteur actionneur)
   return ctrlr_sa.nameExist(name,id);
}
<?php
endif; ?>

jQuery(document).ready(function(){
   // Création du controleur
   ctrlr_sa = new CapteursActionneursController({
      // pour la connexion à l'ihm
      grid_id:'table_sa',
      dialogBox_id:'dlg_sa',
      form_id:'fm_sa',
      submitButton_id:'submit_button_sa',

      // pour la connexion aux models
      editDataSource:'models/update_sensors_actuators.php', // à compléter
      addDataSource: 'models/new_sensors_actuators.php',
      delDataSource: 'models/delete_sensors_actuators.php',
   });
   
   // lien avec d'autres objets
   ctrlr_sa.linkToTranslationController(translationController);
   ctrlr_sa.linkToCredentialController(credentialController); // pour la gestion des habilitations

   // lien spécifique avec la vue (paramètre optionnel du controleur
   ctrlr_sa.setButtonsAnimation('b_edit_sa', 'b_del_sa');
<?php
   if($isadmin==0) : ?>
      ctrlr_sa.setValidate(formValidation_sa);
<?php
   else : ?>
      viewonly_sa();
<?php
   endif; ?>

  // activation du controleur
   ctrlr_sa.start();

   var pg = $("#table_sa").datagrid('getPager');
   var options = pg.pagination('options');
   options.afterPageText=ctrlr_sa._toLocal("of {pages}");
   options.displayMsg=ctrlr_sa._toLocalC("Displaying {from} to {to} of {total} items");
});
</script>


<style>
#fm_sa {
   margin:0;
   padding:10px 30px;
}

.editable_sa {
}

.readonly_sa {
}

.checkbox_sa {
}
</style>


<div style="width:100%;height:100%">
   <div style="width:100%;height:100%;margin: 0 auto;">
      <table id="table_sa"
             title=""
             style="width:100%;height:100%"
             border="false"
             pagination="true"
             pageSize=50
             pageList=[20,50,100]             
             idField="id"
             fitColumns="true"
             singleSelect="true"
             toolbar="#toolbar_sa"
             url="models/get_sensors_actuators.php">
         <thead>
            <tr>
               <th data-options="field:'id',width:10,hidden:true"><?php mea_toLocalC('id'); ?></th>
               <th data-options="field:'id_sensor_actuator',width:10,hidden:true"><?php mea_toLocalC('id_sensor_actuator'); ?></th>

               <th data-options="field:'name',width:100,fixed:true,sortable:true,align:'left'"><?php mea_toLocalC('name'); ?></th>
               <th data-options="field:'tname',width:75,fixed:true,align:'left'"><?php mea_toLocalC('type'); ?></th>
               <th data-options="field:'iname',width:100,fixed:true,align:'left'"><?php mea_toLocalC('interface'); ?></th>
               <th data-options="field:'description',width:200,align:'left'"><?php mea_toLocalC('description'); ?></th>
<!--
               <th data-options="field:'parameters',width:200,align:'left'"><?php mea_toLocalC('parameters'); ?></th>
-->
               <th data-options="field:'lname',width:80,fixed:true,align:'left'"><?php mea_toLocalC('location'); ?></th>
               <th data-options="field:'state',width:50,fixed:true,align:'left',formatter:ctrlr_sa.toCheckmark"><?php mea_toLocalC('state'); ?></th>
               <th data-options="field:'todbflag',width:50,fixed:true,align:'left',formatter:ctrlr_sa.toCheckmark"><?php mea_toLocalC('recorded'); ?></th>
    
               <th data-options="field:'id_type',hidden:true,align:'left'"><?php mea_toLocalC('id_type'); ?></th>
               <th data-options="field:'id_interface',hidden:true,align:'left'"><?php mea_toLocalC('id_interface'); ?></th>
               <th data-options="field:'id_location',hidden:true,align:'left'"><?php mea_toLocalC('id_location'); ?></th>
            </tr>
         </thead>
      </table>
   </div>
</div>


<div id="toolbar_sa">
<?php
   if($isadmin == 0) : ?>
      <a href="#" class="easyui-linkbutton" id="b_add_sa" iconCls="icon-add" plain="true" onclick="ctrlr_sa.add();"><?php mea_toLocalC('add'); ?></a>
      <a href="#" class="easyui-linkbutton" id="b_edit_sa" iconCls="icon-edit" plain="true" onclick="ctrlr_sa.edit();"><?php mea_toLocalC('edit/view'); ?></a>
      <a href="#" class="easyui-linkbutton" id="b_del_sa" iconCls="icon-remove" plain="true" onclick="ctrlr_sa.del();"><?php mea_toLocalC('delete'); ?></a>
<?php
   else : ?>
      <a href="#" class="easyui-linkbutton" id="b_edit_sa" iconCls="icon-edit" plain="true" onclick="ctrlr_sa.view();"><?php mea_toLocalC('view'); ?></a>
<?php
   endif; ?>
</div>


<div id="dlg_sa" class="easyui-dialog" style="width:500px;height:450px;padding:10px 20px" modal="true" closed="true" buttons="#dlg_buttons_sa" data-options="onOpen: updateComboBoxsData_sa">
    <div class="ftitle"><?php mea_toLocalC('sensors/actuators'); ?></div>
    <form id="fm_sa" method="post" novalidate>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('name'); ?></label>
          <input id="sa_name" name="name" class="easyui-textbox editable_sa" style="width:100px;" data-options="required:true,validType:'device_name_validation',missingMessage:translationController.toLocalC('sensor/actuator name is mandatory')">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('type'); ?></label>
           <select id="typeSelection_sa" name="id_type" class="easyui-combobox readonly_sa"  data-options="required:true, editable:false, panelHeight:'auto',valueField:'id_type', textField:'name', groupField:'typeoftype', groupFormatter: strTypeOfType_sa, url:'models/get_types_select.php',onLoadError:select_error_sa_type, onBeforeLoad:function(param) { param.nature='io'; }, missingMessage:translationController.toLocalC('type is mandatory')" style="width:120px;">
            </select>
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('description'); ?></label>
            <input name="description" class="easyui-textbox editable_sa" data-options="multiline:true" style="width:270px;height:50px;">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('interface'); ?></label>
            <select id="interfaceSelection_sa" class="easyui-combobox readonly_sa" name="id_interface" data-options="required:true,editable:false,panelHeight:100,valueField:'id_interface',textField:'name',url:'models/get_interfaces_select.php',onLoadError:select_error_sa_interface,missingMessage:translationController.toLocalC('interface is mandatory')" style="width:120px;">
            </select>
        </div>
        <div class="fitem" style="">
            <label><?php mea_toLocalC_2d('parameters'); ?></label>
            <input name="parameters" class="easyui-textbox editable_sa" data-options="multiline:true" style="width:270px;height:75px">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('location'); ?></label>
          <select id="locationSelection_sa" class="easyui-combobox readonly_sa" name="id_location" data-options="required:true,editable:false,panelHeight:100,valueField:'id_location',textField:'name',url:'models/get_locations_select.php',onLoadError:select_error_sa_location,missingMessage:translationController.toLocalC('location is mandatory')" style="width:120px;">
            </select>
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('state'); ?></label>
            <select class="easyui-combobox readonly_sa" name="state" data-options="required:true,editable:false,panelHeight:42" style="width:120px;">
               <option value=1><?php mea_toLocalC('enabled'); ?></option>
               <option value=0><?php mea_toLocalC('disabled'); ?></option>
            </select>
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('recorded');?></label>
            <input class="checkbox_sa" name="todbflag" type="checkbox" value=1>
        </div>
        
        <input name="sensor_actuator_id" type="hidden">
        <input name="id" type="hidden" id="database_sa_id" defaultValue='-1' value='-1'>
    </form>
</div>
<div id="dlg_buttons_sa">
<?php
   if($isadmin == 0) : ?>
      <a href="javascript:void(0)" id="submit_button_sa"  class="easyui-linkbutton" iconCls="icon-ok" style="width:100px"><?php mea_toLocalC('save'); ?></a>
      <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_sa').dialog('close')" style="width:100px"><?php mea_toLocalC('cancel'); ?></a>
<?php
   else :?>
      <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_sa').dialog('close')" style="width:100px"><?php mea_toLocalC('close'); ?></a>
<?php
   endif; ?>
</div>

