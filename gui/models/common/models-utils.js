function check_field_exist(table,fieldName,fieldValue,id)
{
   var passes=-1;
   if(id!=-1)
      wsdata={table:table, field:fieldName, value:fieldValue, id:id};
   else
      wsdata={table:table, field:fieldName, value:fieldValue};
   $.ajax({ url: 'models/check_field.php',
            async: false,
            type: 'GET',
            dataType: 'json',
            data: wsdata,
            success: function(data){
               if(data.result=="OK")
               {
                  if(data.exist==0)
                     passes=false;
                  else
                     passes=true;
               }
               else
               {
                  // onError(data.error); // à remplacer
               }
            },
//            error: ajax_error // à remplacer
   });
    
   return passes;
}
