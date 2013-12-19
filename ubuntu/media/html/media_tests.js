/*
 vim:se?t ts=2 sw=2 sts=2 et cindent:
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/. 
*/

// Results to be sent to Talos
var results = {};
var baseUrl = 'http://localhost:16932';
var testsTable = document.getElementById('tests');

function log(msg) {
  //var div = document.getElementById("log");
  //div.innerHTML = div.innerHTML + "<p>" + msg + "</p>";
}

// Updates results to be used for Talos eventually
var updateResults = function(test) {
  if (!test.failed) {
    results[test.name] = JSON.parse(test.http_response);
    //update tests table
    var row = document.createElement('tr');
    var nameCell = document.createElement('td');
    nameCell.textContent = test.name;
    var outcomeCell = document.createElement('td');
    outcomeCell.textContent = test.http_response;
    row.appendChild(nameCell);
    row.appendChild(outcomeCell);
    testsTable.appendChild(row);
  }
};


var tests = [
  { 
      name: 'Audio_Playback_Quality',
      src: 'input16.wav',
      timeout: 10,
      test: audioPlayback
  },
  { 
      name: 'Audio_PC_Quality',
      src: 'input16.wav',
      timeout: 10,
      test: audioPCQuality 
  },
];

var nextTestIndex = 0;


var runNextTest = function() {
  var testIndex = nextTestIndex++;
  if (testIndex >= tests.length) {
    var obj = encodeURIComponent(JSON.stringify(results));
    var url = "http://localhost:15707/results?" + obj
    sendTalosResults(url, obj);
    // let's clean up the test framework
    var request = new XMLHttpRequest();
    var url = baseUrl + '/server/config/stop' 
    request.open('GET', url, false);
    request.send();
  }
  var test = tests[testIndex];
  test.test();
};

setTimeout(runNextTest, 100);

