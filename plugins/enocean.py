import re
import string
import sys

try:
    import mea
except:
    import mea_simulation as mea

import mea_utils
from mea_utils import verbose

from sets import Set
import string


def mea_pairingInterface(data):
   fn_name=sys._getframe().f_code.co_name

   return None


def mea_pairingDevices(data):
   fn_name=sys._getframe().f_code.co_name
   try:
      rorg=int(data["_pairing_data"]["RORG"])
      func=int(data["_pairing_data"]["FUNC"])
      type=int(data["_pairing_data"]["TYPE"])
      interface_name=data["_pairing_data"]["interface_name"]
      addr=data["_pairing_data"]["addr"]
      name=data["name"]

   except:
      verbose(2, "ERROR (", fn_name, ") - enocean data error")
      return {}

   if rorg==246:
      devices=[
         {
            "name" : str(interface_name)+str(addr)+"S01",
            "id_type": 0,
            "description": "enocean f6 input 1",
            "parameters": "PLUGIN=enocean_f6-02-01;PLUGIN_PARAMETERS=channel:A",
            "state": 1
         },
         {
            "name": str(interface_name)+str(addr)+"S02",
            "id_type": 0,
            "description": "enocean f6 input 2",
            "parameters": "PLUGIN=enocean_f6-02-01;PLUGIN_PARAMETERS=channel:B",
            "state": 1
         }
      ]
      return devices

   return None
