<div align="center"><h1>ADV-Plugin</h1></div>

<div align="center"><img src="https://github.com/Mashiro-Sorata/ADV-Plugin/blob/master/Image/ADV_demo.png?raw=true"></div>
<div align="center">Powered By <a href="http://mashirosorata.vicp.io">Mashiro_Sorata</a></div>

---

## 目录
1. [简介](#u1)
2. [下载](#u2)
3. [安装](#u3)
4. [使用说明](#u4)
5. [开发接口/API](#u5)
6. [更多](#u6)
7. [License](#u7)

---

<h2 id="u1">简介</h2>

`ADV-Plugin`是[SAO Utils](http://sao.gpbeta.com/)的第三方插件，可以提供系统音频数据的可视化服务。安装启用插件后，仅需用SAO Utils的“桌面网页挂件”打开ADV网页客户端即可显示系统音频频谱。网页客户端可由用户自行编写改变其样式，此插件则为客户端提供数据接口(详见[开发接口/API](#u5))。下载后的插件压缩包内提供了一个简单的网页客户端演示Demo，用户可自行修改其样式。

### 视频演示
[桌面音频可视化插件演示](https://www.bilibili.com/video/BV1uf4y1U7Tx)

### 开发日志

* 2020-05-26：`V1.2.2`版本更新修复了重启插件或切换不可用设备时造成插件出错等问题，对音频捕捉服务增加错误捕获及处理。

* 2020-05-20：`V1.2.1`版本更新支持`EXP 1`实验版的SAO Utils。新增调试版插件下载，可输出日志文件。

* 2020-05-19：`V1.1.0`版本的插件在`beta 1`、`beta 2(steam)`版本的SAO Utils中测试过通过。已知在实验性版本`Exp 1`中会导致**程序崩溃**。~~与SAO Utils实验版兼容的插件会在后续中推出，敬请期待。~~

---

<h2 id="u2">下载</h2>

下载最新版本的插件：[ADV_Plug-in.zip](https://github.com/Mashiro-Sorata/ADV-Plugin/releases/latest)，将下载的 `ADV_Plug-in.zip` 压缩文件解压。
```html
文件夹结构如下
│  readme.txt
│
├─ADV_Client
│      adv.js
│      index.html
│
└─AudioDVServer
        advConfig.ini
        module64.dll
```

---

<h2 id="u3">安装</h2>

将解压缩根目录下的`AudioDVServer`文件夹拷贝至SAO Utils根目录的`Plugins`文件夹中，重启SAO即可。

---

<h2 id="u4">使用说明</h2>

### 启用插件
安装完成后打开SAO Utils首选项中的插件管理页面，将名称为`AudioDVServer`的插件选择启用后点击保存。

### 插件配置
安装完成后打开SAO Utils首选项中的插件管理页面，将名称为`AudioDVServer`的插件选择启用后点击保存。

通过更改`AudioDVServer`文件夹中的`advConfig.ini`文件来配置插件。当配置数据错误或无配置文件时使用默认值，配置值不区分大小写。
- ip：可选，默认值为`local`，指代地址`127.0.0.1`，可更改为`any`，指代地址`0.0.0.0`。只支持`any`与默认参数`local`，定义插件提供服务的地址。
- port：可选，默认值为`5050`，定义插件提供服务的端口号。
- maxClient：可选，默认值为`5`，定义客户端最大连接数。
- logger：可选，默认值为`false`，调试版(Debug Version)专有，发行版(Release Version)可设置，但无效。设置为`true`后可在插件所在目录下输出日志文件`ADV_Log.log`。

`advConfig.ini` 文件示例：
```ini
[Server]
ip = local
port = 5050
maxClient = 4
logger = true
```

### 客户端配置
压缩包内提供了一个频谱显示客户端的示例，可用SAO Utils桌面网页挂件打开。通过设置`ADV_Client`文件夹内的`index.html`文件来配置地址，端口号以及显示频谱的样式。必须保证客户端与插件设置的地址与端口号一致。示例文件已配置好，可以直接使用。也可通过编写HTML文件来定义自己的频谱显示客户端。

可以更改`index.html`中的以下代码来自定义客户端，值得注意的是`IP`与`PORT`需要与插件的设置"一致"：

* 如果插件配置文件`advConfig.ini`中的`ip = any`, 客户端`IP`可选`"local"`，或者是插件所在主机的的局域网`ip`地址（如`"192.168.1.107"`）。
* 如果插件配置文件`advConfig.ini`中的`ip = local`， 客户端`IP`必须为参数`"local"`。

```javascript
var IP = "local";
var PORT = 5050;
//频谱线颜色
var LINE_COLOR = "rgba(0,191,255,0.4)";
//分界线的颜色
var CENTER_COLOR = "rgba(0,0,0,0.9)";
```

### SAO Utils beta版使用注意事项

如果你是SAO Utils beta版用户，请不要频繁刷新网页挂件：与浏览器不同的是，网页挂件刷新后不会立即关闭前一个`WebSocket`连接，需要等待一段时间后才能释放，而新的连接会被插件计数。所以当频繁刷新次数加上已有的客户端数大于设置的最大客户端连接数时，会造成被刷新的那个客户端无法正常显示。如果出现这种情况，可以通过打开SAO Utils首选项中的插件管理页面重启该插件，或者等待之前的客户端自动断开连接。

---

<h2 id="u5">开发接口/API</h2>

`ADV_Client`文件夹内的`adv.js`封装了一个`ADV_Plugin`类，并提供了一个`ondata()`接口方便数据的引用。

引用方法：
```javascript
var IP = "local"; // "192.168.1.107"
var PORT = 5050;
var adv = new ADV_Plugin(IP, PORT);
adv.ondata = function(audioData){ //do something with audioData...};
```

参数`IP`可选值有`"local"`，或者是插件所在主机的的局域网`ip`地址（如`"192.168.1.107"`）。参数`"local"`会被解析为`"localhost"`。

每当客户端收到插件发送的频谱数据就会触发`ondata`事件。
其中`audioData`是数据长度为`128`的数组，前面`64`个数据为左声道FFT数据，后面`64`个数据为右声道FFT数据。每一个声道的FFT数据位从低到高对应频谱频率的由低到高。

---

<h2 id="u6">更多</h2>

我博客的[文章](http://mashirosorata.vicp.io/ADV-Plugin%E2%80%94%E2%80%94SAO%20Utils%E9%9F%B3%E9%A2%91%E5%8F%AF%E8%A7%86%E5%8C%96%E6%8F%92%E4%BB%B6.html)和这个`README`的内容上大致相同，但说明了这个项目的由来与实现，有兴趣的可以看一下。

---

<h2 id="u7">License</h2>

[MIT LICENSE](https://github.com/Mashiro-Sorata/ADV-Plugin/blob/master/LICENSE)
