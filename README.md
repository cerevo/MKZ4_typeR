# MKZ4R

## 概要
MKZ4R([MKZ4](https://maker.cerevo.com/ja/mkz4/) Racing type(仮))のAdruinoソースコードです。

## 環境
開発環境のセットアップ方法は、
[techblogの記事](https://tech-blog.cerevo.com/archives/4952/)を見てください。

## ファイル構成
- MKZ4R.ino  
	Arduinoスケッチです。
- DRV8835.h, DRV8835.cpp  
	DCモータードライバDRV8835用のArduinoライブラリです。
- ledcServo.h, ledcServo.cpp  
	ESP32のLEDC機能（実質ただのPWM）を使ってサーボを制御するライブラリです。
- MutexWrapper.h  
	c++のstlのmutexをラップしただけのクラスです。
- data/  
	このディレクトリの中身はすべてSPIFFS uploaderによりESP32にアップロードするものです。
	- config.json  
		MKZ4R.inoで使われるパラメータのデフォルト値です
	- www/  
		- index.html  
			analogController.html, tiltController.html, config.htmlへのリンクが貼られています。
		- analogController.html, analogContnroller.js  
			画面スワイプで制御できるコントローラーです。
		- tiltController.html, tiltController.js  
			スマートフォンの傾きで制御できるコントローラーです。
		- controllerWebSocket.js
			ESP32とコントローラーの間の通信を行う関数群です。
			上の2つのコントローラーから利用されています。
			自作コントローラーを作るときにも活用してください。
		- config.html, config.js  
			制御用のパラメータ（config.jsonに書かれているもの）を変更するページです。このページで設定した値はESP32の電源OFF後も保存されます。
		- jquery.js  
			jqueryです。

## 手順
1. 開発環境セットアップ
1. Arduinoスケッチビルド、書き込み
1. data/以下のupload
1. ESP32のアクセスポイントへ接続  
	MY1\_ESP32.inoはデフォルトでSSID:"MKZ4_ESP32"、パスワードなしのアクセスポイントとなるので、お手持ちのスマートフォン、タブレット等から接続してください。
1. コントローラーのページへアクセスして遊ぶ  
	上記アクセスポイントへ接続後、ブラウザで http://192.168.4.1/index.html へアクセスすると、コントローラーのページと設定用のページへのリンクが表示されます。コントローラーのページは2種類用意されています(http://192.168.4.1/analogController.html と http://192.168.4.1/tiltController.html)。設定用のページ(http://192.168.4.1/config.html)ではステアリングの切り具合やモーターのデューティー比の設定ができます。

## コントローラーの通信フォーマット
コントローラーを自作したい方向けの簡易的な情報です。
詳しいことはMKZ4R.inoの該当部分やコントローラー関係のhtml, jsファイルを参照してください。

ESP32への制御指令値はJSONデータとしてWebSocketメッセージで送られます。
ESP32側のURIはws://192.168.4.1/wsです。
制御値のJSONフォーマットは以下のようになっています。
~~~
{
	"servoRate": servoRate,
	"motorRate": motorRate
}
~~~

`servoRate`, `motorRate`はそれぞれ[-1, 1]の実数値です。
`servoRate`については、左が負、右が正になるようにステアリングを切ります。
`motorRate`は正で前進、負で後退となるようにモーターを回転させます。
どちらも実際の制御司令はこの値をconfig.htmlで設定した値で補正したものになります。

また、安全のため、ESP32は一定時間内に`"Ping"`というキーの含まれたJSONを受信しなかった場合自動でモーターを停止するようになっています。制限時間はMKZ4R.ino内の`webSocketTimeout`で定義されています。`"Ping"`キーに対応する値はなんでもかまいません。また、`"Ping"`は制御値の通信に含めても、単独で送信しても構いません。



