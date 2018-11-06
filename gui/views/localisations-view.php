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

<script type="text/javascript" src="controllers/localisations-ctrl.js"></script>

<script>

<?php
if($isadmin!=0) : ?>
function viewonly_lo()
{
  $(".editable_lo").textbox({editable: false, disabled:true});
}
<?php
else : ?>
// validation du formulaire avant soumission
function formValidation_lo() {
   var name_exist=-1;
   var id=$("#database_lo_id").val();
   var name=$("#location_name").val();

   // vérification controle de surface
   if(!$('#fm_lo').form('validate'))
   {
      $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC('one or more mandatory fields are empty'),'error');

      return false;
   }

   // vérification unicité (nom du lieu)
   return ctrlr_lo.nameExist(name,id);

   var pg = $("#table_lo").datagrid('getPager');
   var options = pg.pagination('options');
   options.afterPageText=ctrlr_lo._toLocal("of {pages}");
   options.displayMsg=ctrlr_lo._toLocalC("Displaying {from} to {to} of {total} items");
}
<?php
endif;
?>

jQuery(document).ready(function(){
   // Création du controleur
   ctrlr_lo = new LocationsController({
      // pour la connexion à l'ihm
      grid_id:'table_lo',
      dialogBox_id:'dlg_lo',
      form_id:'fm_lo',
      submitButton_id:'submit_button_lo',

      // pour la connexion aux models
      editDataSource:'models/update_locations.php', // à compléter
      addDataSource: 'models/new_locations.php',
      delDataSource: 'models/delete_locations.php'
   });
   
   // lien avec d'autres objets
   ctrlr_lo.linkToTranslationController(translationController);
   ctrlr_lo.linkToCredentialController(credentialController);

   // lien spécifique avec la vue (paramètre optionnel du controleur
   ctrlr_lo.setButtonsAnimation('b_edit_lo', 'b_del_lo');
   
<?php
   if($isadmin==0) : ?>
      ctrlr_lo.setValidate(formValidation_lo);
<?php
   else : ?>
      viewonly_lo();
<?php
   endif; ?>

   // activation du controleur
   ctrlr_lo.start();
});
</script>


<style>
#fm_lo {
   margin:0;
   padding:10px 30px;
}

.editable_lo {
}

</style>


<div style="width:100%;height:100%">
   <div style="width:100%;height:100%;margin: 0 auto;">
      <table id="table_lo"
             title=""
             style="width:100%;height:100%"
             border="false"
             pagination="true"
             pageSize=20
             pageList=[20,50,100]
             idField="id"
             fitColumns="true"
             singleSelect="true"
             toolbar="#toolbar_lo"
             url="models/get_locations.php">
         <thead>
            <tr>
               <th data-options="field:'id',width:10,hidden:true"><?php mea_toLocalC('id'); ?></th>
               <th data-options="field:'name',width:100,sortable:true,align:'left'"><?php mea_toLocalC('name'); ?></th>
               <th data-options="field:'id_location',width:50,hidden:true"><?php mea_toLocalC('location id'); ?></th>
               <th data-options="field:'description',width:200,align:'left'"><?php mea_toLocalC('description'); ?></th>
            </tr>
         </thead>
      </table>
   </div>
</div>


<div id="toolbar_lo">
<?php
   if($isadmin == 0) : ?>
      <a href="#" class="easyui-linkbutton" id="b_add_lo" iconCls="icon-add" plain="true" onclick="ctrlr_lo.add();"><?php mea_toLocalC('add'); ?></a>
      <a href="#" class="easyui-linkbutton" id="b_edit_lo" iconCls="icon-edit" plain="true" onclick="ctrlr_lo.edit();"><?php mea_toLocalC('edit/view'); ?></a>
      <a href="#" class="easyui-linkbutton" id="b_del_lo" iconCls="icon-remove" plain="true" onclick="ctrlr_lo.del();"><?php mea_toLocalC('delete'); ?></a>
<?php
   else : ?>
      <a href="#" class="easyui-linkbutton" id="b_edit_lo" iconCls="icon-edit" plain="true" onclick="ctrlr_lo.view();"><?php mea_toLocalC('view'); ?></a>
<?php
   endif; ?>
</div>


<div id="dlg_lo" class="easyui-dialog" style="width:500px;height:450px;padding:10px 20px" modal="true" closed="true" buttons="#dlg_buttons_lo">
    <div class="ftitle"><?php mea_toLocalC('locations'); ?></div>
    <form id="fm_lo" method="post" novalidate>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('name'); ?></label>
            <input id="location_name" name="name" class="easyui-textbox editable_lo" style="width:100px;" data-options="required:true,validType:'name_validation',missingMessage:translationController.toLocalC('location name is mandatory')">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('description'); ?></label>
            <input name="description" class="easyui-textbox editable_lo" data-options="multiline:true" style="width:270px;height:50px">
        </div>
        <input name="id_location" type="hidden">
        <input name="id" type="hidden" id="database_lo_id" defaultValue='-1' value='-1'>
    </form>
</div>
<div id="dlg_buttons_lo">
<?php
   if($isadmin == 0) : ?>
      <a href="javascript:void(0)" id="submit_button_lo"  class="easyui-linkbutton" iconCls="icon-ok" style="width:100px"><?php mea_toLocalC('save'); ?></a>
      <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_lo').dialog('close')" style="width:100px"><?php mea_toLocalC('cancel'); ?></a>
<?php
   else : ?>
      <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_lo').dialog('close')" style="width:100px"><?php mea_toLocalC('close'); ?></a>
<?php
   endif; ?>
</div>

