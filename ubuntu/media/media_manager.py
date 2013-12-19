# vim:se?t ts=2 sw=2 sts=2 et cindent:
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import os
import optparse
from subprocess import Popen, PIPE

# Talos specific imports 
import talos.utils
import talos.talosProcess
import mozhttpd
import media_utils

'''
API Design:
 <resource>/<component>/<operation>
 Example: audio recorder functionality
          ==> audio/recorder/start, audio/recorder/stop
          For audio pesq functionality
          ==> audio/pesq/compute
'''

__TEST_HTML_PAGE__ = "http://localhost:16932/startup_test/media/html/media_tests.html"

# A handy place to store objects we need during the lifetime of the manager
class ObjectDb(object):
    browser_proc = None  
    httpd_server = None
    audio_utils  = None

def Error(message):
    """raise a talosError for bad parsing of the browser log"""
    raise talos.utils.talosError(message)


@mozhttpd.handlers.json_response
def parseGETUrl(request):
    # Parse the url and invoke appropriate handlers
    url_path = request.path.lower()
    print "ParseUrl: Request is %s " % url_path
    if url_path.find('audio') != -1:
      return (handleAudioRequest(request))
    elif url_path.find('server') != -1:
      return (handleServerConfigRequest(request))
    else:
      return (500, {'ERROR': request.path})
    
    
@mozhttpd.handlers.json_response
def parseUnknowUrl(request):
      return (500, {'ERROR': request.path})

# Handle Configuration Commands
def handleServerConfigRequest(request):
    print "Config Request %s " % request.path
    if request.path == '/server/config/stop':
      ObjectDb.browser_proc.kill()
      ObjectDb.httpd_server.stop()
    else:
      return (500, {'ERROR': request.path})

# Handle Audio Resource Command
def handleAudioRequest(request):
    # is this a recorder API
    if request.path.find('recorder') != -1:
      return (parseAudioRecorderRequest(request))
    elif request.path.find('pesq') != -1:
     return(parsePESQRequest(request))
    else:
      return (500, {'ERROR': request.path})

# Handle all the audio recorder sub-api
def parseAudioRecorderRequest(request):
    request_params = {}
    # check if there are params
    params = request.query.split(',')
    if request.path.find('start') != -1:
      print " Audio Recorder Start "
      for items in params:
        (name, value) = items.split('=')
        if name.startswith('timeout') == True:
          ObjectDb.audio_utils.startRecording(value)
          return (200, {'Start-Recording' : 'Success'})
    elif request.path.find('stop') != -1:
      print " Audio Recorder Stop "
      ObjectDb.audio_utils.stopRecording()
      return (200, {'Stop-Recording' : 'Success'})
    else:
      return (500, {'ERROR' : request.path})

# Parse PESQ Get Request
def parsePESQRequest(request):
    if request.path.find('compute') != -1:
      print "PESQ Compute "
      results = ObjectDb.audio_utils.computePESQScore();
      print "PESQ Results %s" % results
      return (200, {'PESQ-Score' : results}) 
    else:
      return (500, { 'ERROR': request.path})

# Run HTTPD server and setup URL path handlers
def run_server(doc_root):
    print "Start Server Entered"
    httpd_server = mozhttpd.MozHttpd(port=16932, docroot=doc_root,
                                     urlhandlers = [ { 'method'   : 'GET',
                                                       'path'     : '/audio/',
                                                       'function' : parseGETUrl },
                                                     { 'method'   : 'GET',
                                                       'path'     : '/server/?',
                                                       'function' : parseGETUrl } ])
    print "Server %s at %s:%s" % (httpd_server.docroot, httpd_server.host, httpd_server.port)
    ObjectDb.httpd_server = httpd_server
    httpd_server.start(block=True)

# Kick-off firefox process with passed in profile
def open_browser(browser, profile):
    print " Open Browser Entered "
    command = [ browser, '-profile', profile, __TEST_HTML_PAGE__]
    command = [str(s) for s in command]
    print "Command is %s " % command
    #command = '/home/suhasnandakumar/mozilla/mc_1/obj-ff-dbg/dist/bin/firefox'
    browser_proc = talos.talosProcess.talosProcess(command,
                                                   env=os.environ.copy())
    browser_proc.run()
    ObjectDb.browser_proc = browser_proc
    print " Open Browser Exited "

if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option("-p","--profile", dest="profile",
                      help="Firefox User Profile",)
    parser.add_option("-b","--browser", dest="browser",
                      help="Firefox Browser Exeuctable Path",)
    parser.add_option("-l","--browser_log", dest="logfile",
                      help="Browser Log File used by Talos",)
    parser.add_option("-t","--talos", dest="talos_path",
                      help="Talos Path",)
    (options, args) = parser.parse_args()  

    print "Profile Path %s " % options.profile
    print "Browser Path %s " % options.browser
    print "Browser Log File %s " % options.logfile
    print "Talos Path %s " % options.talos_path

    # 1. Create handle to the AudioUtils
    ObjectDb.audio_utils = media_utils.AudioUtils()

    # 2. Kick off the browser
    open_browser(options.browser, options.profile)

    # 3. Start httpd server
    run_server(options.talos_path)
    '''
    if not os.path.exists(profile):
        Error("MediaManager: Profile Path Invalid ")

    if not os.path.exists(browser):
        Error("MediaManager: Browser Path Invalid")
    '''
