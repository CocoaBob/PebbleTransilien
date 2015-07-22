function parseTrainHour(str) {
    var m = str.match(/^(\d{1,2})\/(\d{1,2})\/(\d{4}) (\d{2}):(\d{2})$/);
    return (m) ? new Date(m[3], m[2]-1, m[1], m[4], m[5], 0) : null;
}

function timeIntervalSinceNow(trainHour) {
    if (trainHour == null) return;
    var now = new Date();
    var timeDiff = trainHour.getTime() - now.getTime();
    var isNegative = timeDiff < 0;
    timeDiff = Math.abs(timeDiff);
    
    var hh = Math.floor(timeDiff / 3600000);
    if(hh < 10) {
        hh = '0' + hh;
    }
    timeDiff -= hh * 3600000;
    var mm = Math.floor(timeDiff / 60000);
    if(mm < 10) {
        mm = '0' + mm;
    }
    return (isNegative?"-":"") + hh + ":" + mm;
}

function requestNextTrains(from, to) {
//    var req = new XMLHttpRequest();
//    req.open("POST", "http://transilien.ods.ocito.com/ods/transilien/iphone", false);
//    req.setRequestHeader("Content-Type", "application/json; charset=utf-8");
//    
//    var data = JSON.stringify([{"target": "/transilien/getNextTrains", "map": { "codeDepart": from, "codeArrivee": to }}]);
//    req.send(data);
//    if (req.readyState == 4 && req.status == 200) {
//        var response = JSON.parse(req.responseText);
//        try {
//            Pebble.sendAppMessage({0: 0});
//        } catch (e) {
//        }
//    } else {
//         // TODO: Failed
//    }
    
    var responseText = '[\
{\
"binary": null,\
"data": [\
{\
"trainDock": "A",\
"trainHour": "22/07/2015 18:24",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "POPU",\
"trainNumber": "135984",\
"trainTerminus": "PSL",\
"type": "R"\
},\
{\
"trainDock": "A",\
"trainHour": "22/07/2015 18:27",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "POPA",\
"trainNumber": "131722",\
"trainTerminus": "PSL",\
"type": "R"\
},\
{\
"trainDock": "C",\
"trainHour": "22/07/2015 18:31",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "POBA",\
"trainNumber": "133758",\
"trainTerminus": "PSL",\
"type": "R"\
},\
{\
"trainDock": "D",\
"trainHour": "22/07/2015 18:33",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "SEBO",\
"trainNumber": "134657",\
"trainTerminus": "SNB",\
"type": "R"\
},\
{\
"trainDock": "B",\
"trainHour": "22/07/2015 18:34",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "FOPE",\
"trainNumber": "135981",\
"trainTerminus": "MLF",\
"type": "R"\
},\
{\
"trainDock": "A",\
"trainHour": "22/07/2015 18:43",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "POPU",\
"trainNumber": "135556",\
"trainTerminus": "PSL",\
"type": "R"\
},\
{\
"trainDock": "C",\
"trainHour": "22/07/2015 18:46",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "POBA",\
"trainNumber": "133766",\
"trainTerminus": "PSL",\
"type": "R"\
},\
{\
"trainDock": "B",\
"trainHour": "22/07/2015 18:52",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "FOPE",\
"trainNumber": "135987",\
"trainTerminus": "MLF",\
"type": "R"\
},\
{\
"trainDock": "D",\
"trainHour": "22/07/2015 18:53",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "SEBO",\
"trainNumber": "134671",\
"trainTerminus": "SNB",\
"type": "R"\
},\
{\
"trainDock": "C",\
"trainHour": "22/07/2015 19:00",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "POBA",\
"trainNumber": "133776",\
"trainTerminus": "PSL",\
"type": "R"\
},\
{\
"trainDock": "A",\
"trainHour": "22/07/2015 19:00",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "POPU",\
"trainNumber": "135994",\
"trainTerminus": "PSL",\
"type": "R"\
},\
{\
"trainDock": "A",\
"trainHour": "22/07/2015 19:03",\
"trainLane": null,\
"trainMention": null,\
"trainMissionCode": "POPA",\
"trainNumber": "131738",\
"trainTerminus": "PSL",\
"type": "R"\
}\
]\
}\
]'
    var message = {
        "MESSAGE_KEY_RESPONSE_TYPE": 0,
        "MESSAGE_KEY_RESPONSE_PAYLOAD_LENGTH":payloadLength
    };
    var dataArray = JSON.parse(responseText)[0]["data"];
    var payloadLength = dataArray.length;
    var payloadBeginNumber = 202;// 202 = "MESSAGE_KEY_RESPONSE_PAYLOAD";
    for(var index in dataArray){
        var nextTrain = {};
        for(var attrName in dataArray[index]){
            var attrValue = dataArray[index][attrName];
            if (attrName == "trainLane") {
                if (attrValue != null) {
                    nextTrain["3"] = attrValue;
                }
            } else if (attrName == "trainHour") {
                nextTrain["2"] = timeIntervalSinceNow(parseTrainHour(attrValue));
            } else if (attrName == "trainDock") {
                if (attrValue != null) {
                    nextTrain["3"] = attrValue;
                }
            } else if (attrName == "trainMissionCode") {
                nextTrain["1"] = attrValue;
            } else if (attrName == "trainNumber") {
                nextTrain["0"] = attrValue;
            } else if (attrName == "trainTerminus") {
                nextTrain["4"] = attrValue;
            }
        }
        message[+payloadBeginNumber + +index] = JSON.stringify(nextTrain);
    }
    
    console.log(JSON.stringify(message));
    Pebble.sendAppMessage(message);
}

function requestTrainDetails(trainNumber) {
    Pebble.sendAppMessage({"MESSAGE_KEY_RESPONSE_TYPE": 1,
                          "MESSAGE_KEY_RESPONSE_PAYLOAD": 0});
}

// Called when JS is ready
Pebble.addEventListener("ready",
                        function(e) {
                        });

function bin2Str(array) {
    return String.fromCharCode.apply(String, array);
}

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
                        function(e) {
                        if (!e.payload) return null;
                        if (e.payload.MESSAGE_KEY_REQUEST_TYPE == 0) { // Next trains
                            var code_from = e.payload.MESSAGE_KEY_REQUEST_CODE_FROM;
                            code_from = bin2Str(code_from);
                            var code_to = e.payload.MESSAGE_KEY_REQUEST_CODE_TO;
                            code_to = (typeof(code_to) == 'undefined')?"":bin2Str(code_to);
                            requestNextTrains(code_from,code_to);
                        } else if (e.payload.MESSAGE_KEY_REQUEST_TYPE == 1) { // Train details
                            requestTrainDetails(0);
                        }
                        });