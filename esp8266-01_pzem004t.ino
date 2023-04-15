#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define DEVISE_NAME     "PZ"
#define SETUP_WIFI_PASS NULL

#define PZEM_RX_PIN     3
#define PZEM_TX_PIN     1

#define DISPLAY_ADDRESS   0x3C // or 0x3D
#define DISPLAY_SDA_PIN   2
#define DISPLAY_SCL_PIN   0
#define DISPLAY_WIDTH     128 // OLED display width, in pixels
#define DISPLAY_HEIGHT    64  // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define DISPLAY_RESET_PIN 1 // Reset pin # (or -1 if sharing Arduino reset pin)



Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_RESET_PIN);

SoftwareSerial   pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30      pzem;
ESP8266WebServer http(80);
WiFiManager      wifiManager;


float voltage     = 0;
float amperes     = 0;
float power       = 0;
float powerTotal  = 0;
float frequency   = 0;
float powerFactor = 0;

String viewScreen = "indicators";

int    wifiSignal        = 0;
int    wifiSignalPercent = 0;
String wifiSSID          = "";
String wifiStatus        = "";
String wifiIPAddress     = "";

int debugCount = 0;
String debugText = "";

unsigned long timeLoop      = 0;
unsigned long countInterval = 0;
unsigned long interval      = 2000;


/**
 * Setup
 */
void setup() {

    Serial.begin(115200);


    setupDisplay();
    setupPZEM();

    if ( ! wifiManager.getWiFiIsSaved()) {
        displayWifiSetup();
    }

    setupWifi();
    setupHttp();

    debugRow("Setup success");
}


/**
 *  Loop
 */
void loop() {

    unsigned long currentMillis = millis();

    if (currentMillis - timeLoop >= interval) {
        timeLoop = currentMillis;

        setPzemValues();
        setWifiValues();


        if (viewScreen == "indicators") {
            displayIndicators();

            if (countInterval >= 4) {
                viewScreen    = "indicators2";
                countInterval = 0;
            }

        } else if (viewScreen == "indicators2") {
            displayIndicators2();

            if (countInterval >= 4) {
                viewScreen    = "wifi_info";
                countInterval = 0;
            }

        } else {
            displayWifi();

            if (countInterval >= 4) {
                viewScreen    = "indicators";
                countInterval = 0;
            }
        }

        countInterval++;
    }


    http.handleClient();
    delay(20);
}


/**
 * Дебаг
 */
void debugRow(String row) {

    if (debugCount > 30) {
        debugText = row + "\n";
        debugCount = 0;
    } else {
        debugText += row + "\n";
    }

    debugCount++;
}


/**
 *
 */
void setupPZEM() {

    pzem = PZEM004Tv30(pzemSWSerial);
}



/**
 *
 */
void setupDisplay() {


    Wire.begin(DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);           // set I2C pins (SDA = GPIO2, SCL = GPIO0), default clock is 100kHz


    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS);

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();

    display.println(String("Loading ") + getDeviceName());
}


/**
 *
 */
