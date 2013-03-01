#ifndef _LIGHTS_H_
#define _LIGHTS_H_

typedef enum {
    LIGHT_A,
    LIGHT_B,
    LIGHT_C,
} Light;

typedef struct {
    int r;
    int g;
    int b;
} RGB;

typedef struct {
    RGB red;
    RGB green;
    RGB blue;
    RGB orange;
    RGB purple;
    RGB black;
    RGB white;
} Palette;

extern Palette COLORS;

void initializeLights();

/* Public: Display the given color on the light.
 *
 * light - the light to change.
 * color - the RGB color for the light.
 */
void enable(Light light, RGB color);

/* Public: Display the given color on the light, taking the given duration in
 * ms to change from the light's current color to the new one.
 *
 * light - the light to change.
 * color - the RGB color for the light.
 * duration - the amount of time in ms that it should take to change colors
 *      completely.
 */
void enable(Light light, RGB color, int duration);

/* Public: Turn off a light.
 *
 * light - the light to turn off.
 */
void disable(Light light);

/* Public: Dim a light down to off slowly
 *
 * light - the light to turn off.
 * duration - the amount of time in ms to take to turn the light off.
 */
void disable(Light light, int duration);

/* Public: Flash a light with the given color.
 *
 * light - the light to flash.
 * color - the color to flash on the light.
 * duration - the amount of time the light should remain lighted during the flash.
 */
void flash(Light light, RGB color, int duration);

/* Public: Update the color and status of a board's lights based on whatever
 * status or counts may be interesting. This function is intended to be called
 * each time through the main program loop.
 *
 * Each platform can define this function so that you can change the light
 * status based on platform-specific funcionality (e.g. knowing the Bluetooth
 * CONNECT status).
 */
void updateLights();

/* Public: Light updates that happen on every platform. This function is
 * intended to be called each time through the main program loop.
 */
void updateLightsCommon();

#endif // _LIGHTS_H_
