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

<script type="text/javascript" src="controllers/types-ctrl.js"></script>

<script>
function strKindOfType_ty(value,rows)
{
   if(value==1)
      return translationController.toLocalC('internals types');
   else
      return translationController.toLocalC('users defined types');
}


// Affichage litérale du type du type
function strTypeOfType_ty(val,row)
{
   switch(val)
   {
      case  '0': return translationController.toLocalC("input");
      case  '1': return translationController.toLocalC("output");
      case '10': return translationController.toLocalC("interface");
      default:   return translationController.toLocalC("other");
   }
}


<?php
if($isadmin!=0) : ?>
function viewonly_ty()
{
  $(".editable_ty").textbox({editable: false, disabled:true});
  $(".readonly_ty").textbox({readonly: true, disabled:true});
}
<?php
else : ?>
// validation du formulaire avant soumission
function formValidation_ty()
{
   var name_exist=-1;
   var id=$("#database_ty_id").val();
   var name=$("#type_name").val();

   // vérification controle de surface
   if(!$('#fm_ty').form('validate'))
   {
      $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC('one or more mandatory fields are empty'),'error');
      return false;
   }

   // vérification unicité (nom du capteur actionneur)
   return ctrlr_ty.nameExist(name,id);
}
<?php
endif; ?>

// pour enpécher la selection des types prédéfinis
function onSelectRow_ty(index, row)
{
   if(row.flag==1)
   {
      $('#table_ty').datagrid('unselectRow', index);
      return false;
   }
   else
      return true;
}


jQuery(document).ready(function(){
   // Création du controleur
   ctrlr_ty = new TypesController({
      // pour la connexion à la view
      grid_id:'table_ty',
      dialogBox_id:'dlg_ty',
      form_id:'fm_ty',
      submitButton_id:'submit_button_ty',

      // pour la connexion aux "models"
      editDataSource:'models/update_types.php', // à compléter
      addDataSource: 'models/new_types.php',
      delDataSource: 'models/delete_types.php',
   });
   
   // lien avec d'autres objets
   ctrlr_ty.linkToTranslationController(translationController); // pour la traduction des messages
   ctrlr_ty.linkToCredentialController(credentialController); // pour la gestion des habilitations
   
   // lien spécifique avec la vue (paramètre optionnel du controleur
   ctrlr_ty.setButtonsAnimation('b_edit_ty', 'b_del_ty');
   ctrlr_ty.setOnSelect(onSelectRow_ty);

<?php
   if($isadmin==0) : ?>
      ctrlr_ty.setValidate(formValidation_ty);
<?php
   else : ?>
      viewonly_ty();
<?php
   endif; ?>
   
  // activation du controleur
   ctrlr_ty.start();

   var pg = $("#table_ty").datagrid('getPager');
   var options = pg.pagination('options');
   options.afterPageText=ctrlr_ty._toLocal("of {pages}");
   options.displayMsg=ctrlr_ty._toLocalC("Displaying {from} to {to} of {total} items");
});
</script>


<style>
#fm_ty {
   margin:0;
   padding:10px 30px;
}

.editable_ty {
}

.readonly_ty {
}
</style>


<div style="width:100%;height:100%">
   <div style="width:100%;height:100%;margin: 0 auto;">
      <table id="table_ty"
             title=""
             style="width:100%;height:100%"
             border="false"
             pagination="true"
             pageSize=20
             pageList=[20,50,100]
             idField="id"
             fitColumns="true"
             singleSelect="true"
             toolbar="#toolbar_ty"
             url="models/get_types.php"
             data-options="view:groupview,groupField:'flag',groupFormatter:strKindOfType_ty">
         <thead>
            <tr>
               <th data-options="field:'id',width:10,hidden:true"><?php mea_toLocalC('id'); ?></th>
               <th data-options="field:'name',width:100,fixed:true,sortable:true,align:'left'"><?php mea_toLocalC('name'); ?></th>
               <th data-options="field:'id_type',width:50,fixed:true"><?php mea_toLocalC('type id'); ?></th>
               <th data-options="field:'typeoftype',width:75,fixed:true,formatter:strTypeOfType_ty"><?php mea_toLocalC('type of type'); ?></th>
               <th data-options="field:'description',width:200,align:'left'"><?php mea_toLocalC('description'); ?></th>
