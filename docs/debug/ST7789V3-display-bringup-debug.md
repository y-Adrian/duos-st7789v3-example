# 1 ST7789V3 display bring-up debug

This note records the debug process for the 1.90-inch 170x320 ST7789V3 SPI display on Milk-V Duo S.

The display test program was based on:

```text
https://github.com/zwyzwm/TFT-ST7789.git
```

The original sample uses `wiringX` and hardware SPI. During this debug process, only `SPI0` was available on the board, so the test program was changed to use software SPI with GPIO bit-banging.

## 1.1 Hardware target

Display module information from board silk screen:

```text
Resolution: 170x320
Interface: 4-SPI
Driver IC: ST7789
Pin order on display PCB, left to right:
BLK CS DC RES SDA SCL VCC GND
```

The display PCB pin order is important. It is not the same visual order as some ST7789 examples. The silk screen on the actual display module should be used as the source of truth.

Initial software-SPI pin assignment:

```c
#define SPI_SCK_PIN  23
#define SPI_SDO_PIN  19
#define SPI_RST_PIN  26
#define SPI_DC_PIN   22
#define SPI_CS_PIN   24
#define BL_PIN       15
```

Final software-SPI pin assignment:

```c
#define SPI_SCK_PIN  23
#define SPI_SDO_PIN  19
#define SPI_RST_PIN  3
#define SPI_DC_PIN   5
#define SPI_CS_PIN   24
#define BL_PIN       15
```

Final wiring:

```text
Display BLK -> Duo S PIN15
Display CS  -> Duo S PIN24
Display DC  -> Duo S PIN5
Display RES -> Duo S PIN3
Display SDA -> Duo S PIN19
Display SCL -> Duo S PIN23
Display VCC -> 3.3V
Display GND -> GND
```

## 1.2 Initial problem

The first problem was:

```text
The display had backlight, but no stable image.
```

More detailed symptoms:

```text
1. BLK connected to 3.3V: backlight was always on.
2. Running the ST7789 test program: the screen sometimes flashed a small red area once.
3. Sometimes there was only a small brightness change, with no visible color frame.
4. Restoring full-screen refresh still did not produce full-screen red/green/blue output.
5. The visible color area stayed close to the same small region, even after changing the refresh region.
```

This meant the issue was probably not only the `TFT_SET_ADD()` test rectangle size. At this stage the suspects were:

```text
1. Wrong display pin order or connection.
2. Wrong pinmux mode.
3. Wrong software-SPI timing.
4. CS/DC/RES control signal problem.
5. ST7789V3 initialization or address-window parameters.
```

## 1.3 Test code baseline

The baseline came from `zwyzwm/TFT-ST7789`:

```text
https://github.com/zwyzwm/TFT-ST7789.git
```

Important parts of the original sample:

```c
#include <wiringx.h>

#define TFT_COLUMN_NUMBER 170
#define TFT_LINE_NUMBER   320
#define TFT_COLUMN_OFFSET 35
#define TFT_LINE_OFFSET   0

#define SPI_PORT    0

#define SPI_RST_PIN  50
#define SPI_DC_PIN   48
#define SPI_CS_PIN   46
#define BL_PIN       44
```

The original sample sends data through `wiringXSPIDataRW()`. For this board test, hardware SPI3 was not enabled, so the program was changed to software SPI:

```c
void SPI_SendByte(unsigned char value)
{
    unsigned char i;

    for (i = 0; i < 8; i++) {
        if (value & 0x80) {
            digitalWrite(SPI_SDO_PIN, HIGH);
        } else {
            digitalWrite(SPI_SDO_PIN, LOW);
        }

        delay_us(20);
        digitalWrite(SPI_SCK_PIN, HIGH);
        delay_us(20);
        digitalWrite(SPI_SCK_PIN, LOW);
        delay_us(20);

        value <<= 1;
    }
}
```

The test refresh loop was:

```c
while (1) {
    TFT_full(RED);
    delay_ms(2000);

    TFT_full(GREEN);
    delay_ms(2000);

    TFT_full(BLUE);
    delay_ms(2000);
}
```

## 1.4 Test process

### 1.4.1 Verify whether the red flash came from Display On

Test method:

Temporarily comment out the final display-on command in `TFT_init()`:

```c
// TFT_SEND_CMD(0x29); // Display On
```

