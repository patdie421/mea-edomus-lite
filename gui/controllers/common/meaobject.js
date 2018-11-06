extendClass = function(subClass, baseClass) {
   function __inheritance() {}
   __inheritance.prototype = baseClass.prototype;
   subClass.prototype = new __inheritance();
   subClass.prototype.constructor = subClass;
   subClass.superConstructor = baseClass;
   subClass.superClass = baseClass.prototype;
}


function MeaObject()
{
  this.loginUrl = "login.php"; // à mettre dans credential controller ?
}
