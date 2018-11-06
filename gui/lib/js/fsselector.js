var fsselectorIsInit = false;
var fsselectorTheme = "default";
var fsselectorRandomID = "#fsselector_" + Math.floor((Math.random() * 100000) + 1);


function FSSelector(datasource)
{
    this.datasource = datasource;
    this.selectionButtonText="Select";
    this.cancelButtonText="Cancel";
    this.theme="default";
}


function FSSelectorSetTheme(theme)
{
   if( (theme != fsselectorTheme) || (fsselectorIsInit==false) )
   {
      fsselectorTheme = theme;
      defineIcons(theme);
      fsselectorIsInit=true;
   }
}


function defineIcons(theme)
{
   $(fsselectorRandomID).remove();
   $("<style id="+fsselectorRandomID+">")
      .prop("type", "text/css")
      .html("\
         .icon-rep {\
            background:url('lib/jquery-easyui-1.4.1/themes/"+theme+"/images/tree_icons.png') no-repeat -208px 0;\
         }"
   ).appendTo("head");
}


FSSelector.prototype.setButtonsTexts=function(selectStr,cancelStr)
{
    this.selectionButtonText=selectStr;
    this.cancelButtonText=cancelStr;
};


FSSelector.prototype.directory = function (title, width, height, field) {
    _fsselector = this;

    FSSelectorSetTheme(_fsselector.theme);
    
    id = "dlg_"+Math.floor((Math.random() * 10000) + 1);
    html = "<div id='"+id+"'> \
                  <div style='border:dotted 1px lightgray;margin:auto;width:"+ (width - 16) +"px;height:" + (height-76) + "px;overflow:auto'> \
                     <ul id='tt_"+id+"'></ul> \
                  </div> \
            </div>";
    $('body').append(html);
    
    $('#'+id).dialog({
       title: title,
       width: width,
       height: height,
       closed: false,
       cache: false,
       modal: true,
       buttons:[{
               text:_fsselector.selectionButtonText,
                  handler:function(){
                      node=$('#tt_'+id).tree('getSelected');
                      if(node != null)
                          $("#"+field).textbox("setText", node.id);
                      $('#'+id).dialog("close");
                      $('#'+id).remove();
                  }
               },{
               text:_fsselector.cancelButtonText,
                  handler:function(){
                      $('#'+id).dialog("close");
                      $('#'+id).remove();
                  }
       }]
    });

    $('#'+id).dialog("open");

    $('#tt_'+id).tree({
       url:_fsselector.datasource,
       queryParams: { type: 'directory' },
    });

    $('#tt_'+id).tree({
       onDblClick: function (node) {
          $("#"+field).textbox("setText", node.id);
          $('#'+id).dialog("close");
          $('#'+id).remove();
       }
   });
};


FSSelectorSetTheme("default");
