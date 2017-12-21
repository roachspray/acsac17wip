import json
import os
import subprocess

pskin_vars = {}
pskin_vars['opt'] = '/usr/lib/llvm-4.0/bin/opt'
pskin_vars['clang'] = '/usr/lib/llvm-4.0/bin/clang'
pskin_vars['clang++'] = '/usr/lib/llvm-4.0/bin/clang++'
pskin_vars['afl_fuzz'] = '/usr/local/bin/afl-fuzz'
pskin_vars['afl_clang'] = '/usr/local/bin/afl-clang-fast'
pskin_vars['afl_clang++'] = '/usr/local/bin/afl-clang++-fast'
pskin_vars['objdump'] = '/usr/bin/objdump'

# Updated by modules and stuff
required_keys = [ "opt",
  "clang", 
  "clang++",
  "ps_root",
  "lib_trays",
  "lib_trace",
  "lib_prune",
  "afl_fuzz",
  "afl_clang",
  "afl_clang++",
  "objdump",
  "command",
  "bc_file",
  "depths",
  "fnlist_file",
  "out_dir"
]
#optional so far are:
#  compile_lib which should be a n array for [-L/path -llib... type] 
#    compilation requirements

def loadConfig(path):
  with open(path, "r") as oh:
    jo = json.load(oh);
    for x in pskin_vars.keys():
      if x in jo.keys():
        pskin_vars[x] = jo[x]

    for k in jo.keys():
      if k not in pskin_vars.keys():
        pskin_vars[k] = jo[k]    
    if 'lib_trays' not in pskin_vars.keys():
      pskin_vars['lib_trays'] = os.path.join(pskin_vars['ps_root'], "libtrays/libtrays.a")
    if 'lib_trace' not in  pskin_vars.keys():
      pskin_vars['lib_trace'] = os.path.join(pskin_vars['ps_root'], "build/lib/TraceInject.so")
    if 'lib_prune' not in  pskin_vars.keys():
      pskin_vars['lib_prune'] = os.path.join(pskin_vars['ps_root'], "build/lib/NaivePrune.so")

def verifyConfig():
  for rk in required_keys:
    if rk not in pskin_vars.keys():
      print "Required key '{}' not found in pskin_vars".format(rk) 
      return False
  return True


# returns (exit, stdout+stderr)
def execOpt(lib, inbc, outbc, exargs):
  out = ""
  try:
    args = [pskin_vars['opt']]
    if lib:
      args.extend(["-load", lib])
    if exargs:
      args.extend(exargs) 
    args.extend(["-o", outbc, inbc])
    print "opt args: {}".format(" ".join(args))
    out = subprocess.check_output(args,
      stderr=subprocess.STDOUT)
  except subprocess.CalledProcessError, E:
    return (E.returncode, out)
  return (0, out)

def execClang(insrc, outbin, extra=None):
  out = ""
  try:
    args = [pskin_vars['clang']]
    if 'compile_lib' in pskin_vars.keys():
      args.extend(pskin_vars['compile_lib'])
    args.append(insrc)
    if extra:
      args.extend(extra)

    args.extend(["-o", outbin])
    print args
    out = subprocess.check_output(args,
      stderr=subprocess.STDOUT)
  except subprocess.CalledProcessError, E:
    return (E.returncode, out)
  return (0, out)

def execClangPP(insrc, outbin, extra=None):
  out = ""
  try:
    args = [pskin_vars['clang++']]
    if 'compile_lib' in pskin_vars.keys():
      args.extend(pskin_vars['compile_lib'])
    args.append(insrc)
    if extra:
      args.extend(extra)

    args.extend(["-o", outbin])
    out = subprocess.check_output(args,
      stderr=subprocess.STDOUT)
  except subprocess.CalledProcessError, E:
    return (E.returncode, out)
  return (0, out)

#
# insrc - a bitcode file with trace code injected
# outbin - name of output binary
# cpp - use c++ or c compiler
#
def compileTraceVersion(insrc, outbin, cpp=False):
  libtrays_extra = [pskin_vars["lib_trays"]]
  if cpp == True:
    (ex, o) = execClangPP(insrc, outbin,
      libtrays_extra)
  else:
    (ex, o) = execClang(insrc, outbin,
      libtrays_extra)
  if ex != 0:
    return False
  return True


def prepBitcode(inbc, outbc):
  label_step = outbc + "_prep_inter.bc"
  (ex, s) = execOpt(None, inbc, label_step,
    ["-mem2reg", "-constprop", "-ipconstprop", "-lowerswitch", "-lowerinvoke"])
  if ex != 0:
    return False

  # Add MD labeling
  (ex, s) = execOpt(pskin_vars['lib_trace'], label_step,
    outbc,
    ["-pskin-label-inject", "-pskin-log-file", "heyo"])
  if ex != 0:
    return False
  return True

def injectTrace(inbc, outbc, logname, logpc=False): 

  if logpc == True:
    (ex, s) = execOpt(pskin_vars['lib_trace'], inbc,
      outbc, 
      ["-pskin-inject", "-pskin-log-pc", "-pskin-log-file", logname])
  else:
    (ex, s) = execOpt(pskin_vars['lib_trace'], inbc,
      outbc, 
      ["-pskin-inject", "-pskin-log-file", logname])
  if ex != 0:
    return False
  return True

def pruneBitcode(inbc, outbc, logname, depth):
  (ex, s) = execOpt(pskin_vars['lib_prune'], inbc, outbc,
    ["-pskin-naive-prune", "-pskin-fnlist", 
     pskin_vars["fnlist_file"], "-pskin-log-file",
     logname, "-pskin-depth", str(depth)])
  return (ex, s)

def execAflClang(insrc, outbin):
  out = ""
  try:
    args = [pskin_vars['afl_clang']]
    if 'compile_lib' in pskin_vars.keys():
      args.extend(pskin_vars['compile_lib'])

    args.extend([insrc, "-o", outbin])
    e = dict(os.environ, AFL_CC=pskin_vars['clang'])
    out = subprocess.check_output(args,
      stderr=subprocess.STDOUT,
      env=e) 
  except subprocess.CalledProcessError, E:
    return (E.returncode, out)
  return (0, out)

def execAflClangPP(insrc, outbin):
  out = ""
  try:
    args = [pskin_vars['afl_clang++']]
    if 'compile_lib' in pskin_vars.keys():
      args.extend(pskin_vars['compile_lib'])

    args.extend([insrc, "-o", outbin])
    e = dict(os.environ, AFL_CPP=pskin_vars['clang++'])
    out = subprocess.check_output(args,
      stderr=subprocess.STDOUT,
      env=e)
  except subprocess.CalledProcessError, E:
    return (E.returncode, out)
  return (0, out)
