# WT32-ETH01 (26-Pin) Pinout และสรุปวงจรใช้งาน

เอกสารนี้สรุป **Pinout ของ WT32-ETH01 แบบ 26 ขาจริง** และ **การแม็บขาใช้งานในโปรเจกต์** ตามที่ตกลงล่าสุด

---

## 1) ภาพรวมการใช้งานในโปรเจกต์

โครงสร้างหลักของบอร์ดในโปรเจกต์นี้:

```text
WT32-ETH01
├── CH1 = UART2 → RS485_CH1 → AHF-V4 / RCU หลัก
├── CH2 = UART0 → SN3257 Switch
│         ├── PROGRAM / DEBUG → USB-TTL
│         └── RUN             → RS485_CH2 → Protocol อื่น
├── I2C  → MCP23017
│         └── DIP Switch 8 ช่อง
└── Ethernet → Modbus TCP server
```

---

## 2) WT32-ETH01 ของจริงเป็น 26 ขา

> บอร์ดจริงเป็น **ซ้าย 13 ขา + ขวา 13 ขา = 26 ขา**

### ฝั่งซ้าย (Left, L1–L13)

| ลำดับ | Pin | หน้าที่ |
|---:|---|---|
| L1 | EN | Enable |
| L2 | GND | Ground |
| L3 | 3V3 | 3.3V |
| L4 | EN | Enable |
| L5 | IO32 / CFG | GPIO32 |
| L6 | IO33 / 485_EN | GPIO33 |
| L7 | IO5 / RXD2 | UART2 RX |
| L8 | IO17 / TXD2 | UART2 TX |
| L9 | GND | Ground |
| L10 | 3V3 | 3.3V |
| L11 | GND | Ground |
| L12 | 5V | 5V |
| L13 | LINK | Ethernet Link LED |

### ฝั่งขวา (Right, R1–R13)

| ลำดับ | Pin | หน้าที่ |
|---:|---|---|
| R1 | IO1 / TX0 | UART0 TX |
| R2 | IO3 / RX0 | UART0 RX |
| R3 | IO0 | BOOT |
| R4 | GND | Ground |
| R5 | IO39 | Input only |
| R6 | IO36 | Input only |
| R7 | IO15 | Boot / Strapping |
| R8 | IO14 | GPIO |
| R9 | IO12 | Boot / Strapping |
| R10 | IO35 | Input only |
| R11 | IO4 | GPIO |
| R12 | IO2 | Boot / Strapping |
| R13 | GND | Ground |

---

## 3) ขาที่ใช้จริงในโปรเจกต์นี้

### CH1 RS485 (AHF-V4)

```text
IO17 = TX2
IO5  = RX2
IO33 = DE/RE
```

### CH2 RS485 ผ่าน SN3257

```text
IO1  = TX0
IO3  = RX0
IO14 = DE/RE
```

### I2C ไป MCP23017

```text
IO32 = SDA
IO4  = SCL
```

### Boot

```text
IO0 = BOOT only
```

---

## 4) สรุป Pin I/O ที่ต้องใช้ทั้งหมด

| GPIO | ชื่อในวงจร | ทิศทาง | หน้าที่ |
|---:|---|---|---|
| GPIO17 | CH1_TX | Output | UART2 TX ไป RS485_CH1 |
| GPIO5 | CH1_RX | Input | UART2 RX จาก RS485_CH1 |
| GPIO33 | CH1_DE | Output | คุม DE/RE ของ RS485_CH1 |
| GPIO1 | UART0_TX | Output | UART0 TX เข้า SN3257 |
| GPIO3 | UART0_RX | Input | UART0 RX เข้า SN3257 |
| GPIO14 | CH2_DE | Output | คุม DE/RE ของ RS485_CH2 |
| GPIO32 | I2C_SDA | I/O | SDA ไป MCP23017 |
| GPIO4 | I2C_SCL | I/O | SCL ไป MCP23017 |
| GPIO0 | BOOT | Input | ใช้เฉพาะเข้าโหมดแฟลช |

---

## 5) การแม็บวงจรแต่ละบล็อก

### 5.1 CH1: RS485 หลักสำหรับ AHF-V4

```text
WT32 IO17 / TXD2  → RS485_CH1 DI
WT32 IO5  / RXD2  ← RS485_CH1 RO
WT32 IO33         → RS485_CH1 DE + /RE
3.3V              → RS485 Driver VCC
GND               → RS485 Driver GND
A/B               → RCU / AHF-V4 Bus
```

ลอจิกควบคุม:

```text
IO33 = LOW  → Receive
IO33 = HIGH → Transmit
```

---

### 5.2 CH2: RS485 ช่องที่สองผ่าน SN3257

```text
WT32 IO1 / TX0 → SN3257 → RS485_CH2 DI
WT32 IO3 / RX0 ← SN3257 ← RS485_CH2 RO
WT32 IO14      → RS485_CH2 DE + /RE
3.3V           → RS485 Driver VCC
GND            → RS485 Driver GND
A/B            → Protocol อื่น
```

ลอจิกควบคุม:

```text
IO14 = LOW  → Receive
IO14 = HIGH → Transmit
```

---

### 5.3 SN3257 สำหรับสลับ UART0

ใช้สลับ UART0 ระหว่าง **PROGRAM/DEBUG** กับ **RUN/RS485_CH2**

