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
 * \file       sx1276-LoRaMisc.c
 * \brief      SX1276 RF chip high level functions driver
 *
 * \remark     Optional support functions.
 *             These functions are defined only to easy the change of the
 *             parameters.
 *             For a final firmware the radio parameters will be known so
 *             there is no need to support all possible parameters.
 *             Removing these functions will greatly reduce the final firmware
 *             size.
 *
 * \version    2.0.0 
 * \date       May 6 2013
 * \author     Gregory Cristian
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */

#include "sx1276-LoRaMisc.h"

/*!
 * SX1276 definitions
 */
#define XTAL_FREQ                                   32000000
#define FREQ_STEP                                   61.03515625


void SX1276LoRaSetRFFrequency(tLora *loraConfigure, uint32_t freq )
{
	//printf("%d\r\n", freq);
	
    loraConfigure->LoRaSettings.RFFrequency = freq;

    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    loraConfigure->SX1276LR->RegFrfMsb = ( uint8_t )( ( freq >> 16 ) & 0xFF );
    loraConfigure->SX1276LR->RegFrfMid = ( uint8_t )( ( freq >> 8 ) & 0xFF );
    loraConfigure->SX1276LR->RegFrfLsb = ( uint8_t )( freq & 0xFF );
	//loraConfigure->SX1276LR->RegFrfMsb = 0x6c;
	//printf("%02x %02x %02x\r\n", loraConfigure->SX1276LR->RegFrfMsb, loraConfigure->SX1276LR->RegFrfMid, loraConfigure->SX1276LR->RegFrfLsb);
    loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( REG_LR_FRFMSB, &loraConfigure->SX1276LR->RegFrfMsb, 3 );
}

uint32_t SX1276LoRaGetRFFrequency( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( REG_LR_FRFMSB, &loraConfigure->SX1276LR->RegFrfMsb, 3 );
    loraConfigure->LoRaSettings.RFFrequency = ( ( uint32_t )loraConfigure->SX1276LR->RegFrfMsb << 16 ) | ( ( uint32_t )loraConfigure->SX1276LR->RegFrfMid << 8 ) | ( ( uint32_t )loraConfigure->SX1276LR->RegFrfLsb );
    loraConfigure->LoRaSettings.RFFrequency = ( uint32_t )( ( double )loraConfigure->LoRaSettings.RFFrequency * ( double )FREQ_STEP );

    return loraConfigure->LoRaSettings.RFFrequency;
}

void SX1276LoRaSetRFPower( tLora *loraConfigure, int8_t power )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PACONFIG, &loraConfigure->SX1276LR->RegPaConfig );
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PADAC, &loraConfigure->SX1276LR->RegPaDac );
    
    if( ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( loraConfigure->SX1276LR->RegPaDac & 0x87 ) == 0x87 )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            loraConfigure->SX1276LR->RegPaConfig = ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            loraConfigure->SX1276LR->RegPaConfig = ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            loraConfigure->SX1276LR->RegPaConfig = ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            loraConfigure->SX1276LR->RegPaConfig = ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        loraConfigure->SX1276LR->RegPaConfig = ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
        loraConfigure->SX1276LR->RegPaConfig = ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_PACONFIG, loraConfigure->SX1276LR->RegPaConfig );
    loraConfigure->LoRaSettings.Power = power;
}

