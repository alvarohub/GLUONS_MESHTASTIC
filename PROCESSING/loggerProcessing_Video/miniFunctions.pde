void setupVideoCapture() {
   
  String[] cameras = Capture.list();
  
  if (cameras.length == 0) {
    println("There are no cameras available for capture.");
    exit();
  } else {
    println("Available cameras:");
    for (int i = 0; i < cameras.length; i++) {
      println(cameras[i]);
    }
  }  
  
   // The camera can be initialized directly using an 
    // element from the array returned by list():
    video = new Capture(this,  640, 480, cameras[0], 30); // width, height);
    //Capture(parent, requestWidth, requestHeight, cameraName, frameRate)
    video.start();  

}



void clearLog() {
  logList.clear();
  // how to clear the recorded video?
}

void setGluonsSnifferMode() {
  // Set gluons in sniffer mode: 
  port.write('2'); // activate sniffer mode VERBOSE on all the gluons
  delay(500);
  port.write('3');// synch gluon clocks 
  startTimeStamp=millis();// also, reset local time-stamp
  delay(500);
}

void scanNetwork() {
  port.write('4'); // scan network
}

 void setVideoExport() {
  
  videoExport = new VideoExport(this, nameScenario+".mp4");
  
  // Set video and audio quality.
  // Video quality: 100 is best, lossless video (but big file size)
  //   Set it to 0 (worst) if you enjoy poor quality videos :)
  //   70 is the default.
  // Audio quality: 128 is the default, 192 very good,
  //   256 is near lossless but big file size.
  videoExport.setQuality(80, 128);
 
  // This sets the frame rate of the resulting video file. I has nothing to do
  // with the current Processing frame rate. For instance you could have a 
  // Processing sketch that does heavy computation and renders only one frame 
  // every 5 seconds, but here you could still set that the resulting video should 
  // play at 30 frames per second.  
  videoExport.setFrameRate(20);  
  
  // If your sketch already calls loadPixels(), you can tell videoExport to 
  // not do that again. It's not necessary, but your sketch may perform a 
  // bit better if you avoid calling it twice.
  //videoExport.dontCallLoadPixels();
  
  // If video is being exported correctly, you can call this function to avoid
  // creating .txt files containing debug information.
 // videoExport.dontSaveDebugInfo();
  
 }
