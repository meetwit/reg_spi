# reg_spi
xmc4800 寄存器 SPI驱动 bissc自解析协议
##介绍
该工程时基于infineon XMC4800 单片机编写的SPI驱动。SPI速率10Mbit/s。因为使用XMC库和APP，当速率上升到10Mbit/s，字节将会出出现一小段时间延时。这是因为首字节等待接收数据造成，所以，在这里首字节不再等待数据回来。
