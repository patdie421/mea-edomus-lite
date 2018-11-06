function ViewsController()
{
   this.currentPage="";
   this.views={};
   this.pageChangeCallbacks={};
}


ViewsController.prototype = {
   addView: function(viewname, url, tabname) {
      this.views[viewname]={url:url, tabname:tabname};
   },
   
   displayView: function(viewname) {
      var _this = this;
      if(this.currentPage!=this.views[viewname].url)
      {
         if(this.currentPage in this.pageChangeCallbacks)
            _this.pageChangeCallbacks[this.currentPage]();

         $('#content').panel('refresh',this.views[viewname].url+"?view="+viewname.mea_hexEncode()); // on charge la page
         this.currentPage = this.views[viewname].url;
      }
      else if(this.views[viewname].tabname!="")
      {
         $('#'+this.views[viewname].tabname).tabs('select', viewname);
      }
   },

   addPageChangeCallback: function(page, cb)
   {
      var _this = this;

      _this.pageChangeCallbacks[page]=cb;
   },

   removePageChangeCallback: function(page)
   {
      var _this = this;

      if(page in this.pageChangeCallbacks)
         delete _this.pageChangeCallbacks[page];
   }
};
