/*
if(typeof(GridController)=="undefined"){ // LocationsController héritera de gridController ...
   window.location = "error.html";
}
*/

function LocationsController(params)
{
   LocationsController.superConstructor.call(this,params);
}


extendClass(LocationsController, GridController); // héritage de CommonController


LocationsController.prototype.nameExist=function(name,id)
{
   var _controller = this;
   var name_exist=check_field_exist("locations","name",name,id);
   if(name_exist)
   {
      $.messager.alert(_controller._toLocalC('error')+_controller._localDoubleDot(),_controller._toLocalC('name already used')+". "+_controller._toLocalC('please choose an other name for this location')+'.','error');
      return false;
   }
   return true;
};


LocationsController.prototype.del=function(){
   var _controller = this;
   var row = $('#'+_controller._grid_id).datagrid('getSelected');
   
   if(row){
      var jqxhr = $.get('models/get_field.php',
                        { table:'sensors_actuators', field:'name', where:'id_location='+row.id_location },
                        function(data) {
                           if(data.iserror==true) {
                              if(data.errno==98 || data.errno==99){
                                 $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),_controller._toLocalC('you are not connected')+' !', 'error', function(){ window.location = _controller.loginUrl;});
                              }
                              else {
                                 $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),'server side error'+' ('+data.errMsg+')', 'error');
                              }
                           }
                           else {
                              if(data.values.length > 0){
                                 msg=_controller._toLocalC('location')+' '+row.name+' '+_controller._toLocal("can't be deleted because it is used by following sensor(s)/actuator(s)")+_controller._localDoubleDot()+'</BR></BR>'+data.values+'</BR></BR>';
                                 msg=msg+_controller._toLocalC("update this object(s) before deleting")+'.';
                                 $.messager.alert(_controller._toLocalC('error')+_controller._localDoubleDot(),msg,'error');
                              }
                              else {
                                 _controller._del();
                                 _controller.__b_disable();
                              }
                           }
                        },
                        "json")
                       .done(function() {
                       })
                       .fail(function(jqXHR, textStatus, errorThrown) {
                          $.messager.show({
                                   title:_controller._toLocalC('error')+_controller._localDoubleDot(),
                                   msg: _controller._toLocalC("communication error")+' ('+textStatus+')'
                          });
                       })
                       .always(function() {
                       });
   }
};
