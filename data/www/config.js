function getConfigParameters() {
    var request = new XMLHttpRequest();
    request.open("GET", "config.get");
    request.onreadystatechange = function () {
        if (request.readyState != 4) {
            // リクエスト中
        } else if (request.status != 200) {
            // 失敗
        } else {
            // 取得成功
            // var result = request.responseText;

            var json = JSON.parse(request.responseText);
            document.getElementById("servoLeft").value = parseInt(json.ServoAngle.left);
            document.getElementById("servoRight").value = parseInt(json.ServoAngle.right);
            document.getElementById("servoCenter").value = parseInt(json.ServoAngle.center);
            document.getElementById("maxMotorDuty").value = parseFloat(json.maxMotorDuty);
        }
    };
    request.send(null);
}

getConfigParameters();
