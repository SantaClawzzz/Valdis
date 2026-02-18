#ifndef CONSTANTS_H
#define CONSTANTS_H

// Serial logging switch for all debug prints.
// true  -> enable Serial prints
// false -> disable Serial prints
#define SERIAL_PRINT_ENABLED true

// Select one of two robots. Selection only changes allowed controller MAC.
#define VALDIS 1
#define KAUPO 2

//VALI AKTIIVNE ROBOT
#define ACTIVE_ROBOT KAUPO

#if ACTIVE_ROBOT == VALDIS
  #define CONTROLLER_MAC "40:8e:2c:76:9c:f4"
#elif ACTIVE_ROBOT == KAUPO
  #define CONTROLLER_MAC "ac:8e:bd:5c:9d:ce"
#else
  #error "ACTIVE_ROBOT must be VALDIS or KAUPO"
#endif

// Weapon speeds configured from controller buttons.
// X button sets WEAPON_SPEED_X, Y button sets WEAPON_SPEED_Y.
#define WEAPON_SPEED_X 255
#define WEAPON_SPEED_Y 128

// Joystick deadzone used for motor throttle mapping.
#define JOYSTICK_DEADZONE 15

// pin assignments - not including encoders
#define PWMA 15
#define PWMB 10
#define ESC 18
#define AIN1 13
#define AIN2 14
#define BIN1 12
#define BIN2 11
#define MBEA 7
#define MBEB 6
#define MAEA 17
#define MAEB 16

#endif
