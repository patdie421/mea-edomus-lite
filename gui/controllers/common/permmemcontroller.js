function PermMemController()
{
    this.permMem = {};

    PermMemController.superConstructor.call(this);
}

extendClass(PermMemController, CommonController);

PermMemController.prototype.add = function(key, data) {
   this.permMem[key] = data;
};

PermMemController.prototype.del = function(key) {
   if(key in this.permMem)
      delete this.permMem[key];
};

PermMemController.prototype.get = function(key) {
   if(key in this.permMem)
      return this.permMem[key];
   else
      return false;
};
