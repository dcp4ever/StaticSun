//**************************************************************
//  Name    : Simple Arduino Sun Tracking / Heliostat Control Program  
//  Author  : Phillip Daniel                              
//  Notes   : Code for controlling Servo based solar machines. Based on code by Gabriel Miller (http://www.cerebralmeltdown.com).      
//****************************************************************  
  //////////////////////////////////////////////////  
  //Importing Libraries
  //////////////////////////////////////////////////  
    #include <avr/pgmspace.h>
    #include "Wire.h"  
    #include <Flash.h>  
    #include <Servo.h>
    #include <Helios.h>
    #include <DS1107H.h>
    Helios helios;
    DS1107H clock;

  //////////////////////////////////////////////////  
  //Declaring Variables - User Input
  //////////////////////////////////////////////////  
    //The device will act like a heliostat if this is 1 and a solar panel if this is 0
    int heliostat=0; 
  
    //Put your latitude and longitude here
    float latitude = 42.35;
    float longitude = -71.1;
    
    //Your Heliostat target, with respect to helios's coordinates
    float targetalt = 110;
    float targetaz = 90;
    
  //////////////////////////////////////////////////  
  //Declaring Variables
  //////////////////////////////////////////////////  
    double dAzimuth; //The sun's azimuth angle
    double dElevation; // The sun's elevation/altitude angle
    double offset_Elv=90; //The angular offset to center the Helios's range of motion with the sun's range of motion
    double offset_Az=270; //The angular offset to center the Helios's range of motion with the sun's range of motion.
    float altcommand; //The helios's commanded altitute/elevation angle
    float azcommand; //The helios's commanded azimuch angle
    float altitude, azimuth, delta, h; //Some variables used in functions later
  
  //////////////////////////////////////////////////  
  //Attaching Servos
  //////////////////////////////////////////////////  
    Servo altServoMotor;
    int altServoPin = 6;
    Servo azServoMotor;
    int azServoPin = 9;
  
void setup(){
  //////////////////////////////////////////////////  
  //To set the timer chip specify these numbers based on the time that you upload your code. This is the UTC time.
  //////////////////////////////////////////////////  
    byte second=00;
    byte minute=8;
    byte hour=16;
    byte dayOfWeek=5;
    byte dayOfMonth=6;
    byte month=6;
    byte year=13;
    //REMOVE "//" BELOW WHEN SETTING THE TIME AND REPLACE WHEN FINISHED.
    //clock.setDate(second,minute,hour,dayOfWeek,dayOfMonth,month,year);    
  
  //////////////////////////////////////////////////  
  //Initiating libraries and setting data rate
  //////////////////////////////////////////////////  
    Wire.begin();
    Serial.begin(9600);
    Serial.println("Start Void Setup");
  
  //////////////////////////////////////////////////  
  //Attaching Servos
  //////////////////////////////////////////////////  
    altServoMotor.attach(altServoPin);
    azServoMotor.attach(azServoPin);
}

void loop(){ 
  //////////////////////////////////////////////////  
  //Find the location of the sun
  //////////////////////////////////////////////////  
    clock.getDate();  
    helios.calcSunPos(clock.year,clock.month,clock.dayOfMonth,
    clock.hour, clock.minute,clock.second,longitude,latitude); 
    
    showTime(clock);
    dAzimuth=helios.dAzimuth;show("dAzimuth",dAzimuth,true);
    dElevation=helios.dElevation;show("dElevation",dElevation,true);       

  //////////////////////////////////////////////////  
  //Act like solar panel 
  ////////////////////////////////////////////////// 
    if (heliostat==0){
      altcommand=offset_Elv-dElevation; //So that the servo's range of motion is aligned with the sun's visable range of motion
      azcommand=offset_Az-dAzimuth; //So that the servo's range of motion is aligned with the sun's visable range of motion
      altServoMotor.write(altcommand); //Command the altitude servo to the reported position
      Serial.println(" ");
      Serial.print("Commanded Elevation =");
      Serial.println( altServoMotor.read()); //Print the angle the servo moved to
      Serial.println(" ");
      delay(5000); //Give the Altitude servo time to move to position, so the source voltage doesn't drop below the 4.5 volt limit of the servo
      azServoMotor.write(azcommand); //Command the azimuth servo to the reported position
      Serial.print("Commanded Azimuth =");
      Serial.println(azServoMotor.read()); //Print the angle the servo moved to
      Serial.println(" ");
      delay(5000); //Give the Azimuth servo time to move to position, so the source voltage doesn't drop below the 4.5 volt limit of the servo
     }
      
  //////////////////////////////////////////////////  
  //Act like heliostat
  ////////////////////////////////////////////////// 
    if (heliostat==1) {
    altcommand= FindHeliostatAngle(dElevation, dAzimuth, targetalt, targetaz, 1); //Shifted so that the servo's zero is aligned with the coord. sys zero
    azcommand= FindHeliostatAngle(dElevation, dAzimuth, targetalt, targetaz, 2); //Shifted so that the servo's zero is centered on the coordinate systems values for of 180degrees
    altServoMotor.write(altcommand); //Command the altitude servo to the reported position
    Serial.println(" ");
    Serial.print("Commanded Elevation =");
    Serial.println( altServoMotor.read()); //Print the angle the servo was told to move to
    Serial.println(" ");
    delay(5000); //Give the Altitude servo time to move to position, so the source voltage doesn't drop below 4.5 volts
    azServoMotor.write(azcommand); //Command the azimuth servo to the reported position
    Serial.print("Commanded Azimuth =");
    Serial.println(azServoMotor.read()); //Print the angle the servo moved to
    Serial.println(" ");
    delay(5000); //Give the Altitude servo time to move to position, so the source voltage doesn't drop below 4.5 volts
    }  
}
 
