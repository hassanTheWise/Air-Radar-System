// CHIP SETTINGS (Telling the microcontroller how to behave)
#pragma config FOSC = HS        // We are using a fast external clock (crystal)
#pragma config WDTE = OFF       // Turn off the "watchdog" so the chip doesn't restart on its own
#pragma config PWRTE = ON       // Wait a little bit for power to be stable before starting
#pragma config BOREN = ON       // Reset the chip if the battery/power gets too low
#pragma config LVP = OFF        // Turn off low voltage programming to free up a pin
#pragma config CPD = OFF        // Don't lock the data memory
#pragma config WRT = OFF        // Allow the program memory to be changed
#pragma config CP = OFF         // Don't lock the program code (anyone can read it)

// Tell the code we are running at 20 million ticks per second (20 MHz)
// This helps the delay functions know exactly how long to wait.
#define _XTAL_FREQ 20000000     

#include <xc.h>     // Include the main file that knows all the chip's pins
#include <stdio.h>  // Include standard tools for text and numbers

// PIN NAMES (Giving easy names to the legs of the chip)
#define TRIG  PORTBbits.RB0     // Pin to send the sound wave (Trigger)
#define ECHO  PORTBbits.RB1     // Pin to listen for the echo (Echo)
#define SERVO PORTCbits.RC2     // Pin to control the moving motor (Servo)

// LCD SCREEN SETTINGS
#define LCD_ADDR      0x4E  // The secret "address" to talk to this specific screen
#define LCD_BACKLIGHT 0x08  // Code to turn on the screen's backlight
#define LCD_EN        0x04  // Code to tell the screen "Read this data now!"
#define LCD_RS        0x01  // Code to tell the screen if we are sending text or commands

// Motor movement limits (How far left and right to look)
#define SWEEP_MIN  90       // Minimum angle (One side)
#define SWEEP_MAX  446      // Maximum angle (The other side)
#define SWEEP_STEP 4        // How much to move the motor each step

// A flag to check if the screen is broken or missing. 0 = OK, 1 = Broken.
static unsigned char lcd_dead = 0;


// SERIAL PORT (Talking to the computer via USB/Serial)
void UART_Init(void) {
    TRISCbits.TRISC6 = 0;   // Make the transmit pin an output
    SPBRG = 129;            // Set the speed of our text messages
    TXSTAbits.BRGH = 1;     // Turn on high-speed mode
    TXSTAbits.TXEN = 1;     // Turn on the transmitter
    RCSTAbits.SPEN = 1;     // Turn on the serial port overall
}

// This function sends a single letter to the computer
void putch(char data) {
    unsigned int t = 60000;
    // Wait until the line is free before sending the next letter
    while (!TXSTAbits.TRMT && t--);
    TXREG = data; // Send the letter!
}


// I2C (The two-wire system used to talk to the LCD screen)
void I2C_Init(void) {
    TRISC3 = 1; TRISC4 = 1; // Make the I2C pins act as inputs first
    SSPCON  = 0x28;         // Turn on I2C hardware in the chip
    SSPSTAT = 0x80;         // Set standard speed mode
    SSPADD  = 49;           // Set the clock speed for the two-wire system
}

// Wait for the I2C system to finish what it's doing
static unsigned char I2C_Wait(void) {
    unsigned int t = 60000;
    while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F)) {
        if (--t == 0) return 0; // If it takes too long, give up (return 0)
    }
    return 1; // Success!
}

// Wait for the I2C flag to say the job is done
static unsigned char I2C_WaitFlag(void) {
    unsigned int t = 60000;
    while (!SSPIF) {
        if (--t == 0) return 0; // Give up if it takes too long
    }
    SSPIF = 0; // Clear the flag for next time
    return 1;
}

// Start a conversation on the I2C wires
unsigned char I2C_Start(void) {
    if (!I2C_Wait()) return 0;
    SEN = 1; // Send the "Start" signal
    return I2C_WaitFlag();
}

// End a conversation on the I2C wires
unsigned char I2C_Stop(void) {
    if (!I2C_Wait()) return 0;
    PEN = 1; // Send the "Stop" signal
    return I2C_WaitFlag();
}

// Send one piece of data over the I2C wires
unsigned char I2C_Write(unsigned char d) {
    if (!I2C_Wait()) return 0;
    SSPBUF = d; // Put data in the buffer to send
    return I2C_WaitFlag();
}


// LCD SCREEN CONTROLS (How to draw on the screen)

