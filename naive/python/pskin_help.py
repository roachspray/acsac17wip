#
# Quick hack wrapper ... setup your <target>.json
# as per the readme and then run python pskin_help.py -t <target.json
# to generate the output directory where there a trace compiled
# bin to execute. Execute. Then run pskin_help.py -s <target>json
# to generate slices
#
# all a bit hokey :)

import fnmatch
import os
import subprocess
import sys

sys.path.append(".")

import pskin as PS

def main():
    if len(sys.argv) != 3:
        print "Use -t or -s with json file target"
        sys.exit(1)

    json_config = sys.argv[2]
    if sys.argv[1] == "-t":
        tracePrep(json_config)

    elif sys.argv[1] == "-s":
        slice(json_config)

    else:
        print "Use -t or -s with json file target"
        sys.exit(1)

    sys.exit(0)

def crashrun(json_config):
    PS.loadConfig(json_config)
    if PS.verifyConfig() == False:
        print "Unable to verify config for running\n"
        sys.exit(1)
    od = PS.pskin_vars['out_dir']
    cslogs = os.path.join(od, "cslogs")
    os.mkdir(cslogs)
    deps = PS.pskin_vars["depths"]
    command = PS.pskin_vars["command"]
    print command
    crashsamples = []
    for r, d, f in os.walk(od):
        for filename in fnmatch.filter(f, 'id:*'):
            crashsamples.append(os.path.join(r, filename))

    # Not sure if wise I try with each or not. But I am.
    for d in deps:
        dbctrace = os.path.join(od, "dep{}trace".format(d))
        dtl = os.path.join(od, "dep{}trace.log".format(d))
        for csample in crashsamples:
            ctmp = command
            ctmp = ctmp.replace("@@", csample)
            ctmp = ctmp.split(" ")
            print ctmp
            c = [dbctrace]
            c.extend(ctmp)
            print c
            out = ""
            try:
                out = subprocess.check_output(c,
                    stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError, E:
                pass
            lgm = os.path.join(cslogs, "dep{}log_{}".format(d, os.path.basename(csample)))
            outlog = "{}_stdout".format(lgm)
            os.rename(dtl, lgm)
            with open(outlog, "w+") as oh:
                oh.write(outlog)
        
def slice(json_config):
    PS.loadConfig(json_config)
    if PS.verifyConfig() == False:
        print "Unable to verify config for running\n"
        sys.exit(1)

    od = PS.pskin_vars['out_dir']
    trace_log = os.path.join(od, "trace.log")
    deps = PS.pskin_vars["depths"]
    prep_bc = os.path.join(od, "prep.bc")
    for d in deps:
        dbc = os.path.join(od, "dep{}.bc".format(d))
        dbc_afl = os.path.join(od, "dep{}_afl".format(d))
        dbc_normal = os.path.join(od, "dep{}".format(d))

        # prune and from that build AFL-able and normal versions

        b = PS.pruneBitcode(prep_bc, dbc, trace_log, d)
        if b[0] != 0:
            print "Failed prune:\n{}".format(b[1])
            continue

        b = PS.execAflClang(dbc, dbc_afl)
        if b[0] != 0:
            print "Failed exec afl-clang:\n{}".format(b[1])

        b = PS.execClang(dbc, dbc_normal)
        if b[0] != 0:
            print "Failed exec clang:\n{}".format(b[1])

        trace_bc = os.path.join(od, "dep{}trace.bc".format(d))
        trace_log = os.path.join(od, "dep{}trace.log".format(d))
        b = PS.injectTrace(dbc, trace_bc, trace_log, True)
        if b == False:
            print "Failed to inject trace points for dep{}".format(d)
            continue

        trace_bin = os.path.join(od, "dep{}trace".format(d))
        b = PS.compileTraceVersion(trace_bc, trace_bin, cpp=False)
        if b == False:
            print "Failed to compile trace version of dep{}".format(d)
    
def tracePrep(json_config):
    PS.loadConfig(json_config)
    if PS.verifyConfig() == False:
        print "Unable to verify config for running\n"
        sys.exit(1)

    od = PS.pskin_vars['out_dir']
    try:
        os.mkdir(od)
    except:
        print "move or delete your {} out_dir".format(od)
        sys.exit(1)

    prep_bc = os.path.join(od, "prep.bc") 
    b = PS.prepBitcode(PS.pskin_vars['bc_file'], prep_bc)
    if b == False:
        print "Failed to prep bitcode\n"
        sys.exit(1)

    prep_bin = os.path.join(od, "prep")
    b = PS.execClang(prep_bc, prep_bin)
    if b[0] != 0:
        print "Failed exec clang:\n{}".format(b[1])

    trace_bc = os.path.join(od, "trace.bc")
    trace_log = os.path.join(od, "trace.log")
    b = PS.injectTrace(prep_bc, trace_bc, trace_log, False)
    if b == False:
        print "Failed to inject trace points"
        sys.exit(1)

    trace_bin = os.path.join(od, "trace")
    b = PS.compileTraceVersion(trace_bc, trace_bin, cpp=False)
    if b == False:
        print "Failed to compile trace version of program"
        sys.exit(1)

    
    return

if __name__ == '__main__':
	main()
