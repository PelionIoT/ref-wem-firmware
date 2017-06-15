#ifndef _WEM_SENSOR_H
#define _WEM_SENSOR_H

namespace wem {
namespace sensor {

/** Base classes for sensors to inherit from.
 * Defines a set of methods and members which should be implemented.
 */
class Sensor
{
public:
    /** Get the most current value from the sensor.
     */
    virtual void update(void) = 0;
}; // end Sensor

/** AnalogSensor wrapper to the underlying mBedOS AnalogIn class.
 */
class AnalogSensor : Sensor
{
public:
    /** Initialize the AnalogSensor on the PinName defined by pin.
     *
     * @param pin The pin to initialize the AnalogSensor on.
     */
    AnalogSensor(PinName pin) : _src(pin) { }

    /** Teardown the AnalogSensor and clear up any resources associated with it.
     */
    ~AnalogSensor() { }
protected:
    AnalogIn _src;
}; // end AnalogSensor

} // end namespace sensor
} // end namespace wem

#endif
