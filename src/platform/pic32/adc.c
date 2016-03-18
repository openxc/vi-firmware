#include <plib.h>
#include <stdint.h>


#define CONFIG1     ADC_MODULE_ON           |   \
                    ADC_IDLE_CONTINUE       |   \
                    ADC_FORMAT_INTG16       |   \
                    ADC_CLK_AUTO            |   \
                    ADC_AUTO_SAMPLING_ON    |   \
                    ADC_SAMP_ON

#define CONFIG2     ADC_VREF_AVDD_AVSS      |   \
                    ADC_OFFSET_CAL_DISABLE  |   \
                    ADC_SCAN_OFF            |   \
                    ADC_SAMPLES_PER_INT_1   |   \
                    ADC_ALT_BUF_OFF         |   \
                    ADC_ALT_INPUT_OFF

#define CONFIG3     ADC_SAMPLE_TIME_31      |   \
                    ADC_CONV_CLK_SYSTEM     |   \
                    ADC_CONV_CLK_32Tcy

#define ADCR1VAL       220000.0
#define ADCR2VAL         15000.0
#define ADCVRATIO      1.0*ADCR2VAL/(ADCR2VAL + ADCR1VAL)
#define ADCVINFACTOR  (1.0/ADCVRATIO)        
#define ADCCMAX       1024.0 //10 bit adc         
#define ADCVMAC          3.3    
#define ADCVRES          ADCVMAC/ADCCMAX    

uint16_t adc_get_pval(void){
    
    uint16_t adc;
    // ensure that ADC is disabled
    CloseADC10();
    //Read ADC and set the correct mode
    OpenADC10(CONFIG1, CONFIG2, CONFIG3, ENABLE_AN3_ANA, SKIP_SCAN_ALL);
    SetChanADC10(ADC_CH0_POS_SAMPLEA_AN3 | ADC_CH0_NEG_SAMPLEA_NVREF);
    EnableADC10();
    AcquireADC10();
    while(!BusyADC10());
    ConvertADC10();
    adc = ReadADC10(0);
    CloseADC10();
    return adc;
}