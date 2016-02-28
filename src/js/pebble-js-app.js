var _allStationCodes = ["AB","ABL","ACW","AEE","AEH","AGV","AHM","ALC","ANY","APK","ARK","ARP","ARW","ATH","ATW","AUU","AUW","AVF","BAM","BBN","BCO","BDE","BDY","BEC","BEL","BFM","BFX","BGK","BGV","BIH","BIS","BJN","BJR","BKR","BLA","BLU","BNR","BNY","BOF","BOM","BQA","BQC","BQQ","BQV","BRK","BRN","BRW","BSO","BSR","BSW","BUR","BVA","BVI","BVJ","BWI","BWR","BXG","BXI","BXN","BXP","BXR","BY","BYS","CAZ","CBK","CBV","CEG","CEJ","CES","CEV","CEX","CFD","CFO","CGG","CGJ","CGP","CGW","CHK","CHQ","CHR","CHV","CJN","CJR","CJV","CL","CLC","CLL","CLR","CLX","CLY","CMA","CME","CO","COE","COJ","CPA","CPK","CPM","CPO","CPW","CQQ","CSG","CSH","CTH","CUF","CVF","CVI","CVW","CWJ","CXA","CYC","CYP","CYQ","CYV","CYZ","D","DA","DAM","DDI","DEU","DFR","DMO","DRN","DX","ECZ","ELW","ELY","EM","EN","EPL","EPO","EPV","ERA","ERE","ERM","ERT","ESO","ETP","ETY","EVC","EVR","EY","EYO","EYS","FAF","FFY","FMN","FMP","FMY","FNR","FON","FPB","FPN","FPO","FSB","GAJ","GAQ","GAW","GBG","GBI","GCM","GCR","GDS","GEN","GGG","GGV","GIF","GIS","GMC","GN","GNX","GOU","GPA","GRL","GTL","GU","GUW","GYN","GZ","GZA","HAQ","HAR","HER","HOA","HRY","HSL","IAC","IAP","IBM","IGY","INV","IPO","ISP","ISY","IV","JAS","JUZ","JVL","JVR","JY","KOU","KRW","KVE","LAD","LBJ","LBT","LCB","LDU","LEG","LFA","LFC","LFJ","LFM","LGK","LGY","LIE","LIM","LIU","LJA","LJU","LMU","LNX","LON","LOV","LPE","LPN","LQK","LQN","LSD","LSI","LSW","LUZ","LVZ","LWA","LXJ","LYO","LYQ","LYV","LZO","LZV","MAE","MAG","MAL","MAQ","MBP","MBR","MDN","MDS","MEA","MEL","MFA","MFL","MFY","MHD","MJM","MJW","MKN","MKU","MLB","MLF","MLM","MLR","MLV","MNY","MOF","MOR","MP","MPJ","MPU","MRK","MRT","MS","MSN","MSO","MSX","MTE","MTQ","MTU","MVC","MVH","MVP","MVW","MW","MWI","MWO","MXK","MY","MYD","NAA","NAF","NAN","NAU","NC1","NC2","NC3","NC4","NC5","NC6","NG","NGM","NH","NIO","NLP","NO","NPT","NSL","NSP","NSY","NTN","NUE","NUN","NYC","NYG","NYP","NZL","OBP","OGB","ORM","ORS","ORY","OSN","OY","OZF","PAA","PAN","PAW","PAX","PAZ","PBY","PCX","PDM","PE","PEB","PEX","PG","PIE","PJ","PKY","PLB","PLY","PMP","PNB","PNO","POA","POP","PPD","PPT","PRF","PRO","PRQ","PRR","PRU","PRY","PSE","PSL","PSY","PTC","PTX","PV","PVA","PWR","PWZ","PXO","PYO","PZB","RBI","RBT","RF","RIS","RNS","ROB","RSB","RSS","RSY","RVM","RYR","SAO","SCD","SCR","SCW","SDE","SEV","SF","SFD","SGM","SGT","SHL","SHO","SKX","SLF","SLL","SLT","SME","SNB","SNM","SNN","SOA","SOS","SPP","SQY","SUR","SVL","SVR","SWY","SXE","SXG","TAE","TLP","TMR","TNT","TOC","TOU","TPA","TRH","TSS","TVO","TVY","US","VAI","VBB","VBO","VBV","VC","VCN","VCX","VD","VDA","VDE","VDF","VDO","VDV","VEH","VEP","VET","VFD","VFG","VFR","VGL","VGS","VIB","VII","VMD","VMK","VMS","VNC","VNL","VOM","VPN","VRD","VRG","VRI","VSG","VSM","VSS","VSW","VTV","VUN","VVG","VW","VWC","VWT","VXS","VY","VYL","WEE","XBY","XCS","XFA","XMC","XND","XOA","XPY","YES","ZTN","ZUB"];

