//数据接口: this.ondata(audioData)
//引用方法:
//var adv = new ADV_Plugin("ANY",5050);
//adv.ondata = function(audioData){ do something with audioData...};



var ADV_Plugin = (function(){
    var reader = new FileReader();
    
    function _adv_plugin(ip, port) {
        var _this = this;
        if(_this instanceof _adv_plugin) {
            if("ANY" == ip || "any" == ip) {
                _this.address = "ws://0.0.0.0:" + port;
            } 
            else {
                _this.address = "ws://localhost:" + port;
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

//窗口自适应
window.onresize = function () {
            if (window.innerWidth / window.innerHeight > 1.8333333333333) {
                display.width = window.innerWidth;
                display.height = window.innerWidth / 1980 * 1080;

                window.scrollTo(0, (window.innerHeight - 123) / 16);
            } else {
                display.width = window.innerHeight / 1080 * 1980;
                display.height = window.innerHeight;
            }
        }

window.onresize();