import re
import string
import sys

####################################################################################################
#
# Initialisation d'une interface (en connexion direct, ie. etat = actif dans la declaration de
# l'interface) :
#
# PARAMETERS=<cmd1>[,<cmd2>]...[,<cmdx>]
# avec cmd* = @xx ou @xx:<value>
#   xx : n'importe quel comande AT (2 caracteres)
#   <value> :
#      - entier positif en base 10 (8 bits, 16 bits ou 32 bits)
#      - entier positif en base 16 (8 bits, 16 bits ou 32 bits). Doit commencer 
#        par 0x (ex : 0xFF ou 0x23F4 ou 0x3344FF11)
#      - chaine de carcateres
# exemple :
# PARAMETERS=@NI:toto
#
#
# Commissionnement d'une interface (pour les interfaces en statut "delegue"). Declenchement par le
# bouton commissionning xbee ou la commande AT correspondante.
#
# PARAMETERS=<cmd1>[,<cmd2>]...[,<cmdx>]
# avec cmd* = @xx ou @xx:<value> ou  #<command> ou #<command>:<command_parameter> avec
#   xx : n'importe quel comande AT (2 caracteres)
#   <valeur> :
#      - un entier positif en base 10 (8 bits, 16 bits ou 32 bits)
#      - un entier positif en base 16 (8 bits, 16 bits ou 32 bits). Doit commencer par 0x (ex : 0xFF
#        ou 0x23F4 ou 0x3344FF11)
#      - une chaine de carcateres
#      - l'une des variables internes suivantes :
#        * $local_xbee : adresse de l'interface de connexion (connectee directement via liaison 
#                        serie) au format xxxxxxxx-xxxxxxxx (ex : 0013a200-4078a4c9)
#        * $my_name    : nom de l'interface dans mea-edomus
#   <command> :
#     * d<n> : n = numero d'entree/sortie (0 a 8). Pour cette commande les valeurs possibles pour
#              <command_parameter> sont :
#          digital_in    : l'entree/sortie est utilisee en entree logique
#          digital_out_l : l'entree/sortie est utilisee en sortie positionnee au niveau bas (=0)
#          digital_out_h : l'entree/sortie est utilisee en sortie positionnee au niveau haut (=1)
#          analog_in     : l'entree/sortie est utilisee en entree analogique (pour les I/O 0 a 3
#                          uniquement)
#          default       : l'entree/sortie est utilisee avec sa configuration par defaut (voir
#                          documentation xbee)
#
#     * set_dhdl   : positionne dh et dl de l'interface sur l'adresse xbee specifiee.
#                    <command_parameter> doit contenir une adresse xbee au format xxxxxxxx-xxxxxxxx
#                    ou $local_xbee.
#
#     * set_sleep  : specifie la duree d' "endormissement" (voir ATST de la docummentation
#                    XBEE).
#                    <command_parameter> = valeur entiere de 0 a 2800.
#
#     * set_name   : positionne le nom de l'interface a la valeur specifiee par <command_parameter>
#                    (chaine de caracteres ou variable interne)
#
#     * set_sample : positionne l'interval d'echantillonnage des entrees (logiques ou analogiques).
#                    <command_parameter> = valeur entiere > 0
#
# exemple :
# PARAMETERS=#set_dhdl:$local_xbee,#d3:analog_in,#set_sample:5000,#set_name:$my_name,@WR
#
# remarque : des qu'une entree/sortie est positionnee en entree, l'interval d'echantillonnage est
#            fixee a 5 secondes. si vous souhaitez changer l'interval, la commande #set_sample doit
#            etre positionnee derriere la declaration de la derniere entree/sortie
#            exemple :
#               #d2:digital_in,#d3:analog_in,#set_sample:10000 => echantillonnage = 10 secondes
#               #d3:analog_in,#set_sample:7500,#d2:digital_in => echantillonnage = 5 secondes et non
#               7,5 secondes
#
# Entree/sortie
#
# PARAMETERS=pin:d<n>|a<n>[,type:<type>][,compute:<fn>][,log:yes|no]
# avec D<n> : entree ou sortie logique associee au capteur/actionneur mea_edomus (n de 0 a 8)
#      A<n> : entree analogique associee au capteur/actionneur mea_edomus (n de 0 a 3)
#      type : type qui sera transmis dans le body du message xpl. si pas precise, type=generic
#      compute : transforme la valeur retournee. fn est une expression arithmetique (syntaxe python)
#                avec x representant la valeur mesuree.
#                ex : compute:sin(x)+x
#      log     : yes la valeur sera transmise pour archivage a la base de donnees mysql
#
# exemple :
#    PARAMETERS=pin:d1,type=outpout
#    PARAMETERS=pin:a1,compute:x*2,log:yes
#

