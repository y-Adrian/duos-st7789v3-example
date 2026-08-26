# DuoS ST7789V3 Software SPI Example

这是一个用于 Milk-V Duo S 的 ST7789V3 1.90 英寸 `170x320` SPI 屏幕测试程序。

代码基于 [zwyzwm/TFT-ST7789](https://github.com/zwyzwm/TFT-ST7789.git) 调整而来。原示例使用 `wiringX + SPI`，本仓库当前版本使用 `wiringX GPIO` 软件模拟 SPI，适合只启用了 `SPI0`、但希望先验证 ST7789V3 屏幕可用性的场景。

## 1. Hardware

当前验证通过的屏幕参数：

```text
Display: 1.90-inch TFT
Resolution: 170x320
Interface: 4-SPI
Driver IC: ST7789V3
Logic / power: 3.3V module
```

当前屏幕实物丝印顺序：

```text
BLK CS DC RES SDA SCL VCC GND
```

已验证通过的 Milk-V Duo S 接线：

| Display pin | Duo S pin | Description |
| --- | --- | --- |
| `BLK` | `PIN15` | Backlight |
| `CS` | `PIN24` | Chip select |
| `DC` | `PIN5` | Data / command select |
| `RES` | `PIN3` | Reset |
| `SDA` | `PIN19` | Software SPI MOSI |
| `SCL` | `PIN23` | Software SPI SCK |
| `VCC` | `3.3V` | Power |
| `GND` | `GND` | Ground |

对应代码定义：

```c
#define SPI_SCK_PIN  23
#define SPI_SDO_PIN  19
#define SPI_RST_PIN  3
#define SPI_DC_PIN   5
#define SPI_CS_PIN   24
#define BL_PIN       15
```

注意：调试过程中曾测试 `PIN22` / `PIN26` 作为 `DC` / `RES`，但这两个引脚在当前环境下输出不稳定，因此最终没有使用。

## 2. Build Environment

编译环境准备方式与 [milkv-duo/duo-examples](https://github.com/milkv-duo/duo-examples) 类似。

先准备 Duo examples 环境：

```sh
git clone https://github.com/milkv-duo/duo-examples.git
cd duo-examples
source envsetup.sh
```

选择目标时，Duo S 一般选择：

```text
Product: Duo256M (SG2002) or DuoS (SG2000)
Arch: RISCV64
```

在同一个 terminal 中进入本仓库：

```sh
git clone https://github.com/y-Adrian/duos-st7789v3-example.git
cd duos-st7789v3-example
```

## 3. Configure SYSROOT

编译前需要确认 `Makefile` 中的 `SYSROOT` 指向当前交叉编译工具链对应的 sysroot。

例如：

```make
SYSROOT = /path/to/host-tools/gcc/riscv64-linux-musl-x86_64/sysroot
```

或者在 ARM64 Linux host 上可能类似：

```make
SYSROOT = /path/to/host-tools/gcc/riscv64-linux-musl-aarch64/sysroot
```

关键点是：`SYSROOT` 必须和当前 `TOOLCHAIN_PREFIX` 指向的工具链匹配，否则可能找不到 `wiringx.h` 或链接不到 `libwiringx`。

## 4. Build

确认已经在当前 terminal 执行过 `source envsetup.sh` 后：

```sh
make clean
make
```

编译成功后会生成：

```text
st7789
```

默认构建使用软件 SPI，相当于：

```sh
make DISPLAY_BUS=software DISPLAY_ORIENTATION=landscape
```

默认显示方向是横屏 `320x170`，更适合显示当前拍照得到的 `1920x1080` 横向图片。如果要回到原来的竖屏坐标：

```sh
make clean
make DISPLAY_ORIENTATION=portrait
```

如果要测试用户态硬件 SPI / `spidev`：

```sh
make clean
make DISPLAY_BUS=spidev DISPLAY_ORIENTATION=landscape SPI_DEV=/dev/spidev0.0 SPI_SPEED_HZ=12000000
```

其中：

```text
DISPLAY_BUS=software   使用 wiringX GPIO 软件模拟 SPI，默认值
DISPLAY_BUS=spidev     使用 /dev/spidevX.Y 发送 SPI 数据
DISPLAY_ORIENTATION=landscape  横屏坐标，320x170，默认值
DISPLAY_ORIENTATION=portrait   竖屏坐标，170x320
SPI_DEV                spidev 设备节点，默认 /dev/spidev0.0
SPI_SPEED_HZ           SPI 速度，默认 12000000
```

## 5. Code Structure

当前代码已经把 ST7789 上层逻辑和底层传输拆开：

| File | Description |
| --- | --- |
| `main.c` | 显示测试入口：三色条、渐变、棋盘、图片 |
| `st7789.c` / `st7789.h` | ST7789V3 初始化、地址窗口设置、RGB565 写屏 API |
| `display_bus.c` | 当前底层传输实现，使用 wiringX GPIO 软件模拟 SPI |
| `display_bus.h` | ST7789 上层调用的 bus 接口 |
| `data.c` / `data.h` | 原示例保留的字模测试数据 |
| `picture_data.c` / `picture_data.h` | 当前显示的 RGB565 图片资源 |
| `tools/image_to_rgb565_c.py` | 将 PNG / JPG 转成 `picture_data.c` / `picture_data.h` |

当前仍然是软件 SPI：

```text
st7789.c -> display_bus_write_cmd/data() -> GPIO bit-bang SPI
```

也可以通过 `DISPLAY_BUS=spidev` 切到用户态硬件 SPI：

```text
st7789.c -> display_bus_write_cmd/data() -> /dev/spidevX.Y
```

`DC` / `RES` / `BLK` / `CS` 仍然由 wiringX GPIO 控制。`SCK` / `SDA(MOSI)` 在 `spidev` 模式下由硬件 SPI 控制器输出。

当前可复用的 ST7789 API：

```c
int display_bus_init(void);
void display_bus_backlight_on(void);

void st7789_init(void);
void st7789_set_window(uint16_t x_start, uint16_t y_start,
                       uint16_t x_end, uint16_t y_end);
void st7789_write_data_buf(const uint8_t *data, unsigned int len);
void st7789_fill_rgb565(uint16_t color);
void st7789_draw_rgb565_image(uint16_t x, uint16_t y,
                              uint16_t width, uint16_t height,
                              const uint8_t *data);
void st7789_draw_vertical_gradient(void);
void st7789_draw_checkerboard(uint16_t tile_size);
```

图片数据格式：

```text
1. 每个像素使用 RGB565，16 bit。
2. 每个像素写入 2 个字节：高字节在前，低字节在后。
3. 像素顺序是从左到右、从上到下。
4. 数组长度必须等于 width * height * 2。
```

当前屏幕可用像素总数固定为：

```text
170 * 320 = 54400 pixels
54400 * 2 = 108800 bytes
```

对于项目当前拍到的 `1920x1080` 横向照片，推荐横屏显示：

```text
cover:   320x170，全屏显示，源图约裁掉上下各 30px
contain: 302x170，完整显示，左右各约 9px 黑边
portrait contain: 170x96，完整显示，但上下空白很多
```

所以默认布局采用 `320x170 cover`，在小屏预览时信息密度最高。

## 6. Convert Images To Display Data

仓库提供了一个简单转换脚本：

```sh
python3 -m pip install Pillow
python3 tools/image_to_rgb565_c.py input.png \
    --output picture_data.c
```

脚本默认输出 `320x170`，数组名为 `picture_tab`，并会按图片 EXIF 方向自动转正。对于当前 `1920x1080` 照片，这正好对应横屏整屏预览。

执行后会同时生成：

```text
picture_data.c
picture_data.h
```

`picture_data.h` 中包含尺寸宏，`picture_data.c` 中包含 RGB565 数据，二者需要一起提交或复制。

头文件会生成类似这样的内容：

```c
#include <stdint.h>

#define PICTURE_TAB_WIDTH 320
#define PICTURE_TAB_HEIGHT 170
#define PICTURE_TAB_SIZE (PICTURE_TAB_WIDTH * PICTURE_TAB_HEIGHT * 2)

extern const uint8_t picture_tab[PICTURE_TAB_SIZE];
```

C 文件会生成类似这样的数组：

```c
#include "picture_data.h"

const uint8_t picture_tab[PICTURE_TAB_SIZE] = {
    0xF8, 0x00, 0x07, 0xE0,
};
```

如果要替换仓库内置示例图，直接用新生成的 `picture_data.c` / `picture_data.h` 覆盖仓库里的同名文件即可，不需要再手动修改 `data.c` 或尺寸宏。

脚本支持三种缩放方式：

```text
--fit cover     填满目标尺寸，必要时裁切，默认值
--fit contain   保留完整图片，空白处补黑
--fit stretch   直接拉伸到目标尺寸
```

如果你希望一张照片完整显示、不裁切：

```sh
python3 tools/image_to_rgb565_c.py input.png \
    --fit contain \
    --output picture_data.c
```

## 7. Run On Duo S

将程序复制到 Duo S：

```sh
scp st7789 root@192.168.42.1:/root/
```

在板端执行：

```sh
ssh root@192.168.42.1
cd /root
chmod +x st7789
./st7789
```

正常现象：

```text
1. 背光打开。
2. 程序打印 GPIO 初始化信息。
3. 屏幕循环显示三色条、渐变、棋盘、居中图片。
```

## 8. Pinmux Notes

默认软件 SPI 模式下，`SCK` / `SDA` / `CS` / `DC` / `RES` / `BLK` 都需要能作为普通 GPIO 输出。

如果屏幕只有背光、没有颜色刷新，请先确认：

```text
1. 接线是否严格按照屏幕实物丝印。
2. 相关引脚是否处于 GPIO 模式，而不是 SPI/I2C/ADC 等复用功能。
3. DC 和 RES 是否能稳定输出高低电平。
```

没有万用表时，可以把屏幕 `BLK` 临时接到待测 GPIO 上，用程序拉高/拉低该 GPIO，通过背光是否稳定 2 秒亮/2 秒灭来判断 GPIO 输出是否可靠。

`spidev` 模式下需要额外确认：

```text
1. 板上存在对应设备节点，例如 /dev/spidev0.0。
2. 先确认 /dev/spidevX.Y 实际绑定到哪个 SPI 控制器。
3. 屏幕 SCL 接到该 SPI 控制器的 SCK。
4. 屏幕 SDA 接到该 SPI 控制器的 MOSI。
5. 对应 SCK/MOSI pinmux 已经切到 SPI 复用功能。
6. DC / RES / BLK / CS 仍然接到当前代码配置的 GPIO。
```

可以用下面命令确认 `spidev` 设备实际绑定的控制器：

```sh
readlink -f /sys/class/spidev/spidev0.0/device
```

当前已验证的板端结果：

```text
/sys/devices/platform/41b0000.spi3/spi_master/spi0/spi0.0
```

这说明 `/dev/spidev0.0` 实际对应的是 `spi3` 控制器，不是芯片管脚图里的 `SPI0`。

因此当前 `DISPLAY_BUS=spidev SPI_DEV=/dev/spidev0.0` 的已验证接线是：

```text
Display SCL -> Duo S PIN23 / B15 / SPI3_SCK
Display SDA -> Duo S PIN19 / B13 / SPI3_SDO
Display CS  -> Duo S PIN24             # GPIO manual CS
Display DC  -> Duo S PIN5
Display RES -> Duo S PIN3
Display BLK -> Duo S PIN15
```

对应 pinmux：

```sh
duo-pinmux -w B15/SPI3_SCK
duo-pinmux -w B13/SPI3_SDO
```

注意：如果将 `SCL/SDA` 错接到管脚图中的 `SPI0_SCK/SPI0_SDO`：

```text
Display SCL -> PIN35 / C16 / SPI0_SCK
Display SDA -> PIN30 / C14 / SPI0_SDO
```

屏幕会只有背光，没有画面刷新，因为当前 `/dev/spidev0.0` 并不绑定到这个 SPI0 控制器。

当前 spidev backend 使用：

```text
SPI mode: SPI_MODE_0
CS:       GPIO manual CS, Display CS -> PIN24
```

不要在当前内核上给 spidev 设置 `SPI_NO_CS`。实测会失败：

```text
spidev spi0.0: setup: unsupported mode bits 40
set SPI mode failed: Invalid argument
display bus init failed
```

因此代码只设置 `SPI_MODE_0`。屏幕的 `CS` 仍然由 `display_bus.c` 里的 GPIO `PIN24` 手动控制。

## 9. Hardware SPI Migration

如果后续不再使用软件模拟 SPI，当前仓库已经提供用户态 `spidev` backend：

```text
1. 设备树打开目标 SPI 控制器，并挂 spidev 节点。
2. pinmux 将 SCK / MOSI / 可选 CS 切到 SPI 复用功能。
3. DC / RES / BLK 继续使用 wiringX GPIO 控制。
4. 使用 DISPLAY_BUS=spidev 编译。
5. ST7789 上层初始化和刷屏逻辑保持不变。
```

推荐迁移顺序：

```text
1. 保持当前软件 SPI 版本作为 known-good baseline。
2. 先只抽象 bus 层，确认软件 SPI 不回归。
3. 再使用 DISPLAY_BUS=spidev 验证硬件 SPI。
4. 最后根据性能决定是否继续做内核 framebuffer / DRM。
```

## 10. Debug Notes

完整定位过程见：

```text
docs/debug/ST7789V3-display-bringup-debug.md
```

其中记录了：

```text
1. 只有背光、没有稳定图像的问题现象。
2. 0x29 Display On 触发残留红色闪烁的判断方法。
3. CS 接地隔离测试。
4. 使用 BLK 作为简易 GPIO 指示灯的测试方法。
5. 最终定位 PIN22/PIN26 不稳定，并改用 PIN5/PIN3。
6. 最终全屏 RGB 刷新成功的已知可用配置。
```

## 11. References

- [zwyzwm/TFT-ST7789](https://github.com/zwyzwm/TFT-ST7789.git)
- [milkv-duo/duo-examples](https://github.com/milkv-duo/duo-examples)
- [Milk-V Duo S ST7789 document](https://milkv.io/zh/docs/duo/Accessories/ST7789)