void setupHttp() {

    http.on("/", HTTP_GET, []() {

        String deviseName = getDeviceName();

        String html =
            (String)"<!DOCTYPE html>" +
            "<html class=\"h-100\">" +
                "<head>" +
                    "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">" +
                    "<meta content=\"width=device-width, initial-scale=1\" name=\"viewport\">" +
                    "<title>" + deviseName + "</title>" +
                    "<style>:root{--bs-blue:#0d6efd;--bs-indigo:#6610f2;--bs-purple:#6f42c1;--bs-pink:#d63384;--bs-red:#dc3545;--bs-orange:#fd7e14;--bs-yellow:#ffc107;--bs-green:#198754;--bs-teal:#20c997;--bs-cyan:#0dcaf0;--bs-white:#fff;--bs-gray:#6c757d;--bs-gray-dark:#343a40;--bs-primary:#0d6efd;--bs-secondary:#6c757d;--bs-success:#198754;--bs-info:#0dcaf0;--bs-warning:#ffc107;--bs-danger:#dc3545;--bs-light:#f8f9fa;--bs-dark:#212529;--bs-font-sans-serif:system-ui,-apple-system,\"Segoe UI\",Roboto,\"Helvetica Neue\",Arial,\"Noto Sans\",\"Liberation Sans\",sans-serif,\"Apple Color Emoji\",\"Segoe UI Emoji\",\"Segoe UI Symbol\",\"Noto Color Emoji\";--bs-font-monospace:SFMono-Regular,Menlo,Monaco,Consolas,\"Liberation Mono\",\"Courier New\",monospace;--bs-gradient:linear-gradient(180deg, rgba(255, 255, 255, 0.15), rgba(255, 255, 255, 0))}*,::after,::before{box-sizing:border-box}@media (prefers-reduced-motion:no-preference){:root{scroll-behavior:smooth}}body{margin:0;font-family:var(--bs-font-sans-serif);font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#fff;-webkit-text-size-adjust:100%;-webkit-tap-highlight-color:transparent}.h1,.h2,.h3,.h4,.h5,.h6,h1,h2,h3,h4,h5,h6{margin-top:0;margin-bottom:.5rem;font-weight:500;line-height:1.2}.h1,h1{font-size:calc(1.375rem + 1.5vw)}@media (min-width:1200px){.h1,h1{font-size:2.5rem}}.h2,h2{font-size:calc(1.325rem + .9vw)}@media (min-width:1200px){.h2,h2{font-size:2rem}}.h3,h3{font-size:calc(1.3rem + .6vw)}@media (min-width:1200px){.h3,h3{font-size:1.75rem}}.h4,h4{font-size:calc(1.275rem + .3vw)}@media (min-width:1200px){.h4,h4{font-size:1.5rem}}.h5,h5{font-size:1.25rem}.h6,h6{font-size:1rem}p{margin-top:0;margin-bottom:1rem}ol,ul{padding-left:2rem}dl,ol,ul{margin-top:0;margin-bottom:1rem}ol ol,ol ul,ul ol,ul ul{margin-bottom:0}b,strong{font-weight:bolder}.small,small{font-size:.875em}a{color:#0d6efd;text-decoration:underline}a:hover{color:#0a58ca}a:not([href]):not([class]),a:not([href]):not([class]):hover{color:inherit;text-decoration:none}::-moz-focus-inner{padding:0;border-style:none}iframe{border:0}[hidden]{display:none!important}.display-1{font-size:calc(1.625rem + 4.5vw);font-weight:300;line-height:1.2}@media (min-width:1200px){.display-1{font-size:5rem}}.display-2{font-size:calc(1.575rem + 3.9vw);font-weight:300;line-height:1.2}@media (min-width:1200px){.display-2{font-size:4.5rem}}.display-3{font-size:calc(1.525rem + 3.3vw);font-weight:300;line-height:1.2}@media (min-width:1200px){.display-3{font-size:4rem}}.display-4{font-size:calc(1.475rem + 2.7vw);font-weight:300;line-height:1.2}@media (min-width:1200px){.display-4{font-size:3.5rem}}.display-5{font-size:calc(1.425rem + 2.1vw);font-weight:300;line-height:1.2}@media (min-width:1200px){.display-5{font-size:3rem}}.display-6{font-size:calc(1.375rem + 1.5vw);font-weight:300;line-height:1.2}@media (min-width:1200px){.display-6{font-size:2.5rem}}.container,.container-fluid,.container-lg,.container-md,.container-sm,.container-xl,.container-xxl{width:100%;padding-right:var(--bs-gutter-x,.75rem);padding-left:var(--bs-gutter-x,.75rem);margin-right:auto;margin-left:auto}@media (min-width:576px){.container,.container-sm{max-width:540px}}@media (min-width:768px){.container,.container-md,.container-sm{max-width:720px}}@media (min-width:992px){.container,.container-lg,.container-md,.container-sm{max-width:960px}}@media (min-width:1200px){.container,.container-lg,.container-md,.container-sm,.container-xl{max-width:1140px}}@media (min-width:1400px){.container,.container-lg,.container-md,.container-sm,.container-xl,.container-xxl{max-width:1320px}}.row{--bs-gutter-x:1.5rem;--bs-gutter-y:0;display:flex;flex-wrap:wrap;margin-top:calc(var(--bs-gutter-y) * -1);margin-right:calc(var(--bs-gutter-x) * -.5);margin-left:calc(var(--bs-gutter-x) * -.5)}.row>*{flex-shrink:0;width:100%;max-width:100%;padding-right:calc(var(--bs-gutter-x) * .5);padding-left:calc(var(--bs-gutter-x) * .5);margin-top:var(--bs-gutter-y)}.col{flex:1 0 0%}.d-inline{display:inline!important}.d-inline-block{display:inline-block!important}.d-block{display:block!important}.d-grid{display:grid!important}.d-table{display:table!important}.d-table-row{display:table-row!important}.d-table-cell{display:table-cell!important}.d-flex{display:flex!important}.d-inline-flex{display:inline-flex!important}.d-none{display:none!important}@media (min-width:576px){.col-sm{flex:1 0 0%}}@media (min-width:768px){.col-md{flex:1 0 0%}}@media (min-width:992px){.col-lg{flex:1 0 0%}}@media (min-width:1200px){.col-xl{flex:1 0 0%}}@media (min-width:1400px){.col-xxl{flex:1 0 0%}}.col-auto{flex:0 0 auto;width:auto}.col-1{flex:0 0 auto;width:8.33333333%}.col-2{flex:0 0 auto;width:16.66666667%}.col-3{flex:0 0 auto;width:25%}.col-4{flex:0 0 auto;width:33.33333333%}.col-5{flex:0 0 auto;width:41.66666667%}.col-6{flex:0 0 auto;width:50%}.col-7{flex:0 0 auto;width:58.33333333%}.col-8{flex:0 0 auto;width:66.66666667%}.col-9{flex:0 0 auto;width:75%}.col-10{flex:0 0 auto;width:83.33333333%}.col-11{flex:0 0 auto;width:91.66666667%}.col-12{flex:0 0 auto;width:100%}@media (min-width:576px){.col-sm-auto{flex:0 0 auto;width:auto}.col-sm-1{flex:0 0 auto;width:8.33333333%}.col-sm-2{flex:0 0 auto;width:16.66666667%}.col-sm-3{flex:0 0 auto;width:25%}.col-sm-4{flex:0 0 auto;width:33.33333333%}.col-sm-5{flex:0 0 auto;width:41.66666667%}.col-sm-6{flex:0 0 auto;width:50%}.col-sm-7{flex:0 0 auto;width:58.33333333%}.col-sm-8{flex:0 0 auto;width:66.66666667%}.col-sm-9{flex:0 0 auto;width:75%}.col-sm-10{flex:0 0 auto;width:83.33333333%}.col-sm-11{flex:0 0 auto;width:91.66666667%}.col-sm-12{flex:0 0 auto;width:100%}}@media (min-width:768px){.col-md-auto{flex:0 0 auto;width:auto}.col-md-1{flex:0 0 auto;width:8.33333333%}.col-md-2{flex:0 0 auto;width:16.66666667%}.col-md-3{flex:0 0 auto;width:25%}.col-md-4{flex:0 0 auto;width:33.33333333%}.col-md-5{flex:0 0 auto;width:41.66666667%}.col-md-6{flex:0 0 auto;width:50%}.col-md-7{flex:0 0 auto;width:58.33333333%}.col-md-8{flex:0 0 auto;width:66.66666667%}.col-md-9{flex:0 0 auto;width:75%}.col-md-10{flex:0 0 auto;width:83.33333333%}.col-md-11{flex:0 0 auto;width:91.66666667%}.col-md-12{flex:0 0 auto;width:100%}}.nav{display:flex;flex-wrap:wrap;padding-left:0;margin-bottom:0;list-style:none}.nav-fill .nav-item,.nav-fill>.nav-link{flex:1 1 auto;text-align:center}.nav-justified .nav-item,.nav-justified>.nav-link{flex-basis:0;flex-grow:1;text-align:center}.nav-fill .nav-item .nav-link,.nav-justified .nav-item .nav-link{width:100%}.navbar{position:relative;display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;padding-top:.5rem;padding-bottom:.5rem}.navbar>.container,.navbar>.container-fluid,.navbar>.container-lg,.navbar>.container-md,.navbar>.container-sm,.navbar>.container-xl,.navbar>.container-xxl{display:flex;flex-wrap:inherit;align-items:center;justify-content:space-between}.navbar-brand{padding-top:.3125rem;padding-bottom:.3125rem;margin-right:1rem;font-size:1.25rem;text-decoration:none;white-space:nowrap}.navbar-nav{display:flex;flex-direction:column;padding-left:0;margin-bottom:0;list-style:none}.navbar-nav .nav-link{padding-right:0;padding-left:0}.navbar-nav .dropdown-menu{position:static}.navbar-text{padding-top:.5rem;padding-bottom:.5rem}.navbar-dark .navbar-brand{color:#fff}.navbar-dark .navbar-brand:focus,.navbar-dark .navbar-brand:hover{color:#fff}.navbar-dark .navbar-nav .nav-link{color:rgba(255,255,255,.55)}.navbar-dark .navbar-nav .nav-link:focus,.navbar-dark .navbar-nav .nav-link:hover{color:rgba(255,255,255,.75)}.navbar-dark .navbar-nav .nav-link.disabled{color:rgba(255,255,255,.25)}.navbar-dark .navbar-nav .nav-link.active,.navbar-dark .navbar-nav .show>.nav-link{color:#fff}.navbar-dark .navbar-text{color:rgba(255,255,255,.55)}.navbar-dark .navbar-text a,.navbar-dark .navbar-text a:focus,.navbar-dark .navbar-text a:hover{color:#fff}.card{position:relative;display:flex;flex-direction:column;min-width:0;word-wrap:break-word;background-color:#fff;background-clip:border-box;border:1px solid rgba(0,0,0,.125);border-radius:.25rem}.card-body{flex:1 1 auto;padding:1rem 1rem}.card-text:last-child{margin-bottom:0}.clearfix::after{display:block;clear:both;content:\"\"}.w-25{width:25%!important}.w-50{width:50%!important}.w-75{width:75%!important}.w-100{width:100%!important}.w-auto{width:auto!important}.mw-100{max-width:100%!important}.vw-100{width:100vw!important}.min-vw-100{min-width:100vw!important}.h-25{height:25%!important}.h-50{height:50%!important}.h-75{height:75%!important}.h-100{height:100%!important}.h-auto{height:auto!important}.mh-100{max-height:100%!important}.vh-100{height:100vh!important}.min-vh-100{min-height:100vh!important}.flex-column{flex-direction:column!important}.flex-column-reverse{flex-direction:column-reverse!important}.flex-shrink-0{flex-shrink:0!important}.flex-shrink-1{flex-shrink:1!important}.justify-content-center{justify-content:center!important}.mx-0{margin-right:0!important;margin-left:0!important}.mx-1{margin-right:.25rem!important;margin-left:.25rem!important}.mx-2{margin-right:.5rem!important;margin-left:.5rem!important}.mx-3{margin-right:1rem!important;margin-left:1rem!important}.mx-4{margin-right:1.5rem!important;margin-left:1.5rem!important}.mx-5{margin-right:3rem!important;margin-left:3rem!important}.mx-auto{margin-right:auto!important;margin-left:auto!important}.my-0{margin-top:0!important;margin-bottom:0!important}.my-1{margin-top:.25rem!important;margin-bottom:.25rem!important}.my-2{margin-top:.5rem!important;margin-bottom:.5rem!important}.my-3{margin-top:1rem!important;margin-bottom:1rem!important}.my-4{margin-top:1.5rem!important;margin-bottom:1.5rem!important}.my-5{margin-top:3rem!important;margin-bottom:3rem!important}.my-auto{margin-top:auto!important;margin-bottom:auto!important}.mt-0{margin-top:0!important}.mt-1{margin-top:.25rem!important}.mt-2{margin-top:.5rem!important}.mt-3{margin-top:1rem!important}.mt-4{margin-top:1.5rem!important}.mt-5{margin-top:3rem!important}.mt-auto{margin-top:auto!important}.me-0{margin-right:0!important}.me-1{margin-right:.25rem!important}.me-2{margin-right:.5rem!important}.me-3{margin-right:1rem!important}.me-4{margin-right:1.5rem!important}.me-5{margin-right:3rem!important}.me-auto{margin-right:auto!important}.mb-0{margin-bottom:0!important}.mb-1{margin-bottom:.25rem!important}.mb-2{margin-bottom:.5rem!important}.mb-3{margin-bottom:1rem!important}.mb-4{margin-bottom:1.5rem!important}.mb-5{margin-bottom:3rem!important}.mb-auto{margin-bottom:auto!important}.ms-0{margin-left:0!important}.ms-1{margin-left:.25rem!important}.ms-2{margin-left:.5rem!important}.ms-3{margin-left:1rem!important}.ms-4{margin-left:1.5rem!important}.ms-5{margin-left:3rem!important}.ms-auto{margin-left:auto!important}.py-0{padding-top:0!important;padding-bottom:0!important}.py-1{padding-top:.25rem!important;padding-bottom:.25rem!important}.py-2{padding-top:.5rem!important;padding-bottom:.5rem!important}.py-3{padding-top:1rem!important;padding-bottom:1rem!important}.py-4{padding-top:1.5rem!important;padding-bottom:1.5rem!important}.py-5{padding-top:3rem!important;padding-bottom:3rem!important}.pt-0{padding-top:0!important}.pt-1{padding-top:.25rem!important}.pt-2{padding-top:.5rem!important}.pt-3{padding-top:1rem!important}.pt-4{padding-top:1.5rem!important}.pt-5{padding-top:3rem!important}.pe-0{padding-right:0!important}.pe-1{padding-right:.25rem!important}.pe-2{padding-right:.5rem!important}.pe-3{padding-right:1rem!important}.pe-4{padding-right:1.5rem!important}.pe-5{padding-right:3rem!important}.pb-0{padding-bottom:0!important}.pb-1{padding-bottom:.25rem!important}.pb-2{padding-bottom:.5rem!important}.pb-3{padding-bottom:1rem!important}.pb-4{padding-bottom:1.5rem!important}.pb-5{padding-bottom:3rem!important}.ps-0{padding-left:0!important}.ps-1{padding-left:.25rem!important}.ps-2{padding-left:.5rem!important}.ps-3{padding-left:1rem!important}.ps-4{padding-left:1.5rem!important}.ps-5{padding-left:3rem!important}.text-muted{color:#6c757d!important}.bg-light{background-color:#f8f9fa!important}.bg-dark{background-color:#212529!important}main>.container{padding:60px 15px 0}</style>" +
                    "<script>function httpGetAsync(e,t){var n=new XMLHttpRequest;n.onreadystatechange=function(){4===n.readyState&&200===n.status&&t(n.responseText)},n.open(\"GET\",e,!0),n.send(null)}function resetHardware(){confirm(\"Сбросить настройки устройства?\")&&(httpGetAsync(\"/reset\",function(){}),alert(\"Настройки сброшены. Чтобы заново подключиться к устройству используйте wifi.\"))}setInterval(function(){httpGetAsync(\"/metrics.json\",function(t){try{var e=JSON.parse(t),n=document.querySelector(\"#sensor-voltage .number\"),o=document.querySelector(\"#sensor-amperes .number\"),r=document.querySelector(\"#sensor-power_total .number\"),s=document.querySelector(\"#sensor-frequency .number\"),c=document.querySelector(\"#sensor-power_factor .number\"),u=document.querySelector(\"#sensor-wifi-signal .number\"),a=document.querySelector(\"#sensor-wifi-signal-percent .number\");n.textContent=e.voltage,o.textContent=e.amperes,r.textContent=e.powerTotal,s.textContent=e.frequency,c.textContent=e.powerFactor,u.textContent=e.wifi_signal,a.textContent=e.wifi_signal_percent}catch(e){console.log(t),console.log(e)}})},3e3);</script>" +
                "</head>" +
                "<body class=\"d-flex flex-column h-100\">" +
                    "<header> <nav class=\"navbar navbar-expand-md navbar-dark bg-dark\"> <div class=\"container justify-content-center\"><a class=\"navbar-brand\" href=\"/\">Контроль электроэнергии</a></div> </nav></header><main class=\"flex-shrink-0\"> <div class=\"container\"> <div class=\"row\"> <div class=\"col-sm-6 col-md-4 mb-4\"> <div class=\"card\" id=\"sensor-voltage\"> <div class=\"card-body\"><h5 class=\"card-title\">&#9889; Вольты</h5> <p class=\"card-text\"><span class=\"number display-6\">0</span> <small class=\"text-muted\">V</small> </p></div> </div> </div> <div class=\"col-sm-6 col-md-4 mb-4\"> <div class=\"card\" id=\"sensor-amperes\"> <div class=\"card-body\"><h5 class=\"card-title\">Амперы</h5> <p class=\"card-text\"><span class=\"number display-6\">0</span> <small class=\"text-muted\">A</small></p></div> </div> </div> <div class=\"col-sm-6 col-md-4 mb-4\"> <div class=\"card\" id=\"sensor-power\"> <div class=\"card-body\"><h5 class=\"card-title\">&#128161; Энергопотребление</h5> <p class=\"card-text\"> <span class=\"number display-6\">0</span> <small class=\"text-muted\">w/h</small> </p> </div> </div> </div> <div class=\"col-sm-6 col-md-4 mb-4\"> <div class=\"card\" id=\"sensor-frequency\"> <div class=\"card-body\"><h5 class=\"card-title\">Частота</h5> <p class=\"card-text\"><span class=\"number display-6\">0</span> <small class=\"text-muted\">раз в секунду</small></p></div> </div> </div> <div class=\"col-sm-6 col-md-4 mb-4\"> <div class=\"card\" id=\"sensor-power_factor\"> <div class=\"card-body\"><h5 class=\"card-title\">Коэффициент мощности</h5> <p class=\"card-text\"><span class=\"number display-6\">0</span> <small class=\"text-muted\"></small> </p></div> </div> </div> <div class=\"col-sm-6 col-md-4 mb-4\"> <div class=\"card\" id=\"sensor-power_total\"> <div class=\"card-body\"><h5 class=\"card-title\">&#128161; Потребление за все время</h5> <p class=\"card-text\"><span class=\"number display-6\">0</span> <small class=\"text-muted\">Kw/h</small></p></div> </div> </div> <div class=\"col-sm-6 col-md-4 mb-4\"> <div class=\"card\" id=\"sensor-wifi-signal\"> <div class=\"card-body\"><h5 class=\"card-title\">&#128246; Wifi сигнал</h5> <p class=\"card-text\"><span class=\"number display-6\">0</span> <small class=\"text-muted\">дБм, децибел относительно 1 милливатта</small></p></div> </div> </div> <div class=\"col-sm-6 col-md-4 mb-4\"> <div class=\"card\" id=\"sensor-wifi-signal-percent\"> <div class=\"card-body\"><h5 class=\"card-title\">&#128246; Wifi сигнал</h5> <p class=\"card-text\"><span class=\"number display-6\">0</span> <small class=\"text-muted\">%</small></p></div> </div> </div> </div> </div></main><footer class=\"footer mt-auto py-3 bg-light\"> <div class=\"container\"> <a href=\"/metrics\" class=\"text-muted\">/metrics</a> <a href=\"/metrics.json\" class=\"text-muted\">/metrics.json</a> <a href=\"#\" class=\"text-muted\" style=\"float: right\" onclick=\"resetHardware()\">/reset</a> </div></footer>" +
                "</body>" +
            "</html>";

        http.send(200, "text/html", html);
    });


    http.on("/metrics", HTTP_GET, []() {

        String deviseName = getDeviceName();

        String html =
            (String)"# HELP sensor_voltage Current voltage\n" +
            "# TYPE sensor_voltage counter\n" +
            "sensor_voltage{name=\"" + deviseName + "\"} " + (String)voltage + "\n" +

            "# HELP sensor_amperes Current amperes\n" +
            "# TYPE sensor_amperes counter\n" +
            "sensor_amperes{name=\"" + deviseName + "\"} " + (String)amperes + "\n"+

            "# HELP sensor_power Current power\n" +
            "# TYPE sensor_power counter\n" +
            "sensor_power{name=\"" + deviseName + "\"} " + (String)power + "\n"+

            "# HELP sensor_power_total Current power total\n" +
            "# TYPE sensor_power_total counter\n" +
            "sensor_power_total{name=\"" + deviseName + "\"} " + (String)powerTotal + "\n"+

            "# HELP sensor_frequency Current frequency\n" +
            "# TYPE sensor_frequency counter\n" +
            "sensor_frequency{name=\"" + deviseName + "\"} " + (String)frequency + "\n"+

            "# HELP sensor_power_factor Current power factor\n" +
            "# TYPE sensor_power_factor counter\n" +
            "sensor_power_factor{name=\"" + deviseName + "\"} " + (String)powerFactor + "\n"+

            "# HELP sensor_wifi_signal Wifi Signal\n" +
            "# TYPE sensor_wifi_signal counter\n" +
            "sensor_wifi_signal{name=\"" + deviseName + "\"} " + (String)wifiSignal + "\n"+

            "# HELP sensor_wifi_signal_percent Wifi Signal Percent\n" +
            "# TYPE sensor_wifi_signal_percent counter\n" +
            "sensor_wifi_signal_percent{name=\"" + deviseName + "\"} " + (String)wifiSignalPercent;

        http.send(200, "text/plain", html);
    });


    http.on("/metrics.json", HTTP_GET, []() {

        String html =
            (String)"{" +
            "\"name\": \"" + (String)getDeviceName() + "\"," +
            "\"voltage\": \"" + (String)voltage + "\"," +
            "\"amperes\": \"" + (String)amperes + "\"," +
            "\"power\": \"" + (String)power + "\"," +
            "\"powerTotal\": \"" + (String)powerTotal + "\"," +
            "\"frequency\": \"" + (String)frequency + "\"," +
            "\"powerFactor\": \"" + (String)powerFactor + "\"," +
            "\"wifi_signal\": \"" + (String)wifiSignal + "\"," +
            "\"wifi_signal_percent\": \"" + (String)wifiSignalPercent + "\"" +
            "}";

        http.send(200, "application/json", html);
    });


    http.on("/reset", HTTP_GET, []() {

        resetWifi();

        http.send(200, "application/json", "Reboot...");
    });


    http.begin();
}