Result:

```text
The red flash disappeared.
```

Conclusion:

```text
The red flash was caused by 0x29 Display On.
It was likely residual GRAM content being shown after the panel was turned on.
This did not prove that pixel writing was working.
```

This narrowed the problem:

```text
The display could receive at least some commands, but address-window setup and RGB565 pixel writes were still suspicious.
```

### 1.4.2 Verify display on/off commands

Test method:

Add `0x28 Display Off` and `0x29 Display On` after the first `0x29`:

```c
TFT_SEND_CMD(0x29);   // Display On
delay_ms(1000);
TFT_SEND_CMD(0x28);   // Display Off
delay_ms(1000);
TFT_SEND_CMD(0x29);   // Display On
```

Result:

```text
There was no clear on/off switching effect.
```

Conclusion:

```text
Command transmission was not stable enough.
The problem was probably not only the ST7789V3 init table.
CS/DC/RES and GPIO output behavior needed to be tested before tuning display parameters.
```

### 1.4.3 Change software-SPI timing and CS transaction style

Test method:

Two changes were tried:

```text
1. Use real usleep-based delay instead of empty-loop delay.
2. Keep CS low while sending a full pixel buffer, instead of toggling CS for every data byte.
```

Example full-screen write style:

```c
void TFT_full(unsigned int color)
{
    unsigned int i;
    unsigned int total = TFT_COLUMN_NUMBER * TFT_LINE_NUMBER;

    TFT_SET_ADD(0, 0, TFT_COLUMN_NUMBER - 1, TFT_LINE_NUMBER - 1);

    digitalWrite(SPI_SCK_PIN, LOW);
    SPI_CS_0;
    SPI_DC_1;
    delay_us(2);

    for (i = 0; i < total; i++) {
        SPI_SendByte(color >> 8);
        SPI_SendByte(color & 0xff);
    }

    delay_us(2);
    SPI_CS_1;
}
```

Result:

```text
The red flash became less stable and sometimes disappeared.
```

Conclusion:

```text
The display link was already marginal before this change.
Changing timing did not solve the root problem.
The debug process moved back to GPIO/control-line verification.
```

### 1.4.4 Stabilize GPIO default states

Test method:

Set a deterministic idle state after GPIO initialization and before reset:

```c
SPI_CS_1;
SPI_DC_1;
SPI_RST_1;
digitalWrite(SPI_SCK_PIN, LOW);
digitalWrite(SPI_SDO_PIN, LOW);
BL_1;
delay_ms(50);
```

And reset sequence:

```c
SPI_CS_1;
SPI_DC_1;
digitalWrite(SPI_SCK_PIN, LOW);
digitalWrite(SPI_SDO_PIN, LOW);
delay_ms(20);

SPI_RST_1;
delay_ms(20);
SPI_RST_0;
delay_ms(50);
SPI_RST_1;
delay_ms(150);
```

Result:

```text
The display still only occasionally flashed red.
Most runs only changed brightness slightly.
```

Conclusion:

```text
Default GPIO state was not the main issue.
At least one control line was likely not behaving correctly.
```

### 1.4.5 Test CS by tying it to GND

Test method:

Hardware:

```text
Display CS -> GND
```

Software:

```c
#define SPI_CS_0 do {} while (0)
#define SPI_CS_1 do {} while (0)
```

Result:

```text
The brightness change disappeared.
The display did not become stable.
```

Conclusion:

```text
The issue was not simply a bad CS toggle.
The debug process needed to verify whether each selected GPIO could actually output stable levels.
```

### 1.4.6 Test GPIO outputs without a voltage meter

No voltage meter was available. The display backlight pin was used as a simple GPIO output indicator.

Test wiring:

```text
Display VCC -> 3.3V
Display GND -> GND
Display BLK -> current Duo S GPIO under test
Other display pins disconnected
```

Test code:

```c
#define TEST_PIN 22

int main(void)
{
    if (wiringXSetup("milkv_duos", NULL) == -1) {
        printf("wiringX init failed\n");
        return -1;
    }

    if (wiringXValidGPIO(TEST_PIN) != 0) {
        printf("Invalid GPIO %d\n", TEST_PIN);
        return -1;
    }

    pinMode(TEST_PIN, PINMODE_OUTPUT);

    while (1) {
        printf("PIN %d LOW\n", TEST_PIN);
        digitalWrite(TEST_PIN, LOW);
        sleep(2);

        printf("PIN %d HIGH\n", TEST_PIN);
        digitalWrite(TEST_PIN, HIGH);
        sleep(2);
    }

    return 0;
}
```

