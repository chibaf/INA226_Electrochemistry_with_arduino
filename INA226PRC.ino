//version 2.0 Beta for INA226PRC
#include <Wire.h>
//INA226のレジスターセット
#define ADDRESS  B1000001  //HEX 0x40（INA226のアドレス）
#define CONFIG   B0000000  //HEX 0x00
#define SHUNTV   B0000001  //HEX 0x01
#define BUSV     B0000010  //HEX 0x02
#define POWER    B0000011  //HEX 0x03
#define CURRENT  B0000100  //HEX 0x04
#define CALIB    B0000101  //HEX 0x05
#define MASK     B0000110  //HEX 0x06
#define ALERT    B0000111  //HEX 0x07

//シリアルモニタ速度
#define Monitor 9600

//Battery Capacity CR2032 220mAh,{CR2477N 950mAh,CR2477 1000mAh}
//寿命予測する電池の容量を入力する。
#define BAT_Capacity 220

//ビーコンのアドバタイズ間隔 300msでの場合
#define Adv_Interval 300

//測定値の補正する係数
const float miri = 0.001f;
const float Hosei = 1.0f;

//パケットを100msで出した場合143.36msに少なくとも１回はビーコンからは発射されるので100msに補正する。
//140μsごとに連続的に計測し1024回の平均だと143.36msになる。この数値は当然ながら実際よりも低い数値になる。
//140μS * 1024times = 143.36ms 143.36 / 100 = 1.436 
const float Adj = 1.436f;

//測定の設定　マスター(Arduino)からスレーブ(INA226)へデータを書き込み要求する関数
void Slave_write(byte regset, unsigned short regdata)
{
  //通信開始　指定するスレーブ(0x40)を指定する。
  Wire.beginTransmission(ADDRESS);
  //データをスレーブに書き込みをする。
  Wire.write(regset);
  //バイト単位なので、上位桁から8ビットを送るので
  Wire.write(regdata >> 8);
  //のこり下位８ビットを書き込み要求。
  Wire.write(regdata);
  //スレーブに通信終了の信号を送る。
  Wire.endTransmission();  
}

//測定結果　マスター(Arduino)からスレーブ(INA226)へデータ送信要求し、スレーブからのデータ受信する関数。
unsigned short Slave_read(byte regset)
{
  
  //**　マスターの送信
  short rd = 0;
  //通信開始　指定するスレーブ(0x40)を指定する。
  Wire.beginTransmission(ADDRESS);
  //設定データをスレーブ（0x40）に送る。
  Wire.write(regset);
  //スレーブに通信終了を信号を送る。
  Wire.endTransmission(); 
  
  //** マスターの受信
  // スレーブ(0x40)からの計測データを要求する。2は２バイト（１６ビット）
  Wire.requestFrom(ADDRESS, 2);
  //受信できたバイト数だけ繰り返す。
  while(Wire.available()) {
  //左に１バイト分シフト。（スレーブからくる測定データは上位桁数から読むので8ビット分シフトする）
    rd = rd * 256;
    //reauestFromから受信したデータを取り出し、下の桁の8ビットに代入する。
    rd += Wire.read();
  }
  return rd;
}
void setup() {
  //シリアルモニタに9600bpsで表示する。
  Serial.begin(Monitor);
  //I2C通信を初期化する。
  Wire.begin();
  //コンフィグレーション(0x00)で測定の設定をスレーブに要求し、次のデータ（コンフィンングレーションレジスタ）0B100111011011111は、最下位ビット111は、シャントとバスを連続で測るモード、
  //４から６ビット目011はシャントの測定時間140μ、7ビットから９ビット目011は、Fバス側の測定時間140μ、
  //１０から１２ビット目は、111は１０２４回計測しその平均を出す。
  Slave_write(CONFIG, 0B100111011011111);
  Slave_write(CALIB,  2048);
}

void loop() {
  //計測結果のid
  static unsigned int c_id;
  float bus_v,shunt_v,bus_v125,c; 
  //CSVのフィールド
  static boolean pre;
  //バス電圧データを求め、1ビットあたり1.25mVなので
//  bus_v125 = Slave_read(BUSV) * miri * 1.25f;
  bus_v125 = Slave_read(BUSV) * 1.25f;
  //シャント間の電圧を求める。シャント抵抗の１ビットあたり2.5μVであり平均を１００msに補正する。
//  shunt_v = Slave_read(SHUNTV) * 0.000001 * 2.5f * Adj;
  shunt_v = Slave_read(SHUNTV) * 0.000001 * 2.5f;
  //1mAを0.1mAに修正する
  c = (Slave_read(CURRENT) / Hosei) * 0.1f;
//  c = (Slave_read(CURRENT) / Hosei) * Adj * 0.1f;
  c_id++;
  //CSVのフィールドを表示
//  if (pre == false){
//  Serial.println("id,BusVoltage(V),ShuntVoltage(V),Current(mA),Life(Days)");
//  pre = true;
//  }
  pre=true;
  //csvのレコードを表示
 // Serial.print(String((String)c_id + " "));
  Serial.print(bus_v125,7);
 // Serial.print(bus_v125,2);
  Serial.print(" ");
  Serial.print(shunt_v,7);
  Serial.print(" ");
  Serial.print(c,2);
  Serial.print(" ");
//  Serial.println((BAT_Capacity/c/24)*(Adv_Interval/100),0);
  Serial.println(0.0);
  //１秒に１回を表示する。
  delay(1000);
}

