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
make DISPLAY_BUS=software
```

如果要测试用户态硬件 SPI / `spidev`：

```sh
make clean
make DISPLAY_BUS=spidev SPI_DEV=/dev/spidev0.0 SPI_SPEED_HZ=12000000
```

其中：

```text
DISPLAY_BUS=software   使用 wiringX GPIO 软件模拟 SPI，默认值
DISPLAY_BUS=spidev     使用 /dev/spidevX.Y 发送 SPI 数据
SPI_DEV                spidev 设备节点，默认 /dev/spidev0.0
SPI_SPEED_HZ           SPI 速度，默认 12000000
```

## 5. Code Structure

当前代码已经把 ST7789 上层逻辑和底层传输拆开：

| File | Description |
| --- | --- |
| `st7789.c` | ST7789V3 初始化、地址窗口设置、RGB565 刷屏测试 |
| `display_bus.c` | 当前底层传输实现，使用 wiringX GPIO 软件模拟 SPI |
| `display_bus.h` | ST7789 上层调用的 bus 接口 |
| `data.c` / `data.h` | 图片和字模测试数据 |

当前仍然是软件 SPI：

```text
st7789.c -> display_bus_write_cmd/data() -> GPIO bit-bang SPI
```

也可以通过 `DISPLAY_BUS=spidev` 切到用户态硬件 SPI：

```text
st7789.c -> display_bus_write_cmd/data() -> /dev/spidevX.Y
```

`DC` / `RES` / `BLK` / `CS` 仍然由 wiringX GPIO 控制。`SCK` / `SDA(MOSI)` 在 `spidev` 模式下由硬件 SPI 控制器输出。

## 6. Run On Duo S

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
3. 屏幕循环全屏刷新 RED / GREEN / BLUE。
```

## 7. Pinmux Notes

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
2. 屏幕 SCL 接到该 SPI 控制器的 SCK。
3. 屏幕 SDA 接到该 SPI 控制器的 MOSI。
4. 对应 SCK/MOSI pinmux 已经切到 SPI 复用功能。
5. DC / RES / BLK / CS 仍然接到当前代码配置的 GPIO。
```

## 8. Hardware SPI Migration

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

## 9. Debug Notes

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

## 10. References

- [zwyzwm/TFT-ST7789](https://github.com/zwyzwm/TFT-ST7789.git)
- [milkv-duo/duo-examples](https://github.com/milkv-duo/duo-examples)
- [Milk-V Duo S ST7789 document](https://milkv.io/zh/docs/duo/Accessories/ST7789)