The same program was rebuilt with different `TEST_PIN` values.

Result:

```text
PIN15: stable 2s on / 2s off
PIN19: stable 2s on / 2s off
PIN23: stable 2s on / 2s off
PIN24: stable 2s on / 2s off
PIN3:  stable 2s on / 2s off
PIN5:  stable 2s on / 2s off
PIN22: only flashed briefly when turning on, not stable for 2s
PIN26: only flashed briefly when turning on, not stable for 2s
```

Conclusion:

```text
PIN22 and PIN26 were not reliable output pins in this setup.
They were used by the initial test as DC and RES, which are two critical ST7789 control signals.
```

### 1.4.7 Replace unstable DC and RES pins

Old assignment:

```c
#define SPI_RST_PIN  26
#define SPI_DC_PIN   22
```

New assignment:

```c
#define SPI_RST_PIN  3
#define SPI_DC_PIN   5
```

Rewiring:

```text
Display RES -> Duo S PIN3
Display DC  -> Duo S PIN5
```

Minimal init test:

```c
TFT_init();

while (1) {
    delay_ms(1000);
}
```

Result:

```text
The display stayed gray after initialization.
```

Then the RGB refresh loop was restored.

Result:

```text
A small region could stably refresh red/green/blue.
```

Conclusion:

```text
The hardware communication path was now working.
The remaining issue was display direction, address window, or offset.
```

### 1.4.8 Fix display direction/addressing

Test method:

After DC and RES were moved to stable GPIOs, the display direction and address-window parameters were tested again.

The key change was to use the correct memory access control value for this panel orientation:

```c
TFT_SEND_CMD(0x36);
TFT_SEND_DATA(0x00);
```

Result:

```text
Full-screen red, green, and blue refresh became normal.
```

Conclusion:

```text
After the GPIO issue was fixed, the remaining display issue was resolved by using the correct orientation/addressing settings.
```

## 1.5 Final conclusion

The root cause was a combination of:

```text
1. The original DC and RES GPIO choices were unstable.
2. The display orientation/address-window settings needed to match the 170x320 panel.
```

The most important fix was replacing the unstable control pins:

```text
Old DC  = PIN22: unstable
Old RES = PIN26: unstable
New DC  = PIN5:  stable
New RES = PIN3:  stable
```

After this change, software SPI could write pixels reliably. After correcting the display direction/addressing, full-screen RGB refresh worked.

## 1.6 Final known-good state

Known-good pin assignment:

```c
#define SPI_SCK_PIN  23
#define SPI_SDO_PIN  19
#define SPI_RST_PIN  3
#define SPI_DC_PIN   5
#define SPI_CS_PIN   24
#define BL_PIN       15
```

Known-good wiring:

```text
BLK -> PIN15
CS  -> PIN24
DC  -> PIN5
RES -> PIN3
SDA -> PIN19
SCL -> PIN23
VCC -> 3.3V
GND -> GND
```

Known-good display direction:

```c
TFT_SEND_CMD(0x36);
TFT_SEND_DATA(0x00);
```

Known-good validation:

```text
The screen can continuously refresh full-screen red, green, and blue.
```

## 1.7 Reusable staged test code

These small test snippets are useful when bringing up the display again. They keep each test focused on one possible failure point.

### 1.7.1 GPIO output test with BLK as indicator

Use this when no voltage meter is available.

Test wiring:

```text
Display VCC -> 3.3V
Display GND -> GND
Display BLK -> current Duo S GPIO under test
Other display pins disconnected
```

Test code:

```c
#include <stdio.h>
#include <unistd.h>
#include <wiringx.h>

#define TEST_PIN 22

int main(void)
{
    if (wiringXSetup("milkv_duos", NULL) == -1) {
        printf("wiringX init failed\n");
        return -1;
    }

    if (wiringXValidGPIO(TEST_PIN) != 0) {
        printf("Invalid GPIO %d\n", TEST_PIN);
        return -1;
    }

    pinMode(TEST_PIN, PINMODE_OUTPUT);

    while (1) {
        printf("PIN %d LOW\n", TEST_PIN);
        digitalWrite(TEST_PIN, LOW);
        sleep(2);

        printf("PIN %d HIGH\n", TEST_PIN);
        digitalWrite(TEST_PIN, HIGH);
        sleep(2);
    }

    return 0;
}
```