var _request;
var _request_type;

function stationCode2Index(code) {
    return _allStationCodes.indexOf(code.toUpperCase());
}

function bin2Str(array) {
    return String.fromCharCode.apply(String, array);
}

function str2bin(str) {
    var bytes = [];
    for (var i = 0; i < str.length; ++i) {
        var char = str.charCodeAt(i);
        if (char >= 0 && char <= 255) {
            bytes.push(char);
        }
    }
    return makeCString(bytes);
}

function trimNullCString(bytes) {
    var size = bytes.length;
    while (size > 0 && bytes[0] == 0) {
        bytes.splice(0,1);
        --size;
    }
    while (size > 0 && bytes[size - 1] == 0) {
        bytes.splice(size - 1,1);
        --size;
    }
    return bytes;
}

function makeCString(bytes) {
    bytes = trimNullCString(bytes);
    bytes.push(0);
    return bytes;
}

function int322bin(int32) {
    return [(int32 >> 24) & 0xff, (int32 >> 16) & 0xff, (int32 >> 8) & 0xff, int32 & 0xff];
}

function int162bin(int16) {
    return [(int16 >> 8) & 0xff, int16 & 0xff];
}

function parseTrainHour(str) {
    var m = str.match(/^(\d{1,2})\/(\d{1,2})\/(\d{4}) (\d{2}):(\d{2})$/);
    if (m) {
        return new Date(m[3],   // Year
                        m[2]-1, // Month
                        m[1],   // Day
                        m[4],   // Hour
                        m[5],   // Minute
                        0,      // Second
                        0);     // Millisecond
    }
    
    m = str.match(/^(\d{2}):(\d{2})$/);
    if (m) {
        var now = new Date();
        var date = new Date(now.getUTCFullYear(),   // Year
                            now.getUTCMonth(),      // Month
                            now.getUTCDate(),       // Day
                            m[1],                   // Hour
                            m[2],                   // Minute
                            0,                      // Second
                            0);                     // Millisecond
        if ((now.getUTCHours() - m[1]) > 12) {
            date.setTime(date.getTime() + 86400000);
        }
        return date;
    }
    
    return null;
}

function parseTrainInfo(info) {
    var returnString = info["titre"];
    returnString += "\n";
    
    var content = info["contenu"];
    
    // Delete the contents like
    // <a href=\"url\">Pour les horaires du dimanche dans le sens Paris - Gisors, cliquer ici pour télécharger l'affiche pdf</a>
    var patternToDelete = /<[^>]+>[^<]*pdf[^<]*<\/[^>]+>\.?/ig;
    content = content.replace(patternToDelete, "");
    
    // Keep the contents like
    // Pendant les travaux, covoiturez avec <a href=\"url\">iDVROOM</a>
    var patternToRemoveTags = /<[^>]+>([^<]*)<\/[^>]+>/ig;
    content = content.replace(patternToRemoveTags, function(match,$1) { return $1; } );
    
    // Remove empty rows
    var patterToReduce = /\n+[\s*\n]*/ig;
    content = content.replace(patterToReduce, "\n\n");
    
    returnString += content;
    
    return returnString;
}

