#include <Wire.h>

#define VFD_REG_CTRL         0
#define VFD_REG_OFFSET       1
#define VFD_REG_SCROLL_LEN   2
#define VFD_REG_SCROLL_MODE  3
#define VFD_REG_SCROLL_SPEED 4
#define VFD_REG_DATA         10

#define VFD_SCROLL_DISABLE   0
#define VFD_SCROLL_LEFT      1
#define VFD_SCROLL_RIGHT     2
#define SCROLL_SPEED_MS      200

const int16_t I2C_ADDR = 0x10;
unsigned long lastMessageTime = 0;
const unsigned long INACTIVITY_TIMEOUT = 60000;
bool displayCleared = false;

// Functions for reading and writing

void vfd_read_regs(uint8_t reg, uint8_t* val, uint8_t len) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t) I2C_ADDR, (uint8_t) len);
  for (uint8_t index = 0; index < len; index++) {
    val[index] = Wire.read();
  }
  Wire.endTransmission();
}

void vfd_read_reg(uint8_t reg, uint8_t* val) {
  vfd_read_regs(reg, val, 1);
}

void vfd_write_regs(uint8_t reg, uint8_t* val, uint8_t len) {
  SerialUSB.print("I2C write to register " + String(reg) + ": ");
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(reg);
  for (uint8_t index = 0; index < len; index++) {
    SerialUSB.print(String(val[index], HEX) + ", ");
    Wire.write(val[index]);
  }
  SerialUSB.println();
  Wire.endTransmission();
}

void vfd_write_reg(uint8_t reg, uint8_t val) {
  vfd_write_regs(reg, &val, 1);
}

// Functions for using the control register

void vfd_control_led(bool state) {
  uint8_t val = 0;
  vfd_read_reg(VFD_REG_CTRL, &val);
  SerialUSB.println("LED, READ  " + String(val, HEX));
  val &= ~(1 << 2); // Turn off the LED
  if (state) {
    val |= (1 << 2); // Turn on the LED
  }
  SerialUSB.println("LED, WRITE " + String(val, HEX));
  vfd_write_reg(VFD_REG_CTRL, val);
}

void vfd_control_test(bool state) {
  uint8_t val = 0;
  vfd_read_reg(VFD_REG_CTRL, &val);
  val &= ~(1 << 1); // Turn off the test mode
  if (state) {
    val |= (1 << 1); // Turn on the test mode
  }
  vfd_write_reg(VFD_REG_CTRL, val);
}

void vfd_control_enable(bool state) {
  uint8_t val = 0;
  vfd_read_reg(VFD_REG_CTRL, &val);
  val &= ~(1 << 0); // Turn off the VFD
  if (state) {
    val |= (1 << 0); // Turn on the VFD
  }
  vfd_write_reg(VFD_REG_CTRL, val);
}

void vfd_control(bool enable, bool test, bool led) {
  uint8_t val = 0;
  if (enable) {
    val |= (1 << 0);
  }
  if (test) {
    val |= (1 << 1);
  }
  if (led) {
    val |= (1 << 2);
  }
  vfd_write_reg(VFD_REG_CTRL, val);
}

// Functions for using the scroll features

void vfd_set_offset(uint8_t offset) {
  vfd_write_reg(VFD_REG_OFFSET, offset);
}

void vfd_set_scroll_length(uint8_t len) {
  vfd_write_reg(VFD_REG_SCROLL_LEN, len);
}

void vfd_set_scroll_mode(uint8_t scroll_mode, bool scroll_loop) {
  uint8_t val = scroll_mode & 0x0F;
  if (scroll_loop) {
    val |= (1 << 4);
  }
  vfd_write_reg(VFD_REG_SCROLL_MODE, val);
}

void vfd_set_scroll_speed(uint16_t scroll_speed) {
  uint8_t values[2];
  values[0] = (scroll_speed) & 0xFF;
  values[1] = (scroll_speed >> 8) & 0xFF;
  vfd_write_regs(VFD_REG_SCROLL_SPEED, values, sizeof(uint16_t));
}

// Functions for using the text buffer

