function RulesEditorController(_editorzone, _editordiv, _menu, _mn1)
{
   RulesEditorController.superConstructor.call(this);

   this.current_file=false;

   this.editorzone = _editorzone;
   this.editordiv = _editordiv;
   this.menu = _menu;
   this.mn1 = _mn1;

//   this.contextmenu = _contextmenu;

   this.editor = ace.edit(this.editordiv);
   this.editor.setOption("fixedWidthGutter", true);
   this.editor.setTheme("ace/theme/xcode");
   this.editor.session.setMode("ace/mode/mearules");
   this.editor.session.setTabSize(3);
   this.editor.session.setUseSoftTabs(true);
   this.editor.setOptions({ fontFamily: "'Lucida Console', Monaco, monospace" });
   this.ctrlr_filechooser = new FileChooserController("#"+this.editorzone);
}

extendClass(RulesEditorController, CommonController);


RulesEditorController.prototype.start = function()
{
   var _this = this;

   if (!_this._isAdmin())
   {
   }

   var evnt = "activatetab_" + translationController.toLocalC('rules editor');
   evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
   $(document).off(evnt);
   $(document).on(evnt, function( event, tab, arg2 ) {
      $("#"+_this.editordiv).height($("#"+_this.editorzone).height()-35);
   });
   $("#"+_this.editordiv).height($("#"+_this.editorzone).height()-35);

   $(document).on( "MeaCenterResize", function( event, arg1, arg2 ) {
      setTimeout( function() {
         $("#"+_this.editordiv).height($("#"+_this.editorzone).height()-35);
         _this.editor.resize();
      },
      25);
   });

   _this.editor.setValue("", -1);
   _this.editor.resize();

/*
   _this.editor.container.addEventListener("contextmenu", function(e) {
       _this.open_context_menu(e);
   });
*/
   $("#"+_this.menu).show();
   $("#"+_this.editordiv).show();

   var data = permMemController.get("rulesEditor_data");
   if(data != false) {
      _this.editor.setValue(data, -1);
   }

   var current_file = permMemController.get("rulesEditor_current_file");
   if(current_file != false)
      _this.current_file = current_file;
};


RulesEditorController.prototype._open_context_menu = function(x, y)
{
   var _this = this;

   $("#"+_this.contextMenu).menu('show', {left: x, top: y });
}


RulesEditorController.prototype.open_context_menu = function(e)
{
   var _this = this;

   e.preventDefault();

   _this._open_context_menu(e.pageX, e.pageY);

   return false;
}


