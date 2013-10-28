# vim:set ts=2 sw=2 sts=2 et cindent:
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import utils

"""
Test Definitions for various Media Tests. Future Media related Talos
tests MUST be made subclass of TmBase class.
Unlike other Talos tests, these tests does allow pre and post processing
per test outside browser execution. [ see audio_playback_pesq for an example].
IMPLEMENTATION NOTE:
 @METHOD setup()  -- One should perform those steps needs before browser is
 					 launched to run a test
 @METHOD finish() -- One should perform any needed cleanup steps outside browser
 					 context. Also, any computed results must be returned in the
 					 media result format.
"""

"""
TODO:
0. Get rid of print command and use Talos logging facility
2. Improve Error Checking
"""

class MediaTest(object):
    # Meta class for Media Tests
    cycles = None # number of cycles
    keys = []
    desktop = True
    mobile = True

    @classmethod
    def name(cls):
        return cls.__name__

    @classmethod
    def description(cls):
        if cls.__doc__ == None:
            return "No documentation available yet."
        else:
            doc = cls.__doc__
            description_lines = [i.strip() for i in doc.strip().splitlines()]
            return "\n".join(description_lines)

    @classmethod
    def setup(cls):
    	print "Setup: Dummy Implementation in the Base-Class"

    @classmethod
    def finish(cls):
    	print "Finish: Dummy Implementation in the Base-Class"

    def __init__(self, **kw):
        self.update(**kw)

    def update(self, **kw):
        self.__dict__.update(kw)

    def items(self):
        """
        returns a list of 2-tuples
        """
        retval = [('name', self.name())]
        for key in self.keys:
            value = getattr(self, key, None)
            if value is not None:
                retval.append((key, value))
        return retval

    def __str__(self):
        """string form appropriate for YAML output"""
        items = self.items()

        key, value = items.pop(0)
        lines = ["- %s: %s" % (key, value)]
        for key, value in items:
            lines.append('  %s: %s' % (key, value))
        return '\n'.join(lines)

# BaseClass For Media Tests
class TmBase(MediaTest):
    """abstract base class for tm-style tests"""
    keys = ['url', 'url_timestamp', 'timeout', 'cycles', 'shutdown', 'profile_path']

class audio_playback_pesq(TmBase):
    """
   	HTML5 Audio playback test with PESQ evaluation
    """
    cycles = 1
    timeout = 25
    url = 'media_test/audio_record.html'
    shutdown = True
    audioUtils = utils.AudioUtils()

    @classmethod
    def setup(cls):
    	"""
    	1. Setup Audio recording
    	"""
    	print "In setup "
    	# since the tests are run based on the timeout logic
    	# we need to record atleast till the time test runs.
    	# this might end up adding unwanted silence at the
    	# beginning or end of the recorded file.
    	cls.audioUtils.startRecording(25)

    @classmethod
    def finish(cls):
    	"""
    	0. Process the recorded file for any silence removal
    	1. Run pesq on original and recorded one.
    	"""
    	# Report Format for results
    	_START_ = '__start_media_report'
    	_END_ = '__end_media_report'

    	cls.audioUtils.stopRecording()
    	pesq = cls.audioUtils.calculatePesq()

    	# format the results for reporting.
    	# This is to bypass the results and output creation
    	# and minimize the dependency.
    	if pesq:
    		result = _START_+str(pesq[0]) + ','+str(pesq[1]) + _END_
    		print result
    	else:
    		result = None

    	return result


# global test data
tests = [audio_playback_pesq]
test_dict = dict([(i.name(), i) for i in tests])
