#ifndef _FOTA_SENSOR_GL5528_H
#define _FOTA_SENSOR_GL5528_H

#include "mbed.h"
#include "light.h"
#include "sensor.h"

namespace fota {
namespace sensor {
namespace light {

template<>
class LightSensor<BOARD_GROVE_GL5528> : public Base, AnalogSensor
{
public:
    LightSensor(PinName pin) : AnalogSensor(pin) { }
    ~LightSensor() { }

    /** Get the most current value from the sensor.
     */
    void update(void) {
        setFlux(_src.read());
    }

private:
    /** Since Flux will be obtained from the AnalogSensor we prevent any
     * ability to set the value outside of the class.
     */
    using Base::setFlux;
};

} // end namespace light
} // end namespace sensor
} // end namespace fota

#endif
