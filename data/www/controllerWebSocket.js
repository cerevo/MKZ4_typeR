var ws_uri = "ws://" + location.hostname + "/ws";
var webSocket = null;

function connect() {
    if(webSocket != null) {
        return;
    }
    $("#debugDisp").text("connecting");
    webSocket = new WebSocket(ws_uri);
    webSocket.onopen = onOpen;
    webSocket.onmessage = onMessage;
    webSocket.onclose = onClose;
    webSocket.onerror = onError;
}

function onOpen(event) {
    $("#debugDisp").text("connection was opened");
    $("#stringDisplay").text("connected");

    sendXY(0, 0);
}

function onClose(event) {
    $("#debugDisp").text("connection was closed, reconnection will be tried every 3 seconds");
    $("#stringDisplay").text("disconnected");
    webSocket = null;
    setTimeout(connect, 3000);
}

function onMessage(event) {
    var str = "["+ event.origin +"]" + " " + event.data;
    $("#debugDisp").text(str);
}
function onError(event) {
    $("#debugDisp").text("error");
    if(webSocket != null) {
        webSocket.close();
        webSocket = null;
    } }

$(connect());

function sendCommand(servoRate, motorRate) {
    if(webSocket == null || webSocket.readyState != WebSocket.OPEN) {
        return;
    }
    var message = JSON.stringify(
        {
            "servoRate": servoRate,
            "motorRate": motorRate,
            "Ping": {}
        }
    );
    webSocket.send(message);
}
function sendPing() {
    if(webSocket == null || webSocket.readyState != WebSocket.OPEN) {
        return;
    }
    var message = JSON.stringify(
        {
            "Ping": {}
        }
    );
    webSocket.send(message);
}

