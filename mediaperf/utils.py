# vim:set ts=2 sw=2 sts=2 et cindent:
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"""
 Utilities needed for performing media tests. At present, we do
 0. Audio Recording from the default output
 1. Silence Removal
 1. PESQ
"""

"""
TODO:
0. Get rid of print command and use Talos logging facility
1. Make tools support across platforms
2. Improve Error Checking
3. Allow configuration for AudioTools based on sample rates,
	channels and more.
4. Remove hard-coded stuff from the various tool invocation commands.
"""

import commands
import os
import re
import subprocess
import sys
import threading

# This Directory
here = os.path.dirname(os.path.realpath(__file__))
# Parent
parent = os.path.abspath(os.path.join(here,os.pardir))

# Constants for audio tools, input and processed audio files
_TOOLS_PATH_     		= os.path.join(here,'tools')
_TEST_PATH_      		=  os.path.join(parent,'media_test')
_PESQ_      			= os.path.join(_TOOLS_PATH_, 'PESQ')
_AUDIO_RECORDER_ 		= os.path.join(_TOOLS_PATH_, 'rec')
_SOX_            		= os.path.join(_TOOLS_PATH_, 'sox')
_INPUT_FILE_     		= os.path.join(_TEST_PATH_, 'input.wav')
_RECORDED_FILE_  		= os.path.join(_TEST_PATH_, 'record.wav')
_RECORDED_NO_SILENCE_ 	= os.path.join(_TEST_PATH_, 'record_no_silence.wav')

"""
  Class to record audio while the test is in progress
"""
class AudioRecorder(threading.Thread):

  def __init__(self, output_file):
  	self.duration = 20 # not using as of today.
  	self.output_file = output_file
  	threading.Thread.__init__(self)

  # Set record duration
  def setDuration(self, duration):
  	self.rec_duration = duration

  # Start recording.
  def run(self):
      """
      10 seconds worth of mono-channel audio and 16kHz
      ./rec -r 16000 -c 1 test.wav trim 0 00:10
      """
      cmd = [_AUDIO_RECORDER_, '-r 16000', '-c 1', self.output_file,
      		'trim', 0, self.rec_duration]
      cmd = [str(s) for s in cmd]
      print " Running Record Command: %s" % cmd
      ret = subprocess.call(cmd,stdout=subprocess.PIPE,
      						stderr=subprocess.PIPE)
      if ret != 0:
      	print " Failed to record audio."
      else:
      	print " Done with audio recording"


class AudioUtils(object):

    def __init__(self):
    	print " TOOLS PATH %s" % _TOOLS_PATH_
    	print " PESQ PATH %s" % _PESQ_
    	print " AUDIO RECORDER %s" % _AUDIO_RECORDER_
    	self.recorder = AudioRecorder(_RECORDED_FILE_)

    # Run PESQ on the audio reference file and recorded file
    def calculatePesq(self):
    	print "In Utils.calculatePesq."
    	cmd = [_PESQ_,'+16000', _INPUT_FILE_, _RECORDED_NO_SILENCE_]
    	print "Running PESQ Command %s" % cmd
    	output = subprocess.check_output(cmd)
    	#P.862 Prediction (Raw MOS, MOS-LQO):  = 2.392	2.009
    	result = re.search('Prediction.*= (\d{1}\.\d{3})\t(\d{1}\.\d{3})', output)
    	# delete the recorded file with no silence
    	os.remove(_RECORDED_NO_SILENCE_)
    	if result:
    		return (float(result.group(1)), float(result.group(2)))
    	else:
    		return None

 	# Kick-off Audio Recording Thread
    def startRecording(self, duration):
    	print "In Utils.startRecording."
    	self.recorder.setDuration(duration)
    	self.recorder.start()

    # Let complete Audio Recording and remove silence at the
    # beginning and towards the end of recorded audio.
    def stopRecording(self):
    	print "In Utils.stopRecording."
    	# Stop Recording
    	self.recorder.join()
    	#clean up silence and delete the recorded file
    	"""
    	http://digitalcardboard.com/blog/2009/08/25/the-sox-of-silence/
    	./sox record.wav out1.wav silence 1 0.3 1% 1 0.3 1%
    	"""
    	cmd = [_SOX_,_RECORDED_FILE_,_RECORDED_NO_SILENCE_,'silence',
    			'1', '0.1', '1%', '1', '0.1', '1%']
    	cmd = [str(s) for s in cmd]
    	print " Silence removal command: %s" % cmd
    	ret = subprocess.call(cmd,stdout=subprocess.PIPE,
    							stderr=subprocess.PIPE)
    	# Delete the recorded file
    	os.remove(_RECORDED_FILE_)

