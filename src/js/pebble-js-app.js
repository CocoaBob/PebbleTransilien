function sendPostRequestWithBody(body) {
    var req = new XMLHttpRequest();
    req.open("POST", "http://transilien.ods.ocito.com/ods/transilien/iphone", false);
    req.send(null);
    console.log(req.responseText);
    return req.responseText;
}

function requestNextTrains(from, to) {
    Pebble.sendAppMessage({"MESSAGE_KEY_NEXT_TRAINS": 0});
}

function requestTrainDetails(trainNumber) {
    Pebble.sendAppMessage({"MESSAGE_KEY_TRAIN_DETAILS": 0});
}

// Called when JS is ready
Pebble.addEventListener("ready",
                        function(e) {
                        });

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
                        function(e) {
                        console.log("Received message: " + JSON.stringify(e.payload));
                        if (e.payload.MESSAGE_KEY_TYPE == 0) { // Next trains
                        requestNextTrains(0,0);
                        } else if (e.payload.MESSAGE_KEY_TYPE == 1) { // Train details
                        requestTrainDetails(0);
                        }
                        });