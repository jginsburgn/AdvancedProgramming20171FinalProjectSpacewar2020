// © Alonso Iturbe, Arthur Alves and Jonathan Ginsburg 201705110220

#ifndef MovingSprite_hpp
#define MovingSprite_hpp

#include <iostream>
#include <string>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#define RESOURCE_DIRECTORY (std::string)"./Resources/"

class MovingSprite {
public:
	
	std::string playerName;
	sf::TcpSocket* socket;
	
	//variables
	sf::Sprite * sprite;
	sf::Texture texture;
	int xcoord;
	int ycoord;
	
	sf::Sprite * laser;
	sf::Texture laserTexture;
	bool laserActive;
	int laserX;
	int laserY;
	bool laserDirection; //1 for up, 0 for down
	
	int hits;
	int points;
	
	//bool movingRight;
	
	//functions
	MovingSprite(std::string texturepath, float scale, std::string laserpath, float laserscale, std::string name, sf::TcpSocket *);
	
	void shoot(int current_xcoord, int current_ycoord);
};


#endif /* MovingSprite_hpp */