try:
   import mea
except:
   import mea_simulation as mea

from sets import Set

import mea_utils
from mea_utils import verbose


def getInternalVarValue(data, var):
   if var=="$local_xbee":
      local_xbee="%0*x" % (8,data["ADDR_H"]) + "-" + "%0*x" % (8, data["ADDR_L"])
      return (-1,local_xbee)
   elif var=="$my_name":
      return (-1, data["interface_name"])
   else:
      return (-1, "")


def config_IO(data,pin,val,mem):
   if len(pin)!=3:
      verbose(5, "WARNING (", fn_name, ") - invalid pin :", pin[1:], "not not allowed, skipped")
      return -1
   try:
      pinNum=int(pin[2:3])
      if(pinNum>8):
         verbose(5, "WARNING (", fn_name, ") - invalid pin :", pin[1:3], "not not allowed, skipped")
         return -1
   except:
      verbose(5, "WARNING (", fn_name, ") - invalid pin :", pin[1:3], "not not allowed, skipped")
      return -1
   
   if val=="digital_in":
      atVal=3
   elif val=="digital_out_l":
      atVal=4
   elif val=="digital_out_h":
      atVal=5
   elif val=="default":
      atVal=1
   elif val=="analog_in":
      if pinNum==0 or pinNum==1 or pinNum==2 or pinNum==3:
         atVal=2
      else:
         verbose(5, "WARNING (", fn_name, ") - invalid pin :", pin[1:3], "not not allowed for analog input, skipped")
         return -1
   elif val=="none":
      atVal=0
   else:
      verbose(5, "WARNING (", fn_name, ") - syntaxe error :", i[0], "not an at cmnd, skipped")
      return -1
   
   ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], pin[1:3].upper(), atVal);
   
   if ret == True:
      mem[pin[1:3]]=atVal
   return ret


def config_DHDL(data,alphaVal):
   vals=alphaVal.split("-")
   if len(vals) != 2:
      verbose(5, "WARNING (", fn_name, ") - syntaxe error :", alphaVal, "not an xbee address, skipped")
      return -1
   try:
      h=int(vals[0],16)
      l=int(vals[1],16)
   except:
      verbose(5, "WARNING (", fn_name, ") - syntaxe error :", alphaVal, "not an xbee address, skipped")
      return -1
   
   ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "DH", h)
   ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "DL", l)
   
   return ret


def config_sleep(data, val):
   if val>=0 and val <= 2800:
      # mettre aussi a jour les autres registres
      ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "ST", val)
      return ret
   else:
      return -1


def config_name(data, alphaVal):
   if len(alphaVal)>1:
      ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "NI", alphaVal)
      return ret
   else:
      return -1


def config_sample(data,val):
   if val>=0:
      ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "IR", val)
      return ret
   else:
      return -1


def enable_change_detection(data,mem):
   # a ecrire
   # Bit (IO pin):
   # 0 (DIO0) 4 (DIO4)  8  (DIO8)
   # 1 (DIO1) 5 (DIO5)  9  (DIO9)
   # 2 (DIO2) 6 (DIO6) 10 (DIO10)
   # 3 (DIO3) 7 (DIO7) 11 (DIO11)
   #
   # activer IO Digital Change Detection (ATIC)
   
   mask=0
   for i in mem:
      if i[0:1]=="d":
         if mem[i]==3:
            v=int(i[1:2])
            if v<=8:
               bit=1<<v
               mask=mask+bit
   
   ret=0
   if mask != 0:
      ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "IC", mask)
   return ret


def enable_sample(data,mem):
   analog_in_enabled=False
   for i in mem:
      if i[0:1]=="d":
         if mem[i]==2:
            analog_in_enabled=True
            break
   if analog_in_enabled == True:
      ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "IR", 5*1000)
   else:
      return -1
   return ret


def parseIOData(cmd_data, l_cmd_data):
   io_data={}
   xplMsg={}
   
   digital_mask = cmd_data[2]
   analog_mask = cmd_data[3]
   digital_states = cmd_data[5]
   
   for i in range(0,7):
      if digital_mask & 0x01:
         key="d"+str(i)
         io_data[key]=digital_states & 0x01
      digital_mask=digital_mask>>1
      digital_states=digital_states>>1
   
   j=6
   for i in range(0,3):
      if analog_mask & 0x01:
         key="d"+str(i)
         io_data[key]=cmd_data[j]*256+cmd_data[j+1]
         j=j+2
      analog_mask=analog_mask>>1
   return io_data


