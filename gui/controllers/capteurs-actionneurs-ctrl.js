/*
if(typeof(GridController)=="undefined"){ //  CapteursActionneursController héritera de GridController ...
   window.location = "error.html";
}
*/

function CapteursActionneursController(params)
{
   CapteursActionneursController.superConstructor.call(this,params);
}


extendClass(CapteursActionneursController, GridController); // héritage de CommonController


CapteursActionneursController.prototype.nameExist=function(name,id)
{
   var _controller = this;
   var name_exist=check_field_exist("sensors_actuators","name",name,id);
   if(name_exist)
   {
      $.messager.alert(_controller._toLocalC('error')+_controller._localDoubleDot(),_controller._toLocalC('name already used')+". "+_controller._toLocalC('please choose an other name for this sensor/actuator')+'.','error');
      return false;
   }
   return true;
};

