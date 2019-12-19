mem={}

def getMemory(id):
   try:
      return mem[id]
   except:
      new_mem={}
      mem[id]=new_mem
      return mem[id]
