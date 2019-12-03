import re

try:
    import mea
except:
    import mea_simulation as mea

import mea_utils
from mea_utils import verbose

from sets import Set
import string

debug=0


def mea_xplCmndMsg(data):
    
    try:
        x=data["xplmsg"]
        body=x["body"]
    except:
        return False

    try:
        if x["schema"]!="sensor.request":
           return False
    except:
        return False

    try:
        id_sensor=data["device_id"]
        id_type=data["device_type_id"]
    except:
        verbose(2, "ERROR - (mea_xplCmndMsg) : data not found")
        return False

    target="*"
    if "source" in x:
        target=x["source"]

    mem=mea.getMemory(id_sensor)

    if body["request"]=="current":
        try:
            if id_type==2000:
                type="humidity"
                current=mem["current_h"]
            elif id_type==2001:
                type="temperature"
                current=mem["current_t"]
            elif id_type==2002:
                type="voltage"
                current=mem["current_p"]
            else:
                return False
        except:
            return False

        xplMsg=mea_utils.xplMsgNew("me", target, "xpl-stat", "sensor", "basic")
        mea_utils.xplMsgAddValue(xplMsg,"device", data["device_name"])
        mea_utils.xplMsgAddValue(xplMsg,"current",current)
        mea_utils.xplMsgAddValue(xplMsg,"type",type)
        mea.xplSendMsg(xplMsg)
        return True

    elif body["request"]=="last":
        try:
            if id_type==2000:
                type="humidity"
                last=mem["last_h"]
            elif id_type==2001:
                type="temperature"
                last=mem["last_t"]
            elif id_type==2002:
                type="voltage"
                last=mem["last_p"]
            else:
                return False
        except:
            return False

        xplMsg=mea_utils.xplMsgNew("me", target, "xpl-stat", "sensor", "basic")
        mea_utils.xplMsgAddValue(xplMsg,"device", data["device_name"])
        mea_utils.xplMsgAddValue(xplMsg,"last",last)
        mea_utils.xplMsgAddValue(xplMsg,"type",type)
        mea.xplSendMsg(xplMsg)
        return True

    else:
        return False


def mea_dataFromSensor(data):
    xplMsg = {}
    try:
        id_type=data["device_type_id"]
        id_sensor=data["device_id"]
        cmd=data["cmd"]
    except:
        verbose(2, "ERROR - (mea_dataFromSensor) : data not found")
        return False

    verbose(9, "INFO  (mea_dataFromSensor) : data from ", id_sensor)

    mem=mea.getMemory(id_sensor)

    if debug == 1:
        verbose(9, "INFO - (mea_dataFromSensor) : ", mem)

    # voir http://www.tutorialspoint.com/python/python_reg_expressions.htm pour match
    x=re.match("^\s*<(.*)>\s*$",cmd[12:-2])
    if(x!=None):
        t=x.group(1)
        list=t.split(';')
    else:
        verbose(5, "WARNING - (mea_dataFromSensor) : data error (", cmd[12:-2], ")")
        return False

    for i in list:
        l2=i.split('=')
        
        # traitement Humidite
        if id_type == 2000 and l2[0] == 'H':
            humidite = float(l2[1])
            last_h=0
            try:
                last_h=mem["current_h"]
                mem["last_h"]=mem["current_h"]
            except:
                pass
            mem["current_h"]=humidite
            verbose(9, "INFO  (mea_dataFromSensor) : humidite =", humidite, "%")
        
            if last_h != humidite:
                current=humidite
                type="humidity"
                unit=5
                last=last_h
            else:
                return True

        # traitement temperature
        elif id_type == 2001 and l2[0] == 'T':
            temperature = float(l2[1])
            last_t=0
            try:
                last_t=mem["current_t"]
                mem["last_t"]=mem["current_t"]
            except:
                pass
            mem["current_t"]=temperature
            verbose(9, "INFO  (mea_dataFromSensor) : temperature =", temperature, "C")

            if last_t != temperature:
                current=temperature
                unit=3
                type="temp"
                last=last_t
            else:
               return True

        # traitement niveau piles
        elif id_type == 2002 and l2[0] == 'P':
            pile = float(l2[1])
            last_p=0
            try:
                last_p=mem["current_p"]
                mem["last_p"]=mem["current_p"]
            except:
                pass
            mem["current_p"]=pile
            verbose(9, "INFO  (mea_dataFromSensor) : Pile =", pile, "V")

            if last_p != pile:
                current=pile
                unit=4
                type="volt"
                last=last_p
            else:
                return True
            
        else:
            continue

        mea.addDataToSensorsValuesTable(id_sensor,current,unit,0,"")
        
        xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
        mea_utils.xplMsgAddValue(xplMsg,"device", data["device_name"])
        mea_utils.xplMsgAddValue(xplMsg,"current",current)
        mea_utils.xplMsgAddValue(xplMsg,"type",type)
        mea_utils.xplMsgAddValue(xplMsg,"last",last)
        mea.xplSendMsg(xplMsg)

    return True
