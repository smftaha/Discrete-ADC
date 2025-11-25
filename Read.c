//////////////////////////////////////////////////////////////////////////////////////////////
//this function is called once, at the startup
F_Init ()
{
	//init gpio … . I did’t write their code here

	//A.8.3  Input capture configuration code example
	/* (1) Select the active input TI1 (CC1S = 01),
				 program the input filter for 8 clock cycles (IC1F = 0011),
				 select the rising edge on CC1 (CC1P = 0, reset value)
				 and prescaler at each valid transition (IC1PS = 00, reset value) */
	/* (2) Enable capture by setting CC1E */
	/* (3) Enable interrupt on Capture/Compare */
	/* (4) Enable counter */
	
	RCC->APB1ENR |= RCC_APB1ENR_TIM14EN; /* (0) */
	TIM14->PSC = 0; /* (1) */
	TIM14->ARR = 0xffff; /* (2) */
	TIM14->CCMR1 |= TIM_CCMR1_CC1S_0
							 | TIM_CCMR1_IC1F_0 | TIM_CCMR1_IC1F_1; /* (1)*/
	TIM14->CCER |= TIM_CCER_CC1P | TIM_CCER_CC1E; /* (2) */
	//TIM14->DIER |= TIM_DIER_CC1IE; /* (3) */
	TIM14->CR1  |= TIM_CR1_CEN; /* (4) */
}
//////////////////////////////////////////////////////////////////////////////////////////////
#define		CALIB_VIN_H10V		(uint16_t)(1019)
#define		CALIB_VIN_L10V		(uint16_t)(636)
#define		IO_L_SWA			GPIOB->BRR=	(1<<1);
#define		IO_H_SWA			GPIOB->BSRR=	(1<<1);
#define		IO_L_SWB			GPIOA->BRR=	(1<<5);
#define		IO_H_SWB			GPIOA->BSRR=	(1<<5);
////////////////////////////////////////////////////////
//this function must be called every 5mS
uint16_t	FT_Capt		(void)
{
	static	int32_t		Li32;
	static	uint8_t		LStg;
	static	uint16_t	LAdcSum[3], LAdcMnBuf[3][CNST_ADC_BUFSZ];
	static	uint8_t		LBufIndx, LCnt8;
	uint16_t	Lu16, LChk=0;
	
	IO_H_SWA;
	IO_H_SWB;
	if ( SysTick->VAL > 2100)
		Lu16= SysTick->VAL -2000;
	while ( SysTick->VAL > Lu16);	//delay 1ms- it should be varied!
	switch	(LStg)
	{
		case	0:	//enable ref_L, read capt vin
			LStg	=1;
			IO_L_SWA;
			IO_L_SWB;
			TIM14->CNT = 0;	//or send update
			LAdcSum[2] += TIM14->CCR1;
		break;
		case	1:	//enable ref_H, read ref_L
			LStg	=2;
			IO_L_SWB;
			TIM14->CNT = 0;	//or send update
			LAdcSum[0] += TIM14->CCR1;
		break;
		case	2:	//enable vin, read ref_H
			LStg	=0;
			IO_L_SWA;
			TIM14->CNT = 0;	//or send update
			LAdcSum[1] += TIM14->CCR1;
			LCnt8++;
			if (( LCnt8 &7) ==0)	//each 2cycle of 50Hz
				LChk=1;
		break;
	}
	
	if (LChk)
	{
#ifdef		DBG_MODE
		Dbg.Clb_CptRawMn[0] =	LAdcSum[0]>>3;
		Dbg.Clb_CptRawMn[1] =	LAdcSum[1]>>3;
		Dbg.Clb_CptRawMn[2] =	LAdcSum[2]>>3;
#endif
		LAdcMnBuf[0][LBufIndx]=		LAdcSum[0]>>3;
		LAdcMnBuf[1][LBufIndx]=		LAdcSum[1]>>3;
		LAdcMnBuf[2][LBufIndx++]=	LAdcSum[2]>>3;
		if	(LBufIndx	>=CNST_ADC_BUFSZ)
			LBufIndx =0;
		LAdcSum[0]	=0;
		LAdcSum[1]	=0;
		LAdcSum[2]	=0;
		for	( uint8_t	j=0; j< 3;	j++)
		{
			for	( uint8_t	i=0; i< CNST_ADC_BUFSZ;	i++)
				LAdcSum[j]+=	LAdcMnBuf[j][i];
			LAdcSum[j]=	LAdcSum[j]/	CNST_ADC_BUFSZ;
		}
		Li32= LAdcSum[2] - LAdcSum[0];
		Li32= (CALIB_VIN_H10V - CALIB_VIN_L10V) * Li32;
		Li32= Li32 / (LAdcSum[1] - LAdcSum[0]);
		Li32 += CALIB_VIN_L10V;

		LAdcSum[0]	=0;
		LAdcSum[1]	=0;
		LAdcSum[2]	=0;
#ifdef		DBG_MODE
		Dbg.Vin10 =	Li32;
#endif
	}
	return	Li32;
}
//////////////////////////////////////////////////////////////////////////////////////////////