<!--
               <th data-options="field:'parameters',width:200,align:'left'"><?php mea_toLocalC('parameters'); ?></th>
-->
               <th data-options="field:'flag',hidden:true"><?php mea_toLocalC('flag'); ?></th>
            </tr>
         </thead>
      </table>
   </div>
</div>


<div id="toolbar_ty">
<?php
   if($isadmin == 0) : ?>
      <a href="#" class="easyui-linkbutton" id="b_add_ty" iconCls="icon-add" plain="true" onclick="ctrlr_ty.add(); $('#type_id').textbox({value: ctrlr_ty.getNextTypeId(), editable: true});"><?php mea_toLocalC('add'); ?></a>
      <a href="#" class="easyui-linkbutton" id="b_edit_ty" iconCls="icon-edit" plain="true" onclick="ctrlr_ty.edit();"><?php mea_toLocalC('edit/view'); ?></a>
      <a href="#" class="easyui-linkbutton" id="b_del_ty" iconCls="icon-remove" plain="true" onclick="ctrlr_ty.del();"><?php mea_toLocalC('delete'); ?></a>
<?php
   else : ?>
      <a href="#" class="easyui-linkbutton" id="b_edit_ty" iconCls="icon-edit" plain="true" onclick="ctrlr_ty.view();"><?php mea_toLocalC('view'); ?></a>
<?php
   endif; ?>
</div>


<div id="dlg_ty" class="easyui-dialog" style="width:500px;height:450px;padding:10px 20px" modal="true" closed="true" buttons="#dlg_buttons_ty">
    <div class="ftitle"><?php mea_toLocalC('sensors/actuators'); ?></div>
    <form id="fm_ty" method="post" novalidate>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('type id'); ?></label>
            <input id="type_id" name="id_type" class="easyui-textbox editable_ty" style="width:50px;" data-options="editable: false">
        </div>
        <div class="fitem">
        <label><?php mea_toLocalC_2d('name'); ?></label>
            <input id="type_name" name="name" class="easyui-textbox editable_ty" style="width:100px;" data-options="required:true,validType:'name_validation',missingMessage:translationController.toLocalC('type name is mandatory')">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('type of type'); ?></label>
            <select class="easyui-combobox readonly_ty" name="typeoftype" data-options="required:true,editable:false,panelHeight:105" style="width:120px;">
               <option value=0><?php mea_toLocalC('input'); ?></option>
               <option value=1><?php mea_toLocalC('output'); ?></option>
               <option value=10><?php mea_toLocalC('interface'); ?></option>
               <option value=99><?php mea_toLocalC('other'); ?></option>
            </select>
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('description'); ?></label>
            <input name="description" class="easyui-textbox editable_ty" data-options="multiline:true" style="width:270px;height:50px">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('parameters'); ?></label>
            <input name="parameters" class="easyui-textbox editable_ty" data-options="multiline:true" style="width:270px;height:75px">
        </div>
        <input name="flag" type="hidden" defaultValue='2' value='2'>
        <input name="id" type="hidden" id="database_ty_id" defaultValue='-1' value='-1'>
        
    </form>
</div>

<div id="dlg_buttons_ty">
<?php
   if($isadmin == 0) : ?>
   <a href="javascript:void(0)" id="submit_button_ty"  class="easyui-linkbutton" iconCls="icon-ok" style="width:100px"><?php mea_toLocalC('save'); ?></a>
   <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_ty').dialog('close')" style="width:100px"><?php mea_toLocalC('cancel'); ?></a>
<?php
   else : ?>
   <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_ty').dialog('close')" style="width:100px"><?php mea_toLocalC('close'); ?></a>
<?php
   endif; ?>
</div>

