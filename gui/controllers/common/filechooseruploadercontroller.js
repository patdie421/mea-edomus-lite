function FileChooserUploaderController(attachement)
{
   FileChooserUploaderController.superConstructor.call(this);

   this.attachement = attachement; 
   this.id = "dlg_"+Math.floor((Math.random() * 10000) + 1);
   this.files = false;
   this.showext = true;
}

extendClass(FileChooserUploaderController, FileChooserController);

//                     <select name='"+id+"_selectfiles' id='"+id+"_selectfiles' size='12' style=\"width:305px;font-family:verdana,helvetica,arial,sans-serif;font-size:12px;\"></select> \
FileChooserUploaderController.prototype._getHtml = function(max_x, max_y)
{
   html="<div id='"+id+"' style=\"padding:10px 20px \"> \
            <iframe id='"+id+"_iframe' name='"+id+"_iframe' height='0' width='0' frameborder='0'></iframe> \
            <div style='display:table;margin: 0 auto;'> \
            <div id='"+id+"_title' class='ftitle'></div> \
\
               <form id='"+id+"_fm_ul' action='/models/upload_file.php' method='post' enctype='multipart/form-data'  target='"+id+"_iframe'> \
                  <div style='width:100%'> \
                     <div id='"+id+"_wrapper' style='float:left;padding-top:10px;padding-bottom:10px;'> \
                     </div> \
                     <div style='float:right;padding-top:10px;padding-bottom:10px;'> \
                        <a id='"+id+"_button_ul' href='#' style='height:25px;width:80px' class='easyui-linkbutton'>upload</a> \
                     </div> \
                  </div> \
               </form> \
\
               <form id='"+id+"_fm' method='post' data-options=\"novalidate:false\"> \
                  <div class='fitem'> \
                     <div style='width:100%;height:175px' id='"+id+"_files'></div> \
                  </div> \
                  <div class='fitem' style='padding-top:10px;'> \
                     <input id='"+id+"_filename' style='width:100%;'> \
                  </div> \
               </form> \
\
            </div> \
         </div>";

   return html;
};


FileChooserUploaderController.prototype._loadDialog = function(id, _type, response)
{
   var _this = this;

   var input = "<input id='"+id+"_file' class='easyui-filebox' style='width:205px;height:25px' name='file' data-options=\"prompt:'file to upload ...'\" style='width:auto'>";
   var html = input;

   FileChooserController.prototype._loadDialog.call(_this, id, _type, response);

   $("#"+id+'_wrapper').html(html);
   $("#"+id+'_file').filebox({ buttonText: 'choose' });
   $("#"+id+'_button_ul').linkbutton();
   $("#"+id+'_button_ul').bind('click', function(){ 
      var win = $.messager.progress({
         title: 'Please waiting',
         msg:   'Loading data...',
         text:  '',
         interval: 50
      });
      $("#"+id+'_fm_ul').submit();
   });

   setTimeout(function() {
      $("#"+id+'_iframe').load(function() {
         setTimeout(function() {
            $.messager.progress('close');
            try {
               var resp = JSON.parse(document.getElementById(id+"_iframe").contentWindow.document.body.innerHTML);
               if(resp.iserror === true)
               {
                  $.messager.alert("error",resp.errMsg,'error');
               }
               else
               {
                  $.get("models/get_files_list.php", { type: _type }, function(response) {
                     FileChooserController.prototype._loadDialog.call(_this, id, _type, response);
                  });
                  $.messager.show({
                     title: "info",
                     msg: "upload done"
                  });

                  $("#"+id+"_filename").textbox('setValue', resp.filename);                  
               }
            }
            catch(e) {}
         },
         500);
      });
   },
   10);
};