int8_t SX1276LoRaGetRFPower( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PACONFIG, &loraConfigure->SX1276LR->RegPaConfig );
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PADAC, &loraConfigure->SX1276LR->RegPaDac );

    if( ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( loraConfigure->SX1276LR->RegPaDac & 0x07 ) == 0x07 )
        {
            loraConfigure->LoRaSettings.Power = 5 + ( loraConfigure->SX1276LR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
        else
        {
            loraConfigure->LoRaSettings.Power = 2 + ( loraConfigure->SX1276LR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
    }
    else
    {
        loraConfigure->LoRaSettings.Power = -1 + ( loraConfigure->SX1276LR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
    }
    return loraConfigure->LoRaSettings.Power;
}

void SX1276LoRaSetSignalBandwidth( tLora *loraConfigure, uint8_t bw )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG1, &loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->SX1276LR->RegModemConfig1 = ( loraConfigure->SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_BW_MASK ) | ( bw << 4 );
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_MODEMCONFIG1, loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->LoRaSettings.SignalBw = bw;
}

uint8_t SX1276LoRaGetSignalBandwidth( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG1, &loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->LoRaSettings.SignalBw = ( loraConfigure->SX1276LR->RegModemConfig1 & ~RFLR_MODEMCONFIG1_BW_MASK ) >> 4;
    return loraConfigure->LoRaSettings.SignalBw;
}

void SX1276LoRaSetSpreadingFactor( tLora *loraConfigure, uint8_t factor )
{

    if( factor > 12 )
    {
        factor = 12;
    }
    else if( factor < 6 )
    {
        factor = 6;
    }

    if( factor == 6 )
    {
        SX1276LoRaSetNbTrigPeaks(loraConfigure, 5 );
    }
    else
    {
        SX1276LoRaSetNbTrigPeaks(loraConfigure, 3 );
    }

    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG2, &loraConfigure->SX1276LR->RegModemConfig2 );    
    loraConfigure->SX1276LR->RegModemConfig2 = ( loraConfigure->SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_SF_MASK ) | ( factor << 4 );
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_MODEMCONFIG2, loraConfigure->SX1276LR->RegModemConfig2 );    
    loraConfigure->LoRaSettings.SpreadingFactor = factor;
}

uint8_t SX1276LoRaGetSpreadingFactor( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG2, &loraConfigure->SX1276LR->RegModemConfig2 );   
    loraConfigure->LoRaSettings.SpreadingFactor = ( loraConfigure->SX1276LR->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SF_MASK ) >> 4;
    return loraConfigure->LoRaSettings.SpreadingFactor;
}

void SX1276LoRaSetErrorCoding( tLora *loraConfigure, uint8_t value )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG1, &loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->SX1276LR->RegModemConfig1 = ( loraConfigure->SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_CODINGRATE_MASK ) | ( value << 1 );
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_MODEMCONFIG1, loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->LoRaSettings.ErrorCoding = value;
}

uint8_t SX1276LoRaGetErrorCoding( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG1, &loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->LoRaSettings.ErrorCoding = ( loraConfigure->SX1276LR->RegModemConfig1 & ~RFLR_MODEMCONFIG1_CODINGRATE_MASK ) >> 1;
    return loraConfigure->LoRaSettings.ErrorCoding;
}

void SX1276LoRaSetPacketCrcOn( tLora *loraConfigure, bool enable )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG2, &loraConfigure->SX1276LR->RegModemConfig2 );
    loraConfigure->SX1276LR->RegModemConfig2 = ( loraConfigure->SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( enable << 2 );
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_MODEMCONFIG2, loraConfigure->SX1276LR->RegModemConfig2 );
    loraConfigure->LoRaSettings.CrcOn = enable;
}

void SX1276LoRaSetPreambleLength( tLora *loraConfigure, uint16_t value )
{
    loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( REG_LR_PREAMBLEMSB, &loraConfigure->SX1276LR->RegPreambleMsb, 2 );

    loraConfigure->SX1276LR->RegPreambleMsb = ( value >> 8 ) & 0x00FF;
    loraConfigure->SX1276LR->RegPreambleLsb = value & 0xFF;
    loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( REG_LR_PREAMBLEMSB, &loraConfigure->SX1276LR->RegPreambleMsb, 2 );
}

uint16_t SX1276LoRaGetPreambleLength( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( REG_LR_PREAMBLEMSB, &loraConfigure->SX1276LR->RegPreambleMsb, 2 );
    return ( ( loraConfigure->SX1276LR->RegPreambleMsb & 0x00FF ) << 8 ) | loraConfigure->SX1276LR->RegPreambleLsb;
}

bool SX1276LoRaGetPacketCrcOn( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG2, &loraConfigure->SX1276LR->RegModemConfig2 );
    loraConfigure->LoRaSettings.CrcOn = ( loraConfigure->SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_ON ) >> 1;
    return loraConfigure->LoRaSettings.CrcOn;
}

