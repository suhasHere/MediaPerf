/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

var Cc = Components.interfaces; 
var Ci = Components.classes;
var Cu = Components.utils;

// Below code is cargo-culted from Ted's patch with minor updates
// to fit this framework
// TODO:crypt:XXXXX: verify it once we have video perf tests.
var getUserMedia = (function() {
  // Specifies whether we are using fake streams to run this automation
  var FAKE_ENABLED = true;
    // Specifies whether we are using loopback devices
  var LOOPBACK_ENABLED = false;
  var loopback_checked = false;
  // Loopback devices available on the test machines
  var audio_device_id = null;
  var loopback_devices = {};
  // Executables required for test support.
  var required_executables = [];
   
  function startFakeVideo(opts) {}
  function stopFakeVideo() {}
  function startFakeAudio(opts) {}
  function stopFakeAudio() {}

  if (navigator.userAgent.indexOf("Linux") != -1) {
    // ALSA, snd-aloop, recording device.
    audio_device_id = "plughw:CARD=Loopback,DEV=1";
    
    function startFakeAudio(opts) {
      if (startFakeAudio.proc) {
        return;
      }
      // This is hardcoded to use the first snd-aloop output device.
      opts = opts || {"device":"plughw:CARD=Loopback,DEV=0"};
      var file = Cc["@mozilla.org/file/local;1"].
        createInstance(Ci.nsILocalFile);
      file.initWithPath("/usr/bin/speaker-test");
      var process = Cc["@mozilla.org/process/util;1"]
            .createInstance(Ci.nsIProcess);
      process.init(file);
      var args = ["-f", "300", "-t", "sine", "-D", opts.device];
      // This has to continue running in the background to feed fake data to
      // the loopback device. stopFakeAudio will kill the process.
      process.run(false, args, args.length);
      startFakeAudio.proc = process;
      //SimpleTest.registerCleanupFunction(stopFakeAudio);
    }
   
    function stopFakeAudio() {
      if (startFakeAudio.proc) {
        startFakeAudio.proc.kill();
        startFakeAudio.proc = null;
      }
    }
  }

  function file_exists(path) {
    var file = Cc["@mozilla.org/file/local;1"].
      createInstance(Ci.nsILocalFile);
    file.initWithPath(path);
    return file.exists();
  }
   
  /**
   * Determine if loopback devices are available and set
   * LOOPBACK_ENABLED appropriately.
   */
  function _initLoopbackDevices(aCallback) {
   function run_callback(arg) {
     try {
       aCallback(arg);
     }
     catch (err) {
       unexpectedCallbackAndFinish(err);
     }
   }
  
   if (FAKE_ENABLED &&
       !loopback_checked &&
       audio_device_id != null &&
       required_executables.every(file_exists)) {
     loopback_checked = true;
     // See if both audio_device_id and video_device_id are available.
     SpecialPowers.prototype.wrap(navigator).mozGetUserMediaDevices(function(devices) {
       var audiodevice = null;
       for (var d of devices) {
         var dev = SpecialPowers.prototype.wrap(d).QueryInterface(Ci.nsIMediaDevice);
         if (dev.type == "audio" && dev.id == audio_device_id) {
           audiodevice = SpecialPowers.prototype.unwrap(dev);
         }
       }
  
       if (audiodevice ) {
         // Found both the video and audio devices.
         FAKE_ENABLED = false;
         LOOPBACK_ENABLED = true;
         loopback_devices = {audio: audiodevice };
       }
       run_callback(loopback_devices);
     }, unexpectedCallbackAndFinish);
     return;
   }
   // No loopback devices on this platform, or we've already looked for them.
   loopback_checked = true;
   run_callback(loopback_devices);
 }

/**
  * Wrapper function for mozGetUserMedia to allow a singular area of control
  * for determining whether we run this with fake devices or not, and also
  * for specifying loopback devices if present.
  *
  * @param {Dictionary} constraints
  *        The constraints for this mozGetUserMedia callback
  * @param {Function} onSuccess
  *        The success callback if the stream is successfully retrieved
  * @param {Function} onError
  *        The error callback if the stream fails to be retrieved
  */
 function getUserMedia(constraints, onSuccess, onError) {
   _initLoopbackDevices(function(devices) {
     if (LOOPBACK_ENABLED) {
       if (constraints.video) {
         constraints.videoDevice = devices.video;
         startFakeVideo();
       }
       if (constraints.audio) {
         constraints.audioDevice = devices.audio;
         startFakeAudio();
       }
     } else if (FAKE_ENABLED) {
       constraints["fake"] = true;
     }
     navigator.mozGetUserMedia(constraints, onSuccess, onError);
   });
 }
  
 return getUserMedia;

})();


/**
 * Create the necessary HTML elements for head and body as used by Mochitests
 *
 * @param {object} meta
 *        Meta information of the test
 * @param {string} meta.title
 *        Description of the test
 * @param {string} [meta.bug]
 *        Bug the test was created for
 * @param {boolean} [meta.visible=false]
 *        Visibility of the media elements
 */
