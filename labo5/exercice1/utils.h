#define LEDR_BASE      0x00000000
#define HEX3_HEX0_BASE 0x00000020
#define HEX5_HEX4_BASE 0x00000030
#define SWITCH_BASE    0x00000040
#define KEY_BASE       0x00000050
#define INTERRUPT_MASK 0x00000058
#define EDGE_MASK      0x0000005C
#define SET_VALUE      0xf
#define OFF_VALUE      0x0
#define LEDS_ON 	   0x3ff
#define HEX_1	       0x1
#define HEX_2	       0x2

//Numbers in hexa
uint16_t numbers[] = {
	//0x00, //éteint si 0 pour labo 5
    0x3f, // 0
	0x06, // 1
	0x5b, // 2
	0x4f, // 3
	0x66, // 4
	0x6d, // 5
	0x7d, // 6
	0x07, // 7
	0x7f, // 8
	0x6f, // 9
	0x063f, // 10
	0x0606, // 11
	0x065b, // 12
	0x064f, // 13
	0x0666, // 14
	0x066d, // 15
};