| SN3257 | ต่อกับ | หน้าที่ |
|---|---|---|
| CH1 COM | IO1 / TX0 | TX0 Common |
| CH1 A | USB-TTL RX | PROGRAM |
| CH1 B | RS485_CH2 DI | RUN |
| CH2 COM | IO3 / RX0 | RX0 Common |
| CH2 A | USB-TTL TX | PROGRAM |
| CH2 B | RS485_CH2 RO | RUN |
| SEL | Switch/Jumper | เลือก PROGRAM / RUN |
| EN | Pull-down 10k ไป GND | Enable switch |
| VDD | 3.3V | ไฟเลี้ยง |
| GND | GND | Ground |

ลอจิก:

```text
SEL = 0 → PROGRAM / DEBUG / USB-TTL
SEL = 1 → RUN / RS485_CH2

EN = LOW  → Enable
EN = HIGH → Disable
```

---

### 5.4 USB-TTL Header

ใช้สำหรับ flash/debug เท่านั้น

| USB-TTL | ต่อไปที่ |
|---|---|
| TXD | IO3 / RX0 ผ่าน SN3257 |
| RXD | IO1 / TX0 ผ่าน SN3257 |
| GND | GND |
| 3.3V | Optional |

> ต้องใช้ **USB-TTL 3.3V logic เท่านั้น**

---

### 5.5 MCP23017

ใช้สำหรับอ่าน DIP Switch 8 ช่อง

#### I2C

```text
WT32 IO32 → MCP23017 SDA
WT32 IO4  → MCP23017 SCL
3.3V      → MCP23017 VDD
GND       → MCP23017 VSS
```

ต้องมี pull-up:

```text
SDA → 4.7kΩ → 3.3V
SCL → 4.7kΩ → 3.3V
```

#### Address และ Reset

```text
A0 → GND
A1 → GND
A2 → GND
RESET → 10kΩ → 3.3V
```

I2C Address:

```text
0x20
```

---

## 6) DIP Switch 8 ช่อง ผ่าน MCP23017

ใช้ Port A ทั้งชุด

```text
GPA0 → DIP1 → GND
GPA1 → DIP2 → GND
GPA2 → DIP3 → GND
GPA3 → DIP4 → GND
GPA4 → DIP5 → GND
GPA5 → DIP6 → GND
GPA6 → DIP7 → GND
GPA7 → DIP8 → GND
```

ให้เปิด internal pull-up ของ MCP23017 ใน firmware

ลอจิก:

```text
DIP OFF = 1
DIP ON  = 0
```

Firmware reads Port A at startup and uses the active-low value as the board ID:

```text
board_id = (~GPIOA) & 0xFF
DIP1 = bit0
DIP8 = bit7
```

ค่าที่เปิดให้อ่านผ่าน Modbus TCP ตอนนี้มี `board_id_valid` เป็น discrete
input สำหรับช่วย debug สายหรือ MCP23017 ไม่ตอบ ส่วน input registers ถูกสงวนให้
ตรงกับ `Inpbuf[0..5]` ของ RCU.

ถ้าอ่าน MCP23017/DIP ไม่ได้ firmware จะใช้ค่า fallback:

```text
board_id = 1
board_id_valid = false
```

> **ไม่ใช้ interrupt**

---

## 7) ขาที่ไม่แนะนำให้ใช้

### Boot / Strapping Pins

| GPIO | สถานะ | หมายเหตุ |
|---:|---|---|
| GPIO0 | BOOT | ใช้เฉพาะเข้าโหมดแฟลช |
| GPIO2 | Strapping | ไม่แนะนำ |
| GPIO12 | Strapping | ไม่แนะนำอย่างยิ่ง |
| GPIO15 | Strapping | ไม่แนะนำ |

ลอจิกตอน boot ที่ควรทราบ:

```text
GPIO2  ควร LOW หรือ floating
GPIO12 ควร LOW
GPIO15 ควร HIGH หรือ floating
```

### Input Only

| GPIO | หมายเหตุ |
|---:|---|
| GPIO35 | Input only |
| GPIO36 | Input only |
| GPIO39 | Input only |

### Ethernet Related

| GPIO | เหตุผล |
|---:|---|
| GPIO16 | Ethernet PHY power |
| GPIO18 | Ethernet MDIO |
| GPIO23 | Ethernet MDC |

---

## 8) ขาที่เหลือหลังจัดวงจรนี้

ขาที่เหลือและยังพอใช้งานได้:

| GPIO | สถานะ |
|---:|---|
| GPIO35 | ว่าง / Input only |
| GPIO36 | ว่าง / Input only |
| GPIO39 | ว่าง / Input only |

---

## 9) สรุปแบบสั้น

```text
CH1 RS485 (AHF-V4)
IO17 = TX2
IO5  = RX2
IO33 = DE/RE

CH2 RS485 ผ่าน SN3257
IO1  = TX0
IO3  = RX0
IO14 = DE/RE

I2C MCP23017
IO32 = SDA
IO4  = SCL

BOOT
IO0 = BOOT only

MCP23017
GPA0-GPA7 = DIP Switch 1-8
```

---

## 10) หมายเหตุ

- วงจรนี้เหมาะกับ WT32-ETH01 แบบ **26 ขาจริง**
- CH1 ใช้ UART2 เป็น serial หลัก
- CH2 ใช้ UART0 ผ่าน SN3257 เพื่อให้ยัง flash/debug ได้
- MCP23017 ใช้ลดการใช้ GPIO บน WT32 และรองรับ DIP 8 ช่องได้ง่าย
- ถ้าต้องการเพิ่ม input อีกในอนาคต สามารถใช้ GPIO35/36/39 ได้ (input-only)