def mea_xplCmndMsg(data):
   fn_name=sys._getframe().f_code.co_name
   try:
      id_sensor=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return 0
   
   mem=mea.getMemory(id_sensor)
  
   paramsDict=mea_utils.parseKeyValueDatasToDictionary(data["device_parameters"], ",", ":")
   
   if "pin" in paramsDict:
      pin=paramsDict["pin"].lower()
   else:
      return False
   
   if "sensor_type" in paramsDict:
      type=paramsDict["sensor_type"]
   else:
      type="generic"
   
   x=data["xplmsg"]
   body=x["body"]

   target="*"
   if "source" in x:
      target=x["source"]

   if x["schema"]=="sensor.request":
      current_key=pin+"_current"
      last_key=pin+"_last"
      if body["type"]==type:
         try:
            xplMsg=mea_utils.xplMsgNew("me", target, "xpl-stat", "sensor", "basic")
            mea_utils.xplMsgAddValue(xplMsg,"device", data["device_name"].lower())
            if body["request"]=="current":
               mea_utils.xplMsgAddValue(xplMsg,"current",mem[current_key])
            elif body["request"]=="last":
               mea_utils.xplMsgAddValue(xplMsg,"last",mem[last_key])
            else:
               verbose(2, "ERROR (", fn_name, ") - invalid request (", body["request"],")")
               return False
            mea_utils.xplMsgAddValue(xplMsg,"type",type)
         except:
            verbose(2, "ERROR (", fn_name, ") - can't create xpl message")
            return False
      else:
         verbose(2, "ERROR (", fn_name, ") - invalid type (", body["type"],")")
         return False

      mea.xplSendMsg(xplMsg)
      return True

   elif x["schema"]=="control.basic":
      if body["type"]=="output":
         if body["current"].lower()=="low":
            mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], pin, 4);
            mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "AC", "");
            return True
         elif body["current"].lower()=="high":
            mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], pin, 5);
            mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], "AC", "");
            return True
         else:
            return False
      else:
         verbose(2, "ERROR (", fn_name, ") - invalid request (", body["request"],")")
         return False
   else:
      verbose(2, "ERROR (", fn_name, ") - invalid schema (", x["schema"],")")
      return False


def mea_dataFromSensor(data):
   fn_name=sys._getframe().f_code.co_name
   xplMsg = {}
   
   try:
      id_sensor=data["device_id"]
      cmd_data=data["cmd_data"]
      l_cmd_data=data["l_cmd_data"]
      data_type=data["data_type"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False
   
   mem=mea.getMemory(id_sensor)
   
   if data_type==0x92:
      mem["iodata"]=parseIOData(cmd_data, l_cmd_data)
      
      paramsDict=mea_utils.parseKeyValueDatasToDictionary(data["device_parameters"], ",", ":")
      
      if "pin" in paramsDict:
         pin=paramsDict["pin"]
      else:
         return False
      
      if "sensor_type" in paramsDict:
         type=paramsDict["sensor_type"]
      else:
         type="generic"
      
      current_key=pin+"_current"
      last_key=pin+"_last"
      
      if pin in mem["iodata"].lower():
         val=mem["iodata"][pin]
         last_val=0
         
         try:
            mem[last_key]=mem[current_key]
         except:
            mem[last_key]=0
         
         strVal=""
         if pin[0]=="d":
            if val==1:
               mem[current_key]="high"
            else:
               mem[current_key]="low"
            logval=val
         else:
            if "compute" in paramsDict:
               x=val
               y=eval(paramsDict["compute"])
               logval=round(y,2)
            else:
               logval=val
            mem[current_key]=logval

         if mem[current_key] != mem[last_key]:
            if "log" in paramsDict and paramsDict["log"].lower()=="yes":
               if "unit" in paramsDict:
                  unit=int(paramsDict["unit"])
               else:
                  unit=0
               mea.addDataToSensorsValuesTable(id_sensor,logval,unit,val,"")

            xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
            mea_utils.xplMsgAddValue(xplMsg,"device", data["device_name"].lower())
            mea_utils.xplMsgAddValue(xplMsg,"current", mem[current_key])
            mea_utils.xplMsgAddValue(xplMsg,"type", type)
            mea_utils.xplMsgAddValue(xplMsg,"last",mem[last_key])
            mea.xplSendMsg(xplMsg)

            return True

   return False


def mea_commissionningRequest(data):
   fn_name=sys._getframe().f_code.co_name
   
   if "interface_parameters" in data:
      mem={}
      try:
         mem=mea.getMemory(data["interface_id"])
      except:
         verbose(2, "ERROR (", fn_name, ") - can't acces share memory")
         return False
      
      sample_set=False
      paramsList=mea_utils.parseKeyValueDatasToList(data["interface_parameters"], ",", ":")
      if paramsList != None:
         for i in paramsList:
            numVal=-1;
            alphaVal="";
            if i[0][0] != "#" and i[0][0] !="@":
               verbose (3, "ERROR (", fn_name, ") - syntaxe error :", i[0], "not a command, skipped")
               continue
            
            if i[1]!=None:
               if i[1][0:2]=="0x":
                  try:
                     numVal=int(i[1],16)
                  except:
                     alphaVal=i[1]
               elif i[1][0]=="$":
                  (numVal,alphaVal)=getInternalVarValue(data, i[1])
               else:
                  try:
                     numVal=int(i[1],10)
                  except:
                     alphaVal=i[1]
               # commande AT ?
               if i[0][0] == "@":
                  if len(i[0])==3:
                     if numVal>=0:
                        ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], i[0][1:3].upper(), numVal);
                        if ret == 0:
                           verbose(5, "WARNING (", fn_name, ") - Transmission error for", i[0], "= ", numVal)
                     else:
                        ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], i[0][1:3].upper(), alphaVal);
                        if ret == 0:
                           verbose(5, "WARNING (", fn_name, ") - Transmission error for", i[0], "= ", alphaVal)
                  else:
                     verbose(9, "WARNING (", fn_name, ") - syntaxe error :", i[0], "not an at cmnd, skip")
                  continue
               
               # commande special ?
               if i[0][0:2]=="#d":
                  ret=config_IO(data, i[0], alphaVal, mem)
                  continue
               
               if i[0] == "#set_dhdl":
                  ret=config_DHDL(data,alphaVal)
                  if ret==-1:
                     verbose(5, "WARNING (", fn_name, ") - value error :", i[1], "not an xbee adress, skip")
                  continue
               
               if i[0] == "#set_sleep":
                  ret=config_sleep(data,numVal)
                  continue
               
               if i[0] == "#set_name":
                  ret=config_name(data, alphaVal)
                  if ret==-1:
                     verbose(5, "WARNING (", fn_name, ") - value error :", i[1], "not an xbee adress, skip")
                  continue
               
               
               if i[0] == "#set_sample":
                  sample_set=True
                  config_sample(data,numVal)
                  continue
            
            else:
               if len(i[0])==3:
                  ret=mea.sendXbeeCmd(data["ID_XBEE"], data["ADDR_H"], data["ADDR_L"], i[0][1:3].upper(), "");
                  if ret == 0:
                     verbose(3, "ERROR (", fn_name, ") - Transmission error for", i[0], "= ", numVal)
         enable_change_detection(data,mem)
         if sample_set==False:
            enable_sample(data,mem)
         return True
      
      else:
         return False
   else:
      return False


