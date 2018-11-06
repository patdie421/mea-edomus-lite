/*
if(typeof(GridController)=="undefined"){ // LocationsController héritera de gridController ...
   window.location = "error1.html";
}
*/

function UsersController(params)
{
   UsersController.superConstructor.call(this,params);
}


extendClass(UsersController, GridController); // héritage de CommonController


UsersController.prototype.nameExist=function(name,id)
{
   var _controller = this;
   var name_exist=check_field_exist("users","name",name,id);
   if(name_exist)
   {
      $.messager.alert(_controller._toLocalC('error')+_controller._localDoubleDot(),_controller._toLocalC('name already used')+". "+_controller._toLocalC('please choose an other name for this user')+'.','error');
      return false;
   }
   return true;
};

