extendClass(OrderedFilesListSelector, CommonController);

function OrderedFilesListSelector(toSelect, selected, button_in, button_out, button_up, button_down, type, callback)
{
   this.toSelect = toSelect;
   this.selected = selected;
   this.button_in = button_in;
   this.button_out = button_out;
   this.button_up = button_up;
   this.button_down = button_down;
   this.type = type;
   this.callback = callback;

   _this = this;

   $('#'+_this.toSelect).datalist({
      valueField: 'f1',
      textField:  'f2',
      line: false,
      singleSelect: false,
      ctrlSelect: true,
      onDblClickRow: function(index, field) {
         var fields = [];
         fields.push(field);
         toSelected(fields);
         if(window.getSelection)
            window.getSelection().removeAllRanges();
         else if(document.selection)
            document.selection.empty();
      },
      onClickRow: function(index, field) {
      }
   });

   $('#'+_this.selected).datalist({
      valueField: 'f1',
      textField:  'f2',
      line: false,
      singleSelect: true,
      onDblClickRow: function(index, field) {
         if(window.getSelection)
            window.getSelection().removeAllRanges();
         else if(document.selection)
            document.selection.empty();

      },
      onClickRow: function(index, field) {
      }
   });
 
   _this._getFilesList(function() {}, function() {});

   function toSelected(fields)
   {
      var data_s = $('#'+_this.toSelect).datalist('getData')["rows"];
      var data_d = $('#'+_this.selected).datalist('getData')["rows"];

      $.each(fields, function(i, val) {
         data_d.push(val);
         var index = data_s.indexOf(val);
         if(index>=0)
            data_s.splice(index, 1);
      });

      $('#'+_this.toSelect).datalist({data:data_s});
      $('#'+_this.selected).datalist({data:data_d});

      if(_this.callback)
         _this.callback(data_d, data_s);
   }

   $('#'+_this.button_in).click(function() {
      var sels = $('#'+_this.toSelect).datalist('getSelections');
      if(sels!="")
         toSelected(sels);
   });


   $('#'+_this.button_out).click(function() {
      var sels = $('#'+_this.selected).datalist('getSelections');
      if(sels!="")
      {
         var data_s = $('#'+_this.selected).datalist('getData')["rows"];
         var data_d = $('#'+_this.toSelect).datalist('getData')["rows"];

         $.each(sels, function(i, val) {
            data_d.push(val);
            var index = data_s.indexOf(val);
            if(index>=0)
               data_s.splice(index, 1);
         });

         $('#'+_this.toSelect).datalist({data:data_d});
         $('#'+_this.selected).datalist({data:data_s});

         if(_this.callback)
            _this.callback(data_s, data_d);
      }
   });


   $('#'+_this.button_up).click(function() {
      var sel = $('#'+_this.selected).datalist('getSelected');
      if(sel)
      {
         var data = $('#'+_this.selected).datalist('getData')["rows"]; 
         var index = data.indexOf(sel);
         if(index>0)
         {
            var tmp = data[index];
            data[index]=data[index-1];
            data[index-1]=tmp;
            $('#'+_this.selected).datalist({data:data});
            $('#'+_this.selected).datalist('selectRow',index-1);
         }
      }
   });


   $('#'+_this.button_down).click(function() {
      var sel = $('#'+_this.selected).datalist('getSelected');
      if(sel)
      {
         var data = $('#'+_this.selected).datalist('getData')["rows"];
         var index = data.indexOf(sel);
         if(index<data.length-1)
         {
            var tmp = data[index];
            data[index]=data[index+1];
            data[index+1]=tmp;
            $('#'+_this.selected).datalist({data:data});
            $('#'+_this.selected).datalist('selectRow',index+1);
         }
      }
   });
}


OrderedFilesListSelector.prototype.get = function()
{
   var _this = this;
   var data=[];

   var selected = $('#'+_this.selected).datalist('getData')["rows"];
   if(selected.length<=0)
      return false;

   $.each(selected, function(i, val) {
      data.push(val["f2"]);
   });

   return data;
}


