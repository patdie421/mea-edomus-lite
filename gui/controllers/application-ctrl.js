/*
if(typeof(CommonController)=="undefined")  {// ControlPanel héritera de CommonController ... qui doit donc être défini !
   window.location = "error.html";
}
*/

function ApplicationPrefsController()
{
   ApplicationPrefsController.superConstructor.call(this);
}


extendClass(ApplicationPrefsController, CommonController); // héritage de CommonController


ApplicationPrefsController.prototype.ceate_querydb=function(querydb){
    $.ajax({
        url: 'models/create_querydb.php',
        async: true,
        type: 'GET',
        data: { 'querydb': querydb },
        dataType: 'json',
        timeout: 5000,
        success: function(data) {
            if(data.iserror == true)
            {
               if(data.errno==99){
                  $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),_controller._toLocalC('you are not connected')+' !', 'error', function(){ window.location = _controller.loginUrl;});
               }
               else if(data.errno==98){
                  $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),_controller._toLocalC('you are not administrator')+' !', 'error');
               }
               else {
                  $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),'server side error'+' ('+data.errorMsg+')', 'error');
               }
            }
            else
            {
               $.messager.alert(_controller._toLocalC("Succes")+_controller._localDoubleDot(),_controller._toLocalC('db succesfull initialized'),'info');
            }
        },
        error: function(jqXHR, textStatus, errorThrown) { $(loading).dialog('close');
            ajax_error(jqXHR, textStatus, errorThrown);
        }
    });
};


ApplicationPrefsController.prototype.checkMysql=function(server, port, base, user, password) {
   var _controller = this;
   var _timeout = 10000;
   var _extraTime = 1.2;
   var _timerRef = 0;

   function progress(msg){
      var win = $.messager.progress({
      title:'please waiting',
      msg:msg,
      text:"",
      interval:_timeout*_extraTime/10
      });
   }

   _timerRef = setTimeout(function(){
      $.messager.progress('close');
      // faire ici quelque chose de plus ...
   },_timeout*_extraTime);
   
   progress("trying to connect");
   
   $.ajax({
      url: 'models/check_mysql_connexion.php',
      async: true,
      type: 'GET',
      data: { 'server': server, 'port':port, 'base':base, 'user':user, 'password':password },
      dataType: 'json',
      timeout: _timeout,
      success: function(data){
         clearTimeout(_timerRef);
         $.messager.progress('close');
         if(data.iserror == true)
         {
            if(data.errno==99){
               $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),_controller._toLocalC('you are not connected')+' !', 'error', function(){ window.location = _controller.loginUrl;});
            }
            else if(data.errno==98){
               $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),_controller._toLocalC('you are not administrator')+' !', 'error');
            }
            else {
               $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),'connection error from server'+_controller._localDoubleDot()+data.errorMsg, 'error');
            }
         }
         else
         {
            $.messager.alert(_controller._toLocalC("Succes")+_controller._localDoubleDot(),_controller._toLocalC('succesfull connection'),'info');
         }
      },
      error: function(jqXHR, textStatus, errorThrown ){
         clearTimeout(_timerRef);
         $.messager.progress('close');
         $.messager.show({
            title: _controller._toLocalC('error')+_controller._localDoubleDot(),
            msg: _controller._toLocalC('communication error')+' ('+textStatus+')',
         });
      }
   });
};

ApplicationPrefsController.prototype.load=function(){
   var _controller = this;
   $.ajax({
      url: 'models/get_config.php',
      async: true,
      type: 'GET',
      dataType: 'json',
      success: function(data){
         // voir ici les habilitations ... analyse retour de la requete
         if(data.iserror==true && data.errno !=98)
         {
            if(data.errno==99){
               $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),_controller._toLocalC('you are not connected')+' !', 'error', function(){ window.location = _controller.loginUrl;});
            }
            else {
               $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),'server side error'+' ('+data.errMsg+')', 'error');
            }
         }
         else
         {
            for(var p in data) {
               if(p!='iserror')
               {
                  var object = data[p];
                  $("#"+object['key']).textbox('setValue',object['value']);
               }
            }
         }
      },
      error: function(jqXHR, textStatus, errorThrown ){
         $.messager.show({
            title: _controller._toLocalC('error')+_controller._localDoubleDot(),
            msg: _controller._toLocalC('communication error')+' ('+textStatus+')',
         });
      }
   });
};


ApplicationPrefsController.prototype.save=function(){
   var _controller = this;
   var data = {};
   data["VENDORID"]=$("#VENDORID").val();
   data["DEVICEID"]=$("#DEVICEID").val();
   data["INSTANCEID"]=$("#INSTANCEID").val();
   data["PLUGINPATH"]=$("#PLUGINPATH").val();
   data["DBSERVER"]=$("#DBSERVER").val();
   data["DBPORT"]=$("#DBPORT").val();
   data["DATABASE"]=$("#DATABASE").val();
   data["USER"]=$("#USER").val();
   data["PASSWORD"]=$("#PASSWORD").val();
   data["BUFFERDB"]=$("#BUFFERDB").val();
   
   $.ajax({
      url: 'models/set_config.php',
      async: true,
      type: 'GET',
      dataType: 'json',
      data: data,
      success: function(data){
         if(data.iserror == true)
         {
            if(data.errno==99){
               $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),_controller._toLocalC('you are not connected')+' !', 'error', function(){ window.location = _controller.loginUrl;});
            }
            else if(data.errno==98){
               $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),_controller._toLocalC('you are not administrator')+' !', 'error');
            }
            else {
               $.messager.alert(_controller._toLocalC("error")+_controller._localDoubleDot(),'server side error'+' ('+data.errMsg+')', 'error');
            }
         }
         else
         {
            $.messager.alert(_controller._toLocalC("Succes")+_controller._localDoubleDot(),_controller._toLocalC('preferences saved'),'info');
         }
         return;
      },
      error: function(jqXHR, textStatus, errorThrown ){
         $.messager.show({
            title: _controller._toLocalC('error')+_controller._localDoubleDot(),
            msg: _controller._toLocalC('communication error')+' ('+textStatus+')'
         });
      }
  });
};
