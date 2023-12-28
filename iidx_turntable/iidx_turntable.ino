#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define PIN 10
#define NUMPIXELS 9

//!回転閾値(逆回転誤判定防止)
#define rotate_rate 3
//!分解閾値(digi_cntカウント速度)
#define pulse_rate 1 
//!loop関数実行間隔(μ秒)
#define loop_run 100

//digi_cntがいくら以上で押下判定を加えるか
#define start_rate 1
//loop関数周期で、いくら値が変わったかを比較し閾値以内の場合停止したと判断しallreleaceする
#define stop_rate 50
//キー出力判定時、どの程度の周期入力を実施するか
//#define push_rate 20 長押し判定のため削除
//キー入力後強制的に長押し以外の出力を禁止する時間
#define wait_time 1

//色遷移周期
#define rainbow_time 7

//#include <Keyboard.h>

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const int A_pin = 3; // 割り込みピン
const int B_pin = 4;
//const char key_A = 'c';
//const char key_B = 'v';
const int out_A = 5;
const int out_B = 6;

bool on_released = false;
int count_a = 0;
int count_b = 0;

int cnt_all = 0;
long cnt_allrotate = 0;

int digi_cnt = 0;
int old_cnt = 0;
int wait_cnt = 0;

int led_count = 0;
int led_mode = 0;
int led_loop = 0;
long firstPixelHue = 0;
int now_led = 0;
 
void setup() {
  pinMode(A_pin, INPUT_PULLUP);
  pinMode(B_pin, INPUT_PULLUP);
  pinMode(out_A, OUTPUT);
  pinMode(out_B, OUTPUT);

  Serial.begin(9600);

  digitalWrite(out_A, HIGH);
  digitalWrite(out_B, HIGH);

  attachInterrupt(digitalPinToInterrupt(A_pin), pulse_counter, CHANGE);
  // initialize control over the keyboard:
  //Keyboard.begin();
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  pixels.begin();

  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 0, 255)); //RGB
  }
  pixels.show();
}
 
void loop() {
  //Serial.println(wait_cnt);
  if (wait_cnt > 0) wait_cnt--;
  //0の時は強制リリース
  if (digi_cnt == 0 && on_released == false) {
    //Serial.pzqqqgrintln("R");
    //Keyboard.releaseAll();
    digitalWrite(out_A, HIGH);
    digitalWrite(out_B, HIGH);
    //

    on_released = true;
  } else if (digi_cnt == 0) {} 
  else { //digi_cntが動作している（何かしらのキーが押されている -> 比較し回転中と開店停止を検知
    cnt_allrotate += digi_cnt;
    cnt_all++;
    if (cnt_all % stop_rate == 0) { //比較するループ回の時、cnt_allrotateをdigi_cntで割り切れたら値が変わってないと判断する
      if (cnt_allrotate % digi_cnt == 0) digi_cnt = 0;
      cnt_allrotate = 0;
      cnt_all = 0;
    }
  }
  
  //rainbow();
  rotate_led();
  delayMicroseconds(loop_run);
}
 
void pulse_counter() {
  on_released = false;
  if (wait_cnt != 0) return;
  //各回転検知
  if(digitalRead(A_pin) ^ digitalRead(B_pin)) {
    count_a = 0;
    count_b++;
  } else {
    count_a++;
    count_b = 0;
  }
  //int型制限
  if(count_a < 0 || count_b < 0) {
    count_a = 0;
    count_b = 0;
  }

  //回転中に一瞬だけ逆回転入力が入った時の判定阻止条件と分解レートを元にした値変動
  if(count_a > rotate_rate && count_a % pulse_rate == 0) {
    if(digi_cnt < 0) digi_cnt = 0;
    digi_cnt++;
  } else if(count_b > rotate_rate && count_b % pulse_rate == 0) {
    if(digi_cnt > 0) digi_cnt = 0;
    digi_cnt--;
  }
  Serial.println(digi_cnt);

  //スタートレート以上ならキー出力
  if (digi_cnt == start_rate) {
    if(!digitalRead(out_B)) digitalWrite(out_B, HIGH); //同時押し回避
    //Serial.println("A");
    //Keyboard.press(key_A);
    digitalWrite(out_A, LOW);
    wait_cnt = wait_time;
    digi_cnt++; //重複動作防止
  } else if (digi_cnt == -start_rate) {
    if(!digitalRead(out_A)) digitalWrite(out_A, HIGH); //同時押し回避
    //Serial.println("B");
    //Keyboard.press(key_B);
    digitalWrite(out_B, LOW);
    wait_cnt = wait_time;
    digi_cnt--; //重複動作防止
  }
  //Serial.println(digi_cnt);
}
/*
void rainbow() {
  led_count+=1;
  if (led_count != rainbow_time) {
    return;
  } else {
    led_count = 0;
  }
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<pixels.numPixels()*rainbow_time; i++) { 
      int pixelHue = firstPixelHue + (i/rainbow_time * 65536L / pixels.numPixels());
      pixels.setPixelColor(i/rainbow_time, pixels.gamma32(pixels.ColorHSV(pixelHue)));
    }
    pixels.show();
  }
}
*/
void rotate_led() {
  //nミリ秒ごとに実行
  led_count += 1;
  int n = 100;
  if (led_count != 1000/loop_run * n) {
    return;
  } else {
    led_count = 0;
  }

  if(digi_cnt >= start_rate) led_mode = 1;
  else if (digi_cnt <= -start_rate) led_mode = 2;
  else led_mode = 0;
  //Serial.println(led_mode);

  set_led();
  pixels.show();
}

void set_led() {
  if(led_loop == 0) {
    firstPixelHue = 0;
    now_led = 0;
  }
  led_loop += 1;

  if (led_mode == 0) {
    //Serial.println(firstPixelHue);
    if (firstPixelHue < 65536) {
      for(int i=0; i<pixels.numPixels()*rainbow_time; i++) { 
        int pixelHue = firstPixelHue + (i/rainbow_time * 65536L / pixels.numPixels());
        pixels.setPixelColor(i/rainbow_time, pixels.gamma32(pixels.ColorHSV(pixelHue)));
      } 
      firstPixelHue += 256*8;
    } else {
      firstPixelHue = 0;
    }
  } else if(led_mode == 1) {
    if (led_count % 10 == 0) now_led -= 1;
    if (now_led < 0 ) now_led = NUMPIXELS;
    for(int i=0; i<NUMPIXELS; i++) {
      if (now_led >= 1 && now_led == i-1 ||
          now_led == i ||
          now_led <= NUMPIXELS-1 && now_led == i+1) pixels.setPixelColor(i, pixels.Color(255, 0, 0)); //RGB
      else pixels.setPixelColor(i, pixels.Color(0, 0, 0)); //RGB
    }

  } else if(led_mode == 2) {
    if (led_count % 10 == 0) now_led += 1;
    if (now_led >= NUMPIXELS) now_led = 0;
    for(int i=0; i<NUMPIXELS; i++) {
      if (now_led >= 1 && now_led == i-1 ||
          now_led == i ||
          now_led <= NUMPIXELS-1 && now_led == i+1) pixels.setPixelColor(i, pixels.Color(0, 0, 255)); //RGB
      else pixels.setPixelColor(i, pixels.Color(0, 0, 0)); //RGB
    }
  }
}