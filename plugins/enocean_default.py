# coding: utf8

import re
import string
import sys
import json

try:
   import mea
except:
   import mea_simulation as mea

import mea_utils
from mea_utils import verbose


def mea_init(data):
   fn_name=sys._getframe().f_code.co_name
   verbose(9,"INFO  (", fn_name, ") - mea_init")

   return True


def pairing_get_devices(data):
   fn_name=sys._getframe().f_code.co_name
   verbose(9,"INFO  (", fn_name, ") - pairing")

   result=data

   return result