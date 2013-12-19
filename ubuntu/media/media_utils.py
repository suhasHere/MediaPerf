# vim:set ts=2 sw=2 sts=2 et cindent:
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"""
 Utilities needed for performing media tests. At present, we do
 0. Audio Recording from the monitor of default sink device.
 1. Silence Removal.
 2. PESQ based quality measurement.
 3. Only .WAV input file format is supported.
 4. Only Linux platform has been verified.
"""

"""
TODO:
0. Support different audio formats and lengths
1. Provide tools support across platforms. Currently we support LINUX only
2. Improve Error Checking and handling
3. Optionally, allow configuration for tools based on sample rates,
   channels and encoding.
   3.a. Remove hard-coded defines from the various tool invocation commands.
4. Support mulitprocessing to enable setup.py, cleanup.py and test.py to
   share information and control the recording process.
5. Improve log file access logic via inter process communication
"""

"""
:OPEN ISSUES:Some details on points 4 and 5 above
*Recently Talos framework was updated to support setup and cleanup
 steps for tests  in the form a individual python processes respectively.
 Unfortunately, this doesn't fit very well with the requirements for
 media performance tests ( as part of the initial investigation though)

*The way the media PESQ test is run involvles the PulseAudio recorder
 to be started before loading the test page and it is terminated once
 the test page exists followed by post processing of the audio file
 to remove silence and compute PESQ scores.

*Since the Setup.py and Cleanup.py are expected to run to completion
 before the test page is loaded, starting the recorder in the Setup.py
 just halts Talos from continuing.

*As a consequence of this, the recorder sub-process is started in Test.py
 rather than in the Setup.py. This has a NASTY side-effect due to the way
 Test class instances are created in PerfConfigurator.py when Talos is
 loaded. This causes the recoder to be started as soon as the Talos is
 started and thus not allowing to have more than one media test in the
 Test Suite. Thus for this version, only one media test can be executed
 and media test MUST BE the only activeTest.

"""
import os
import re
import subprocess
import threading
import time
from subprocess  import Popen, PIPE

# This Directory
here = os.path.dirname(os.path.realpath(__file__))
# Parent
parent = os.path.abspath(os.path.join(here,os.pardir))

# Constants for audio tools, input and processed audio files
# For Linux platform, PulseAudio and LibSox are assumed to be
# installed.
_TOOLS_PATH_             = os.path.join(here,'tools')
_PESQ_                   = os.path.join(_TOOLS_PATH_, 'PESQ')
_TEST_PATH_              = os.path.join(here,'html')
_INPUT_FILE_             = os.path.join(_TEST_PATH_, 'input16.wav')
_RECORDED_FILE_          = os.path.join(_TEST_PATH_, 'record.wav')
_RECORDED_NO_SILENCE_    = os.path.join(_TEST_PATH_, 'record_no_silence.wav')

# Constants used as parameters to Sox recorder and silence trimmer
#TODO: Make these dynamically configurable
_SAMPLE_RATE_             = '16000' #16khz
_NUM_CHANNELS_            = '1'     # mono channel
_SOX_ABOVE_PERIODS_       = '1'     # One period of silence in the beginning
_SOX_ABOVE_DURATION_      = '2'     # Duration to trim till proper audio
_SOX_ABOVE_THRESHOLD_     = '5%'    # Audio level to trim at the beginning
_SOX_BELOW_PERIODS_       = '1'     # One period of silence in the beginning
_SOX_BELOW_DURATION_      = '2'     # Duration to trim till proper audio
_SOX_BELOW_THRESHOLD_     = '5%'    # Audio level to trim at the beginning

