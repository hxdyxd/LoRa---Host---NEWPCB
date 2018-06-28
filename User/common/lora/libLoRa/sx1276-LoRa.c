/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND 
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, SEMTECH SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) SEMTECH S.A.
 */
/*! 
 * \file       sx1276-LoRa.c
 * \brief      SX1276 RF chip driver mode LoRa
 *
 * \version    2.0.0 
 * \date       May 6 2013
 * \author     Gregory Cristian
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */
#include <string.h>
#include "sx1276-LoRa.h"
#include "sx1276-LoRaMisc.h"

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164.0
#define RSSI_OFFSET_HF                              -157.0

/*!
 * Frequency hopping frequencies table
 */
const int32_t HoppingFrequencies[] =
{
    916500000,
    923500000,
    906500000,
    917500000,
    917500000,
    909000000,
    903000000,
    916000000,
    912500000,
    926000000,
    925000000,
    909500000,
    913000000,
    918500000,
    918500000,
    902500000,
    911500000,
    926500000,
    902500000,
    922000000,
    924000000,
    903500000,
    913000000,
    922000000,
    926000000,
    910000000,
    920000000,
    922500000,
    911000000,
    922000000,
    909500000,
    926000000,
    922000000,
    918000000,
    925500000,
    908000000,
    917500000,
    926500000,
    908500000,
    916000000,
    905500000,
    916000000,
    903000000,
    905000000,
    915000000,
    913000000,
    907000000,
    910000000,
    926500000,
    925500000,
    911000000,
};

#define MODULE_SX1276RF1IAS 1
#define MODULE_SX1276RF1JAS 0

// Default settings
tLoRaSettings LoRaSettings =
{
    433000000,        // RFFrequency
    20,               // 12 Power
    8,                // 9 SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    9,                // 7 SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    2,                // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    true,             // CrcOn [0: OFF, 1: ON]
    false,            // ImplicitHeaderOn [0: OFF, 1: ON]
    0,                // RxSingleOn [0: Continuous, 1 Single]
    0,                // FreqHopOn [0: OFF, 1: ON]
    4,                // HopPeriod Hops every frequency hopping period symbols
    1000,              // TxPacketTimeout
    2000,              // RxPacketTimeout
    64,              // PayloadLength (used for implicit header mode)
};


void SX1276LoRaDeinit(tLora *loraConfigure)
{
    loraConfigure->SX1276LR = ( tSX1276LR* )(loraConfigure->SX1276Regs);
    loraConfigure->RFLRState = RFLR_STATE_IDLE;
    loraConfigure->RxPacketSize = 0;
    loraConfigure->RxGain = 1;
    loraConfigure->RxTimeoutTimer = 0;
    loraConfigure->TxPacketSize = 0;
    memcpy(&loraConfigure->LoRaSettings, &LoRaSettings, sizeof(LoRaSettings));
    //memcpy(loraConfigure->HoppingFrequencies, HoppingFrequencies, sizeof(HoppingFrequencies));
	
	loraConfigure->opModePrev = RFLR_OPMODE_STANDBY;
    loraConfigure->antennaSwitchTxOnPrev = true;
	
	loraConfigure->LoRaOn = false;
	loraConfigure->LoRaOnState = false;
}


void SX1276LoRaSetLoRaOn( tLora *loraConfigure, bool enable )
{
	//static bool LoRaOn = false;
	//static bool LoRaOnState = false;
    if( loraConfigure->LoRaOnState == enable )
    {
        return;
    }
    loraConfigure->LoRaOnState = enable;
    loraConfigure->LoRaOn = enable;

    if( loraConfigure->LoRaOn == true )
    {
        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_SLEEP );
        
        loraConfigure->SX1276LR->RegOpMode = ( loraConfigure->SX1276LR->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON;
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_OPMODE, loraConfigure->SX1276LR->RegOpMode );
        
        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_STANDBY );
                                        // RxDone               RxTimeout                   FhssChangeChannel           CadDone
        loraConfigure->SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                        // CadDetected          ModeReady
        loraConfigure->SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
        loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( REG_LR_DIOMAPPING1, &loraConfigure->SX1276LR->RegDioMapping1, 2 );
        
        loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( REG_LR_OPMODE, loraConfigure->SX1276Regs + 1, 0x70 - 1 );
    }
    else
    {
        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_SLEEP );
        
        loraConfigure->SX1276LR->RegOpMode = ( loraConfigure->SX1276LR->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF;
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_OPMODE, loraConfigure->SX1276LR->RegOpMode );
        
        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_STANDBY );
        
        loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( REG_OPMODE, loraConfigure->SX1276Regs + 1, 0x70 - 1 );
    }
}

