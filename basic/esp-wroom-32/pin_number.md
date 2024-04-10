0~39 (Max 40)

Left side of picture from top

    EN
    VP
    VN
    D34
    D35
    D32
    D33
    D25
    D26
    D27
    D14
    D12
    D13
    GND
    VN

Right side of picture from top

    D23
    D22
    TX0
    RX0
    D21
    D19
    D18
    D5
    D17
    D16
    D4
    D2
    D15
    GND
    3V3



number mapping

    0   
    1   TXD0      (esp32-wroom-32_datasheet_en.pdf)
    2   pin
    3   RXD0      (esp32-wroom-32_datasheet_en.pdf)
    4   pin
    5   pin
    6   SCK/CLK*  (esp32-wroom-32_datasheet_en.pdf)
    7   SDO/SD0*  (esp32-wroom-32_datasheet_en.pdf) 
    8   SDI/SD1*  (esp32-wroom-32_datasheet_en.pdf)
    9   SHD/SD2*  (esp32-wroom-32_datasheet_en.pdf)
    10  SWP/SD3*  (esp32-wroom-32_datasheet_en.pdf)
    11  SCS/CMD*  (esp32-wroom-32_datasheet_en.pdf)
    12  pin
    13  pin
    14  pin
    15  pin
    16  pin
    17  pin
    18  pin
    19  pin
    20
    21  pin
    22  pin
    23  pin
    24
    25  pin
    26  pin
    27  pin
    28
    29
    30
    31
    32  pin
    33  pin
    34  pin
    35  pin
    36  SENSOR_VP (esp32-wroom-32_datasheet_en.pdf)
    37
    38
    39  SENSOR_VN (esp32-wroom-32_datasheet_en.pdf)
    40

    Pins 
      SCK/CLK, SDO/SD0, SDI/SD1, SHD/SD2, SWP/SD3, SCS/CMD, namely,
      GPIO6 to GPIO11
    are connected to the integrated SPI flash integrated on the module and
    are not recommended for other uses