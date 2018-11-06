function GridController(params)
{
   GridController.superConstructor.call(this);

   this._grid_id=params.grid_id;
   this._dlgbox_id=params.dialogBox_id;
   this._form_id=params.form_id;
   this._submit_button_id=params.submitButton_id;
   this._editDataSource=params.editDataSource;
   this._addDataSource=params.addDataSource;
   this._delDataSource=params.delDataSource;
   
   this.validate=function() { return true; };
   this.onSelect=function(index,row) { return true; };
   
   this.__b_enable=function() { return true; };
   this.__b_disable=function() { return true; };
   this.__animationInit=function() { return true; };

//   var pg = $('#'+this._grid_id).datagrid('getPager');
//   console.log(pg);
//   pg.pagination({afterPageText: "de {pages}"});
}

extendClass(GridController, CommonController);

GridController.prototype.setValidate=function(fn)
{
   this.validate=fn;
};
   
GridController.prototype.setOnSelect=function(fn)
{
   this.onSelect=fn;
};
   
GridController.prototype.setButtonsAnimation=function(b_edit_id,b_del_id)
{
   if(typeof(b_edit_id)!="undefined" && typeof(b_del_id) != "undefined")
   {
      this.__b_enable = function() {
         $('#'+b_edit_id).linkbutton('enable');
         $('#'+b_del_id).linkbutton('enable');
      };
      
      this.__b_disable = function() {
         $('#'+b_edit_id).linkbutton('disable');
         $('#'+b_del_id).linkbutton('disable');
      };
      
      this.__animationInit = function() {
         $('#'+b_edit_id).linkbutton({disabled:true});
         $('#'+b_del_id).linkbutton({disabled:true});
      }
   }
};

GridController.prototype.__update=function(_controller,action)
{
   datasource_url="";
   switch(action)
   {
      case "edit": datasource_url=_controller._editDataSource; break;
      case "add" : datasource_url=_controller._addDataSource; break;
      default: return -1;
   }
   if(!_controller.validate())
      return false;

   $.post(datasource_url,
      $('#'+_controller._form_id).serialize(),
      function(result) {
//    var result = eval('('+result+')');
         if (result.iserror>0) {
            $.messager.show({
               title: _controller._toLocalC('error'),
               msg: _controller._toLocalC("server side error")+' ('+result.errorMsg+' / errno='+result.errno+')'
            });
         }
         else {
            $('#'+_controller._dlgbox_id).dialog('close');  // close the dialog
            $('#'+_controller._grid_id).datagrid('reload'); // reload the user data
         }
      },
      'json' // I expect a JSON response
   ).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_controller._toLocalC('error')+_controller._localDoubleDot(),
          msg: _controller._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
};


GridController.prototype._add=function()
{
   var _controller=this;

   $('#'+_controller._grid_id).datagrid('unselectAll');

   $('#'+_controller._dlgbox_id).dialog('open').dialog('setTitle',_controller._toLocalC('creation')+' ...');
   $('#'+_controller._submit_button_id).unbind();
   $('#'+_controller._submit_button_id).bind('click', function(e){ e.preventDefault(); _controller.__update(_controller,"add");});
   $('#'+_controller._form_id).form('clear');
   $('#'+_controller._form_id).form('reset');
};

   
GridController.prototype._edit=function()
{
   var _controller=this;
      
   var row = $('#'+_controller._grid_id).datagrid('getSelected');

   $('#'+_controller._grid_id).datagrid('unselectAll');
      
   if (row){
      $('#'+_controller._dlgbox_id).dialog('open').dialog('setTitle',_controller._toLocalC('updating')+' ...');
      console.log($('#'+_controller._submit_button_id));
      $('#'+_controller._submit_button_id).unbind();
      $('#'+_controller._submit_button_id).attr("href", "http://www.google.com/")
      $('#'+_controller._form_id).form('load',row);
      $('#'+_controller._submit_button_id).bind('click', function(e){ e.preventDefault(); _controller.__update(_controller,'edit')});
   }
};


GridController.prototype._view=function()
{
   var _controller=this;
      
   var row = $('#'+_controller._grid_id).datagrid('getSelected');

   $('#'+_controller._grid_id).datagrid('unselectAll');
      
   if (row){
      $('#'+_controller._dlgbox_id).dialog('open').dialog('setTitle',_controller._toLocalC('view')+' ...');
      $('#'+_controller._submit_button_id).unbind();
      $('#'+_controller._form_id).form('load',row);
   }
};


