#ifndef _FOTA_SENSOR_LIGHT_H
#define _FOTA_SENSOR_LIGHT_H

#include "mbed.h"

#include "sensor.h"

namespace fota {
namespace sensor {
namespace light {

/** List of currently supported boards.
 */
enum BOARDS {
    BOARD_GROVE_GL5528 = 0, /*!< Grove GL5528 Light Sensor */
    BOARD_GROVE_LS06, /*!< Grove LS06 Light Sensor */
    BOARD_COUNT /*!< Number of current boards supported */
};

/** Base class for the light sensors.
 */
class Base
{
public:
    /** Initialize the resources associated with the Light base class.
     */
    Base() : _lux(0.0f) { }

    /** Teardown the Base class and clear up any resources associated with it.
     */
    ~Base() { }

    /** Set the current flux value in luxes.
     *
     * @param flux The flux value measuere in luxes.
     */
    virtual void setFlux(float flux) {
        _lux = flux;
    };

    /** Get the current flux value.
     *
     * @return Returns the current flux value in luxes.
     */
    virtual float getFlux(void) {
        return _lux;
    }

protected:
    float _lux;
}; // end light::Base

/** Template class for creating new boards.
 */
template <enum BOARDS B>
class LightSensor
{
}; // end template<> LightSensor

} // end namespace light
} // end namespace sensor
} // end namespace fota
#endif
