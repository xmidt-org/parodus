#-----------------------------------------------------------------
#  curl_parodus.py
#-----------------------------------------------------------------

import subpro as SUBP
import datetime as DT
import time
import sys, os

HEADER_AUTH = "Authorization"

SERVICE_NAME = "config"

if len(sys.argv) < 3:
  print "Expecting 2 arguments:"
  print " 1) device_id"
  print " 2) command file"
  sys.exit (4)

device_id = sys.argv[1]
file_name = sys.argv[2]

if len(sys.argv) > 3:
  SERVICE_NAME = sys.argv[3]

if len(device_id) != 12:
  print "Expecting 12 digit mac address"
  sys.exit(4)

auth_str = os.environ.get ("WEBPA_AUTH_HEADER", "")
if not auth_str:
  print "Missing environment variable WEBPA_AUTH_HEADER"
  sys.exit(4)

BASIC_AUTH_VALUE = "Basic " + auth_str

try:
  fin = open (file_name, "r")
except StandardError, e:
  print "Error opening file", file_name
  print e
  sys.exit(4)

def get_errno(e):
  if hasattr (e, "errno"):
    return e.errno
  if hasattr (e, "args"):
    return e.args[0]
  return -1

def timestamp():
  dt = DT.datetime.now()
  return "%04d/%02d/%02d %02d:%02d:%02d " % \
    (dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
#end def

set_list = []
for line in fin:
  line = line.strip()
  if line.endswith ('\n'):
    line = line[0:-1]
  line = line.strip()
  if line.endswith (','):
    line = line[0:-1]
  set_list.append (line)
#end for

payload_str = ",".join (set_list)

url_base = "https://api-beta.webpa.comcast.net:8090/api/v2/device/"

cmd_url = "%smac:%s/%s?names=%s" % (url_base, device_id, SERVICE_NAME, payload_str)
#print cmd_url
#print

auth_str = "%s: %s" % (HEADER_AUTH, BASIC_AUTH_VALUE)

cmd = ["curl", "-H", auth_str, "-k", "--max-time", "10", cmd_url]
#cmd = ["curl", "-H", auth_str, "-k", 
#       "--connect-timeout", "150", "--max-time", "180", cmd_url]
err_cnt = 0

time.sleep (1.0)
try:
  while True:
    rtn = SUBP.call (cmd, abort=False)
    if rtn == 0:
      print "\n%s Timeout Errors %d" % (timestamp(), err_cnt)
    elif rtn == 28:
      print "Command timed out"
      err_cnt += 1
    else:
      break
    time.sleep (5.0)
    #sub_pro = SUBP.TSubProc (cmd)
    #if sub_pro.errors:
      #open_err_cnt += 1
      #continue
    #for line in sub_pro.read_stdout (show_lines=False):
      #sys.stdout.write (timestamp() + line)
    #if sub_pro.errors: 
      #read_err_cnt += 1
  #end while
except KeyboardInterrupt, e:
  print e
  print "Timeout Errors", err_cnt
except StandardError, e:
  print e
  print "Timeout Errors", err_cnt
 

#req = URL.Request (cmd_url)
#req.add_header (HEADER_AUTH, BASIC_AUTH_VALUE)

#f = URL.urlopen (req)


#  -k https://scytale-beta.webpa.comcast.net:8090/api/v2/device/mac:14cfe213e1d2/config?names=
