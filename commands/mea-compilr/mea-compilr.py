#!/usr/bin/env python
# coding: utf8

import os
import re
#import string
import sys
import json
from optparse import OptionParser

def isname(str):
   if not str[0].isalpha():
      return False
   if not str[1:].isalnum():
      return False
   return True


def optimize_boolean(b,o):
   if o!=True:
      return b
   _b=b.lower()
   if _b in ["&true", "&high"]:
      return "&1"
   elif _b in ["&false", "&high"]:
      return "&0"
   else:
      return b;


def printError(errortojson, num, file, line, text):
   if errortojson == True:
      iserror=True;
      if num==0:
         iserror=False
      error = { "iserror" : iserror, "errno" : num, "file": file, "line" : line, "message" : text }
      print json.dumps(error)
   else:
      linenum=""
      if line != 0:
         linenum="Line "+str(line)+":"
      print "Error#"+str(num), linenum, text 


def checkValue(value):
   ## à ecrire ...
   return True


def checkPostAction(action):
   ## à ecrire ...
   return True


def rulesToJSON(file, verboseFlag, debugFlag, numFile, optimize, errortojson):
   inputs_rules=[]
   outputs_rules=[]
   files=[]
   try: 
      f = open(file, 'r')
   except IOError as e:
      printError(errortojson, 1, 0, "", "can't open file '"+file+"': "+e.strerror)
      return False
   except:
      printError(errortojson, 1, 0, "", "unexpected error: "+sys.exc_info())
      return False

   numLine=0
   numStartLine=0
   numRule=1
   line=""
   for _line in f:
      rule={}
      conditions={}
      numLine=numLine+1

      # nettoyage de la ligne
      _line=_line.strip()        # suppression des blancs devant et derrière
      if line=="":
         line=_line
         numStartLine=numLine
      else:
         line=line+' '+_line

      if line[-1:]=='\\':
         line=line[:-1].split("**")[0]+' ' # suppression du '\' et des commentaires intermédiaires et ajout d'un blanc
         continue

      line=line.split("**")[0].strip()
      if line[-1:]=='\\':
         line=line[:-1]
         continue
      
