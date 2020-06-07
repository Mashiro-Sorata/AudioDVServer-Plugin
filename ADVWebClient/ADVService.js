//数据接口: this.ondata(audioData)
//引用方法:
//var adv = new ADV_Plugin("ANY",5050);
//adv.ondata = function(audioData){ do something with audioData...};



var ADV_Plugin = (function(){
    var reader = new FileReader();
    
    function _adv_plugin(ip, port) {
        var _this = this;
        if(_this instanceof _adv_plugin) {
            if("local" == ip.toLocaleLowerCase()) {
            	_this.address = "ws://localhost:" + port;
                
            } 
            else {
                _this.address = "ws://" + ip + ":" + port;
            }

            _this.ondata = function(audioData){};

            var ws = new WebSocket(_this.address);

            ws.onmessage = function(evt) {
                var _data = evt.data;
                reader.readAsArrayBuffer(_data);
            };

            reader.onload = function (e){
                var audioData = new Float32Array(reader.result);
                _this.ondata(audioData);
            };

        } else {
            return new _adv_plugin(ip, port);
        }
    }
    return _adv_plugin;

})();

