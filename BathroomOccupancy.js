(function () {
    'use strict';
    var express = require('express');
    var app = express();
    var bathroomStatus = true;
    var espTime = 0;
    var espConnectivityTimeout = 600000;//1201000;
    function updateEspStatus() {
        var d = new Date();
        d = d.getTime();
        espTime = d;
        console.log("esp is connected");
    }
    app.get('/updateBathroomStatus', function (req, res) {
        console.log('Esp connectivity updated undirectly');
        updateEspStatus();
        var query = req.query;
        console.log('query' + JSON.stringify(req.query));
        if(query.status == "1") {
            bathroomStatus = true;
            console.log("free");
        }
        if(query.status == "0") {
            bathroomStatus = false;
            console.log("Occupied");
        }
        res.send("bathroom status got updated!!");
    });
    app.get('/updateDeviceStatus', function (req,res){
        console.log('Esp connectivity updated directly');
        updateEspStatus();
        res.send("esp status got updated!");
    });
    app.get('/', function(req, res) {
        var curTime = new Date();
        curTime = curTime.getTime();
        var head = "<!DOCTYPE html><html>\
                        <head>\
                          <title>Bathroom - Status</title>\
                        </head>";
        var body = "<body bgcolor='#ffd900' align='center' style='font-family:sans-serif'>";
        var end = "</body></html>";
        var contentCheckConnection = "<h2>Please check device connection! </h2>";
        var contentBathroomFree = "<h2>Yayy!!!</h2> \
                                    <h4>Bathroom is free.</h4>";
        var contentBathroomOccupied = "<h2>Oops...</h2>\
                                     <h4>Hold on bathroom is occupied. Please recheck after some time.</h4>";
        var link = "<a href='/' style='margin-left:10p;'>recheck</a>";

        if (curTime - espTime > espConnectivityTimeout){
            console.log("Device is disconnected");
            var text = head + body + contentCheckConnection + end + link;
            res.send(text);
        }
        else{
            var text = (bathroomStatus) ? head + body + contentBathroomFree + end + link : head + body + contentBathroomOccupied + end + link;
            res.send(text);
        }
    });
    app.listen(8080, function () {
        console.log('server is running on port 8080');
    });
})();