void SX1276LoRaReset( void )
{
	uint32_t startTick;
    GPIO_ResetBits(GPIOA, GPIO_Pin_3);
    
    // Wait 1ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 1 ) );    

    GPIO_SetBits(GPIOA, GPIO_Pin_3);
    
    // Wait 6ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 6 ) );
}

void SX1276LoRaInit( tLora *loraConfigure )
{
	loraConfigure->LoraLowLevelFunc.SX1276InitIo( );
	
	//set lora mode
	SX1276LoRaSetLoRaOn(loraConfigure, true );
	
    loraConfigure->RFLRState = RFLR_STATE_IDLE;

    SX1276LoRaSetDefaults(loraConfigure);
    
    loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( REG_LR_OPMODE, loraConfigure->SX1276Regs + 1, 0x70 - 1 );
    
    loraConfigure->SX1276LR->RegLna = RFLR_LNA_GAIN_G1;

    loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( REG_LR_OPMODE, loraConfigure->SX1276Regs + 1, 0x70 - 1 );

	//SX1276LoRaSetPreambleLength(loraConfigure, 8);
	//printf("%d\n\n", SX1276LoRaGetPreambleLength(loraConfigure) );
    // set the RF settings 
    SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->LoRaSettings.RFFrequency );
    SX1276LoRaSetSpreadingFactor(loraConfigure, loraConfigure->LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
    SX1276LoRaSetErrorCoding(loraConfigure, loraConfigure->LoRaSettings.ErrorCoding );
    SX1276LoRaSetPacketCrcOn(loraConfigure, loraConfigure->LoRaSettings.CrcOn );
    SX1276LoRaSetSignalBandwidth(loraConfigure, loraConfigure->LoRaSettings.SignalBw );

    SX1276LoRaSetImplicitHeaderOn(loraConfigure, loraConfigure->LoRaSettings.ImplicitHeaderOn );
    SX1276LoRaSetSymbTimeout(loraConfigure, 0x3FF );
    SX1276LoRaSetPayloadLength(loraConfigure, loraConfigure->LoRaSettings.PayloadLength );
    SX1276LoRaSetLowDatarateOptimize(loraConfigure, true );

#if( ( MODULE_SX1276RF1IAS == 1 ) || ( MODULE_SX1276RF1KAS == 1 ) )
    if( loraConfigure->LoRaSettings.RFFrequency > 860000000 )
    {
        SX1276LoRaSetPAOutput(loraConfigure, RFLR_PACONFIG_PASELECT_RFO );
        SX1276LoRaSetPa20dBm(loraConfigure, false );
        loraConfigure->LoRaSettings.Power = 14;
        SX1276LoRaSetRFPower(loraConfigure, loraConfigure->LoRaSettings.Power );
    }
    else
    {
        SX1276LoRaSetPAOutput(loraConfigure, RFLR_PACONFIG_PASELECT_PABOOST );
        SX1276LoRaSetPa20dBm(loraConfigure, true );
        loraConfigure->LoRaSettings.Power = 20;
        SX1276LoRaSetRFPower(loraConfigure, loraConfigure->LoRaSettings.Power );
    } 
#elif( MODULE_SX1276RF1JAS == 1 )
    if( loraConfigure->LoRaSettings.RFFrequency > 860000000 )
    {
        SX1276LoRaSetPAOutput(loraConfigure, RFLR_PACONFIG_PASELECT_PABOOST );
        SX1276LoRaSetPa20dBm(loraConfigure, true );
        loraConfigure->LoRaSettings.Power = 20;
        SX1276LoRaSetRFPower(loraConfigure, loraConfigure->LoRaSettings.Power );
    }
    else
    {
        SX1276LoRaSetPAOutput(loraConfigure, RFLR_PACONFIG_PASELECT_RFO );
        SX1276LoRaSetPa20dBm(loraConfigure, false );
        loraConfigure->LoRaSettings.Power = 14;
        SX1276LoRaSetRFPower(loraConfigure, loraConfigure->LoRaSettings.Power );
    } 
#endif

    SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_STANDBY );
}