def mea_init(data):
   fn_name=sys._getframe().f_code.co_name
   
   verbose(9,"INFO  (",fn_name,") - lancement mea_init")
   
   if "interface_parameters" in data:
      mem={}
      try:
         mem=mea.getMemory(data["interface_id"])
      except:
         verbose(2, "ERROR (", fn_name, ") - can't acces share memory")
         return False
      
      paramsList=mea_utils.parseKeyValueDatasToList(data["interface_parameters"], ",", ":")
      if paramsList != None:
         for i in paramsList:
            numVal=-1;
            alphaVal="";
            
            if i[0][0] !="@":
               verbose (3, "ERROR (", fn_name, ") - syntaxe error :", i[0], "not an AT command, skipped")
               continue
            
            if i[1]!=None:
               if i[1][0:2]=="0x":
                  try:
                     numVal=int(i[1],16)
                  except:
                     alphaVal=i[1]
               else:
                  try:
                     numVal=int(i[1],10)
                  except:
                     alphaVal=i[1]
               # commande AT ?
               if len(i[0])==3:
                  if numVal>=0: # commande at avec parametre numerique
                     ret=mea.sendXbeeCmd(data["ID_XBEE"], -1, -1, i[0][1:3].upper(), numVal);
                     if ret == 0:
                        verbose(5, "WARNING (", fn_name, ") - Transmission error for", i[0], "= ", numVal)
                  else: # commande at avec parametre alpha-numerique
                     ret=mea.sendXbeeCmd(data["ID_XBEE"], -1, -1, i[0][1:3].upper(), alphaVal);
                     if ret == 0:
                        verbose(5, "WARNING (", fn_name, ") - Transmission error for", i[0], "= ", alphaVal)
               else:
                  verbose(5, "WARNING (", fn_name, ") - syntaxe error :", i[0], "not an at cmnd, skip")
            else:
               if len(i[0])==3: # commande at sans parametre
                  ret=mea.sendXbeeCmd(data["ID_XBEE"], -1, -1, i[0][1:3].upper(), "");
                  if ret == 0:
                     verbose(3, "ERROR (", fn_name, ") - Transmission error for", i[0], "= ", numVal)
         return True
      
      else:
         return False
   else:
      return False
