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

<script type="text/javascript" src="controllers/utilisateurs-ctrl.js"></script>

<script>

function strProfile_us(val,row)
{
   switch(val)
   {
      case  '0': return translationController.toLocalC("user");
      case  '1': return translationController.toLocalC("administrator");
      case  '2': return translationController.toLocalC("maps only");
   }
}

<?php
if($isadmin!=0) : ?>
function viewonly_us()
{
  $(".editable_us").textbox({editable: false, disabled:true});
  $(".readonly_us").textbox({readonly: true, disabled:true});
}
<?php
else : ?>
// validation du formulaire avant soumission
function formValidation_us() {
   var id=$("#database_us_id").val();
   var name=$("#user_name").val();
   
   // vérification controle de surface
   if(!$('#fm_us').form('validate'))
   {
      $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC('one or more mandatory field(s) is/are empty or badly formed'),'error');

      return false;
   }

   var password=$("#password").val();
   var password2=$("#password2").val();
   if(password != password2)
   {
      $.messager.alert(translationController.toLocalC('error')+translationController.localDoubleDot(),translationController.toLocalC("password and password validation don't match"),'error');

      return false;
   }

   // vérification unicité (nom du lieu)
   return ctrlr_us.nameExist(name,id);
}
<?php
endif; ?>

jQuery(document).ready(function(){
   // Création du controleur
   ctrlr_us = new UsersController({
      // pour la connexion à l'ihm
      grid_id:'table_us',
      dialogBox_id:'dlg_us',
      form_id:'fm_us',
      submitButton_id:'submit_button_us',

      // pour la connexion aux models
      editDataSource:'models/update_users.php', // à compléter
      addDataSource: 'models/new_users.php',
      delDataSource: 'models/delete_users.php'
   });
   
   // lien avec d'autres objets
   ctrlr_us.linkToTranslationController(translationController);

   // lien spécifique avec la vue (paramètre optionnel du controleur
   ctrlr_us.setButtonsAnimation('b_edit_us', 'b_del_us');

<?php
if($isadmin==0) : ?>
   ctrlr_us.setValidate(formValidation_us);
<?php
else : ?>
   viewonly_us();
<?php
endif; ?>   
   
   // activation du controleur
   ctrlr_us.start();

   var pg = $("#table_us").datagrid('getPager');
   var options = pg.pagination('options');
   options.afterPageText=ctrlr_us._toLocal("of {pages}");
   options.displayMsg=ctrlr_us._toLocalC("Displaying {from} to {to} of {total} items");
   console.log(pg);
});
</script>


<style>
#fm_us {
   margin:0;
   padding:10px 30px;
}

.editable_us {
}

.readonly_us {
}
</style>


<div style="width:100%;height:100%">
   <div style="width:100%;height:100%;margin: 0 auto;">
      <table id="table_us"
             title=""
             style="width:100%;height:100%"
             border="false"
             pagination="true"
             pageSize=50
             pageList=[20,50,100]
             idField="id"
             fitColumns="true"
             singleSelect="true"
             toolbar="#toolbar_us"
             url="models/get_users.php">
         <thead>
            <tr>
               <th data-options="field:'id',width:10,hidden:true"><?php mea_toLocalC('id'); ?></th>
               <th data-options="field:'id_user',width:10,hidden:true"><?php mea_toLocalC('user_id'); ?></th>
               <th data-options="field:'name',width:100,sortable:true,align:'left'"><?php mea_toLocalC('name'); ?></th>
               <th data-options="field:'password',width:10,hidden:true"><?php mea_toLocalC('password'); ?></th>
               <th data-options="field:'description',width:200,align:'left'"><?php mea_toLocalC('description'); ?></th>
               <th data-options="field:'profil',width:50,align:'center',formatter:strProfile_us"><?php mea_toLocalC('profile'); ?></th>
               <th data-options="field:'language',width:50,align:'center'"><?php mea_toLocalC('language'); ?></th>
               <th data-options="field:'flag',width:10,hidden:true"><?php mea_toLocalC('flag'); ?></th>
            </tr>
         </thead>
      </table>
   </div>
</div>