function abortLastRequest() {
    if (_request != null) {
        _request.abort();
        _request = null;
    }
}

function sendAppMessageForError(errorCode) {
    Pebble.sendAppMessage({
                          "MESSAGE_KEY_RESPONSE_TYPE": -1,
                          "MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT":1,
                          "MESSAGE_KEY_RESPONSE_PAYLOAD":errorCode
                          });
}

function sendAppMessageForNextTrains(responseText) {
    response = JSON.parse(responseText)[0];
    var dataArray = response["data"];
    if (dataArray == null) {
        sendAppMessageForError(2);
        return;
    }
    
    var payloadLength = dataArray.length;
    // To avoid consuming too much ram of Aplite, limit to 6 trains
    if (Pebble.getActiveWatchInfo && Pebble.getActiveWatchInfo().platform === 'aplite' && payloadLength > 6) {
        payloadLength = 6;
    }
    // Prepare message
    var message = {
        "MESSAGE_KEY_RESPONSE_TYPE": _request_type,
        "MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT":payloadLength
    };
    // Add next trains
    var payloadKey = 1000;// 1000 = "MESSAGE_KEY_RESPONSE_PAYLOAD";
    for(var index in dataArray) {
        if (index >= payloadLength) {
            break;
        }
        var nextTrainDict = dataArray[index];
        
        var itemData = [];
        
        // Train mission code
        var trainMissionCode = nextTrainDict["trainMissionCode"];
        itemData = itemData.concat(str2bin(trainMissionCode));
        // Train hour
        var trainHour = nextTrainDict["trainHour"];
        trainHour = parseTrainHour(trainHour);
        if (trainHour != null) {
            trainHour = trainHour.getTime() / 1000;
            trainHour = int322bin(trainHour);
        } else {
            trainHour = [];
        }
        // trainHour must be 5 bytes
        while (trainHour.length < 5) {
            trainHour.push(0);
        }
        itemData = itemData.concat(trainHour);
        // Train platform
        var trainLane = nextTrainDict["trainLane"];
        var trainDock = nextTrainDict["trainDock"];
        var trainPlatform = (trainLane != null)?trainLane:((trainDock != null)?trainDock:"");
        itemData = itemData.concat(str2bin(trainPlatform.substr(0,2)));
        // Train terminus
        var trainTerminus = nextTrainDict["trainTerminus"];
        if (trainTerminus != null) {
            trainTerminus = stationCode2Index(trainTerminus);
            trainTerminus = int162bin(trainTerminus)
            trainTerminus = makeCString(trainTerminus);
        } else {
            trainTerminus = "";
        }
        itemData = itemData.concat(trainTerminus);
        // Train number
        var trainNumber = nextTrainDict["trainNumber"];
        itemData = itemData.concat(str2bin(trainNumber));
        // Train mention (canceled or delayed)
        var trainMention = nextTrainDict["trainMention"];
        trainMention = (trainMention != null)?trainMention:"";
        itemData = itemData.concat(str2bin(trainMention));
        
        message[+payloadKey + +index] = itemData;
    }
    // Extra info
    if (Pebble.getActiveWatchInfo && Pebble.getActiveWatchInfo().platform !== 'aplite') {
        var infos = response["listOfMap"];
        var trainInfo = [];
        for(var index in infos) {
            var info = infos[index];
            if (index != 0) {
                trainInfo = trainInfo.concat(str2bin("\n"));
            }
            var parsedTrainInfo = parseTrainInfo(info);
            trainInfo = trainInfo.concat(str2bin(parsedTrainInfo));
        }
        message["MESSAGE_KEY_RESPONSE_EXTRA"] = trainInfo;
    }
    // Send message
    try {
        Pebble.sendAppMessage(message);
    } catch (e) {
        console.log(e.message);
    }
}

