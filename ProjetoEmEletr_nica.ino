

/* ======================================== Bibliotecas */
#include "esp_camera.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include "ESP32_MailClient.h"
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
/* ======================================== */

/* ======================================== Defining variables for Email */

#define emailSenderAccount      "sistemaseguranca31@gmail.com"
#define emailSenderAppPassword  "owma vffx dezt ywep"
#define smtpServer              "smtp.gmail.com"
#define smtpServerPort          465
#define emailSubject            "ESP32-CAM Photo Captured"
#define emailRecipient          "mateus.rs10002@gmail.com"
/* ======================================== */

/* ======================================== Defining the Camera Model and GPIO */
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not selected"
#endif
/* ======================================== */
 
#define FILE_PHOTO "/photo.jpg" //--> Photo File Name to save in SPIFFS

/* ======================================== Variables for network */
// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "MATEUSespcam";
const char* password = "12345678";
/* ======================================== */

SMTPData smtpData; //--> The Email Sending data object contains config and data to send
WiFiServer server(80);
/* ________________________________________________________________________________ Subroutine for Capture Photo and Save it to SPIFFS */
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; //--> pointer
  bool ok = 0; //--> Boolean indicating if the picture has been taken correctly

  /* ---------------------------------------- The process of capturing photos and saving to SPIFFS */
  do {
    Serial.println("Taking a photo...");

    /* :::::::::::::::::::::::::::::::::::::::::::::::: Take a photo with the camera */
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed.");
      return;
    }
    /* :::::::::::::::::::::::::::::::::::::::::::::::: */
    
    /* :::::::::::::::::::::::::::::::::::::::::::::::: Photo file name */
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
    /* :::::::::::::::::::::::::::::::::::::::::::::::: */

    /* :::::::::::::::::::::::::::::::::::::::::::::::: Insert the data in the photo file */
    if (!file) {
      Serial.println("Failed to open file in writing mode.");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes.");
    }
    /* :::::::::::::::::::::::::::::::::::::::::::::::: */
    
    file.close(); //--> Close the file
    esp_camera_fb_return(fb); //--> return the frame buffer back to the driver for reuse.

    /* :::::::::::::::::::::::::::::::::::::::::::::::: check if file has been correctly saved in SPIFFS */
    Serial.println("Checking if the picture file has been saved correctly in SPIFFS...");
    ok = checkPhoto(SPIFFS);
    if (ok == 1) {
      Serial.println("The picture file has been saved correctly in SPIFFS.");
    } else {
      Serial.println("The picture file is not saved correctly in SPIFFS.");
      Serial.println("Recapture photos.");
      Serial.println();
    }
    /* :::::::::::::::::::::::::::::::::::::::::::::::: */
  } while ( !ok );
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to check if photos are saved correctly in SPIFFSl */
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for send photos via Email */
void sendPhoto( void ) {
  Serial.println("Sending email...");

  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderAppPassword); 
  
  // Set the sender name and Email
  smtpData.setSender("ESP32-CAM UTEH STR", emailSenderAccount); 
  
  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High"); 

  // Set the subject
  smtpData.setSubject(emailSubject); 

  // Set the email message in HTML format
  smtpData.setMessage("<h2>Photo captured with ESP32-CAM and attached in this email.</h2>", true); 
  // Set the email message in text format
  //smtpData.setMessage("Photo captured with ESP32-CAM and attached in this email.", false); 

  // Add recipients, can add more than one recipient
  smtpData.addRecipient(emailRecipient); 
  //smtpData.addRecipient(emailRecipient2);

  // Add attach files from SPIFFS
  smtpData.addAttachFile(FILE_PHOTO, "image/jpg"); 

  // Set the storage type to attach files in your email (SPIFFS)
  smtpData.setFileStorageType(MailClientStorageType::SPIFFS); 

  // sendCallback
  smtpData.setSendCallback(sendCallback); 
  
  // Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData))
  Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

  // Clear all data from Email object to free memory
  smtpData.empty();
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine to get the Email sending status */
// Callback function to get the Email sending status
void sendCallback(SendStatus msg) {
  Serial.println(msg.info()); //--> Print the current status
}
/* ________________________________________________________________________________ */





 
/* ________________________________________________________________________________ VOID SETTUP() */
void setup() {
  // put your setup code here, to run once:

  pinMode(12,INPUT);  //pino pir
   pinMode(13,OUTPUT);  //botão app
  
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //--> disable brownout detector
  
  Serial.begin(115200);
  delay(1000);
  Serial.println();

  /* ---------------------------------------- Connect to Wi-Fi */
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  /* ---------------------------------------- */

  /* ---------------------------------------- Print ESP32 Local IP Address */
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* ---------------------------------------- */
 server.begin();
  /* ---------------------------------------- Starting to mount SPIFFS */
  Serial.println("Starting to mount SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    Serial.println("ESP32 Cam Restart...");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Camera configuration. */
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; //--> FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    /*
     * From source https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/ :
     * - The image quality (jpeg_quality) can be a number between 0 and 63.
     * - Higher numbers mean lower quality.
     * - Lower numbers mean higher quality.
     * - Very low numbers for image quality, specially at higher resolution can make the ESP32-CAM to crash or it may not be able to take the photos properly.
     */
    config.jpeg_quality = 20;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Initialize camera */
  Serial.println();
  Serial.println("Camera initialization...");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.print("Camera initialization was successful.");
  Serial.println();
  /* ---------------------------------------- */
  
}
/* ________________________________________________________________________________ */
int value = 0;
/* ________________________________________________________________________________ VOID LOOP */
void loop() {
  if(digitalRead(13)==1 and digitalRead(12)==1 ){
    capturePhotoSaveSpiffs(); //--> Calling the capturePhotoSaveSpiffs() subroutine.
  sendPhoto(); //--> Calling the sendPhoto() subroutine.

  
  }
 
  delay(10);


   WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 13 on.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 13 off.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(13, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(13, LOW);                // GET /L turns the LED off
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");

    
  }
  Serial.print(digitalRead(12));
    Serial.print(digitalRead(13));
    delay(500);
  
}
/* ________________________________________________________________________________ */
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
