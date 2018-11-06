// gestion des mots de passe et connexion
// à faire, utilisé pour login.php et change_password2.php

function PasswordController(loginsource,setpasswordsource)
{
   PasswordController.superConstructor.call(this);
   this.loginsource = loginsource;
   this.setpasswordsource = setpasswordsource;
}
 
extendClass(PasswordController, CommonController);


PasswordController.prototype.login = function(userid,password,destination,chgPasswordDest)
{
   var _controller = this;
   if(userid!="")
   {
      $.get(_controller.loginsource, {
            user_password: password,
            user_name: userid
         },
         function(data) {
            if(data.retour==1) {
               if(data.flag==1) {
                  if(chgPasswordDest === false)
                     window.location = "/login.php";
               
                  $.messager.alert(_controller._toLocalC("first login ..."),_controller._toLocalC("password must be changed !"), 'info', function(){
                     window.location = chgPasswordDest;
                  });
               }
               else {
                  window.location = destination;
               }
            }
            else {
               $.messager.alert(_controller._toLocalC("can't login ..."),_controller._toLocalC("invalid user id and/or password !"), 'error');
            }
         }).fail(function(jqXHR, textStatus, errorThrown) {
            $.messager.show({
               title:_controller._toLocalC('error')+_controller._localDoubleDot(),
               msg: _controller._toLocalC("communication error")+' ('+textStatus+')'
         });
      });
   }
   else {
      $.messager.alert(_controller._toLocalC("can't login ..."),_controller._toLocalC("invalid user id and/or password !"), 'error');
   }
};


PasswordController.prototype.validePassword=function(password){
   _controller = this;
   
   var nb_num=0, nb_alpha=0, nb_other=0, i=0;
   for(i=0;i<password.length;i++)
   {
      var c=password.charAt(i);
      if(c>='0' && c<='9')
         nb_num++;
      else if((c>='a' && c<='z') || (c>='A' && c<='Z'))
         nb_alpha++;
      else nb_other++;
   }
     
   if(i<8)
      return false;
   if(!nb_num && !nb_other)
      return false;
   return true;
};


PasswordController.prototype.chgpasswd=function(passwd, prev_passwd)
{
   var _controller = this;
   var set_password_params = false;
   
   if(prev_passwd == false)
      set_password_params={new_password:passwd};
   else
      set_password_params={old_password:prev_passwd, new_password:passwd};
   
   var retour = false;
   $.ajax({ url: _controller.setpasswordsource,
      async: false,
      type: 'GET',
      dataType: 'json',
      data: set_password_params,
      success: function(data) {
         if(data.iserror==true) {
            $.messager.alert(_controller._toLocalC("error"),_controller._toLocalC("server side error")+' ('+data.errorMsg+')', 'info');
            retour=-1;
         }
         else
         {
            if(data.retour==0)
            {
               $.messager.alert(_controller._toLocalC("password changed"),_controller._toLocalC("password succesfully changed"), 'info');
               retour=0;
            }
            else
            {
               $.messager.alert(_controller._toLocalC("password not changed"),_controller._toLocalC("try again ..."), 'info');
               retour=1;
            }
         }
      },
      error:function(jqXHR, textStatus, errorThrown) {
         $.messager.show({
            title:_controller._toLocalC('error')+_controller._localDoubleDot(),
            msg: _controller._toLocalC("communication error")+' ('+textStatus+')'
         })
      }
  });
  console.log(retour);
  return retour;
};