function createHTML(meta) {
  var test = document.getElementById('test');

  // Create the head content
  var elem = document.createElement('meta');
  elem.setAttribute('charset', 'utf-8');
  document.head.appendChild(elem);

  var title = document.createElement('title');
  title.textContent = meta.title;
  document.head.appendChild(title);

  // Create the body content
  var anchor = document.createElement('a');
  anchor.setAttribute('target', '_blank');

  if (meta.bug) {
    anchor.setAttribute('href', 'https://bugzilla.mozilla.org/show_bug.cgi?id=' + meta.bug);
  }

  anchor.textContent = meta.title;
  document.body.insertBefore(anchor, test);

  var display = document.createElement('p');
  display.setAttribute('id', 'display');
  document.body.insertBefore(display, test);

  var content = document.createElement('div');
  content.setAttribute('id', 'content');
  content.style.display = meta.visible ? 'block' : "none";
  document.body.appendChild(content);
}


/**
 * Create the HTML element if it doesn't exist yet and attach
 * it to the content node.
 *
 * @param {string} type
 *        Type of media element to create ('audio' or 'video')
 * @param {string} label
 *        Description to use for the element
 * @return {HTMLMediaElement} The created HTML media element
 */
function createMediaElement(type, label) {
  var id = label + '_' + type;
  var element = document.getElementById(id);

  // Sanity check that we haven't created the element already
  if (element)
    return element;

  element = document.createElement(type === 'audio' ? 'audio' : 'video');
  element.setAttribute('id', id);
  element.setAttribute('height', 100);
  element.setAttribute('width', 150);
  element.setAttribute('controls', 'controls');
  document.getElementById('content').appendChild(element);

  return element;
}

/**
 * Checks that the media stream tracks have the expected amount of tracks
 * with the correct kind and id based on the type and constraints given.
 *
 * @param {Object} constraints specifies whether the stream should have
 *                             audio, video, or both
 * @param {String} type the type of media stream tracks being checked
 * @param {sequence<MediaStreamTrack>} mediaStreamTracks the media stream
 *                                     tracks being checked
 */
function checkMediaStreamTracksByType(constraints, type, mediaStreamTracks) {
  /*
  if(constraints[type]) {
    is(mediaStreamTracks.length, 1, 'One ' + type + ' track shall be present');

    if(mediaStreamTracks.length) {
      is(mediaStreamTracks[0].kind, type, 'Track kind should be ' + type);
      ok(mediaStreamTracks[0].id, 'Track id should be defined');
    }
  } else {
    is(mediaStreamTracks.length, 0, 'No ' + type + ' tracks shall be present');
  }
  */
}

/**
 * Check that the given media stream contains the expected media stream
 * tracks given the associated audio & video constraints provided.
 *
 * @param {Object} constraints specifies whether the stream should have
 *                             audio, video, or both
 * @param {MediaStream} mediaStream the media stream being checked
 */
function checkMediaStreamTracks(constraints, mediaStream) {
  checkMediaStreamTracksByType(constraints, 'audio',
    mediaStream.getAudioTracks());
  checkMediaStreamTracksByType(constraints, 'video',
    mediaStream.getVideoTracks());
}

/**
 * Utility methods
 */

/**
 * Returns the contents of a blob as text
 *
 * @param {Blob} blob
          The blob to retrieve the contents from
 * @param {Function} onSuccess
          Callback with the blobs content as parameter
 */
function getBlobContent(blob, onSuccess) {
  var reader = new FileReader();

  // Listen for 'onloadend' which will always be called after a success or failure
  reader.onloadend = function (event) {
    onSuccess(event.target.result);
  };

  reader.readAsText(blob);
}

/**
 * Generates a callback function fired only under unexpected circumstances
 * while running the tests. The generated function kills off the test as well
 * gracefully.
 *
 * @param {String} [message]
 *        An optional message to show if no object gets passed into the
 *        generated callback method.
 */
function unexpectedCallbackAndFinish(message) {
  var stack = new Error().stack.split("\n");
  stack.shift(); // Don't include this instantiation frame

  /**
   * @param {object} aObj
   *        The object fired back from the callback
   */
  return function (aObj) {
    if (aObj && aObj.name && aObj.message) {
      //ok(false, "Unexpected callback for '" + aObj.name + "' with message = '" +
      //   aObj.message + "' at " + JSON.stringify(stack));
    } else {
      //ok(false, "Unexpected callback with message = '" + message +
      //   "' at: " + JSON.stringify(stack));
    }
    //SimpleTest.finish();
  }
}

/**
 * Generates a callback function fired only for unexpected events happening.
 *
 * @param {String} description
          Description of the object for which the event has been fired
 * @param {String} eventName
          Name of the unexpected event
 */
function unexpectedEventAndFinish(message, eventName) {
  var stack = new Error().stack.split("\n");
  stack.shift(); // Don't include this instantiation frame

  return function () {
    //ok(false, "Unexpected event '" + eventName + "' fired with message = '" +
    //   message + "' at: " + JSON.stringify(stack));
    //SimpleTest.finish();
  };
}