/**
 *
 */
void setupWifi() {

    String deviceName = getDeviceName();

    //Set new hostname
    WiFi.hostname(deviceName.c_str());
    wifiManager.setHostname(deviceName.c_str());

    //Assign fixed IP
    wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    wifiManager.autoConnect(deviceName.c_str(), SETUP_WIFI_PASS);
}


/**
 * Установка данных PZEM
 */
void setPzemValues() {

    float pz_voltage   = pzem.voltage();
    float pz_amperes   = pzem.current();
    float pz_power     = pzem.power();
    float pz_energy    = pzem.energy();
    float pz_frequency = pzem.frequency();
    float pz_pf        = pzem.pf();

    if ( ! isnan(pz_voltage)) {
        voltage = pz_voltage;
    } else {
        voltage = 0;
        debugRow("Error reading voltage");
    }

    if ( ! isnan(pz_amperes)) {
        amperes = pz_amperes;
    } else {
        amperes = 0;
        debugRow("Error reading amperes");
    }

    if ( ! isnan(pz_power)) {
        power = pz_power;
    } else {
        power = 0;
        debugRow("Error reading power");
    }

    if ( ! isnan(pz_energy)) {
        powerTotal = pz_energy;
    } else {
        powerTotal = 0;
        debugRow("Error reading energy");
    }

    if ( ! isnan(pz_frequency)) {
        frequency = pz_frequency;
    } else {
        frequency = 0;
        debugRow("Error reading frequency");
    }

    if ( ! isnan(pz_pf)) {
        powerFactor = pz_pf;
    } else {
        powerFactor = 0;
        debugRow("Error reading power factor");
    }
}