void SX1276LoRaSetImplicitHeaderOn( tLora *loraConfigure, bool enable )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG1, &loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->SX1276LR->RegModemConfig1 = ( loraConfigure->SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( enable );
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_MODEMCONFIG1, loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->LoRaSettings.ImplicitHeaderOn = enable;
}

bool SX1276LoRaGetImplicitHeaderOn( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG1, &loraConfigure->SX1276LR->RegModemConfig1 );
    loraConfigure->LoRaSettings.ImplicitHeaderOn = ( loraConfigure->SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_ON );
    return loraConfigure->LoRaSettings.ImplicitHeaderOn;
}

void SX1276LoRaSetRxSingleOn( tLora *loraConfigure, bool enable )
{
    loraConfigure->LoRaSettings.RxSingleOn = enable;
}

bool SX1276LoRaGetRxSingleOn( tLora *loraConfigure )
{
    return loraConfigure->LoRaSettings.RxSingleOn;
}

void SX1276LoRaSetFreqHopOn( tLora *loraConfigure, bool enable )
{
    loraConfigure->LoRaSettings.FreqHopOn = enable;
}

bool SX1276LoRaGetFreqHopOn( tLora *loraConfigure )
{
    return loraConfigure->LoRaSettings.FreqHopOn;
}

void SX1276LoRaSetHopPeriod( tLora *loraConfigure, uint8_t value )
{
    loraConfigure->SX1276LR->RegHopPeriod = value;
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_HOPPERIOD, loraConfigure->SX1276LR->RegHopPeriod );
    loraConfigure->LoRaSettings.HopPeriod = value;
}

uint8_t SX1276LoRaGetHopPeriod( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_HOPPERIOD, &loraConfigure->SX1276LR->RegHopPeriod );
    loraConfigure->LoRaSettings.HopPeriod = loraConfigure->SX1276LR->RegHopPeriod;
    return loraConfigure->LoRaSettings.HopPeriod;
}

void SX1276LoRaSetTxPacketTimeout( tLora *loraConfigure, uint32_t value )
{
    loraConfigure->LoRaSettings.TxPacketTimeout = value;
}

uint32_t SX1276LoRaGetTxPacketTimeout( tLora *loraConfigure )
{
    return loraConfigure->LoRaSettings.TxPacketTimeout;
}

void SX1276LoRaSetRxPacketTimeout( tLora *loraConfigure, uint32_t value )
{
    loraConfigure->LoRaSettings.RxPacketTimeout = value;
}

uint32_t SX1276LoRaGetRxPacketTimeout( tLora *loraConfigure )
{
    return loraConfigure->LoRaSettings.RxPacketTimeout;
}

void SX1276LoRaSetPayloadLength( tLora *loraConfigure, uint8_t value )
{
    loraConfigure->SX1276LR->RegPayloadLength = value;
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_PAYLOADLENGTH, loraConfigure->SX1276LR->RegPayloadLength );
    loraConfigure->LoRaSettings.PayloadLength = value;
}

uint8_t SX1276LoRaGetPayloadLength( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PAYLOADLENGTH, &loraConfigure->SX1276LR->RegPayloadLength );
    loraConfigure->LoRaSettings.PayloadLength = loraConfigure->SX1276LR->RegPayloadLength;
    return loraConfigure->LoRaSettings.PayloadLength;
}

void SX1276LoRaSetPa20dBm( tLora *loraConfigure, bool enale )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PADAC, &loraConfigure->SX1276LR->RegPaDac );
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PACONFIG, &loraConfigure->SX1276LR->RegPaConfig );

    if( ( loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {    
        if( enale == true )
        {
            loraConfigure->SX1276LR->RegPaDac = 0x87;
        }
    }
    else
    {
        loraConfigure->SX1276LR->RegPaDac = 0x84;
    }
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_PADAC, loraConfigure->SX1276LR->RegPaDac );
}

