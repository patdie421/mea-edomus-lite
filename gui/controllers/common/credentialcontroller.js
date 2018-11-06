function CredentialController(dataSource)
{
   this.userid=null;
   this.profil=null;
   this.loggedIn=null;
   this.last=0;
   this.cacheTime=5; // 5 secondes entre deux appels
   this.dataSource=dataSource;
}


CredentialController.prototype.get = function()
{
   var _this = this;
   var _controller = _this;
   var retour=false;
   var now = Date.now();
      
   if((now - 5*1000) > _this.last) {
      $.ajax({
         url: _this.dataSource,
         async: false,
         type: 'GET',
         dataType: 'json',
         data: {},
//         beforeSend: function(){
//         },
         success: function(data) {
            if(data.result=="OK") {
               _this.userid=data.userid;
               _this.profil=parseInt(data.profil);
               _this.loggedIn=true;
               _this.last=now;
               retour=true;
            }
         },
         error: function(jqXHR, textStatus, errorThrown){
            // afficher message d'alerte ici ?
            _this.userid=-1;
            _this.profil=-1;
            _this.loggedIn=false;
            _this.last=0;
            retour=false;
           console.log("Communication Error : "+textStatus+" ("+errorThrown+")");
         }
      });
   }
   else {
      if(_this.loggedIn!=-1)
         retour=true;
      else
         retour=false;
   }
   return retour;
};

   
CredentialController.prototype.isAdmin = function()
{
  if(this.get()==true) {
      if(this.profil)
         return true;
      else
         return false;
  }
  else
    return false;
};


CredentialController.prototype.isLoggedIn = function() {
   return this.get();
};

   
CredentialController.prototype.__auth = function(_controller, methode, param)
{
   var _this = this;

   var jqxhr = $.get(this.dataSource,
      {}, // pas de parametre
      function(data) {
         if(data.result=="OK"){
            var now = Date.now();
            _this.userid=data.userid;
            _this.profil=parseInt(data.profil);
            _this.loggedIn=true;
            _this.last=now;

            _controller[methode](param);
         }
         else {
            $.messager.alert(_controller._toLocalC('error')+_controller._localDoubleDot(),_controller._toLocalC('you are not connected')+' !', 'error', function(){ window.location = "login.php"; });
         }
      },
      "json"
   )
   .done(function() {
   })
   .fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_controller._toLocalC('error')+_controller._localDoubleDot(),
         msg: _controller._toLocalC('communication error')+'('+textStatus+')'
      });
   })
   .always(function() {
   });
};

