/*
if(typeof(GridController)=="undefined"){ //  TypeController héritera de GridController ...
   window.location = "error6.html";
}
*/
function TypesController(params)
{
   TypesController.superConstructor.call(this,params);
}


extendClass(TypesController, GridController); // héritage de CommonController


TypesController.prototype.nameExist=function(name,id)
{
   var _controller = this;
   var name_exist=check_field_exist("types","name",name,id);
   if(name_exist)
   {
      $.messager.alert(_controller._toLocalC('error')+_controller._localDoubleDot(),_controller._toLocalC('name already used')+". "+_controller._toLocalC('please choose an other name for this type')+'.','error');
      return false;
   }
   return true;
};


TypesController.prototype.getNextTypeId=function()
{
   var _controller = this;
   var next_id=0;

   $.ajax({ url: 'models/get_type_next_id.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: {},
            success: function(data){
               if(data.iserror==false) {
                  next_id=data.next_id;
               }
               else {
                  $.messager.show({
	                   title:_controller._toLocalC('error')+_controller._localDoubleDot(),
	                   msg:_controller._toLocalC('server side error')+' ('+data.errMsg+')'
                  });
               }
            },
            error: function(jqXHR, textStatus, errorThrown) {
               $.messager.show({
	                title:_controller._toLocalC('error')+_controller._localDoubleDot(),
	                msg:_controller._toLocalC('communication error')+' ('+textStatus+')'
               });
            }
   });
   return next_id;
};


TypesController.prototype.del=function(){
   var _controller = this;
   var row = $('#'+_controller._grid_id).datagrid('getSelected');
   
   if(row){
      var jqxhr = $.get('models/get_2fields.php',
                        { table1:'sensors_actuators', field1:'name', where1:'id_type='+row.id_type, table2:'interfaces', field2:'name', where2:'id_type='+row.id_type },
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
                              if(data.values1.length > 0 || data.values2.length > 0){
                                 msg=_controller._toLocalC('type')+' '+row.name+' '+_controller._toLocal("can't be deleted because it is used by")+_controller._localDoubleDot()+'</BR></BR>';
                                 if(data.values1.length) {
                                    msg=msg+_controller._toLocalC('sensor(s)/actuator(s)')+_controller._localDoubleDot()+data.values1+'</BR>';
                                 }
                                 if(data.values2.length) {
                                    msg=msg+_controller._toLocalC('interface(s)')+_controller._localDoubleDot()+data.values2+'</BR></BR>';
                                    msg=msg+_controller._toLocalC("update this object(s) before deleting")+'.';
                                 }
                                 msg=msg+'</BR>';
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