// The screen can only take 4 bits (a half-byte or "nibble") at a time
void LCD_Send_Nibble(unsigned char data) {
    if (lcd_dead) return; // If screen is missing, don't try to talk to it

    // Keep the backlight on while sending data
    unsigned char data_with_light = data | LCD_BACKLIGHT;

    // Start talking to the screen's address
    if (!I2C_Start()) { lcd_dead = 1; return; }
    if (!I2C_Write(LCD_ADDR)) { I2C_Stop(); lcd_dead = 1; return; }

    // If the screen answered us (ACK)
    if (ACKSTAT == 0) {
        // 1. Send data and push the "Enable" button ON
        I2C_Write(data_with_light | LCD_EN);
        __delay_us(50);
        
        // 2. Turn the "Enable" button OFF to lock the data into the screen
        I2C_Write(data_with_light & ~LCD_EN);
        __delay_us(50);
    } else {
        lcd_dead = 1; // The screen didn't answer
    }
    I2C_Stop(); // End conversation
}

// Send a command to the screen (like "clear screen" or "move cursor")
void LCD_Cmd(unsigned char cmd) {
    if (lcd_dead) return;
    
    // Split the 8-bit command into two 4-bit pieces
    unsigned char upper = (cmd & 0xF0);             // Get the top half
    unsigned char lower = ((cmd << 4) & 0xF0);      // Get the bottom half and move it up
    
    LCD_Send_Nibble(upper); // Send top half
    LCD_Send_Nibble(lower); // Send bottom half
    
    if (cmd == 0x01) {
        __delay_ms(2); // Clearing the screen takes a little extra time
    }
}

// Send a visible letter or number to the screen
void LCD_Char(unsigned char c) {
    if (lcd_dead) return;
    
    // Split the letter into two halves, and add the "RS" code to say it's text
    unsigned char upper = (c & 0xF0) | LCD_RS;
    unsigned char lower = ((c << 4) & 0xF0) | LCD_RS;
    
    LCD_Send_Nibble(upper);
    LCD_Send_Nibble(lower);
}

// Send a whole word or sentence to the screen
void LCD_String(const char *s) {
    // Keep sending letters until we hit the end of the sentence
    while (*s && !lcd_dead) LCD_Char(*s++);
}

// Move the blinking cursor to a specific row and column
void LCD_SetCursor(unsigned char row, unsigned char col) {
    // Row 0 starts at address 0x80. Row 1 starts at 0xC0.
    unsigned char addr = (row == 0) ? 0x80 : 0xC0;
    LCD_Cmd(addr + col); // Move to the exact spot
}

// Setup the screen for the very first time
void LCD_Init(void) {
    __delay_ms(50); // Wait for the screen to power up
    
    // A special knock to wake up the screen
    LCD_Send_Nibble(0x30);
    __delay_ms(5);
    LCD_Send_Nibble(0x30);
    __delay_us(150);
    LCD_Send_Nibble(0x30);
    
    // Tell it to use 4-bit mode
    LCD_Send_Nibble(0x20); 
    
    // Setup how the screen looks
    LCD_Cmd(0x28); // 2 lines of text, normal font
    LCD_Cmd(0x0C); // Turn display ON, hide the blinking cursor
    LCD_Cmd(0x06); // Make the cursor move right after typing a letter
    LCD_Cmd(0x01); // Erase everything on the screen
    __delay_ms(5);
    
    // Tell the computer if the screen is working or not
    if (lcd_dead) {
        printf("LCD:FAIL\n");
    } else {
        printf("LCD:OK\n");
    }
}

// Turn a math number into text and print it on the screen
void LCD_Number(unsigned int n, unsigned char digits) {
    if (lcd_dead) return;
    char buf[6];
    unsigned char i = 0, j;
    
    // Do the math to split the number into individual text digits
    if (n == 0) { buf[i++] = '0'; }
    else { unsigned int t = n; while (t) { buf[i++] = '0' + (t % 10); t /= 10; } }
    
    // Fill leftover empty space with blank spaces
    while (i < digits) buf[i++] = ' ';
    
    // Print it backwards (because the math pulls the last digit first)
    for (j = i; j > 0; j--) LCD_Char(buf[j-1]);
}

// Update the angle and distance numbers on the screen
void LCD_ShowReading(int angle, unsigned int dist) {
    if (lcd_dead) return;
    
    LCD_SetCursor(0, 4); // Go to row 0, space 4
    LCD_Number((unsigned int)angle, 3); // Print the angle
    
    LCD_SetCursor(1, 4); // Go to row 1, space 4
    LCD_Number(dist, 4); // Print the distance
}