void show(char nameStr[], double val, boolean newline) {
  Serial.print(nameStr);  
  Serial.print("=");
  if (newline)
    Serial.println(val);
  else Serial.print(val);
}

void showTime(DS1107H timerChip) {
  Serial.print("UT ");
  Serial.print(timerChip.hour, DEC);
  Serial.print(":");
  Serial.print(timerChip.minute, DEC);
  Serial.print(":");
  Serial.print(timerChip.second, DEC);
  Serial.print("  ");
  Serial.print(timerChip.month, DEC);
  Serial.print("/");
  Serial.print(timerChip.dayOfMonth, DEC);
  Serial.print("/"); 
  Serial.print(timerChip.year, DEC);
  Serial.print("  Day_of_week:");
  Serial.println(timerChip.dayOfWeek, DEC);  
}
//////////////////////////////////////////////////  
//This code calculates the angles for the heliostat
//(returnaltaz = 1 will return alt, 2 returns az)
////////////////////////////////////////////////// 
  float FindHeliostatAngle(float altitude, float azimuth, float targetalt, float targetaz, int returnAltAz){
    float x,y,z,z1,z2,x1,x2,y1,y2,hyp,dist,machinealt,machineaz;
    
      //////////////////////////////////////////////////  
      //The cartisian location of the sun with respect to Helios
      ////////////////////////////////////////////////// 
        z1 = cos(to_rad(offset_Elv-altitude)); //The altitude with respect to helios
        hyp =sin(to_rad(offset_Elv-altitude));//The altitude with respect to helios
        x1 = hyp*cos(to_rad(offset_Az-azimuth));//The azimuth with respect to helios
        y1 = hyp*sin(to_rad(offset_Az-azimuth));//The azimuth with respect to helios
        
      //////////////////////////////////////////////////  
      //The cartisian location of the target with respect to Helios
      ////////////////////////////////////////////////// 
        z2 = cos(to_rad(targetalt)); //The target with respect to helios
        hyp = sin(to_rad(targetalt)); //The target with respect to helios
        x2 = hyp*cos(to_rad(targetaz)); //The target with respect to helios
        y2 = hyp*sin(to_rad(targetaz)); //The target with respect to helios
    
      //////////////////////////////////////////////////  
      //The cartisian definition for the vector pointing to along 
      //the bisector of the sun's location and the target's location
      //////////////////////////////////////////////////     
        x=(x1+x2);
        y=(y1+y2);
        z=(z1+z2);
        dist=sqrt(x*x+y*y+z*z);
        
      //////////////////////////////////////////////////  
      //The spherical definition of the vector pointing along 
      //the bisector of the sun's location and the target's location
      ////////////////////////////////////////////////// 
        if ((dist>-0.00000001) && (dist <0.00000001)){
          Serial.println('Impossible to reflect to target with these angles');
        }else{
          machinealt=to_deg(acos(z/dist)); //Angle below the vertical, how this servo is controlled.
        }
        
        if (x==0){x=0.000001;}
        machineaz=to_deg(atan2(y,x));
        if (returnAltAz == 1){return machinealt;}
        if (returnAltAz == 2){return machineaz;} 
  }
        
  float to_rad(float angle){
    return angle*(pi/180);
  }
  
  float to_deg(float angle){
    return angle*(180/pi);
}
