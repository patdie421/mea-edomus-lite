function TabsPageController(tabs_id)
{
   TabsPageController.superConstructor.call(this);
   this.tabs_id=tabs_id;
}

extendClass(TabsPageController, CommonController);

TabsPageController.prototype.start=function(tabName)
{
   var _tabsPageController = this;

   $('#'+_tabsPageController.tabs_id).tabs ({
      onSelect:function(tabName) {
         if(_tabsPageController._isLoggedIn()==false) {
            $.messager.alert(_tabsPageController._toLocalC('error')+_tabsPageController._localDoubleDot(),_tabsPageController._toLocalC('you are not logged in'),function(){ window.location = "login.php?view="+tabName;});
            return false;
         }
         var evnt = "activatetab_"+tabName;
         evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
         $(document).trigger( evnt, [ tabName, "" ] );

         return true;
      },
      onUnselect:function(tabName) {
         var evnt = "unactivatetab_"+tabName;
         evnt=evnt.replace(/[^a-zA-Z0-9]/g,'_');
         $(document).trigger( evnt, [ tabName, "" ] );
         return true;
      },
      selected: tabName
   });

   $('#'+_tabsPageController.tabs_id).tabs('select', tabName);
};
 
TabsPageController.prototype.selectTab=function(tabName) {
   var _tabsPageController = this;

   $('#'+_tabsPageController.tabs_id).tabs('select', tabName);
};