void vfd_write_text(String text) {
  // This function writes an ASCII string into
  // register 10 to 255

  const size_t max_len = 245; // can store text in registers 10 to 255
  size_t len = text.length();

  if (len > max_len) {
    // Silently limit the length of the string
    len = max_len;
  }

  Wire.beginTransmission(I2C_ADDR);
  Wire.write(VFD_REG_DATA);
  for (size_t index = 0; index < len; index++) {
    Wire.write(text[index]);
  }
  Wire.endTransmission();
}

void clearVFDText() {
    const int MAX_LEN = 245; // full buffer size
    char blank[MAX_LEN];
    memset(blank, ' ', MAX_LEN);
    vfd_write_text(String(blank));  // or write as bytes if avoiding String
}

// Functions for Arduino program

void setup() {
  SerialUSB.begin(115200);
  Wire.begin(); // Initialize the I2C bus

  // Configure the display
  vfd_set_scroll_mode(VFD_SCROLL_DISABLE, false);
  vfd_set_offset(0); // Move to beginning of text buffer
  clearVFDText();
  displayVFDText("VFD Clippy ");
  vfd_control(true, false, true); // Turn on VFD and LED
  delay(1000);

}

void loop() {
    static String inputBuffer = "";

    // Read incoming serial data
    while (SerialUSB.available() > 0) {
        char c = SerialUSB.read();
        SerialUSB.print("Received char: '");
        if (c == '\r') SerialUSB.print("\\r");
        else if (c == '\n') SerialUSB.print("\\n");
        else SerialUSB.print(c);
        SerialUSB.println("'");

        inputBuffer += c;
    }

    // Check if buffer has a newline (end of text)
    int newlineIndex = inputBuffer.indexOf('\n');
    if (newlineIndex != -1) {
        String message = inputBuffer.substring(0, newlineIndex);
        inputBuffer = inputBuffer.substring(newlineIndex + 1);

        SerialUSB.print("Complete message received: '");
        SerialUSB.print(message);
        SerialUSB.println("'");

        SerialUSB.print("Message length: ");
        SerialUSB.println(message.length());

        // Turn display back on if it was cleared
        if (displayCleared) {
            vfd_control_enable(true);
            displayCleared = false;
        }
        
        // Reset inactivity timer
        lastMessageTime = millis();

        displayVFDText(message);
    }

    // Check for inactivity timeout
    if (!displayCleared && (millis() - lastMessageTime > INACTIVITY_TIMEOUT)) {
        vfd_control_enable(false); // turn off VFD
        displayCleared = true;
    }
}

void displayVFDText(const String& text) {
    constexpr int DISPLAY_CHARS  = 12;   // Characters visible on screen
    constexpr int MAX_SCROLLABLE = 255 - DISPLAY_CHARS;  // Maximum scrollable buffer

    vfd_set_scroll_mode(VFD_SCROLL_DISABLE, false);
    vfd_set_offset(0);
    clearVFDText();

    if (text.length() <= DISPLAY_CHARS) {
        // Short text: display as-is, no scrolling
        String paddedText = text;
        while (paddedText.length() < DISPLAY_CHARS) paddedText += ' '; // pad to fill display
        vfd_write_text(paddedText);
        return;
    }

    // Long text: truncate to max scrollable length
    int maxTextLen = MAX_SCROLLABLE - 2 * DISPLAY_CHARS; // leave space for padding
    String truncated = text;
    if (truncated.length() > maxTextLen) truncated = truncated.substring(0, maxTextLen);

    String padding = "";
    for (int i = 0; i < DISPLAY_CHARS; i++) {
        padding += ' ';
    }
    String paddedText = padding + truncated + padding;

    vfd_write_text(paddedText);

    // Enable scrolling
    uint8_t scrollLen = paddedText.length() - DISPLAY_CHARS;
    if (scrollLen > 255) scrollLen = 255;  // hardware limit
    vfd_set_scroll_length(scrollLen);
    vfd_set_scroll_speed(SCROLL_SPEED_MS);
    vfd_set_scroll_mode(VFD_SCROLL_LEFT, true); // continuous loop
}