OrderedFilesListSelector.prototype._getFilesList = function(on_success, on_error)
{
   var _this = this;

   on_success = typeof on_success !== 'undefined' ? on_success : false;
   on_error   = typeof on_error   !== 'undefined' ? on_error   : false;

   $.get("models/get_files_list.php", { type: _this.type }, function(response) {
      if(response.iserror === false)
      {
         var data = [];
         for(var i in response.values)
            data.push({f1: i, f2:response.values[i].slice(0, -(_this.type.length+1))});

         $('#'+_this.toSelect).datalist({data: data});

         if(on_success !== false)
            on_success();
      }
      else
      {
         if(on_error !== false)
            on_error();
      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
};


OrderedFilesListSelector.prototype.reload = function()
{
   var _this = this;
   var _type = _this.type;

   $.get("models/get_files_list.php", { type: _type }, function(response) {

      var data_d = $('#'+_this.selected).datalist('getData')["rows"];
      var data_s = [];
      if(response.iserror === false)
      {
         $.each(data_d, function(i, val) {
            val["f1"]=-1;
         });

         for(var i in response.values)
         {
            var name = response.values[i].slice(0, -(_type.length+1));
            found = false;
            for(j in data_d)
            {
               if(data_d[j]["f2"]==name)
               {
                  data_d[j]["f1"]=i;
                  found = true;
                  break;
               }
            }
            if(found===false)
               data_s.push({f1: i, f2: name});
         }

         for(var i=0;i<data_d.length;)
         {
            if(data_d[i]["f1"]==-1)
               data_d.splice(i, 1);
            else
               i++;
         }
         $('#'+_this.toSelect).datalist({data:data_s});
         $('#'+_this.selected).datalist({data:data_d});
         if(_this.callback)
            _this.callback(data_d, data_s);

      }
   }).done(function() {
   }).fail(function(jqXHR, textStatus, errorThrown) {
      $.messager.show({
         title:_this._toLocalC('error')+_this._localDoubleDot(),
         msg: _this._toLocalC("communication error")+' ('+textStatus+')'
      });
   });
}


OrderedFilesListSelector.prototype.del = function(name, type, checkflag)
{
   var _this = this;

   var _del = function() {
      $.post("models/del_file.php", { name: name, type: type }, function(response) {

         if(response.iserror === false)
         {
         }
         else
         {
            $.messager.alert(_this._toLocalC('error'),
                             _this._toLocalC("can't delete "+name)+_this._localDoubleDot()+response.errMsg,
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

    var ok = $.messager.defaults.ok;
    var cancel = $.messager.defaults.cancel;
    $.messager.defaults.ok = _this._toLocalC('Yes');
    $.messager.defaults.cancel = _this._toLocalC('No');
    $.messager.confirm(_this._toLocalC('deleting')+' "'+type+'" ...',_this._toLocalC('are you sure you want to delete')+' "'+name+'"'+_this._localDoubleDot(), function(r)
    {
       if(r)
          _del(name);
    });
    $.messager.defaults.ok = ok;
    $.messager.defaults.cancel = cancel;
}


OrderedFilesListSelector.prototype.set = function(data, setCallback, userdata)
{
   var _this = this;

   __set = function(data, setCallback, userdata)
   {
      var data_s = $('#'+_this.toSelect).datalist('getData')["rows"];
      var data_d = [];

      var notfound = [];
      for (var i in data)
      {
         var found = false;

         $.each(data_s, function(_i, val) {
            if(val["f2"] === data[i])
            {
               var index = data_s.indexOf(val);
                  if(index>=0)
               data_s.splice(index, 1);
               data_d.push(val);
               found = true;
               return false;
               }
            });

         if (found===false)
            notfound.push(data[i]);

      }
      $('#'+_this.toSelect).datalist({data:data_s});
      $('#'+_this.selected).datalist({data:data_d});

      if(_this.callback)
         _this.callback(data_d, data_s);

      if(notfound.length != 0)
         $.messager.alert(_this._toLocalC('error'), _this._toLocalC('\''+type+'\' file not found')+_this._localDoubleDot()+JSON.stringify(notfound), 'error');

      if(setCallback)
         setCallback(name, data_d, userdata);

      $.messager.progress('close');
   }

   $.messager.progress();

   _this._getFilesList(function() { __set(data, setCallback, userdata); }, function() { alert("error"); } );
};
