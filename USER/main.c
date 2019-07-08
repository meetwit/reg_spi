#include "xmc4800.h"
#include "GPIO.h"
#include "do_crc6.h"

#define TICKS_PER_SECOND 1000000

struct bissc{
	//uint8_t head;		//  3bit
	uint8_t start;		//  1bit
	uint8_t csd;		//  1bit
	uint32_t data;		// 20bit
	uint8_t zero;		//  6bit
	uint8_t err;		//  1bit
	uint8_t war;		//  1bit
	uint8_t crc;		//  6bit
	//uint8_t foot;		//  1bit
  };

	
uint8_t RxPacket[5],crc_ans=0,crc_input[6];
uint32_t crc_erro_count=0,crc_cor_count=0,crc_3_count=0,crc_4_count=0;
struct bissc bissc_test={0};
uint32_t sys_time = 0,temp_sys_time[2]={0};
uint8_t temp_sys=0;

	void SPI_ConfigureBaudRate(void)
{

	uint32_t Brg_PDiv = 0;

	Brg_PDiv = 9;			//144/2/(pdiv+1)	9的话约等于7.2M

	USIC0_CH0->FDR |= 0x3ff43ff;

	USIC0_CH0->BRG = (((uint32_t)Brg_PDiv << USIC_CH_BRG_PDIV_Pos) & USIC_CH_BRG_PDIV_Msk)|0x00080;

}

/*****************************************************************************
 * Function:      void SPI_vInit(void)
 * Description:   SPI初始化
 * Caveats:       2013-11-01 00:42
 *****************************************************************************/
void SPI_vInit(void)
{

	//-------------------------------------------------------------------------
  // Step 1: 内核状态寄存器配置 - 使能模块，模块 + MODEN位保护
	//         Configuration of the U0C1 Kernel State Configuration
	//-------------------------------------------------------------------------

	 SCU_RESET->PRCLR0 |= SCU_RESET_PRCLR0_USIC0RS_Msk;					//clear PRCLR0 USIC0
	 while (SCU_RESET->PRSTAT0 &	SCU_RESET_PRSTAT0_USIC0RS_Msk);

	 USIC0_CH0->KSCFG    =  0x00000003;      // load U0C0 kernel state configuration
	                                         // register

	//-------------------------------------------------------------------------
  // Step 2:  选择分数分频器，配置波特率,并给相应寄存器赋值
	//     		Configuration of baud rate
	//-------------------------------------------------------------------------

	SPI_ConfigureBaudRate();

	//-------------------------------------------------------------------------
  // Step 3: 通道1引脚设置：P2.2、P2.5、P2.4和P2.3;  配置输入级
	//-------------------------------------------------------------------------

	Control_P4_7(INPUT, STRONG);							//Control_P2_2(INPUT, STRONG);							//Control_P2_2(INPUT, STRONG);								//MRST
	Control_P1_7(OUTPUT_PP_AF2, STRONG);			//Control_P3_13(OUTPUT_PP_AF2, STRONG);			//Control_P2_5(OUTPUT_PP_AF2, STRONG);				//MTSR
	Control_P1_1(OUTPUT_PP_AF2,STRONG);				//Control_P3_0(OUTPUT_PP_AF2,STRONG);				//Control_P2_4(OUTPUT_PP_AF2, STRONG);				//SCLK
	Control_P1_8(OUTPUT_PP_AF2, STRONG);			//Control_P2_3(OUTPUT_PP_AF2, STRONG);			//Control_P2_3(OUTPUT_PP_AF2, STRONG);				//SELO


	WR_REG(USIC0_CH0->BRG,USIC_CH_BRG_SCLKCFG_Msk,USIC_CH_BRG_SCLKCFG_Pos,3);			//与clock同步，极性反转，为了空闲时输出为高 3

	//Configuration of USIC Input Stage
	WR_REG(USIC0_CH0->DX0CR, USIC_CH_DX0CR_DSEL_Msk, USIC_CH_DX0CR_DSEL_Pos, 2);		//000	DX0C	P4.7
	WR_REG(USIC0_CH0->DX0CR, USIC_CH_DX0CR_INSW_Msk, USIC_CH_DX0CR_INSW_Pos, 1);		//由自选择DXn决定输入

	//-------------------------------------------------------------------------
  // Step 4:配置USIC移位控制寄存器
	//-------------------------------------------------------------------------
	WR_REG(USIC0_CH0->SCTR, USIC_CH_SCTR_PDL_Msk, USIC_CH_SCTR_PDL_Pos, 1);		//The passive data level is . here is change 1
	WR_REG(USIC0_CH0->SCTR, USIC_CH_SCTR_TRM_Msk, USIC_CH_SCTR_TRM_Pos, 1);		//The shift control signal is considered active if it is at 1-level. 
																																						//This is the setting to be programmed to allow data transfers.
	WR_REG(USIC0_CH0->SCTR, USIC_CH_SCTR_SDIR_Msk, USIC_CH_SCTR_SDIR_Pos, 1);	//MSB first
	//Set Word Length (WLE) & Frame Length (FLE)
	WR_REG(USIC0_CH0->SCTR, USIC_CH_SCTR_FLE_Msk, USIC_CH_SCTR_FLE_Pos, 39);	//frame length
	WR_REG(USIC0_CH0->SCTR, USIC_CH_SCTR_WLE_Msk, USIC_CH_SCTR_WLE_Pos, 7);		//word length


	//-------------------------------------------------------------------------
  // Step 5: 配置发送控制状态寄存器
	//----------------------------------------------------------  ---------------
	//TBUF Data Enable (TDEN) = 1, TBUF Data Single Shot Mode (TDSSM) = 1
	WR_REG(USIC0_CH0->TCSR, USIC_CH_TCSR_TDEN_Msk, USIC_CH_TCSR_TDEN_Pos, 1);			//配置为01 TDV等于1时传输开始
	//WR_REG(USIC0_CH0->TCSR, USIC_CH_TCSR_TDV_Msk, USIC_CH_TCSR_TDV_Pos, 1);				//add me
	WR_REG(USIC0_CH0->TCSR, USIC_CH_TCSR_TDSSM_Msk, USIC_CH_TCSR_TDSSM_Pos, 1);		//单次

	//-------------------------------------------------------------------------
  // Step 6: 配置协议控制寄存器
	//-------------------------------------------------------------------------
	WR_REG(USIC0_CH0->PCR_SSCMode, USIC_CH_PCR_SSCMode_MSLSEN_Msk, USIC_CH_PCR_SSCMode_MSLSEN_Pos, 1);		//使能MSLSEN，主模式必须使能
	WR_REG(USIC0_CH0->PCR_SSCMode, USIC_CH_PCR_SSCMode_SELCTR_Msk, USIC_CH_PCR_SSCMode_SELCTR_Pos, 1);		//SELO输出方式 直接
	WR_REG(USIC0_CH0->PCR_SSCMode, USIC_CH_PCR_SSCMode_SELINV_Msk, USIC_CH_PCR_SSCMode_SELINV_Pos, 0);		//反转SELO输出
	WR_REG(USIC0_CH0->PCR_SSCMode, USIC_CH_PCR_SSCMode_SELO_Msk, USIC_CH_PCR_SSCMode_SELO_Pos, 1);				//SELOx active control by SELCTR

	//-------------------------------------------------------------------------
  // Step 7: 配置通道控制寄存器
	//-------------------------------------------------------------------------
	WR_REG(USIC0_CH0->CCR, USIC_CH_CCR_MODE_Msk, USIC_CH_CCR_MODE_Pos, 1);
	WR_REG(USIC0_CH0->CCR, USIC_CH_CCR_AIEN_Msk, USIC_CH_CCR_AIEN_Pos, 1);

}

