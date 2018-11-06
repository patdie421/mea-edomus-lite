function TranslationController()
{
   this.localStrings = {};
}

TranslationController.prototype = {
   loadDictionaryFromJSON: function(file) {
      var _controller = this;
      var jqxhr = $.ajax({
         url: file,
         async: false,
         dataType: 'json',
         success: function (data) {
            if(Object.keys(data).length == 1 && data.error) {
                console.log("error: ",data.error);
            }
            else {
               _controller.localStrings = data;
            }
         },
         error: function(jqXHR, textStatus, errorThrown ) {
             console.log(textStatus+" ("+errorThrown+")");
           _controller.localStrings = {};
         }
      });
   },

   
   addFromObject: function(object) {
      this.localStrings=jQuery.extend(this.localStrings, object);
   },
   
   toLocal: function(string){
      var str=string.toLowerCase();
      if(this.localStrings[str])
         return this.localStrings[str];
      else
         return (str);
   },
   
   toLocalC: function(string){
      var str=this.toLocal(string);
      return str.charAt(0).toUpperCase() + str.slice(1);
   },

   localDoubleDot: function(){
      return ": ";
   },
    
   add: function(str,local){
      this.localStrings[str.toLowerCase()]=local.toLowerCase();
   },
    
   del: function(str){
      delete this.localStrings[str.toLowerCase()];
   },
};

