var motorRate = 0;

var absolute = false;
var alpha = 0;
var beta = 0;
var gamma = 0;

function handleOrientation(event) {
    absolute = event.absolute;
    alpha    = event.alpha;
    beta     = event.beta;
    gamma    = event.gamma;

    $('#abs').text(absolute);
    $('#alpha').text(alpha.toString());
    $('#beta').text(beta.toString());
    $('#gamma').text(gamma.toString());

    sendCommand(cropBeta()/90, motorRate);
}

window.addEventListener("deviceorientation", handleOrientation, true);

function cropBeta() {
    return Math.max(Math.min(beta, 90), -90);
}

//ms
var sendInterval = 100;
setInterval(
    function() {
        sendCommand(cropBeta()/90, motorRate);
    },
    sendInterval
);

$(
    function() {
        $("#motorRate").on('input', function() {
            motorRate = $("#motorRate").val();
            sendCommand(cropBeta()/90, motorRate);
        });
    }
)
