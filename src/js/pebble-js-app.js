// Function to send a message to the Pebble using AppMessage API
function sendMessage() {
    Pebble.sendAppMessage({"status": 0});
    
    // PRO TIP: If you are sending more than one message, or a complex set of messages,
    // it is important that you setup an ackHandler and a nackHandler and call
    // Pebble.sendAppMessage({ /* Message here */ }, ackHandler, nackHandler), which
    // will designate the ackHandler and nackHandler that will be called upon the Pebble
    // ack-ing or nack-ing the message you just sent. The specified nackHandler will
    // also be called if your message send attempt times out.
}

function sendPostRequestWithBody(body) {
    var req = new XMLHttpRequest();
    req.open("POST", "http://transilien.ods.ocito.com/ods/transilien/iphone", false);
    req.send(null);
    console.log(req.responseText);
    return req.responseText;
}

function requestNextTrains(from, to) {
    
}

function requestTrainDetails(trainNumber) {
    
}

// Called when JS is ready
Pebble.addEventListener("ready",
                        function(e) {
                        });

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
                        function(e) {
                        console.log("Received Status: " + e.payload.status);
                        sendMessage();
                        });