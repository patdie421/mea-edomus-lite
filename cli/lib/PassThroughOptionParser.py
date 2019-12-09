from optparse import OptionParser, BadOptionError

class PassThroughOptionParser(OptionParser):

   def _process_long_opt(self, rargs, values):
      try:
         OptionParser._process_long_opt(self, rargs, values)
      except BadOptionError, err:
         self.largs.append(err.opt_str)

   def _process_short_opts(self, rargs, values):
      try:
         OptionParser._process_short_opts(self, rargs, values)
      except BadOptionError, err:
         self.largs.append(err.opt_str)