#      line=line.split("**")[0] # suppression des commentaires
      line=line.strip()        # suppression des blancs
      if len(line)==0:         # plus rien dans la ligne ?
         continue              # lecture de la ligne suivante

      # découpage de la ligne "complete" et contrôle du nombre d'élément
      s=re.split('(is:|if:|onmatch:|onnotmatch:|do:|with:|when:|elseis:)',line) # découpage suivant les 6 mots clés

      nb_elems=len(s)
      if not nb_elems in [3, 5, 7, 9, 11]:
         printError(errortojson, 2, file, numLine, "malformed rule, check syntax")
         return False

      if len(s[0])==0:
         printError(errortojson, 2, file, numLine, "no rule name found, check syntax")
         return False

      # "nettoyage" (suppression des blancs en debut et fin de chaine)  des éléments résultats du découpage
      for i in range(0, len(s)):
         s[i]=s[i].strip()

      toInput=False
      toOutput=False
      ifFound=False
      # récupération nom et valeur
      try:
         if s[1]=='is:':
            toInput=True
         elif s[1]=='do:':
            toOutput=True
         else:
            printError(errortojson, 3, file, numLine, "'is:' or 'do:' is mandatory")
            return False

         if isname(s[0])<>True:
            printError(errortojson, 4, file, numLine, "name format error")
            return False

         if toInput==True:
            rule={ "name" : s[0] , "value"  : optimize_boolean(s[2], optimize) }
         elif toOutput==True:
            rule={ "name" : s[0] , "action" : s[2] }


         if debugFlag==True:
            rule["file"]=numFile
            # rule["line"]=numLine
            rule["line"]=numStartLine
      except:
         printError(errortojson, 5, file, numLine, "unexpected error - "+str(sys.exc_info()))
         return False

      if toInput==True:
         # récupération des conditions si elle existe
         n=3
         if nb_elems>n:
            if s[n]=='if:':
               ifFound=True
               rule['conditions']=[]
               _conditions = s[n+1]
               if _conditions[:1]=='(' and _conditions[-1:]==')':
                  conditionsList=_conditions[1:-1].split(",")
                  for i in conditionsList:
                     i=i.strip()
                     c=re.split('(==|!=|<=|>=|<|>)',i)
                     # validation des expressions gauche et droite
                     if len(c)!=3:
                        printError(errortojson, 28, file, numLine, "expression error ("+i+")")
                        return False
                        
                     if not checkValue(c[0]):
                        printError(errortojson, 26, file, numLine, "value expression error ("+c[0]+")")
                        return False

                     if not checkValue(c[1]):
                        printError(errortojson, 26, file, numLine, "value expression error ("+c[1]+")")
                        return False
                     try:
                        rule['conditions'].append({ 'value1' : optimize_boolean(c[0].strip(), optimize), 'op' : c[1].strip(), 'value2' : optimize_boolean(c[2].strip(),optimize) })
                     except:
                        printError(errortojson, 21, file, numLine, "operator error ("+i+")")
                        return False
                        
               else:
                  printError(errortojson, 6, file, numLine, "conditions or syntax error neer ("+_conditions+")")
                  return False
               n=n+2

         # elseis:
         if nb_elems>n:
            if s[n]=='elseis:':
               if ifFound == True:
                  rule['altvalue']=optimize_boolean(s[n+1], optimize)
                  n=n+2
               else:
                  printError(errortojson, 20, file, numLine, "elseis: without if:, check syntax")
                  return False

         # onmatch: ou onnotmatch:
         onmatch=-1;
         if nb_elems>n:
            if s[n]=='onmatch:' or s[n]=='onnotmatch:':
               rule[s[n][:-1]]=s[n+1]
               if s[n]=='onmatch:':
                  onmatch=1;
               else:
                  onmatch=0;
               n=n+2
            else:
               printError(errortojson, 22, file, numLine, "only onmatch:/onnotmatch: allowed at this, check syntax neer ("+s[n]+")")
               return False

         # onmatch: ou onnotmatch:
         if nb_elems>n:
            if s[n]=='onmatch:' or s[n]=='onnotmatch:':
               if s[n]=='onmatch:' and onmatch == 1:
                  printError(errortojson, 23, file, numLine, "too many onmatch:, check syntax")
                  return False
               if s[n]=='onnotmatch:' and onmatch == 0:
                  printError(errortojson, 24, file, numLine, "too many onnotmatch:, check syntax")
                  return False
               rule[s[n][:-1]]=s[n+1]
               n=n+2
            else:
               printError(errortojson, 25, file, numLine, "only onmatch:/onnotmatch: allowed at this position, check syntax nerr ("+s[n]+")")
               return False


      elif toOutput==True:
         if nb_elems <> 7:
            printError(errortojson, 7, file, numLine, "malformed rule, check syntax")
            return False
         else:
            n=7
         if s[3]!='with:' or s[5]!='when:':
            printError(errortojson, 8, file, numLine, "malformed rule, check syntax")
            return False

         rule['parameters']={}
         _parameters=s[4]
         if _parameters[:1]=='(' and _parameters[-1:]==')':
            _parametersList=_parameters[1:-1].split(",")
            for i in _parametersList:
               i=i.strip()
               c=re.split('(=)',i)
               rule['parameters'][c[0].strip()]=c[2].strip()

         condition=re.split('(rise|fall|change|new)',s[6])
         if len(condition) != 3:
            printError(errortojson, 9, file, numLine, "malformed rule, check syntax")
            return False
         for i in range(0, len(condition)-1):
            condition[i]=condition[i].strip()
#         rule['condition']={ condition[0] : condition[1] }
         cond = condition[1].lower()
         cond_id = -1
         if cond=='rise':
            cond_id=1
         elif cond=='fall':
            cond_id=2
         elif cond=='change':
            cond_id=3
         elif cond=='new':
            cond_id=4
         if cond_id!=-1:
            rule['condition']={ condition[0] : cond_id }

      if n==nb_elems:
         if toInput == True:
            rule["num"]=numRule
            numRule=numRule+1
            inputs_rules.append(rule)
         if toOutput == True:
            outputs_rules.append(rule)
      else:
         printError(errortojson, 9, file, numLine, "not a rule, check syntax (probably 'if:',  'elseis:' or 'onmatch:' dupplication, or if:/onmatch:/elseis: inversion)'")
         return False

      line=""

   f.close()

   rules={}
   rules['inputs']=inputs_rules;
   rules['outputs']=outputs_rules;

   return rules;


