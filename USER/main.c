#include "xmc4800.h"
#include "GPIO.h"

uint8_t read[5];

	void SPI_ConfigureBaudRate(void)
{

	uint32_t Brg_PDiv = 0;

	Brg_PDiv = 6;			//144/2/(pdiv+1)	约等于10M

	USIC0_CH1->FDR |= 0x3ff43ff;

	USIC0_CH1->BRG = (((uint32_t)Brg_PDiv << USIC_CH_BRG_PDIV_Pos) & USIC_CH_BRG_PDIV_Msk)|0x00080;

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

	 USIC0_CH1->KSCFG    =  0x00000003;      // load U0C1 kernel state configuration
	                                         // register

	//-------------------------------------------------------------------------
  // Step 2:  选择分数分频器，配置波特率,并给相应寄存器赋值
	//     		Configuration of baud rate
	//-------------------------------------------------------------------------

	SPI_ConfigureBaudRate();

	//-------------------------------------------------------------------------
  // Step 3: 通道1引脚设置：P2.2、P2.5、P2.4和P2.3;  配置输入级
	//-------------------------------------------------------------------------

	Control_P2_2(INPUT, STRONG);							//Control_P2_2(INPUT, STRONG);								//MRST
	Control_P3_13(OUTPUT_PP_AF2, STRONG);			//Control_P2_5(OUTPUT_PP_AF2, STRONG);				//MTSR
	Control_P3_0(OUTPUT_PP_AF2,STRONG);				//Control_P2_4(OUTPUT_PP_AF2, STRONG);				//SCLK
	Control_P2_3(OUTPUT_PP_AF2, STRONG);			//Control_P2_3(OUTPUT_PP_AF2, STRONG);				//SELO

	WR_REG(USIC0_CH1->BRG,USIC_CH_BRG_SCLKCFG_Msk,USIC_CH_BRG_SCLKCFG_Pos,1);			//与clock同步，极性反转，为了空闲时输出为高

	//Configuration of USIC Input Stage
	WR_REG(USIC0_CH1->DX0CR, USIC_CH_DX0CR_DSEL_Msk, USIC_CH_DX0CR_DSEL_Pos, 0);		//000	DX1A	P2.2
	WR_REG(USIC0_CH1->DX0CR, USIC_CH_DX0CR_INSW_Msk, USIC_CH_DX0CR_INSW_Pos, 1);		//由自选择DXn决定输入

	//-------------------------------------------------------------------------
  // Step 4:配置USIC移位控制寄存器
	//-------------------------------------------------------------------------
	WR_REG(USIC0_CH1->SCTR, USIC_CH_SCTR_PDL_Msk, USIC_CH_SCTR_PDL_Pos, 1);		//The passive data level is 1.
	WR_REG(USIC0_CH1->SCTR, USIC_CH_SCTR_TRM_Msk, USIC_CH_SCTR_TRM_Pos, 1);		//The shift control signal is considered active if it is at 1-level. 
																																						//This is the setting to be programmed to allow data transfers.
	WR_REG(USIC0_CH1->SCTR, USIC_CH_SCTR_SDIR_Msk, USIC_CH_SCTR_SDIR_Pos, 1);	//MSB first
	//Set Word Length (WLE) & Frame Length (FLE)
	WR_REG(USIC0_CH1->SCTR, USIC_CH_SCTR_FLE_Msk, USIC_CH_SCTR_FLE_Pos, 39);	//frame length
	WR_REG(USIC0_CH1->SCTR, USIC_CH_SCTR_WLE_Msk, USIC_CH_SCTR_WLE_Pos, 7);		//word length


	//-------------------------------------------------------------------------
  // Step 5: 配置发送控制状态寄存器
	//----------------------------------------------------------  ---------------
	//TBUF Data Enable (TDEN) = 1, TBUF Data Single Shot Mode (TDSSM) = 1
	WR_REG(USIC0_CH1->TCSR, USIC_CH_TCSR_TDEN_Msk, USIC_CH_TCSR_TDEN_Pos, 1);			//配置为01 TDV等于1时传输开始
	WR_REG(USIC0_CH1->TCSR, USIC_CH_TCSR_TDV_Msk, USIC_CH_TCSR_TDV_Pos, 1);				//add me
	WR_REG(USIC0_CH1->TCSR, USIC_CH_TCSR_TDSSM_Msk, USIC_CH_TCSR_TDSSM_Pos, 1);		//单次

	//-------------------------------------------------------------------------
  // Step 6: 配置协议控制寄存器
	//-------------------------------------------------------------------------
	WR_REG(USIC0_CH1->PCR_SSCMode, USIC_CH_PCR_SSCMode_MSLSEN_Msk, USIC_CH_PCR_SSCMode_MSLSEN_Pos, 1);		//使能MSLSEN，主模式必须使能
	WR_REG(USIC0_CH1->PCR_SSCMode, USIC_CH_PCR_SSCMode_SELCTR_Msk, USIC_CH_PCR_SSCMode_SELCTR_Pos, 1);		//SELO输出方式 直接
	WR_REG(USIC0_CH1->PCR_SSCMode, USIC_CH_PCR_SSCMode_SELINV_Msk, USIC_CH_PCR_SSCMode_SELINV_Pos, 0);		//反转SELO输出
	WR_REG(USIC0_CH1->PCR_SSCMode, USIC_CH_PCR_SSCMode_SELO_Msk, USIC_CH_PCR_SSCMode_SELO_Pos, 1);				//SELOx actiive control by SELCTR

	//-------------------------------------------------------------------------
  // Step 7: 配置通道控制寄存器
	//-------------------------------------------------------------------------
	WR_REG(USIC0_CH1->CCR, USIC_CH_CCR_MODE_Msk, USIC_CH_CCR_MODE_Pos, 1);
	WR_REG(USIC0_CH1->CCR, USIC_CH_CCR_AIEN_Msk, USIC_CH_CCR_AIEN_Pos, 1);

}

	
//void BISS_ConfigureBaudRate(uint32_t uwBaudRate)
//{
//	SPI_ConfigureBaudRate();
//}

