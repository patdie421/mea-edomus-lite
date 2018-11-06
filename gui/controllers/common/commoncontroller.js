String.prototype.mea_hexDecode = function(){
    var j;
    var hexes = this.match(/.{1,4}/g) || [];
    var back = "";
    for(j = 0; j<hexes.length; j++) {
        back += String.fromCharCode(parseInt(hexes[j], 16));
    }

    return back;
}


String.prototype.mea_hexEncode = function(){
    var hex, i;

    var result = "";
    for (i=0; i<this.length; i++) {
        hex = this.charCodeAt(i).toString(16);
        result += ("000"+hex).slice(-4);
    }

    return result
}


// voir aussi pour les class : http://www.42hacks.com/notes/fr/20111213-comment-ecrire-du-code-oriente-objet-propre-et-maintenable-en-javascript/
function CommonController()
{
   CommonController.superConstructor.call(this);
}


extendClass(CommonController, MeaObject);

CommonController.prototype = {

// from translation controller
   _toLocal: function(string)
   {
      return string.toLowerCase();
   },

   _toLocalC: function(string)
   {
      str=this._toLocal(string);
      return str.charAt(0).toUpperCase() + str.slice(1).toLowerCase();
   },

   _localDoubleDot: function()
   {
      return ": ";
   },

   linkToTranslationController: function(translationController)
   {
      var _toLocal = null;
      try {
         _toLocal = translationController.toLocal;
         this._toLocal = _toLocal.bind(translationController);
      }
      catch(e) {
         console.log("probably bad object");
      }

      var _localDoubleDot = null;
      try {
         _localDoubleDot = translationController.localDoubleDot;
         _localDoubleDot();
         this._localDoubleDot = _localDoubleDot.bind(translationController);
         this._localDoubleDot();
      }
      catch(e) {
         console.log("probably bad object");
      }
   },


// from credentials controller
   _isLoggedIn: function() {
      return true;
   },

   _isAdmin: function() {
      return true;
   },

   __auth: function(_controller, methode) {
      _controller[methode]();
   },

   linkToCredentialController: function(_credentialController) {
      var _isLoggedIn = null;
      try {
         _isLoggedIn = _credentialController.isLoggedIn;
         this._isLoggedIn = _isLoggedIn.bind(_credentialController);
      }
      catch(e) {
         console.log("probably bad object");
      }

      var _isAdmin = null;
      try {
         _isAdmin = _credentialController.isAdmin;
         this._isAdmin = _isAdmin.bind(_credentialController);
      }
      catch(e) {
         console.log("probably bad object");
      }

      var __auth = null;
      try {
         __auth = _credentialController.__auth;
         this.__auth = __auth.bind(_credentialController);
      }
      catch(e) {
         console.log("probably bad object");
      }
   },

   errorAlert: function(title, msg) {
      $.messager.alert(title, msg, 'error');
   }
}