RulesEditorController.prototype.open = function(name, type, checkflag)
{
   var _this = this;


   _this.current_file=name;
   if(checkflag === true)
      return;

   $.get("models/get_file.php", { type: type, name: name }, function(response) {
      if(response.iserror===false)
      {
         _this.editor.setValue(response.file, -1);
         if(_this._isAdmin()!==true)
            _this.editor.setReadOnly(true);
         else
            _this.editor.setReadOnly(false);
      }
      else
      {
         $.messager.alert(_this._toLocalC('error'), _this._toLocalC("can't load file")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
};


RulesEditorController.prototype.saveas = function(name, type, checkflag, afterSave)
{
   var _this = this;

   afterSave = typeof afterSave !== 'undefined' ? afterSave : false;

   if(_this._isAdmin()!==true)
      return;

   var data = _this.editor.getValue();

   _this.current_file=name;
   $.post("models/put_file.php", { type: type, name: name, file: data }, function(response) {
      if(response.iserror===false)
      {
         if(afterSave != false)
            afterSave();
      }   
      else
      {
         _this.current_file=false;
         $.messager.alert(_this._toLocalC('error'), _this._toLocalC("can't save file")+ " (" + _this._toLocal('server message')+_this._localDoubleDot()+response.errMsg+")", 'error');
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
};


RulesEditorController.prototype.delete = function(name, type, checkflag)
{
   var _this = this;

   if(_this._isAdmin()!==true)
      return;

   var _delete_rules = function() {
      $.post("models/del_file.php", { name: name, type: type }, function(response) {

         if(response.iserror === false)
         {
         }
         else
         {
            $.messager.alert(_this._toLocalC('error'),
                             _this._toLocalC("can't delete rules source file")+_this._localDoubleDot()+response.errMsg,
                             'error');
         }
      }).done(function() {
      }).fail(function(jqXHR, textStatus, errorThrown) {
         $.messager.show({
            title:_this._toLocalC('error')+_this._localDoubleDot(),
            msg: _this._toLocalC("communication error")+' ('+textStatus+')'
         });
      });
   }

   $.messager.defaults.ok = _this._toLocalC('Yes');
   $.messager.defaults.cancel = _this._toLocalC('No');
   $.messager.confirm(_this._toLocalC('deleting')+' '+_this._toLocal('rules source file ...'),_this._toLocalC('are you sure you want to delete')+' "'+name+'"'+_this._localDoubleDot(), function(r)
   {
      if(r)
         _delete_rules(name);
   });
}


RulesEditorController.prototype.domenu = function(action, afterAction)
{
   var _this = this;

   afterAction = typeof afterAction !== 'undefined' ? afterAction : false;

   var __open_srules = _this.open.bind(_this);
   var __saveas_srules = _this.saveas.bind(_this);
   var __delete_srules = _this.delete.bind(_this);

   switch(action)
   {
      case 'new':
         _this.current_file=false;
         _this.editor.setValue("");
         break;

      case 'open':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules sources ..."), _this._toLocalC("open")+_this._localDoubleDot(), _this._toLocalC("open"), _this._toLocalC("cancel"), "srules", true, true, _this._toLocalC("file does not exist, new file ?"), __open_srules);
         break;

      case 'saveas':
         _this.ctrlr_filechooser.open(_this._toLocalC("choose rules sources ..."), _this._toLocalC("save as")+_this._localDoubleDot(), _this._toLocalC("save as"), _this._toLocalC("cancel"), "srules", true, false, _this._toLocalC("file exist, overhide it ?"), __saveas_srules);
         break;

      case 'save':
         if(_this.current_file!=false)
            __saveas_srules(_this.current_file,"srules",false, afterAction);
         else
            _this.ctrlr_filechooser.open(_this._toLocalC("choose rules sources ..."), _this._toLocalC("save as")+_this._localDoubleDot(), _this._toLocalC("save as"), _this._toLocalC("cancel"), "srules", true, false, _this._toLocalC("file exist, overhide it ?"), __saveas_srules, afterAction);
         break;

      case 'delete':
            _this.ctrlr_filechooser.open(_this._toLocalC("choose rules sources ..."), _this._toLocalC("delete")+_this._localDoubleDot(), _this._toLocalC("delete"), _this._toLocalC("cancel"), "srules", false, false, "", __delete_srules);
         break;
      case 'buildactivate':
           _this.buildactivate();
      default:
         break;
   }
}


RulesEditorController.prototype.leaveViewCallback = function()
{
   var _this = this;

   var data = _this.editor.getValue();

   permMemController.add("rulesEditor_data", data);
   permMemController.add("rulesEditor_current_file", this.current_file);

   $('#'+_this.mn1).remove();
}


RulesEditorController.prototype.buildactivate = function()
{
   var _this = this;

   var __restart_automator = function(r)
   {
      if(r==true)
      {
         var isadmin = _this._isAdmin();

         if(isadmin != true)
            return -1;

         $.ajax({
            url: 'CMD/automatorrestart.php',
            async: true,
            type: 'GET',
            dataType: 'json',
            success: function(data){
               if(data.iserror == true)
               {
                  $.messager.show({
                     title: _this._toLocalC('error')+_controlPanel._localDoubleDot(),
                     msg: _this._toLocalC('activation error')+' ('+textStatus+')',
                  });
               }
            },
            error: function(jqXHR, textStatus, errorThrown ){
               $.messager.show({
                  title: _this._toLocalC('error')+_controlPanel._localDoubleDot(),
                  msg: _this._toLocalC('server or communication error')+' ('+textStatus+')',
               });
            }
         });
      }
   }

   var __activate_rules = function(name)
   {
      $.get("models/apply_rules.php", { name: name }, function(response) {
         if(response.iserror === false)
         {
            var old_ok = $.messager.defaults.ok;
            var old_cancel = $.messager.defaults.cancel;

            $.messager.defaults.ok = _this._toLocal('Do it');
            $.messager.defaults.cancel = _this._toLocal('later');
            $.messager.confirm(_this._toLocalC('apply rules ?'), _this._toLocalC('rules succesfull applied')+'. '+_this._toLocalC("would you commit it")+'?', __restart_automator);
            $.messager.defaults.ok = old_ok;
            $.messager.defaults.cancel = old_cancel;
         }
         else
         {
            $.messager.alert(_this._toLocalC('error'),
                             _this._toLocalC("can't apply rules")+_this._localDoubleDot()+response.errMsg,
                             'error');
         }
      }).done(function() {
      }).fail(function(jqXHR, textStatus, errorThrown) {
         $.messager.show({
            title:_this._toLocalC('error')+_this._localDoubleDot(),
            msg: _this._toLocalC("server or communication error")+' ('+textStatus+')'
         });
      });
   }

   __buildactivate_rules = function(name)
   {
      function basename(file) {
         return file.split('/').reverse()[0];
      }

      var files = [];
      files.push(name);

      $.get("models/build_rules.php", { name: name, files: files }, function(response) {
         if(response.iserror === false)
         {
             __activate_rules(name);
         }
         else
         {
            if(response.file && response.line)
            {
               $.messager.alert(_this._toLocalC('error'),
                                _this._toLocalC('compilation error')+_this._localDoubleDot()+response.message+
                                '.<BR><BR><div align=center>'+_this._toLocal('file')+_this._localDoubleDot()+basename(response.file).slice(0, -7)+'<BR>'+_this._toLocal('line')+_this._localDoubleDot()+response.line+"</div>",
                                'error');
            }
            else
            {
               $.messager.alert(_this._toLocalC('error'),
                                _this._toLocalC('compilation error')+_this._localDoubleDot()+response.errMsg,
                                'error');
            }
         }
      }).done(function() {
      }).fail(function(jqXHR, textStatus, errorThrown) {
         $.messager.show({
            title:_this._toLocalC('error')+_this._localDoubleDot(),
            msg: _this._toLocalC("server or communication error")+' ('+textStatus+')'
         });
      });
   }

   var afterSave = function() {
      __buildactivate_rules(_this.current_file);
   }

   _this.domenu('save', afterSave);
}
