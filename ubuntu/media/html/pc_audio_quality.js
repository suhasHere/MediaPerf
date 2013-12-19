
var pc1;
var pc2;
var pc1_offer;
var pc2_answer;

function failed(code) {
  log("Failure callback: " + code);
}

// pc1.createOffer finished, call pc1.setLocal
function step1(offer) {
 pc1_offer = offer;
 pc1.setLocalDescription(offer, step2, failed);
}

// pc1.setLocal finished, call pc2.setRemote
function step2() {
  pc2.setRemoteDescription(pc1_offer, step3, failed);
};

// pc2.setRemote finished, call pc2.createAnswer
function step3() {
  pc2.createAnswer(step4, failed);
}

// pc2.createAnswer finished, call pc2.setLocal
function step4(answer) {
  pc2_answer = answer;
  pc2.setLocalDescription(answer, step5, failed);
}

// pc2.setLocal finished, call pc1.setRemote
function step5() {
  pc1.setRemoteDescription(pc2_answer, step6, failed);
}

// pc1.setRemote finished, media should be running!
function step6() {
  log("Peer Connection Signaling Completed Successfully !");
}

function end(status) {
}

// Test Function
var audioPCQuality = function() {
  var test = this
  var localaudio = document.createElement('audio');
  var pc1audio   = document.createElement('audio');
  var pc2audio   = document.createElement('audio');

  pc1 = new mozRTCPeerConnection();
  pc2 = new mozRTCPeerConnection();

  pc1.onaddstream = function(obj) {
    log("pc1 got remote stream from pc2 " + obj.type);
  }

  pc2.onaddstream = function(obj) {
    log("pc2 got remote stream from pc1 " + obj.type);
    pc1audio.mozSrcObject = obj.stream;
    pc1audio.play();
  }

  localaudio.src = test.src 

  initiateAudioRecording(test);
  if (test.failed) {
    return;
  }

  localaudio.addEventListener('ended', function(evt) {
    // stop the recorder
    cleanupAudioRecording(test);
    if (!test.failed) {
      getPESQScores(test);
      updateResults(test);
      runNextTest();
    }
  });

  localaudio.vol = 0.9;
  localAudioStream = localaudio.mozCaptureStreamUntilEnded();
  localaudio.play();
  //audio mediastream from the <audio> to the peer connection
  pc1.addStream(localAudioStream);
  navigator.mozGetUserMedia({audio:true,fake:true}, function(audio2) {
    pc2.addStream(audio2);
    // Start the signaling.
    pc1.createOffer(step1, failed);
  }, failed);
};
