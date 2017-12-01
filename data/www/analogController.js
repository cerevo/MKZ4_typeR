var offset = 50;
document.body.style.height = document.body.clientHeight + offset + 'px';
document.body.style.width = document.body.clientWidth + offset + 'px';
document.getElementsByTagName("html")[0].style.height = document.body.style.height + 'px';
document.getElementsByTagName("html")[0].style.width = document.body.style.width + 'px';
var moveHomePosition = function() {
    document.body.scrollTop = offset / 2;
    document.body.scrollLeft = offset / 2;
};
setTimeout(moveHomePosition, 500);

//画面のxyは右、下がx，yで正なので、yはsendCommandの呼び出し時に符号を入れ替える
var startX = 0;
var startY = 0;
var currentX = 0;
var currentY = 0;


document.body.ontouchstart = function(event) {
    startX = event.touches[0].clientX;
    startY = event.touches[0].clientY;
    currentY = 0;
    currentY = 0;
};

document.body.ontouchmove = function(event) {
    currentX = event.touches[0].clientX - startX;
    currentY = event.touches[0].clientY - startY;
};

document.body.ontouchend = function(event) {
    currentX = 0;
    currentY = 0;
};

//ms
var sendInterval = 100;
setInterval(
    function(){
        sendCommand(
            currentX / (window.innerWidth / 3),
            -currentY / (window.innerHeight / 3)
        );
    },
    sendInterval
);