bool SX1276LoRaGetPa20dBm( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PADAC, &loraConfigure->SX1276LR->RegPaDac );
    
    return ( ( loraConfigure->SX1276LR->RegPaDac & 0x07 ) == 0x07 ) ? true : false;
}

void SX1276LoRaSetPAOutput( tLora *loraConfigure, uint8_t outputPin )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PACONFIG, &loraConfigure->SX1276LR->RegPaConfig );
    loraConfigure->SX1276LR->RegPaConfig = (loraConfigure->SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_MASK ) | outputPin;
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_PACONFIG, loraConfigure->SX1276LR->RegPaConfig );
}

uint8_t SX1276LoRaGetPAOutput( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PACONFIG, &loraConfigure->SX1276LR->RegPaConfig );
    return loraConfigure->SX1276LR->RegPaConfig & ~RFLR_PACONFIG_PASELECT_MASK;
}

void SX1276LoRaSetPaRamp( tLora *loraConfigure, uint8_t value )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PARAMP, &loraConfigure->SX1276LR->RegPaRamp );
    loraConfigure->SX1276LR->RegPaRamp = ( loraConfigure->SX1276LR->RegPaRamp & RFLR_PARAMP_MASK ) | ( value & ~RFLR_PARAMP_MASK );
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_PARAMP, loraConfigure->SX1276LR->RegPaRamp );
}

uint8_t SX1276LoRaGetPaRamp( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_PARAMP, &loraConfigure->SX1276LR->RegPaRamp );
    return loraConfigure->SX1276LR->RegPaRamp & ~RFLR_PARAMP_MASK;
}

void SX1276LoRaSetSymbTimeout( tLora *loraConfigure, uint16_t value )
{
    loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( REG_LR_MODEMCONFIG2, &loraConfigure->SX1276LR->RegModemConfig2, 2 );

    loraConfigure->SX1276LR->RegModemConfig2 = ( loraConfigure->SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( ( value >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK );
    loraConfigure->SX1276LR->RegSymbTimeoutLsb = value & 0xFF;
    loraConfigure->LoraLowLevelFunc.SX1276WriteBuffer( REG_LR_MODEMCONFIG2, &loraConfigure->SX1276LR->RegModemConfig2, 2 );
}

uint16_t SX1276LoRaGetSymbTimeout( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276ReadBuffer( REG_LR_MODEMCONFIG2, &loraConfigure->SX1276LR->RegModemConfig2, 2 );
    return ( ( loraConfigure->SX1276LR->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) << 8 ) | loraConfigure->SX1276LR->RegSymbTimeoutLsb;
}

void SX1276LoRaSetLowDatarateOptimize( tLora *loraConfigure, bool enable )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG3, &loraConfigure->SX1276LR->RegModemConfig3 );
    loraConfigure->SX1276LR->RegModemConfig3 = ( loraConfigure->SX1276LR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( enable << 3 );
    loraConfigure->LoraLowLevelFunc.SX1276Write( REG_LR_MODEMCONFIG3, loraConfigure->SX1276LR->RegModemConfig3 );
}

bool SX1276LoRaGetLowDatarateOptimize( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( REG_LR_MODEMCONFIG3, &loraConfigure->SX1276LR->RegModemConfig3 );
    return ( ( loraConfigure->SX1276LR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_ON ) >> 3 );
}

void SX1276LoRaSetNbTrigPeaks( tLora *loraConfigure, uint8_t value )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( 0x31, &loraConfigure->SX1276LR->RegDetectOptimize );
    loraConfigure->SX1276LR->RegDetectOptimize = ( loraConfigure->SX1276LR->RegDetectOptimize & 0xF8 ) | value;
    loraConfigure->LoraLowLevelFunc.SX1276Write( 0x31, loraConfigure->SX1276LR->RegDetectOptimize );
}

uint8_t SX1276LoRaGetNbTrigPeaks( tLora *loraConfigure )
{
    loraConfigure->LoraLowLevelFunc.SX1276Read( 0x31, &loraConfigure->SX1276LR->RegDetectOptimize );
    return ( loraConfigure->SX1276LR->RegDetectOptimize & 0x07 );
}