/**
 * Установка данных wifi
 */
void setWifiValues() {

    int wifiSignalValue        = WiFi.RSSI();
    int wifiSignalPercentValue = getRSSIasQuality(WiFi.RSSI());
    uint8_t wifiStatusRaw      = WiFi.status();
    wifiSSID                   = WiFi.SSID();

    switch (wifiStatusRaw) {
        case WL_NO_SHIELD:       wifiStatus = "NO_SHIELD"; break;
        case WL_IDLE_STATUS:     wifiStatus = "IDLE_STATUS"; break;
        case WL_NO_SSID_AVAIL:   wifiStatus = "NO_SSID_AVAIL"; break;
        case WL_SCAN_COMPLETED:  wifiStatus = "SCAN_COMPLETED"; break;
        case WL_CONNECTED:       wifiStatus = "CONNECTED"; break;
        case WL_CONNECT_FAILED:  wifiStatus = "CONNECT_FAILED"; break;
        case WL_CONNECTION_LOST: wifiStatus = "CONNECTION_LOST"; break;
        case WL_DISCONNECTED:    wifiStatus = "DISCONNECTED"; break;
        default:                 wifiStatus = "UNDEFINED"; break;
    }


    if (wifiStatusRaw == WL_CONNECTED) {
        wifiIPAddress = WiFi.localIP().toString();
    } else {
        wifiIPAddress = "";
    }

    if ( ! isnan(wifiSignalValue)) {
        wifiSignal = wifiSignalValue;
    } else {
        wifiSignal = 0;
    }

    if ( ! isnan(wifiSignalPercentValue)) {
        wifiSignalPercent = wifiSignalPercentValue;
    } else {
        wifiSignalPercent = 0;
    }
}


