# Valdis

## First test bench<br>
![Test](Pics/TestBench.jpg)

## Info

**HETKEL PINOUT ON KÕIK VALE**<br>
Läksime üle PCB designile ja kasutuses nüüd ESP32 S3 (NanoS3)

**Mini-battle bot Läti võistluseks (450g)**

### Komponendid

* 3-phase relva mootor
* ESC ([Emax_bullet_35A](https://www.hobbydrone.cz/en/emax-bullet-35a/))
* 2x Mootorit liikumiseks (kursuse omad)
* 1x TB6612FNG ([Kiip](https://www.digikey.com/en/products/detail/toshiba-semiconductor-and-storage/tb6612fng-c-8-el/1730070) mitte breakout)
* 1x ESP32-S3 ([NanoS3](https://unexpectedmaker.com/shop.html#!/NanoS3/p/577095557/category=0))
* 3S aku
* USB-C ([USB4085-GF-A](https://www.digikey.com/en/products/detail/gct/USB4085-GF-A/9859662))
* Switching 3.3V ([RAA2118054GP3#JA0](https://www.digikey.com/en/products/detail/renesas-electronics-corporation/RAA2118054GP3-JA0/22162357?s=N4IgTCBcDaIEoEEFgIwoBwAYCsAWA4gAoDMAxAFIKYgC6AvkA))

Kasutusse võtti ESP32-S3 (NanoS3 devkit)<br>
Meil vaja 13:<br>
* Driver võtab 3per mootor = 6 kokku
* Mootori enkooderid veel 2x2=4 kokku
* ESC võtab PWM, ehk 1 kokku
* EN nupp = 1 kokku
* Buzzer = 1 kokku

Lisaks on vaba pinne 7tk (+3 power pin):
* IO35
* IO36
* IO37
* IO9 (SCL)
* IO8 (SDA)
* RX
* TX

Power:
* 5V
* 3V3
* GND


### Keha

Keha tuleks Resin või mingi 3D print, kõik komponendid kaaluvad 202g, ehk 230g enam vähem mängimis ruumi.<br>
Enne tehtud proto (mis ka liiga väike), on 64g, et suht OK

Relvale teha mingi lindiga või sarnane ülekande mehhanism, et saaks mootori keresse peita ja ei saaks otse laksu teistelt

### Proge/RC

Kaugelt juhib läbi bluetoothi, arvatavasti sama millega Jaan juhib<br>
Puldiks võtta mingi Xbox controller millel on bluetooth receiver juba olemas

### MCU pinout

* Mootorid:
  * PWM
    * PWMA - IO15
    * PWMB - IO10
  * A IN
    * AIN2 - IO14
    * AIN1 - IO13
  * B IN
    * BIN2 - IO11
    * BIN1 - IO12

  * Enkooder Mootor 1 (A)
    * Kollane (B) - IO16
    * Roheline (A) - IO17
  * Enkooder Mootor 2 (B)
    * Kollane (B) - IO6
    * Rohleine (A) - IO7

* ESC PWM - IO18

* Buzzer - IO34

* DBG LED - IO5

* BAT VOL - IO1

### Power

* 12V aku - regulator - 3.3V
  * 3.3V toidab MCU ja DRV loogika ja enkooderi VCC
* 12V toidab DRV switching
* STBY - otse 3.3V ehk HIGH ja constant ON

Toite jaoks on analoog pin mis mõõdab seda
* Selle jaoks pinge jagur (1000R/200R, 12.6V/10.8V)
  * 2.1V when full
  * 1.8V when empty (just kui)

### Enkooder

Enkoodril on 7x35 impulssi pöörde kohta (just kui)<br>
Vaja teha koodis et kontrolliks kiirust ja hoiaks sama erinevalt pingest ja nh soidaks ka otse kui uks vasib ara

### ESC

ESC on väline, joodab otse külge, kannatab 35A ja on current surge protection ka olemas

### Switch

Power switch ühendub JST-PH-2.0 pistikuga, ja power lüliti kontroll on low-side FET