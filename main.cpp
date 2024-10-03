/********************************************************************************
 * @brief Demonstration of GPIO device drivers in C++. 
 ********************************************************************************/
#include "gpio.h"
#include "timer.h"
#include "watchdog.h"
#include "LinReg.h"
#include "serial.h"
#include "adc.h"

using namespace driver;

namespace 
{
/********************************************************************************
 * @brief Constants used in the embedded system.
 *
 * @param tempSensorPin The analog pin to read the temperature sensor.
 * @param Vcc The supply voltage of the temperature sensor.
 ********************************************************************************/

constexpr uint8_t tempSensorPin{2}; // Temperature sensor connected to pin 2.
constexpr double Vcc{5.0};          // Supply voltage of the temperature sensor.

/********************************************************************************
 * @brief Devices used in the embedded system.
 *
 * @param errorLed    LED connected to pin 9, blinking every 100 ms when enabled.
 * @param predictionButton Button connected to pin 13, used to toggle the LED.
 * @param debounceTimer  Timer used to reduced the effect of contact bounces when
 *                pressing the button.
 * @param predictionTimer  Timer used to toggle the LED every 100 ms when enabled.
 ********************************************************************************/
GPIO errorLed{9, GPIO::Direction::Output};
GPIO predictionButton{13, GPIO::Direction::InputPullup};
Timer debounceTimer{Timer::Circuit::debounceTimer, 300};        
Timer predictionTimer{Timer::Circuit::predictionTimer, 60000};  

/********************************************************************************
 * @brief Reads the input voltage from the temperature sensor.
 *
 * @param pin The analog pin to read (A0 - A5 / PORTC0 - PORTC5).
 *
 * @return The input voltage in volts.
 ********************************************************************************/

double inputVoltage(const uint8_t pin) 
{
    return adc::getDutyCycle(pin) * Vcc;
}

/********************************************************************************
 * @brief Callback routine called when predictionButton is pressed or released.
 *        Every time predictionButton is pressed, predictionTimer is toggled, which indirectly
 *        toggles the LED (since predictionTimer is responsible for toggling errorLed).
 *        Pin change interrupts are disabled for 300 ms on the button's I/O port
 *        to reduce the effects of contact bounces.
 ********************************************************************************/
void buttonCallback(void) 
{
     predictionButton.disableInterruptsOnIoPort();
     debounceTimer.start();

     if (predictionButton.read())
     {
         // Prediktera temperaturen.
         // Nollst�ll 60-sekunderstimern.
     }
}

/********************************************************************************
 * @brief Enables pin change interrupts on the button's I/O port 300 ms after
 *        press or release to reduce the effects of contact bounces.
 ********************************************************************************/
void debounceTimerCallback(void) 
{
    debounceTimer.stop();
    predictionButton.enableInterruptsOnIoPort();
}

/********************************************************************************
 * @brief Predicts the temperature every 60 seconds.
 ********************************************************************************/
void predictionTimerCallback(void) 
{
    // Efter 60 sekunder, prediktera temperaturen.
}

/********************************************************************************
 * @brief Sets callback routines, enabled pin change interrupt on predictionButton and
 *        enables the watchdog timer in system reset mode.
 ********************************************************************************/
inline void setup(void) 
{
    adc::init();

    serial::init();

    // Training data for linear regression.
    const container::Vector<double> trainingInput{0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    const container::Vector<double> treiningOutput{-50, -40, -30, -20, -10, 0, 10, 20, 30, 40, 50};
        
    ml::LinReg linReg{0.0, 0.0, trainingInput, treiningOutput, 0.1};
    
    // Train the model.
    if (!linReg.train(100))
    {
        errorLed.set();
        serial::printf("Training failed!\n");
        return;
    }
    
    // Predict the temperature. if the prediction is negative, round down, otherwise round up.
    for (const auto& input : trainingInput)
    {
        if(linReg.predict(input) < 0){
            serial::printf("%d ", static_cast<int>(linReg.predict(input) - 0.5));   
        }
        else{
            serial::printf("%d ", static_cast<int>(linReg.predict(input) + 0.5));
        }            
    }
    
    predictionButton.addCallback(buttonCallback);
    debounceTimer.addCallback(debounceTimerCallback);
    predictionTimer.addCallback(predictionTimerCallback);
    predictionButton.enableInterrupt();
    
    watchdog::init(watchdog::Timeout::Timeout1024ms);
    watchdog::enableSystemReset();
}

} // namespace

/********************************************************************************
 * @brief Perform a setup of the system, then running the program as long as
 *        voltage is supplied. The hardware is interrupt controlled, hence the
 *        while loop is almost empty. If the program gets stuck anywhere, the
 *        watchdog timer won't be reset in time and the program will then restart.
 ********************************************************************************/
int main(void)
{
    setup();
    
    while (1) 
    {   
        watchdog::reset();
    }
    return 0;
}