function requestNextTrains(from, to) {
    abortLastRequest();
    
    _request = new XMLHttpRequest();
    _request.open("POST", "http://transilien.ods.ocito.com/ods/transilien/iphone", true);
    _request.setRequestHeader("Content-Type", "application/json; charset=utf-8");
    _request.onload = function (e) {
        if (_request.readyState == 4 && _request.status == 200) {
            sendAppMessageForNextTrains(_request.responseText);
        } else {
            sendAppMessageForError(1);
            return;
        }
    };
    
    var postBody = JSON.stringify([{"target": "/transilien/getNextTrains", "map": { "codeDepart": from, "codeArrivee": to, "theoric": "false" }}]);
    _request.send(postBody);
}

function sendAppMessageForTrainDetails(responseText) {
    var parseResult = JSON.parse(responseText)[0];
    var dataArray = parseResult["data"];
    if (dataArray == null) {
        sendAppMessageForError(2);
        return;
    }
    var payloadLength = dataArray.length;
    var message = {
        "MESSAGE_KEY_RESPONSE_TYPE": _request_type,
        "MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT":payloadLength
    };
    var payloadKey = 1000;// 1000 = "MESSAGE_KEY_RESPONSE_PAYLOAD";
    for(var index in dataArray){
        var nextTrainDict = dataArray[index];
        
        var itemData = [];
        
        // Time
        var time = nextTrainDict["time"];
        time = parseTrainHour(time);
        if (time != null) {
            time = time.getTime() / 1000;
            time = int322bin(time);
        } else {
            time = [];
        }
        // time must be 5 bytes
        while (time.length < 5) {
            time.push(0);
        }
        itemData = itemData.concat(time);
        
        // Station
        var station = nextTrainDict["codeGare"];
        station = stationCode2Index(station);
        station = int162bin(station)
        station = makeCString(station);
        itemData = itemData.concat(station);
        
        message[+payloadKey + +index] = itemData;
    }
    
    try {
        Pebble.sendAppMessage(message);
    } catch (e) {
        console.log(e.message);
    }
}

function requestTrainDetails(trainNumber) {
    abortLastRequest();
    
    _request = new XMLHttpRequest();
    _request.open("POST", "http://transilien.ods.ocito.com/ods/transilien/iphone", true);
    _request.setRequestHeader("Content-Type", "application/json; charset=utf-8");
    _request.onload = function (e) {
        if (_request.readyState == 4 && _request.status == 200) {
            sendAppMessageForTrainDetails(_request.responseText);
        } else {
            sendAppMessageForError(1);
            return;
        }
    };
    
    var postBody = JSON.stringify([{"target": "/transilien/getTrainDetails", "map": { "trainNumber": trainNumber, "theoric": "false"}}]);
    _request.send(postBody);
}

// Called when JS is ready
Pebble.addEventListener("ready",
                        function(e) {
                        Pebble.sendAppMessage({});
                        });

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
                        function(e) {
                        if (!e.payload) return null;
                        
                        _request_type = e.payload.MESSAGE_KEY_REQUEST_TYPE;
                        if (_request_type == 0 ||
                            _request_type == 1) { // Next trains
                            var code_from = e.payload.MESSAGE_KEY_REQUEST_CODE_FROM;
                            code_from = bin2Str(code_from);
                            var code_to = e.payload.MESSAGE_KEY_REQUEST_CODE_TO;
                            code_to = (typeof(code_to) == 'undefined')?"":bin2Str(code_to);
                            requestNextTrains(code_from,code_to);
                        } else if (_request_type == 2) { // Train details
                            var train_number = e.payload.MESSAGE_KEY_REQUEST_TRAIN_NUMBER;
                            train_number = bin2Str(train_number);
                            requestTrainDetails(train_number);
                        }
                        });