GridController.prototype._del=function()
{
   var _controller=this;
      
   var row = $('#'+_controller._grid_id).datagrid('getSelected');
      
   $('#'+_controller._grid_id).datagrid('unselectAll');

   if (row)
   {
      $.messager.confirm(_controller._toLocalC('deleting')+"...",_controller._toLocalC('Are you sure ?'),function(r)
      {
         if (r)
         {
            $.post(_controller._delDataSource,{id:row.id}, function(result) {
               if (!result.iserror) {
                  $('#'+_controller._grid_id).datagrid('reload'); // reload the user data
               }
               else {
                  $.messager.show({
                     title: _controller._toLocalC('error'),
                     msg: _controller._toLocalC("server side error")+' ('+result.errorMsg+' / errno='+result.errno+')'
                  });
               }
            },'json')
            .done(function() {
            })
            .fail(function(jqXHR, textStatus, errorThrown) {
               $.messager.show({
                  title:_controller._toLocalC('error')+_controller._localDoubleDot(),
                  msg: _controller._toLocalC("communication error")+' ('+textStatus+')'
               });
            })
            .always(function() {
            });
         }
      });
   }
};
   
GridController.prototype.view=function() // wrapper pour _add avec controle.
{
   var _controller=this;
   _controller.__auth(_controller,'_view');
   _controller.__b_disable();
};

GridController.prototype.add=function() // wrapper pour _add avec controle.
{
   var _controller=this;
   _controller.__auth(_controller,'_add');
   _controller.__b_disable();
};

GridController.prototype.del=function() // wrapper pour _del avec controle.
{
   var _controller=this;
   _controller.__auth(_controller,'_del');
   _controller.__b_disable();
};
   
GridController.prototype.edit=function() // wrapper pour _edit avec controle.
{
   var _controller=this;
   _controller.__auth(_controller,'_edit');
   _controller.__b_disable();
},
   
GridController.prototype.start=function()
{
   var _controller=this;

   $('#'+_controller._grid_id).datagrid({
      onDblClickRow: function(index, row) { // si double click on modifie
         // clear selection après double click
         if(window.getSelection)
            window.getSelection().removeAllRanges();
         else if(document.selection)
            document.selection.empty();
            
            _controller.edit();
      },

      onLoadSuccess: function(data){
         if(data.iserror)
         {
            $.messager.show({
               title: _controller._toLocalC('error'),
               msg: _controller._toLocalC("server side error")+' ('+data.errorMsg+' / errno='+data.errno+')'
            });
         }
      },

      onLoadError: function() {
         $.messager.show({
            title:_controller._toLocalC('error')+_controller._localDoubleDot(),
            msg: _controller._toLocalC("can't load data")+' ('+data.errorMsg+' / errno='+data.errno+')'
            });
      },
         
      onSelect: function(index,row) {
         if(_controller.onSelect(index,row))
         {
            _controller.__b_enable();
         }
      },

      onUnselect: _controller.__b_disable,
   });
   _controller.__animationInit();
};


// remplacement d'une valeur (0 / 1) par une coche si 1 ou rien (chaine vide) sinon (à mutualiser)
GridController.prototype.toCheckmark = function(val,row)
{
   if(val == 1) {
      return "<div class='icon-ok' style='margin:auto;height:16px;width:16px;'></div>";
   }
   else if(val == 2) {
      return "<div class='icon-redo' style='margin:auto;height:16px;width:16px;'></div>";
   }
   else{
      return  "";
   }
}


// pour la validation du format d'une chaine (à mutualiser dans une lib)
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


// pour les contrôles de surface "automatique" => à déplacer dans un autre fichier ...
$.extend($.fn.validatebox.defaults.rules, {
   name_validation: {
      validator: function(value, param){
         return checkRegexp( value, /^[0-9A-Za-z]{3,8}$/);
      },
      message: "3 à 8 caractères dans l'intervalle [a-zA-Z0-9]."
   },
   device_name_validation: {
      validator: function(value, param){
         return checkRegexp( value, /^[0-9A-Za-z]{3,16}$/);
      },
      message: "3 à 16 caractères de l'ensemble [a-zA-Z0-9]."
   },
   password_validation: {
      validator: function(value, param){
         var nb_num=0, nb_alpha=0, nb_other=0, i=0;
         for(i=0;i<value.length;i++)
         {
            var c=value.charAt(i);
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
      },
      message: "8 caractères minimum dont au moins un chiffre ou un caractère spécial"
   }
});