Expected result:

```text
The backlight should stay off for 2 seconds, then stay on for 2 seconds.
If it only flashes briefly, the pin should not be used for ST7789 DC/RES/CS/SCK/SDA.
```

### 1.7.2 Init-only test

Use this after wiring changes. It checks whether reset and basic display initialization are stable before writing pixels.

```c
int main(void)
{
    point = &picture_tab[0];

    if (wiringx_init() != 0) {
        printf("wiringX init failed\n");
        return -1;
    }

    BL_1;
    printf("Backlight ON\n");

    TFT_init();
    printf("TFT_init done\n");
    fflush(stdout);

    while (1) {
        delay_ms(1000);
    }

    return 0;
}
```

Expected result:

```text
The display should enter a stable state after initialization.
During this project debug, the known-good result was a stable gray screen.
```

### 1.7.3 Display On residual-GRAM test

Use this to check whether the visible flash is caused by `0x29 Display On`.

Test A:

```c
// TFT_SEND_CMD(0x29); // Display On
```

Expected result:

```text
If the red flash disappears, the flash was caused by Display On showing residual GRAM.
```

Test B:

```c
TFT_SEND_CMD(0x29);   // Display On
delay_ms(1000);
TFT_SEND_CMD(0x28);   // Display Off
delay_ms(1000);
TFT_SEND_CMD(0x29);   // Display On
```

Expected result:

```text
If the panel clearly changes state, command transmission is at least partly stable.
If there is no clear change, verify DC/CS/RES and GPIO output before tuning the init table.
```

### 1.7.4 CS isolation test

Use this to check whether software CS control is the main problem.

Hardware:

```text
Display CS -> GND
```

Code:

```c
#define SPI_CS_0 do {} while (0)
#define SPI_CS_1 do {} while (0)
```

Expected result:

```text
If the display becomes stable, the original CS pin or CS software control is suspicious.
If the display does not become stable, continue checking DC/RES/SCK/SDA.
```

### 1.7.5 Full-screen RGB refresh test

Use this only after the init-only test is stable.

```c
void TFT_full(unsigned int color)
{
    unsigned int i;
    unsigned int total = TFT_COLUMN_NUMBER * TFT_LINE_NUMBER;

    TFT_SET_ADD(0, 0, TFT_COLUMN_NUMBER - 1, TFT_LINE_NUMBER - 1);

    digitalWrite(SPI_SCK_PIN, LOW);
    SPI_CS_0;
    SPI_DC_1;
    delay_us(2);

    for (i = 0; i < total; i++) {
        SPI_SendByte(color >> 8);
        SPI_SendByte(color & 0xff);
    }

    delay_us(2);
    SPI_CS_1;
}

int main(void)
{
    point = &picture_tab[0];

    if (wiringx_init() != 0) {
        printf("wiringX init failed\n");
        return -1;
    }

    BL_1;
    TFT_init();

    while (1) {
        printf("RED\n");
        fflush(stdout);
        TFT_full(RED);
        delay_ms(2000);

        printf("GREEN\n");
        fflush(stdout);
        TFT_full(GREEN);
        delay_ms(2000);

        printf("BLUE\n");
        fflush(stdout);
        TFT_full(BLUE);
        delay_ms(2000);
    }

    return 0;
}
```

Expected result:

```text
The display should continuously refresh full-screen red, green, and blue.
This is the baseline before LVGL integration.
```

## 1.8 Follow-up notes

Keep these points in mind when integrating this display into Pocket:

```text
1. Do not use PIN22/PIN26 for ST7789 DC/RES unless their pinmux/output behavior is verified again.
2. Software SPI requires all selected pins to be GPIO, not SPI alternate functions.
3. When debugging without a voltage meter, BLK can be used as a simple GPIO output indicator.
4. A red flash after 0x29 Display On usually means residual GRAM is visible, not that pixel writing is already correct.
5. Stable full-screen RGB refresh should be the baseline before LVGL integration.
6. The test program is based on https://github.com/zwyzwm/TFT-ST7789.git.
```
