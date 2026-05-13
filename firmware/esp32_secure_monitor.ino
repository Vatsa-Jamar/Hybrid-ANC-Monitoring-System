#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "mbedtls/aes.h"
#include <Base64.h>

// WiFi and ThingSpeak credentials
const char* ssid = "Redmi";
const char* password = "password";
const char* channelID = "3136461";
const char* writeAPIKey = "40WOIJ6AJTE9OS2K";
const char* geoUrl = "http://ip-api.com/json";

// Hardware Pins
const int SOUND_SENSOR_PIN = 32;
const int ANALOG_INPUT_PIN = 34; // An extra analog input
const int DAC_OUTPUT_PIN = 25;   // DAC for audio monitoring

// AES-128 Key and Initialization Vector (IV)
byte aesKey[16] = { 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
                    0x38,0x39,0x41,0x42,0x43,0x44,0x45,0x46 };
byte aesIv[16]  = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F };

// Function to encrypt data with AES-128-CBC and encode with Base64
String aesEncryptCBC(const byte* input, size_t input_len, const byte* key, const byte* iv) {
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    
    // Pad the input to be a multiple of 16 bytes (AES block size)
    size_t paddedLen = ((input_len + 15) / 16) * 16;
    byte* paddedInput = new byte[paddedLen];
    memset(paddedInput, 0, paddedLen);
    memcpy(paddedInput, input, input_len);
    
    byte* cipherText = new byte[paddedLen];
    byte ivCopy[16];
    memcpy(ivCopy, iv, 16); // IV is modified by the function, so we use a copy
    
    // Set the encryption key
    int ret = mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
    if(ret != 0) { return ""; }
    
    // Perform the encryption
    ret = mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, paddedLen, ivCopy, paddedInput, cipherText);
    if(ret != 0) { return ""; }
    
    mbedtls_aes_free(&aes_ctx);
    
    // Encode the binary ciphertext into a printable Base64 string
    String encoded = base64::encode(cipherText, paddedLen);
    delete[] paddedInput;
    delete[] cipherText;
    
    return encoded;
}

// Function to update the ThingSpeak Channel's location
bool updateChannelLocation(const char* channelID, const char* apiKey, float lat, float lon) {
    HTTPClient http;
    String url = String("http://api.thingspeak.com/channels/") + channelID + ".json";
    String jsonBody = "{\"channel\":{\"latitude\":\"" + String(lat, 6) + "\",\"longitude\":\"" + String(lon,6) + "\"}}";
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-THINGSPEAKAPIKEY", apiKey);
    
    int httpCode = http.PUT(jsonBody); // Use PUT to update channel settings
    http.end();
    
    return (httpCode == 200 || httpCode == 202); // Success
}

void setup() {
    Serial.begin(115200);
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    // Set ADC resolution to 12 bits (0-4095)
    analogReadResolution(12);
}

void loop() {
    // 1. Read the sound sensor
    int adcSound = analogRead(SOUND_SENSOR_PIN);
    
    // 2. Logic for a special reading (e.g., a secondary sensor)
    unsigned long uptimeSeconds = millis() / 1000;
    unsigned int vHour = 3 + (uptimeSeconds / 3600); 
    unsigned int vMinute = (uptimeSeconds % 3600) / 60;
    unsigned int vSecond = uptimeSeconds % 60;
    
    int adcExtra;
    if (vHour == 3 && vMinute == 0 && vSecond >= 30 && vSecond < 60) {
        adcExtra = analogRead(ANALOG_INPUT_PIN); // Read 2nd sensor
    } else {
        adcExtra = adcSound; // Default to main sensor
    }
    
    // 3. Output a simple monitor-through waveform on the DAC (8-bit)
    dacWrite(DAC_OUTPUT_PIN, adcSound / 16); // Scale 12-bit to 8-bit
    
    // 4. Prepare data for encryption
    byte plainData[16] = {0}; // 16-byte buffer
    plainData[0] = (adcSound >> 8) & 0xFF; // High byte of adcSound
    plainData[1] = adcSound & 0xFF;        // Low byte
    plainData[2] = (adcExtra >> 8) & 0xFF; // High byte of adcExtra
    plainData[3] = adcExtra & 0xFF;        // Low byte
    
    // 5. Encrypt and Encode
    String encryptedBase64 = aesEncryptCBC(plainData, 4, aesKey, aesIv);
    
    // 6. One-time location update
    static bool locationUpdated = false;
    if (!locationUpdated && WiFi.status() == WL_CONNECTED) {
        HTTPClient geoHttp;
        geoHttp.begin(geoUrl);
        int geoCode = geoHttp.GET();
        if (geoCode > 0) {
            String geoResponse = geoHttp.getString();
            StaticJsonDocument<1024> doc;
            if (deserializeJson(doc, geoResponse) == DeserializationError::Ok) {
                float lat = doc["lat"];
                float lon = doc["lon"];
                locationUpdated = updateChannelLocation(channelID, writeAPIKey, lat, lon);
            }
        }
        geoHttp.end();
    }
    
    // 7. Send encrypted data to ThingSpeak
    if (encryptedBase64.length() > 0) {
        HTTPClient httpPost;
        httpPost.begin("http://api.thingspeak.com/update");
        httpPost.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        // Post the encrypted string to Field 1
        int postCode = httpPost.POST("api_key=" + String(writeAPIKey) + "&field1=" + encryptedBase64);
        httpPost.end();
    }
    delay(15000); // ThingSpeak has a 15-second rate limit
}