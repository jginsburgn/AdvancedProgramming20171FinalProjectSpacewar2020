// © Alonso Iturbe, Arthur Alves and Jonathan Ginsburg 201705110220

#ifndef MainFunctions_hpp
#define MainFunctions_hpp

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unistd.h>
#include <string>
#include "MovingSprite.hpp"

#define BUTTON_A sf::Joystick::X // A button on controller
#define BUTTON_X sf::Joystick::Z // X button on controller
#define BUTTON_B sf::Joystick::Y // B button on controller
#define BUTTON_Y sf::Joystick::R // Y button on controller
#define BUTTON_LB sf::Joystick::U // Left bumper on controller
#define BUTTON_RB sf::Joystick::V // Right bumper on controller

#define AXIS_LEFT_X sf::Joystick::Axis::X //  Left stick x value
#define AXIS_LEFT_Y sf::Joystick::Axis::Y // Left stick y value

#define AXIS_RIGHT_X sf::Joystick::Axis::U // Right stick x value
#define AXIS_RIGHT_Y sf::Joystick::Axis::R // Right stick y value

#define TRIGGERS sf::Joystick::Axis::Z // Positive Z = LT, and Negative Z = RT

#define BUTTON_BACK sf::Joystick::PovX // Back button on controller
#define BUTTON_START sf::Joystick::PovY // Start button on controller

#define DPAD_X sf::Joystick::Axis::PovY // Dpad left and right
#define DPAD_Y sf::Joystick::Axis::PovX // Dpad Up and down

#define ALIEN_SPEED 5
#define PLAYER_SPEED 10
#define LASER_SPEED 30
#define EXPLOSION_OFFSET 60
#define MIN_X 0
#define MIN_Y 0
#define MAX_X 1000
#define MAX_Y 700

// makes sure the sprite is not drawn outside the active window
void keepWithinBounds(MovingSprite * sprite);

// makes sure the sprite is not drawn outside the active window
bool laserWithinBounds(MovingSprite * sprite);

// checks the WASD keys/tab for player input
void checkWASDInput(MovingSprite * sprite);

// checks the arrow keys/space for player input
void checkArrowInput(MovingSprite * sprite);

// checks the joystick (xbox 360 controller) for player input. laser fire with B
void checkJoystickInput(MovingSprite * sprite);

// checks for a collision between two STANDARD sprites
bool collision(sf::Sprite * player, sf::Sprite * enemy);

//updates the laser position for a player
void updateLaser(MovingSprite * player, bool goingUp);

// set up the window, music and icon
int initialSetup(sf::RenderWindow * window, sf::Music * music, sf::Image * icon, sf::Font * font, sf::Text * scoreboard);

// draws a player and his laser's explosion, if it has collided
void drawPlayer(MovingSprite * player, MovingSprite * opponent, sf::RenderWindow * window, sf::Sprite * explosion);

// draws the scoreboard
void drawScoreboard(MovingSprite * player1, MovingSprite * player2, sf::Text * scoreboard, sf::RenderWindow * window);

#endif /* MainFunctions_hpp */