void BISS_ConfigureFIFO(void)
{
	USIC0_CH0->PCR_SSCMode = 0x000003B;         // disable tiwen   fmclk  FEM 1
	WR_REG(USIC0_CH0->PCR_SSCMode,USIC_CH_PCR_SSCMode_TIWEN_Msk, USIC_CH_PCR_SSCMode_TIWEN_Pos, 0);			//no delay between world ?
	USIC0_CH0->TCSR &= 0xfffffffE;							//The automatic update of SCTR.WLE and TCSR.EOF is disabled
	USIC0_CH0->TCSR &= 0xffffffBE;							//The data word in TBUF is not considered as last word of an SSC frame
	USIC0_CH0->TCSR |= 0x40;										//The data word in TBUF is valid for transmission and a transmission start is possible. 
																							//New data should not be written to a TBUFx input location while TDV = 1.
	USIC0_CH0->RBCTR=0;
	USIC0_CH0->RBCTR  =0x0002800;              // fifo size 64  limit 40   DP 0
	USIC0_CH0->RBCTR  =0x6002800;								//0x6003e00
	USIC0_CH0->TBCTR=0;
	USIC0_CH0->TBCTR  =0x0000800;              // fifo size 2  limit  2    DP 62
	USIC0_CH0->TBCTR  =0x3000828;								//0x100023e

}


void BISS_vSensorModeSingle(uint8_t uwDataLength)
{
	int i=0;
	
	for(i=0;i<uwDataLength;i++){
		while((USIC0_CH0->TRBSR&0x00000800)!=0x00000800);		//等待发送缓冲器为空
		USIC0_CH0->IN[0]  = i;											//发送数据
	}
}