// ULTRASONIC SENSOR (Using sound to measure distance)
unsigned int get_distance(void) {
    unsigned int timeout;

    // Setup a timer (Timer1) to count how long the sound takes
    T1CON = 0x10;
    TMR1  = 0; // Reset timer to 0

    // Send a tiny burst of sound (Trigger)
    TRIG = 1; 
    __delay_us(10); 
    TRIG = 0;

    // Wait for the sound to bounce back (Echo goes high)
    timeout = 10000;
    while (!ECHO && timeout) timeout--;
    if (!timeout) return 400; // If it never bounces back, say it's 400cm away

    // Start the stopwatch!
    T1CONbits.TMR1ON = 1;
    // Keep counting while the echo pin is HIGH (receiving sound)
    while (ECHO && TMR1 < 40000);
    // Stop the stopwatch
    T1CONbits.TMR1ON = 0;

    // Do math to turn "time" into "centimeters"
    unsigned int dist = TMR1 / 145;
    
    // Don't let the distance go over 400cm (sensor limit)
    return (dist > 400) ? 400 : dist;
}

// SERVO MOTOR (Moving the radar arm)
// This function sends a special pulse to tell the motor where to point
void servo_pulse(int j) {
    SERVO = 1; // Turn motor signal ON
    int c = j;
    
    // Keep it ON for a very specific amount of time based on 'j'
    while (c > 0) { __delay_us(1); c--; NOP(); }
    
    SERVO = 0; // Turn motor signal OFF
    __delay_ms(18); // Wait a little bit before sending the next signal
}


// MAIN PROGRAM (This is where the actual action happens!)
void main(void) {
    __delay_ms(1000); // Give everything 1 second to power up

    // Setup which pins are Inputs (1) and Outputs (0)
    TRISB = 0x02; // RB1 is an input (Echo). Everything else on B is output.
    TRISCbits.TRISC2 = 0; // Motor pin is an output
    TRISCbits.TRISC6 = 0; // Serial TX pin is an output

    // Turn on our features
    UART_Init();    // Start serial to computer
    I2C_Init();     // Start I2C to screen
    LCD_Init();     // Turn on the screen

    // Print a welcome message on the screen
    if (!lcd_dead) {
        LCD_SetCursor(0, 0); LCD_String("  Radar Ready   ");
        LCD_SetCursor(1, 0); LCD_String("                ");
        __delay_ms(1000); // Show it for 1 second
        
        LCD_Cmd(0x01); // Clear the screen
        __delay_ms(5);
        
        // Draw the static labels that won't change
        LCD_SetCursor(0, 0); LCD_String("Ang:     deg");
        LCD_SetCursor(1, 0); LCD_String("Dst:     cm ");
    }
    
    // Print header to the computer
    printf("angle,distance,distance\n");
    
    int loop_count = 0; // Keep track of how many times we've scanned
    
    // Infinite loop! This runs forever.
    while (1) {
        
        // SCAN RIGHT: Move motor from Minimum to Maximum
        for (int j = SWEEP_MIN; j <= SWEEP_MAX; j += SWEEP_STEP) {
            servo_pulse(j);      // Move the motor a tiny bit
            __delay_ms(40);      // Give the motor time to get there
            unsigned int d = get_distance(); // Shoot sound and see how far an object is

            // Calculate the actual angle in degrees (0 to 180)
            int angle = (int)( ((long)(j - SWEEP_MIN) * 180) / (SWEEP_MAX - SWEEP_MIN) );
            loop_count++;
            
            // Only update the screen every 2nd step so it doesn't flicker too much
            if (loop_count % 2 == 0) {
                LCD_ShowReading(angle, d); 
            }
            
            // Send the data to the computer
            printf("%d,%u,%u\n", angle, d, d);
        }

        // SCAN LEFT: Move motor from Maximum back to Minimum
        for (int j = SWEEP_MAX; j >= SWEEP_MIN; j -= SWEEP_STEP) {
            servo_pulse(j);      // Move the motor a tiny bit
            __delay_ms(40);      // Give the motor time to get there
            unsigned int d = get_distance(); // Shoot sound and measure

            // Calculate angle
            int angle = (int)( ((long)(j - SWEEP_MIN) * 180) / (SWEEP_MAX - SWEEP_MIN) );
            loop_count++;
            
            // Update screen
            if (loop_count % 2 == 0) {
                LCD_ShowReading(angle, d); 
            }
            
            // Send to computer
            printf("%d,%u,%u\n", angle, d, d);
        }
    }
}
