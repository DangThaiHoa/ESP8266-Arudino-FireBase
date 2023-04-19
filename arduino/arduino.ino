//RGB Led Pin
int redPin = 9;  
int greenPin = 10; 
int bluePin = 11;
int Relay = 12;  

//SplitString
String splitString (String str, String delim, uint16_t pos){
  String tmp = str;

  for(int i=0;i<pos;i++){
    tmp = tmp.substring(tmp.indexOf(delim)+1);

    if(tmp.indexOf(delim) == -1 && i != pos - 1)
      return "";
  }
  return tmp.substring(0,tmp.indexOf(delim));
}

void setup() {
  Serial.begin(115200);
  //config RGB
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(Relay, OUTPUT);
}

void loop() {

  //LED RGB
  RGB();

}

void RGB(){

  if(Serial.available()){

    String msg = Serial.readString();
  
    String red = splitString(msg,",",0);
    String green = splitString(msg,",",1);
    String blue = splitString(msg,",",2);
    String gTrig = splitString(msg,":",1);
    Serial.print(gTrig);

    if(gTrig == "1"){
      digitalWrite(Relay,HIGH);
    }else{
      digitalWrite(Relay,LOW);
    }
    
    analogWrite(redPin,red.toInt());
    analogWrite(greenPin,green.toInt());
    analogWrite(bluePin,blue.toInt());
  }
}