void SX1276LoRaSetDefaults( tLora *loraConfigure )
{
    // REMARK: See SX1276 datasheet for modified default values.

    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_VERSION, &loraConfigure->SX1276LR->RegVersion );
}

void SX1276LoRaSetOpMode( tLora *loraConfigure , uint8_t opMode )
{
    //static uint8_t opModePrev = RFLR_OPMODE_STANDBY;
    //static bool antennaSwitchTxOnPrev = true;
    bool antennaSwitchTxOn = false;

    loraConfigure->opModePrev = loraConfigure->SX1276LR->RegOpMode & ~RFLR_OPMODE_MASK;

    if( opMode != loraConfigure->opModePrev )
    {
        if( opMode == RFLR_OPMODE_TRANSMITTER )
        {
            antennaSwitchTxOn = true;
        }
        else
        {
            antennaSwitchTxOn = false;
        }
        if( antennaSwitchTxOn != loraConfigure->antennaSwitchTxOnPrev )
        {
            loraConfigure->antennaSwitchTxOnPrev = antennaSwitchTxOn;
            RXTX( antennaSwitchTxOn ); // Antenna switch control
        }
        loraConfigure->SX1276LR->RegOpMode = ( loraConfigure->SX1276LR->RegOpMode & RFLR_OPMODE_MASK ) | opMode;

        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_OPMODE, loraConfigure->SX1276LR->RegOpMode );        
    }
}

uint8_t SX1276LoRaGetOpMode(  tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_OPMODE, &loraConfigure->SX1276LR->RegOpMode );
    
    return loraConfigure->SX1276LR->RegOpMode & ~RFLR_OPMODE_MASK;
}

uint8_t SX1276LoRaReadRxGain(  tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_LNA, &loraConfigure->SX1276LR->RegLna );
    return( loraConfigure->SX1276LR->RegLna >> 5 ) & 0x07;
}

double SX1276LoRaReadRssi(  tLora *loraConfigure )
{
    // Reads the RSSI value
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_RSSIVALUE, &loraConfigure->SX1276LR->RegRssiValue );

    if( loraConfigure->LoRaSettings.RFFrequency < 860000000 )  // LF
    {
        return RSSI_OFFSET_LF + ( double )loraConfigure->SX1276LR->RegRssiValue;
    }
    else
    {
        return RSSI_OFFSET_HF + ( double )loraConfigure->SX1276LR->RegRssiValue;
    }
}

uint8_t SX1276LoRaGetPacketRxGain(  tLora *loraConfigure )
{
    return loraConfigure->RxGain;
}

int8_t SX1276LoRaGetPacketSnr(  tLora *loraConfigure )
{
    return loraConfigure->RxPacketSnrEstimate;
}

double SX1276LoRaGetPacketRssi(  tLora *loraConfigure )
{
    return loraConfigure->RxPacketRssiValue;
}

void SX1276LoRaStartRx(  tLora *loraConfigure )
{
    SX1276LoRaSetRFState( loraConfigure, RFLR_STATE_RX_INIT );
}

void SX1276LoRaGetRxPacket(  tLora *loraConfigure , void *buffer, uint16_t *size )
{
    *size = loraConfigure->RxPacketSize;
    loraConfigure->RxPacketSize = 0;
    memcpy( ( void * )buffer, ( void * )loraConfigure->RFBuffer, ( size_t )*size );
}

void SX1276LoRaSetTxPacket( tLora *loraConfigure , const void *buffer, uint16_t size )
{
    loraConfigure->TxPacketSize = size;
    memcpy( ( void * )loraConfigure->RFBuffer, buffer, ( size_t )loraConfigure->TxPacketSize ); 

    loraConfigure->RFLRState = RFLR_STATE_TX_INIT;
}

uint8_t SX1276LoRaGetRFState( tLora *loraConfigure )
{
    return loraConfigure->RFLRState;
}

void SX1276LoRaSetRFState( tLora *loraConfigure , uint8_t state )
{
    loraConfigure->RFLRState = state;
}

/*!
 * \brief Process the LoRa modem Rx and Tx state machines depending on the
 *        SX1276 operating mode.
 *
 * \retval rfState Current RF state [RF_IDLE, RF_BUSY, 
 *                                   RF_RX_DONE, RF_RX_TIMEOUT,
 *                                   RF_TX_DONE, RF_TX_TIMEOUT]
 */