"""
  Thread to record audio
"""
class AudioRecorder(threading.Thread):

  def __init__(self, parent, output_file):
        self.__parent = parent
        self.output_file = output_file
        threading.Thread.__init__(self)

  # Set record duration, typically set to length
  # of the audio test being run. Check test.py
  def setDuration(self, duration):
        self.rec_duration = duration

  # Set source monitor for recording
  # We pick the first monitor for the sink available
  def setRecordingDevice(self, device):
        self.rec_device = device
        # Adjust volime of the sink to 100%, since quality was
        # bit degraded when this was not done.
        cmd = ['pacmd', 'set-source-volume', self.rec_device, '65536']
        cmd = [str(s) for s in cmd]
        print "Running PACMD for Setting Volume %s" % cmd
        p = subprocess.Popen(cmd, stdout=PIPE, stderr=PIPE)
        output, error = p.communicate()
        if p.returncode !=0:
          print "Failed to set Volume to 100% with error %s" % error
          return error

  # Start recording.
  def run(self):

      # PACAT command to record 16 bit singned little endian mono channel
      # audio from the sink self.rec_device
      pa_command = ['pacat', '-r', '-d', self.rec_device, '--format=s16le',
                    '--rate=16000', '--channels=1']
      pa_command =  [str(s) for s in pa_command]

      # Sox command to convert raw audio from PACAT output to .wav format"
      sox_command = ['sox', '-t', 'raw', '-r',_SAMPLE_RATE_,'-sLb', 16,
                    '-c', _NUM_CHANNELS_, '-', self.output_file, 'trim', 0,
                    self.rec_duration]
      sox_command =  [str(s) for s in sox_command]

      # We sleep till the page gets loaded which takes approximiately
      # 15-18 seconds. This is needed in order to avoid HUGE delay
      # generated by the parec towards the beginning of the file.
      #time.sleep(12)

      print "Monitor Device is %s" % self.rec_device
      print "PACAT Command is %s" % pa_command
      print "SOX Command is %s"   %   sox_command

      # Run the commands the PIPE them together
      p1 = Popen(pa_command, stdout=PIPE)
      p2 = Popen(sox_command, stdin=p1.stdout, stdout=PIPE)
      retcode = p2.communicate()[0]
      if retcode != 0:
        print " Failed to record audio."
      else:
        print " Done with audio recording"
      p1.kill()
      # Let's indicate the parent process to perform any cleanup
      # We dont process return code as this point
      #self.__parent.doneRecording()


'''
Utility class for managing pre and post recording operations
'''
class AudioUtils(object):

    # Reference to the recorder thread.
    recorder = None
    browser_log = None

    def setBrowserLogFile(self, log_file):
      browser_log = log_file

    # Function to find the monitor for sink available
    # on the platform. (Linux Only)
    def setupAudioDeviceForRecording(self):
      # Use pactl to find the monitor for our Sink
      # This picks the first sink available
      cmd = ['pactl', 'list']
      print "Running PATCL Command %s" % cmd
      output = subprocess.check_output(cmd)
      result = re.search('\s*Name: (\S*\.monitor)',output)
      if result:
	self.recorder.setRecordingDevice(result.group(1))


    # Run PESQ on the audio reference file and recorded file
    def computePESQScore(self):
      print "In Utils.calculatePesq."
      _START_ = '__start_media_report'
      _END_   = '__end_media_report'
      cmd = [_PESQ_,'+16000', _INPUT_FILE_, _RECORDED_NO_SILENCE_]
      print "Running PESQ Command %s" % cmd
      output = subprocess.check_output(cmd)
      #P.862 Prediction (Raw MOS, MOS-LQO):  = 2.392        2.009
      result = re.search('Prediction.*= (\d{1}\.\d{3})\t(\d{1}\.\d{3})', output)
      # We need to sleep for the browser_log to be available.
      # time.sleep(1)
      # delete the recorded file with no silence
      #os.remove(_RECORDED_NO_SILENCE_)
      pesq_score = ""
      if result:
        pesq_score = str(result.group(1)) + ',' + str(result.group(2))
        report = _START_ + pesq_score + _END_
        f = open('browser_output.txt', "a+")
        f.write(report)
        f.close()
      time.sleep(2)
      return pesq_score

    # Kick-off Audio Recording Thread
    def startRecording(self, duration):
      self.recorder = AudioRecorder(self,_RECORDED_FILE_)
      self.setupAudioDeviceForRecording()
      self.recorder.setDuration(duration)
      print " INVOKING RECORDER START "
      self.recorder.start()
      print " DONE INVOKING RECORDER START "

    def stopRecording(self):
      print " In Utils.StopRecording "
      self.doneRecording()

    # Let complete Audio Recording and remove silence at the
    # beginning and towards the end of recorded audio.
    # This function is invoked by the Recoder thread once it
    # completes
    def doneRecording(self):
      print "In Utils.stopRecording."
      #clean up silence and delete the recorded file
      """
      http://digitalcardboard.com/blog/2009/08/25/the-sox-of-silence/
      ./sox record.wav out1.wav silence 1 2 5% 1 2 5% reverse silence 1 2 5%
       """
      cmd = ['sox',_RECORDED_FILE_,_RECORDED_NO_SILENCE_,'silence',
             _SOX_ABOVE_PERIODS_, _SOX_ABOVE_DURATION_, _SOX_ABOVE_THRESHOLD_,
             'reverse', 'silence', _SOX_BELOW_PERIODS_, _SOX_BELOW_DURATION_,
             _SOX_BELOW_THRESHOLD_, 'reverse']
      cmd = [str(s) for s in cmd]
      print "Running Silence Command %s" % cmd
      retCode = subprocess.call(cmd,stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
      # Delete the recorded file
      #os.remove(_RECORDED_FILE_)