void BISS_ConfigureFIFO(void)
{

	USIC0_CH1->PCR_SSCMode = 0x000003B;         // disable tiwen   fmclk  FEM 1
	USIC0_CH1->TCSR &= 0xfffffffE;
	USIC0_CH1->TCSR &= 0xffffffBE;
	USIC0_CH1->TCSR |= 0x40;
	USIC0_CH1->RBCTR=0;
	USIC0_CH1->RBCTR  =0x0003e00;              // fifo size 64  limit 62   DP 0
	USIC0_CH1->RBCTR  =0x6003e00;
	USIC0_CH1->TBCTR=0;
	USIC0_CH1->TBCTR  =0x000023e;              // fifo size 2  limit  2    DP 62
	USIC0_CH1->TBCTR  =0x100023e;

}


void BISS_vSensorModeSingle(uint8_t uwDataLength)
{
	unsigned long Result=0;
	int i=0;
	
		//while((USIC0_CH1->TRBSR&0x1000)==0x1000);		//等待发送缓冲器为空
		USIC0_CH1->IN[0]  = 0;											//发送数据
		//while((USIC0_CH1->TRBSR&0x0008)==0x0008);		//等待接收缓存有数据
		read[i]= (USIC0_CH1->OUTR&0x000000ff);			//取数据低8位，根据配置的位数读取，最多16位
	
	for(i=1;i<uwDataLength;i++){
		while((USIC0_CH1->TRBSR&0x1000)==0x1000);		//等待发送缓冲器为空
		USIC0_CH1->IN[0]  = i;											//发送数据
		while((USIC0_CH1->TRBSR&0x0008)==0x0008);		//等待接收缓存有数据
		read[i]= (USIC0_CH1->OUTR&0x000000ff);			//取数据低8位，根据配置的位数读取，最多16位
	}

//	for(i=0;i<uwDataLength;i++){
//		while((USIC0_CH1->TRBSR&0x1000)==0x1000);		//等待发送缓冲器为空
//		USIC0_CH1->IN[0]  = i;											//发送数据
//		while((USIC0_CH1->TRBSR&0x0008)==0x0008);		//等待接收缓存有数据
//		read[i]= (USIC0_CH1->OUTR&0x000000ff);			//取数据低8位，根据配置的位数读取，最多16位
//	}
}


int main(){
	
	SystemCoreClockUpdate();
	SPI_vInit();
	BISS_ConfigureFIFO();										//FIFO
	
	while(1){
		BISS_vSensorModeSingle(6);
	}
}