uint32_t SX1276LoRaProcess(  tLora *loraConfigure )
{
    uint32_t result = RF_BUSY;
    
    switch( loraConfigure->RFLRState )
    {
    case RFLR_STATE_IDLE:
        break;
    case RFLR_STATE_RX_INIT:
        
        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_STANDBY );

        loraConfigure->SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                    //RFLR_IRQFLAGS_RXDONE |
                                    //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                    RFLR_IRQFLAGS_VALIDHEADER |
                                    RFLR_IRQFLAGS_TXDONE |
                                    RFLR_IRQFLAGS_CADDONE |
                                    //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                    RFLR_IRQFLAGS_CADDETECTED;
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGSMASK, loraConfigure->SX1276LR->RegIrqFlagsMask );

        if( loraConfigure->LoRaSettings.FreqHopOn == true )
        {
            loraConfigure->SX1276LR->RegHopPeriod = loraConfigure->LoRaSettings.HopPeriod;

            loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_HOPCHANNEL, &loraConfigure->SX1276LR->RegHopChannel );
			
			/**************************************** RANDOM Hopping *******************************************/
            SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->random_getRandomFreq(loraConfigure->HoppingFrequencieSeed, loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK) );
			/**************************************** RANDOM Hopping *******************************************/
        }
        else
        {
            loraConfigure->SX1276LR->RegHopPeriod = 255;
        }
        
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_HOPPERIOD, loraConfigure->SX1276LR->RegHopPeriod );
                
                                    // RxDone                    RxTimeout                   FhssChangeChannel           CadDone
        loraConfigure->SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                    // CadDetected               ModeReady
        loraConfigure->SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
        loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( REG_LR_DIOMAPPING1, &loraConfigure->SX1276LR->RegDioMapping1, 2 );
    
        if( loraConfigure->LoRaSettings.RxSingleOn == true ) // Rx single mode
        {

            SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_RECEIVER_SINGLE );
        }
        else // Rx continuous mode
        {
            loraConfigure->SX1276LR->RegFifoAddrPtr = loraConfigure->SX1276LR->RegFifoRxBaseAddr;
            loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_FIFOADDRPTR, loraConfigure->SX1276LR->RegFifoAddrPtr );
            
            SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_RECEIVER );
        }
        
        memset( loraConfigure->RFBuffer, 0, ( size_t )RF_BUFFER_SIZE );

        loraConfigure->PacketTimeout = loraConfigure->LoRaSettings.RxPacketTimeout;
        loraConfigure->RxTimeoutTimer = GET_TICK_COUNT( );
        loraConfigure->RFLRState = RFLR_STATE_RX_RUNNING;
        break;
    case RFLR_STATE_RX_RUNNING:
        
        //if( DIO0 == 1 ) // RxDone
		if(loraConfigure->LoraLowLevelFunc.read_single_reg(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_RXDONE)
        {
            loraConfigure->RxTimeoutTimer = GET_TICK_COUNT( );
            if( loraConfigure->LoRaSettings.FreqHopOn == true )
            {
                loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_HOPCHANNEL, &loraConfigure->SX1276LR->RegHopChannel );
                //SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->HoppingFrequencies[loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
				/**************************************** RANDOM Hopping *******************************************/
				SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->random_getRandomFreq(loraConfigure->HoppingFrequencieSeed, loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK) );
				/**************************************** RANDOM Hopping *******************************************/				
            }
            // Clear Irq
            loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE  );
            loraConfigure->RFLRState = RFLR_STATE_RX_DONE;
        }
        //if( DIO2 == 1 ) // FHSS Changed Channel
		if(loraConfigure->LoraLowLevelFunc.read_single_reg(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL)
        {
            loraConfigure->RxTimeoutTimer = GET_TICK_COUNT( );
            if( loraConfigure->LoRaSettings.FreqHopOn == true )
            {
                loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_HOPCHANNEL, &loraConfigure->SX1276LR->RegHopChannel );
                //SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->HoppingFrequencies[loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
				/**************************************** RANDOM Hopping *******************************************/
				SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->random_getRandomFreq(loraConfigure->HoppingFrequencieSeed, loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK) );
				/**************************************** RANDOM Hopping *******************************************/
            }
            // Clear Irq
            loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
            // Debug
            loraConfigure->RxGain = SX1276LoRaReadRxGain( loraConfigure );
        }

        if( loraConfigure->LoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            if( ( GET_TICK_COUNT( ) - loraConfigure->RxTimeoutTimer ) > loraConfigure->PacketTimeout )
            {
                loraConfigure->RFLRState = RFLR_STATE_RX_TIMEOUT;
            }
        }
        break;
    case RFLR_STATE_RX_DONE:
        loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_IRQFLAGS, &loraConfigure->SX1276LR->RegIrqFlags );
        if( ( loraConfigure->SX1276LR->RegIrqFlags & RFLR_IRQFLAGS_PAYLOADCRCERROR ) == RFLR_IRQFLAGS_PAYLOADCRCERROR )
        {
            // Clear Irq
            loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR  );
            
            if( loraConfigure->LoRaSettings.RxSingleOn == true ) // Rx single mode
            {
                loraConfigure->RFLRState = RFLR_STATE_RX_INIT;
            }
            else
            {
                loraConfigure->RFLRState = RFLR_STATE_RX_RUNNING;
            }
            break;
        }
        
        {
            uint8_t rxSnrEstimate;
            loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PKTSNRVALUE, &rxSnrEstimate );
            if( rxSnrEstimate & 0x80 ) // The SNR sign bit is 1
            {
                // Invert and divide by 4
                loraConfigure->RxPacketSnrEstimate = ( ( ~rxSnrEstimate + 1 ) & 0xFF ) >> 2;
                loraConfigure->RxPacketSnrEstimate = -loraConfigure->RxPacketSnrEstimate;
            }
            else
            {
                // Divide by 4
                loraConfigure->RxPacketSnrEstimate = ( rxSnrEstimate & 0xFF ) >> 2;
            }
        }
        
        loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PKTRSSIVALUE, &loraConfigure->SX1276LR->RegPktRssiValue );
    
        if( loraConfigure->LoRaSettings.RFFrequency < 860000000 )  // LF
        {    
            if( loraConfigure->RxPacketSnrEstimate < 0 )
            {
                loraConfigure->RxPacketRssiValue = RSSI_OFFSET_LF + ( ( double )loraConfigure->SX1276LR->RegPktRssiValue ) + loraConfigure->RxPacketSnrEstimate;
            }
            else
            {
                loraConfigure->RxPacketRssiValue =  RSSI_OFFSET_LF + ( 1.0666 * ( ( double )loraConfigure->SX1276LR->RegPktRssiValue ) );
			}
        }
        else                                        // HF
        {    
            if( loraConfigure->RxPacketSnrEstimate < 0 )
            {
                loraConfigure->RxPacketRssiValue = RSSI_OFFSET_HF + ( ( double )loraConfigure->SX1276LR->RegPktRssiValue ) + loraConfigure->RxPacketSnrEstimate;
            }
            else
            {    
                loraConfigure->RxPacketRssiValue = RSSI_OFFSET_HF + ( 1.0666 * ( ( double )loraConfigure->SX1276LR->RegPktRssiValue ) );
            }
        }

        if( loraConfigure->LoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            loraConfigure->SX1276LR->RegFifoAddrPtr = loraConfigure->SX1276LR->RegFifoRxBaseAddr;
			//printf("set REG_LR_FIFOADDRPTR= 0x%02x\r\n", loraConfigure->SX1276LR->RegFifoAddrPtr);
            loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_FIFOADDRPTR, loraConfigure->SX1276LR->RegFifoAddrPtr );   //0x0d

            if( loraConfigure->LoRaSettings.ImplicitHeaderOn == true )
            {
                loraConfigure->RxPacketSize = loraConfigure->SX1276LR->RegPayloadLength;
                loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( 0, loraConfigure->RFBuffer, loraConfigure->SX1276LR->RegPayloadLength );
            }
            else
            {
                loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_NBRXBYTES, &loraConfigure->SX1276LR->RegNbRxBytes );
                loraConfigure->RxPacketSize = loraConfigure->SX1276LR->RegNbRxBytes;
                loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( 0, loraConfigure->RFBuffer, loraConfigure->SX1276LR->RegNbRxBytes );
            }
        }
        else // Rx continuous mode
        {
            loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_FIFORXCURRENTADDR, &loraConfigure->SX1276LR->RegFifoRxCurrentAddr );
			//printf("get REG_LR_FIFORXCURRENTADDR= 0x%02x\r\n", loraConfigure->SX1276LR->RegFifoRxCurrentAddr );
			
            if( loraConfigure->LoRaSettings.ImplicitHeaderOn == true )
            {
                loraConfigure->RxPacketSize = loraConfigure->SX1276LR->RegPayloadLength;
                loraConfigure->SX1276LR->RegFifoAddrPtr = loraConfigure->SX1276LR->RegFifoRxCurrentAddr;
                loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_FIFOADDRPTR, loraConfigure->SX1276LR->RegFifoAddrPtr );
                loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( 0, loraConfigure->RFBuffer, loraConfigure->SX1276LR->RegPayloadLength );
            }
            else
            {
                loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_NBRXBYTES, &loraConfigure->SX1276LR->RegNbRxBytes );
                loraConfigure->RxPacketSize = loraConfigure->SX1276LR->RegNbRxBytes;
                loraConfigure->SX1276LR->RegFifoAddrPtr = loraConfigure->SX1276LR->RegFifoRxCurrentAddr;
                loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_FIFOADDRPTR, loraConfigure->SX1276LR->RegFifoAddrPtr );
                loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( 0, loraConfigure->RFBuffer, loraConfigure->SX1276LR->RegNbRxBytes );
            }
        }
        
        if( loraConfigure->LoRaSettings.RxSingleOn == true ) // Rx single mode
        {
            loraConfigure->RFLRState = RFLR_STATE_RX_INIT;
        }
        else // Rx continuous mode
        {
            loraConfigure->RFLRState = RFLR_STATE_RX_RUNNING;
        }
        result = RF_RX_DONE;
        break;
    case RFLR_STATE_RX_TIMEOUT:
        loraConfigure->RFLRState = RFLR_STATE_RX_INIT;
        result = RF_RX_TIMEOUT;
        break;
    case RFLR_STATE_TX_INIT:

        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_STANDBY );

        if( loraConfigure->LoRaSettings.FreqHopOn == true )
        {
            loraConfigure->SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        //RFLR_IRQFLAGS_TXDONE |
                                        RFLR_IRQFLAGS_CADDONE |
                                        //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                        RFLR_IRQFLAGS_CADDETECTED;
            loraConfigure->SX1276LR->RegHopPeriod = loraConfigure->LoRaSettings.HopPeriod;

            loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_HOPCHANNEL, &loraConfigure->SX1276LR->RegHopChannel );
            //SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->HoppingFrequencies[loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
			/**************************************** RANDOM Hopping *******************************************/
            SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->random_getRandomFreq(loraConfigure->HoppingFrequencieSeed, loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK) );
			/**************************************** RANDOM Hopping *******************************************/
        }
        else
        {
            loraConfigure->SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                        RFLR_IRQFLAGS_RXDONE |
                                        RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                        RFLR_IRQFLAGS_VALIDHEADER |
                                        //RFLR_IRQFLAGS_TXDONE |
                                        RFLR_IRQFLAGS_CADDONE |
                                        RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                        RFLR_IRQFLAGS_CADDETECTED;
            loraConfigure->SX1276LR->RegHopPeriod = 0;
        }
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_HOPPERIOD, loraConfigure->SX1276LR->RegHopPeriod );
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGSMASK, loraConfigure->SX1276LR->RegIrqFlagsMask );

        // Initializes the payload size
        loraConfigure->SX1276LR->RegPayloadLength = loraConfigure->TxPacketSize;
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_PAYLOADLENGTH, loraConfigure->SX1276LR->RegPayloadLength );
        
        loraConfigure->SX1276LR->RegFifoTxBaseAddr = 0x00; // Full buffer used for Tx
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_FIFOTXBASEADDR, loraConfigure->SX1276LR->RegFifoTxBaseAddr );

        loraConfigure->SX1276LR->RegFifoAddrPtr = loraConfigure->SX1276LR->RegFifoTxBaseAddr;
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_FIFOADDRPTR, loraConfigure->SX1276LR->RegFifoAddrPtr );
        
        // Write payload buffer to LORA modem
        loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( 0, loraConfigure->RFBuffer, loraConfigure->SX1276LR->RegPayloadLength );
                                        // TxDone               RxTimeout                   FhssChangeChannel          ValidHeader         
        loraConfigure->SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_01;
                                        // PllLock              Mode Ready
        loraConfigure->SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_01 | RFLR_DIOMAPPING2_DIO5_00;
		
		//printf("set RegDioMapping1= 0x%02x RegDioMapping2= 0x%02x\r\n",
		//		loraConfigure->SX1276LR->RegDioMapping1, loraConfigure->SX1276LR->RegDioMapping2);
		
        loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( REG_LR_DIOMAPPING1, &loraConfigure->SX1276LR->RegDioMapping1, 2 );

        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_TRANSMITTER );

        loraConfigure->RFLRState = RFLR_STATE_TX_RUNNING;
        break;
    case RFLR_STATE_TX_RUNNING:
        //if( DIO0 == 1 ) // TxDone
		if(loraConfigure->LoraLowLevelFunc.read_single_reg(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_TXDONE)
        {
            // Clear Irq
            loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE  );
            loraConfigure->RFLRState = RFLR_STATE_TX_DONE;   
        }
        //if( DIO2 == 1 ) // FHSS Changed Channel
		if(loraConfigure->LoraLowLevelFunc.read_single_reg(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL)
        {
            if( loraConfigure->LoRaSettings.FreqHopOn == true )
            {
                loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_HOPCHANNEL, &loraConfigure->SX1276LR->RegHopChannel );
                //SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->HoppingFrequencies[loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK] );
				/**************************************** RANDOM Hopping *******************************************/
				SX1276LoRaSetRFFrequency(loraConfigure, loraConfigure->random_getRandomFreq(loraConfigure->HoppingFrequencieSeed, loraConfigure->SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK) );
				/**************************************** RANDOM Hopping *******************************************/				
            }
            // Clear Irq
            loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
        }
        break;
    case RFLR_STATE_TX_DONE:
        // optimize the power consumption by switching off the transmitter as soon as the packet has been sent
        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_STANDBY );

        loraConfigure->RFLRState = RFLR_STATE_IDLE;
        result = RF_TX_DONE;
        break;
    case RFLR_STATE_CAD_INIT:    
        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_STANDBY );
    
        loraConfigure->SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                    RFLR_IRQFLAGS_RXDONE |
                                    RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                    RFLR_IRQFLAGS_VALIDHEADER |
                                    RFLR_IRQFLAGS_TXDONE |
                                    //RFLR_IRQFLAGS_CADDONE |
                                    RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL; // |
                                    //RFLR_IRQFLAGS_CADDETECTED;
        loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGSMASK, loraConfigure->SX1276LR->RegIrqFlagsMask );
           
                                    // RxDone                   RxTimeout                   FhssChangeChannel           CadDone
        loraConfigure->SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                    // CAD Detected              ModeReady
        loraConfigure->SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
        loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( REG_LR_DIOMAPPING1, &loraConfigure->SX1276LR->RegDioMapping1, 2 );
            
        SX1276LoRaSetOpMode( loraConfigure, RFLR_OPMODE_CAD );
        loraConfigure->RFLRState = RFLR_STATE_CAD_RUNNING;
        break;
    case RFLR_STATE_CAD_RUNNING:
        //if( DIO3 == 1 ) //CAD Done interrupt
		if(loraConfigure->LoraLowLevelFunc.read_single_reg(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_CADDONE)
        { 
            // Clear Irq
            loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE  );
            //if( DIO4 == 1 ) // CAD Detected interrupt
			if(loraConfigure->LoraLowLevelFunc.read_single_reg(REG_LR_IRQFLAGS) & RFLR_IRQFLAGS_CADDETECTED)
            {
				printf("CAD Detected\r\n");
                // Clear Irq
                loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED  );
                // CAD detected, we have a LoRa preamble
                loraConfigure->RFLRState = RFLR_STATE_RX_INIT;
                result = RF_CHANNEL_ACTIVITY_DETECTED;
            } 
            else
            {    
                // The device goes in Standby Mode automatically    
                loraConfigure->RFLRState = RFLR_STATE_IDLE;
                result = RF_CHANNEL_EMPTY;
            }
        }   
        break;
    
    default:
        break;
    } 
    return result;
}