<div id="toolbar_us">
<?php
   if($isadmin == 0) : ?>
      <a href="#" class="easyui-linkbutton" id="b_add_us" iconCls="icon-add" plain="true" onclick="ctrlr_us.add();"><?php mea_toLocalC('add'); ?></a>
      <a href="#" class="easyui-linkbutton" id="b_edit_us" iconCls="icon-edit" plain="true" onclick="ctrlr_us.edit();"><?php mea_toLocalC('edit/view'); ?></a>
      <a href="#" class="easyui-linkbutton" id="b_del_us" iconCls="icon-remove" plain="true" onclick="ctrlr_us.del();"><?php mea_toLocalC('delete'); ?></a>
<?php
   else : ?>
      <a href="#" class="easyui-linkbutton" id="b_edit_us" iconCls="icon-edit" plain="true" onclick="ctrlr_us.view();"><?php mea_toLocalC('view'); ?></a>
<?php
   endif; ?>
</div>


<div id="dlg_us" class="easyui-dialog" style="width:500px;height:450px;padding:10px 20px" modal="true" closed="true" buttons="#dlg_buttons_us">
    <div class="ftitle"><?php mea_toLocalC('users'); ?></div>
    <form id="fm_us" method="post" novalidate>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('name'); ?></label>
            <input id="user_name" name="name" class="easyui-textbox editable_us" style="width:100px;" data-options="required:true,validType:'name_validation',missingMessage:translationController.toLocalC('location name is mandatory')">
        </div>
<?php
   if($isadmin==0) : ?>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('password'); ?></label>
            <input id="password" name="password" class="easyui-textbox editable_us" type="password" style="width:100px;" data-options="required:true,validType:'password_validation',missingMessage:translationController.toLocalC('password is mandatory')">
        </div>

        <div class="fitem">
            <label><?php mea_toLocalC_2d('confirmation'); ?></label>
            <input id="password2" name="password" class="easyui-textbox editable_us" style="width:100px;" type="password" data-options="required:true,validType:'password_validation',missingMessage:translationController.toLocalC('confirmation password is mandatory')">
        </div>
<?php
   endif; ?>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('description'); ?></label>
            <input name="description" class="easyui-textbox editable_us" data-options="multiline:true" style="width:270px;height:50px">
        </div>
        <div class="fitem">
            <label><?php mea_toLocalC_2d('profile'); ?></label>
            <select class="easyui-combobox editable_us" name="profil" data-options="required:true,editable:false,panelHeight:105" style="width:120px;">
               <option value=0><?php mea_toLocalC('user'); ?></option>
               <option value=1><?php mea_toLocalC('administrator'); ?></option>
               <option value=2><?php mea_toLocalC('maps only'); ?></option>
            </select>
        </div>
       
        <div class="fitem">
            <label><?php mea_toLocalC_2d('language'); ?></label>
            <select class="easyui-combobox editable_us" name="language" data-options="required:true,editable:false,panelHeight:105" style="width:120px;">
               <option value="default"><?php mea_toLocalC('default (en)'); ?></option>
               <option value="fr"><?php mea_toLocalC('français (fr)'); ?></option>
            </select>
        </div>
 
        <div class="fitem">
            <label><?php mea_toLocalC_2d('password option'); ?></label>
            <select class="easyui-combobox readonly_us" name="flag" data-options="required:true,editable:false,panelHeight:105" style="width:120px;">
               <option value=1><?php mea_toLocalC("must be changed at first login"); ?></option>
               <option value=0><?php mea_toLocalC("must not be changed at first login"); ?></option>
               <option value=2><?php mea_toLocalC("can't be changed by user"); ?></option>
            </select>
        </div>
        <input name="id_user" type="hidden">
        <input name="id" type="hidden" id="database_us_id" defaultValue='-1' value='-1'>
    </form>
</div>
<div id="dlg_buttons_us">
<?php
   if($isadmin == 0) : ?>
      <a href="javascript:void(0)" id="submit_button_us"  class="easyui-linkbutton" iconCls="icon-ok" style="width:100px"><?php mea_toLocalC('save'); ?></a>
      <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_us').dialog('close')" style="width:100px"><?php mea_toLocalC('cancel'); ?></a>
<?php
   else :?>
      <a href="javascript:void(0)" class="easyui-linkbutton" iconCls="icon-cancel" onclick="javascript:$('#dlg_us').dialog('close')" style="width:100px"><?php mea_toLocalC('close'); ?></a>
<?php
   endif; ?>
</div>