/**
 * Сброс настроек wifi
 */
void resetWifi() {

    wifiManager.resetSettings();
    ESP.restart();
}


/**
 * Вывод на дисплей данных pzem
 */
void displayIndicators() {

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    display.println(String("") + voltage + "V");
    display.println(String("") + amperes + "A");
    display.println(String("") + power + "W");
    display.println(String("") + frequency + "freq");
    display.display();
}


/**
 * Вывод на дисплей данных pzem
 */
void displayIndicators2() {

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    display.println(String("") + powerFactor + "pf");
    display.println(String("") + powerTotal + "kW/h");
    display.display();
}


/**
 * Вывод на дисплей данных wifi
 */
void displayWifi() {

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);


    display.println(String("WiFi: ") + wifiStatus);
    display.println(String("Name: ") + wifiSSID);
    display.println(String("IP: ") + wifiIPAddress);
    display.println(String("Signal: ") + wifiSignal + "dbi " + wifiSignalPercent + "%");
    display.display();
}


/**
 * Вывод на дисплей данных для подключения к wifi
 */
void displayWifiSetup() {

    String name = getDeviceName();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.println(String("Setup"));
    display.println(String("Connect to wifi"));
    display.println(String(""));

    display.setTextSize(2);
    display.println(name);
    display.display();
}


/**
 * Получение качества Wifi в процентах
 * @param RSSI
 * @return int
 */
int getRSSIasQuality(int RSSI) {
    int quality = 0;

    if (RSSI <= -100) {
        quality = 0;
    } else if (RSSI >= -50) {
        quality = 100;
    } else {
        quality = 2 * (RSSI + 100);
    }
    return quality;
}


/**
 * Название устройства
 * @return String
 */
String getDeviceName() {

    String hostString = String(ESP.getChipId(), HEX);
    hostString.toUpperCase();

    return (String)DEVISE_NAME + "_" + hostString;
}