def main():
   usage = "usage: %prog [options] rules_files"

   parser = OptionParser(usage)

   parser.add_option("-o", "--output",    dest="outputfile",   help="write result to FILE", metavar="FILE", default=False)
   parser.add_option("-i", "--indent",    action="store_true", dest="indent", help="JSON indented output", default=False)
   parser.add_option("-v", "--verbose",   action="store_true", dest="verbose", help="verbose", default=False)
   parser.add_option("-d", "--debug",     action="store_true", dest="debug", help="debug", default=False)
   parser.add_option("-O", "--optimize",  action="store_true", dest="optimize", help="optimize result code", default=False)
   parser.add_option("-s", "--set",       dest="rulessetfile", help="from rules set", default=False)
   parser.add_option("-p", "--rulespath", dest="rulespath",    help="default rules files path", default=False)
   parser.add_option("-j", "--errorjson", action="store_true", dest="errorjson", help="error in json format", default=False)

   (options, args) = parser.parse_args()
   if options.rulessetfile==False and len(args) < 1:
      if options.errorjson==False:
         parser.error("incorrect number of arguments")
      else:
         printError(options.errorjson, 15, "", 0, "incorrect number of arguments")
      return 1 

   files=[]
   fileNum=0;
   rules={}
   rules["inputs"]=[]
   rules["outputs"]=[]
   _rules=False

   if options.rulessetfile!=False:
      try:
         with open(options.rulessetfile) as rulesfileslist:    
            rulesfileslist = json.load(rulesfileslist)
            for i in rulesfileslist:
               rulesfile=i+".srules"
               if options.rulespath!=False:
                  rulesfile=options.rulespath+"/"+rulesfile
               ret=rulesToJSON(rulesfile, options.verbose, options.debug, fileNum, options.optimize, options.errorjson)
               if ret!=False:
                  try:
                     rules["inputs"]=rules["inputs"]+ret["inputs"]
                     rules["outputs"]=rules["outputs"]+ret["outputs"]
                  except:
                     printError(options.errorjson, 14, "", 0, "unexpected error: "+str(sys.exc_info()))
                     os._exit(1)
               else:
                  os._exit(1)
               files.append(rulesfile)
               fileNum=fileNum+1

      except IOError as e:
         printError(errortojson, 10, "", 0, "can't open file '"+options.rulessetfile+"': "+e.strerror)
         os._exit(1)
      except:
         printError(options.errorjson, 11, "", 0, "unexpected error: "+str(sys.exc_info()))
         os._exit(1)

   for i in args:
      rulefile=i
      if options.rulespath!=False:
         rulefile=options.rulespath+"/"+i
      if options.verbose == True:
         print "Processing file : "+rulefile
      ret=rulesToJSON(rulefile, options.verbose, options.debug, fileNum, options.optimize, options.errorjson)
      if ret!=False:
         try:
            rules["inputs"]=rules["inputs"]+ret["inputs"]
            rules["outputs"]=rules["outputs"]+ret["outputs"]
         except:
            printError(options.errorjson, 12, "", 0, "unexpected error: "+str(sys.exc_info()))
            os._exit(1)
      else:
         return 1
      files.append(rulefile)
      fileNum=fileNum+1

   if options.debug == True:
      rules["files"]=files;

   if options.indent==True:
      _rules=json.dumps(rules, sort_keys=False, indent=4, separators=(',', ': '))
   else:
      _rules=json.dumps(rules)

   if options.outputfile!=False:
      outputfile=options.outputfile
      if options.rulespath!=False:
         outputfile=options.rulespath+'/'+options.outputfile
      try:
         f = open(outputfile, 'w')
         f.write(_rules)
         f.close()
         if options.errorjson==True:
            printError(True, 0, "", 0, "no error")

      except IOError as e:
         printError(options.errorjson, 12, "", 0, "can't create file '"+outputfile+"': "+e.strerror)
         os._exit(1)
      except:
         printError(options.errorjson, 13, "", 0, "unexpected error: "+str(sys.exc_info()))
         os._exit(1)
   else:
      print _rules 


if __name__ == "__main__":
   main()


