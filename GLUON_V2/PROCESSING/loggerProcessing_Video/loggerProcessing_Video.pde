import com.hamoid.*;
import processing.video.*;
import processing.serial.*;

String nameScenario = "test";//scenario6b_Aruko";

Capture video;
VideoExport videoExport;
Serial port;   // serial port object

PrintWriter outputfile;
StringList logList;
PFont fontA;
int timeStamp, startTimeStamp=millis();

boolean recording = false;
boolean firstTime= true;

int heightRow = 20; 
int numRowsLog;

void settings() {
  size(640*2, 480);
  //fullScreen();
  
  numRowsLog = 480 / heightRow;
  
}


void setup() {
  background(0);
   noStroke();
  smooth();
  
  fontA = loadFont("ArialMT-48.vlw"); 
  textFont(fontA, heightRow - 4); // Set the font and its size (in units of pixels)

  setupVideoCapture();

  setVideoExport();
 
  // Create a new file in the sketch directory
  outputfile = createWriter(nameScenario+".txt");
  logList= new StringList();

  println(Serial.list());
  String portName = Serial.list()[2];
  // String portName = "/dev/ttyUSB0";
  port = new Serial(this, portName, 57600);   // from the sniffer gluon

  // Set the trigger for the serialEvent function: a call to this function will be placed whenever the buffer
  // receives a carriage return code. This is better than 
  // checking all the time to see if something is in the buffer while the packet is not completed.
  port.bufferUntil('\n'); 
  //Note: on Arduino side, I use println which produces a human-readable ASCII text followed by a newline character (ASCII 10, or '\n')
  // followed by a carriage return character (ASCII 13, or '\r').

  println("Press R to toggle recording");
  println("Press ENTER to SAVE recording");
  println("Press DELETE to START again (delete all recording)");
}

void draw() {
  //background(0);

  if (video.available()) { 
    video.read(); 
    image(video, 0, 0, 640, 480);
  }

 
  fill(0, 0, 0, 50); noStroke();
  rect(640, 0, 640, 480);
  int startRow = (logList.size() < numRowsLog? 0 : logList.size()-numRowsLog);
  fill(20, 180, 20); textSize(heightRow-4); 
  for (int i=startRow; i<logList.size(); i++) 
    text(str(i) + ":    "+logList.get(i), 640+10, heightRow*(i+1-startRow));

  // Video Timestamp:

  // REAL CLOCK: 
  //int s = second();  // Values from 0 - 59
  //int m = minute();  // Values from 0 - 59
  //int h = hour();    // Values from 0 - 23
  //text(str(h)+":"+str(m)+":"+str(s), 40, height-30); 
  
  timeStamp=millis()-startTimeStamp;
  int h = timeStamp / 3600000;
  int m = (timeStamp - h*3600000)/60000;
  int s = (timeStamp - h*3600000 - m*60000)/1000;

   fill(100, 100, 100);
   rect(35, height-25, 97, -30);
   fill(255,255,255); textSize(26); 
   text(str(h)+":"+(m>9? str(m) : "0"+str(m)) +":"+(s>9? str(s) : "0"+str(s)), 40, height-30); 
  //text("Time Stamp: " + str(timeStamp), 40, height-30); 

  if (recording) {
    fill(255, 0, 0);
    ellipse(40, 50, 10, 10);
    videoExport.saveFrame();
  } else {
    noFill();
    stroke(255, 0, 0);
    ellipse(40, 50, 10, 10);
  }
}

void serialEvent(Serial serialPort) {

  String message = serialPort.readStringUntil('\n'); 
  
  timeStamp=millis()-startTimeStamp;
   
  String myStringLog=str(timeStamp)+ " " +message; 
  outputfile.print(myStringLog);
   
  String myStringDraw="T = "+ str(timeStamp)+"ms      MSG:  "+message; 
  logList.append(myStringDraw);
}

void keyPressed() {

  if (key == CODED) { // (UP, DOWN, LEFT, and RIGHT) as well as ALT, CONTROL, and SHIFT. 
    // NOTE: BACKSPACE, TAB, ENTER, RETURN, ESC, and DELETE do not require keyCode
    // if (keyCode==CONTROL) {}
 } 
 
 else if ((key==ENTER)||(key==RETURN)) {
    println("SAVING FILE..."); 
    outputfile.flush();  // Writes the remaining data to the file
    outputfile.close();  // Finishes the file
    println("SAVED. EXIT PROGRAM.");
    videoExport.dispose();
    exit();  // Stops the program
  } 

else if (key == 'c' || key == 'C') {
  if (firstTime) {
      clearLog();
      setGluonsSnifferMode();
      firstTime=false;
    }
}

else if (key == 's' || key == 'S' ) scanNetwork();

else if (key == 'r' || key == 'R') {
    recording = !recording;
    println("Recording is " + (recording ? "ON" : "OFF"));
  } 

else if (key == BACKSPACE || key == DELETE) {
    recording = false;
    firstTime=true;
    println("Recording is OFF");
  }
  
}