/*
 vim:se?t ts=2 sw=2 sts=2 et cindent:
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/. 
*/

// 1. audioPlayback Test plays an input file from a given
// <audio> and records the same to compute PESQ scores
var audioPlayback = function() {
  var test = this;
  var cleanupTimeout = 5000;
  var audio = document.createElement('audio');

  // start audio recorder
  initiateAudioRecording(test);
  if (test.failed) {
    return;
  }

  audio.addEventListener('ended', function(evt) {
    // stop the recorder
    cleanupAudioRecording(test);
    if (!test.failed) {
      getPESQScores(test);
      updateResults(test);
      runNextTest();
    }
  });
          
  audio.volume = 0.9;
  audio.src = test.src;
  audio.play();
};
