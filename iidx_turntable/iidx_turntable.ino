#define rotate_rate 80 //回転閾値(逆回転誤判定防止)
#define pulse_rate 3 //分解閾値(digi_cntカウント速度)
#define start_rate 2 //digi_cntがいくら以上で押下判定を加えるか
#define stop_rate 23//1ミリ秒周期で、いくら値が変わったかを比較し閾値以内の場合停止したと判断しallreleaceする
#define push_rate 20 //キー出力判定時、どの程度の周期(ミリ秒)入力を実施するか

#include <Keyboard.h>

const int A_pin = 3; // 割り込みピン
const int B_pin = 4;
const char key_A = 'c';
const char key_B = 'v';

bool on_released = false;
int count_a = 0;
int count_b = 0;

int cnt_all = 0;
long cnt_allrotate = 0;

int digi_cnt = 0;
int old_cnt = 0;
 
void setup() {
  pinMode(A_pin, INPUT_PULLUP);
  pinMode(B_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(A_pin), pulse_counter, CHANGE);
  //Serial.begin(9600);
  // initialize control over the keyboard:
  Keyboard.begin();
}
 
void loop() {
  //0の時は強制リリース
  if (digi_cnt == 0 && on_released == false) {
    //Serial.println("R");
    Keyboard.releaseAll();
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

  delay(1);
}
 
void pulse_counter() {
  on_released = false;
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

  //スタートレート以上ならキー出力
  if (digi_cnt == start_rate) {
    //Serial.println("A");
    Keyboard.press(key_A);
    digi_cnt++; //重複動作防止
  } else if (digi_cnt == -start_rate) {
    //Serial.println("B");
    Keyboard.press(key_B);
    digi_cnt--; //重複動作防止
  }
  //Serial.println(digi_cnt);
}