/*
解决第一个8位数据后有200ns高电平
*/
void BISS_vSensorModeSingle2(uint8_t uwDataLength)
{
	while((USIC0_CH0->TRBSR&0x00000800)!=0x00000800);		//等待发送缓冲器为空
	//赋值到发送FIFO中
	USIC0_CH0->IN[0]  = 0;											
	USIC0_CH0->IN[1]  = 0xf0;										
	USIC0_CH0->IN[2]  = 0x0f;								
	USIC0_CH0->IN[3]  = 0;								
	USIC0_CH0->IN[4]  = 0xff;			
}


int main(){
	uint8_t st=0;
	
	SystemCoreClockUpdate();
	SPI_vInit();
	BISS_ConfigureFIFO();										//FIFO
	SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);
	
		BISS_vSensorModeSingle(1);		//推空缓存？
	while((USIC0_CH0->TRBSR&0x0008)==0x0008){;}	//等待接收缓存有数据
			RxPacket[st]= (USIC0_CH0->OUTR&0x000000ff);			//取数据低8位，根据配置的位数读取，最多16位

	while(1){
		temp_sys_time[0] = sys_time;
		BISS_vSensorModeSingle(5);
		
		for(st=0;st<5;st++){
			while((USIC0_CH0->TRBSR&0x0008)==0x0008);		//等待接收缓存有数据
			RxPacket[st]= (USIC0_CH0->OUTR&0x000000ff);			//取数据低8位，根据配置的位数读取，最多16位
		}
		if(RxPacket[0]&0x10){
			crc_3_count++;
			//bissc_test.head 	= (RxPacket[0]&0xe0)>>5;		//3bit
			bissc_test.start 	= (RxPacket[0]&0x10)>>4;		//1bit	always value 1
			bissc_test.csd 		= (RxPacket[0]&0x08)>>3;		//1bit
			bissc_test.data 	= (((uint32_t)RxPacket[0]&0x07) << 17) | (((uint32_t)RxPacket[1])<<9) | (((uint32_t)RxPacket[2])<<1)|(((uint32_t)RxPacket[3]&0x80) >>7) ;		//20bit
			bissc_test.zero 	= (RxPacket[3]&0x7e)>>1;		//6bit
			bissc_test.err 		= RxPacket[3]&0x1;				//1bit
			bissc_test.war 		= (RxPacket[4]&0x80)>>7;		//1bit
			bissc_test.crc 		= (RxPacket[4]&0x7e)>>1;		//6bit
			//bissc_test.foot 	= RxPacket[4]&0x1;				//1bit
		}
		else if(RxPacket[0]&0x08){
			crc_4_count++;
			//bissc_test.head 	= (RxPacket[0]&0xe0)>>5;		//4bit
			bissc_test.start 	= (RxPacket[0]&0x08)>>3;		//1bit	always value 1
			bissc_test.csd 		= (RxPacket[0]&0x04)>>2;		//1bit
			bissc_test.data 	= (((uint32_t)RxPacket[0]&0x03) << 18) | (((uint32_t)RxPacket[1])<<10) | (((uint32_t)RxPacket[2])<<2)|(((uint32_t)RxPacket[3]&0xc0) >>6) ;		//20bit
			bissc_test.zero 	= (RxPacket[3]&0x3f)>>0;		//6bit
			bissc_test.err 		= (RxPacket[4]&0x80)>>7;		//1bit
			bissc_test.war 		= (RxPacket[4]&0x40)>>6;		//1bit
			bissc_test.crc 		= (RxPacket[4]&0x3f)>>0;		//6bit
			//bissc_test.foot 	= RxPacket[4]&0x1;				//0bit
		}

		crc_input[0]=(bissc_test.data&0xf0000)>>16;
		crc_input[1]=(bissc_test.data&0xfc00)>>10;
		crc_input[2]=(bissc_test.data&0x3f0)>>4;
		crc_input[3]=(bissc_test.data&0xf)<<2;
		crc_input[4]=(bissc_test.err<<1)|(bissc_test.war<<0);
		crc_input[5]=0;
		crc_ans = fast_crc6_2(crc_input,6);


		if(crc_ans!=bissc_test.crc){
			crc_erro_count++;
		}else{
			crc_cor_count++;
		}
		
		temp_sys_time[1] = sys_time;
		
		temp_sys = temp_sys_time[1] - temp_sys_time[0];
		
		st=1;
		while(st++);
		st=1;
		while(st++);
		st=1;
		while(st++);
		st=1;
		while(st++);
		st=1;
		while(st++);		//间隔10us，模拟时间片轮询，此时执行其他任务
										//也避免编码器在无CPU干预情况下结束对下一次的错误读取，该时间400ns左右。
	}
}




void SysTick_Handler (void)          
{
	sys_time++;
}
