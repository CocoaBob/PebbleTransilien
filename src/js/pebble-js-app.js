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
//            Pebble.sendAppMessage({"MESSAGE_KEY_NEXT_TRAINS": 0});
//        } catch (e) {
//        }
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
    console.log(responseText);
    var responseJson = JSON.parse(responseText);
}

function requestTrainDetails(trainNumber) {
    Pebble.sendAppMessage({"MESSAGE_KEY_TRAIN_DETAILS": 0});
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
                        if (e.payload.MESSAGE_KEY_TYPE == 0) { // Next trains
                            var code_from = e.payload.MESSAGE_KEY_CODE_FROM;
                            code_from = bin2Str(code_from);
                            var code_to = e.payload.MESSAGE_KEY_CODE_TO;
                            code_to = (typeof(code_to) == 'undefined')?"":bin2Str(code_to);
                            requestNextTrains(code_from,code_to);
                        } else if (e.payload.MESSAGE_KEY_TYPE == 1) { // Train details
                            requestTrainDetails(0);
                        }
                        });