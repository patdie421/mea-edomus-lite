$.extend($.fn.validatebox.defaults.rules, {
   filename_validation: {
      validator: function(value, param){
         return checkRegexp( value, /^[_0-9A-Za-z\.]*$/);
      },
      message: "caractères dans l'intervalle [a-zA-Z0-9] uniquement."
   }
});


function checkRegexp( str,regexp)
{
   $.trim(str);
   if ( !(regexp.test( str )) ) {
      return false;
   }
   else {
      return true;
   }
}


function FileChooserController(attachement)
{
   FileChooserController.superConstructor.call(this);

   this.showext = false;
   this.attachement = attachement; 
   this.id = "dlg_"+Math.floor((Math.random() * 10000) + 1);
}


extendClass(FileChooserController, MeaObject);

FileChooserController.prototype = {
   _getHtml: function(max_x, max_y)
   {
      var _this=this;
      var id=_this.id;

      var w=max_x-55;
      var h=max_y-200;
      if(h<400)
         h=max_y-170;
      h=h-5; 

      html="<div id='"+id+"' style=\"padding:10px 20px\"> \
               <div id='"+id+"_title' class='ftitle'> \
               </div> \
               <form id='"+id+"_fm' method='post' data-options=\"novalidate:false\"> \
                  <div class='fitem'> \
                     <div id='"+id+"_files' style='width:"+w+"px;height:"+h+"px'></div> \
                  </div> \
                  <div class='fitem' style=\"padding-top:10px;\"> \
                     <input id='"+id+"_filename' style=\"width:100%;\"> \
                  </div> \
               </form> \
            </div>";

      return html;
   },

   _loadDialog: function(id, _type, response, displayext,__do_param)
   {
      var _this = this;

      var div_files = $('#'+id+'_files');

      div_files.datalist({
         valueField: 'f1',
         textField:  'f2',
         lines: false,
         onDblClickRow: function(index, field) {
            $('#'+id+"_filename").textbox('setValue', field.f2);
            __do(response.values, __do_param);
         },
         onClickRow: function(index, field) {
            $('#'+id+"_filename").textbox('setValue', field.f2);
         } 
      });

      var files = [];

      for(var i in response.values)
         if(displayext===false)
            files.push({f1: i, f2:response.values[i].slice(0, -(_type.length+1))}); 
         else
            files.push({f1: i, f2:response.values[i]}); 

      div_files.datalist({data: files});
   },

   open: function(_title, _title2, _buttonOKText, _buttonCancelText, _type, _checkflag, _mustexist, _checkmsg, _do, _do_param)
   {
      var _this = this;
      id=_this.id;

      __do = function(files, __do_param) 
      {
         name = $('#'+id+"_filename").val();
         if(name==="")
            return -1;
         filename=name+"."+_type;

         if(_checkflag === true)
         { 
            inarray=$.inArray(filename, files);
            if(  (inarray == -1 && _mustexist == true)    // pas dans la liste mais doit exister
              || (inarray != -1 && _mustexist == false) ) // dans la liste mais ne doit pas y etre 
            {
               $.messager.confirm('Confirm', _checkmsg, function(r) {
                  if (r) {
                     _this.close();
                     _do(name, _type, true, __do_param);
                  }
               });
            }
            else
            {
               _this.close();
               _do(name, _type, false, __do_param); 
            }
         }
         else
         {
            _this.close();
            _do(name, _type, false, __do_param);
         }
      };

      var max_x=$('body').width();
      var max_y=$('body').height();
      var w=350;
      var h=400;
      if(max_x<350)
         w=300;
      if(max_y<400)
         h=280;

      $.get("models/get_files_list.php", { type: _type }, function(response) {
         if(response.iserror === false)
         {
            $('#'+id).empty();
            $('#'+id).remove();
            html=_this._getHtml(w,h);

            $('body').append(html);
            $('#'+id+'_title').text(_title2);
            $('#'+id+'_filename').textbox({ required: true, validType: 'filename_validation'});
            $("#"+id+"_filename").textbox('setValue', "");

            $('#'+id).dialog({
               title: _title,
               width: w,
               height: h,
               closed: false,
               cache: false,
               modal: true,
//               left:0,
//               top:0,
               buttons:[
               {
                  text: _buttonOKText,
                  iconCls:'icon-ok',
                  handler:function() {
                     if(!$('#'+id+'_fm').form('validate'))
                        return -1;
                     __do(response.values, _do_param);
                  }
               }, {
                  text: _buttonCancelText,
                  iconCls:'icon-cancel',
                  handler:function() {
                     _this.close();
                  }
               }],
               onClose: function() { _this.clean(); },
            });

            _this._loadDialog(id, _type, response, _this.showext, _do_param);
            $('#'+id).dialog("open");
         }
      });
   },

   clean: function() {
      $('#'+id).empty();
      $('#'+id).remove();
   },

   close: function() {
      $('#'+id).dialog("close");
   }
};
