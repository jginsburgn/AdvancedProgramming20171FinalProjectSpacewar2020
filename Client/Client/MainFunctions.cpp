// © Alonso Iturbe, Arthur Alves and Jonathan Ginsburg 201705110220

#include "MainFunctions.hpp"

// makes sure the sprite is not drawn outside the active window
void keepWithinBounds(MovingSprite * sprite){
	if (sprite->xcoord > MAX_X - sprite->sprite->getGlobalBounds().width) {
		sprite->xcoord = MAX_X - sprite->sprite->getGlobalBounds().width;
	} else if (sprite->xcoord < MIN_X) {
		sprite->xcoord = MIN_X;
	} else if (sprite->ycoord > MAX_Y - sprite->sprite->getGlobalBounds().height) {
		sprite->ycoord = MAX_Y - sprite->sprite->getGlobalBounds().height;
	} else if (sprite->ycoord < MIN_Y) {
		sprite->ycoord = MIN_Y;
	}
}

// makes sure the sprite is not drawn outside the active window
bool laserWithinBounds(MovingSprite * sprite){
	if (sprite->laserY < MIN_Y - sprite->laser->getGlobalBounds().height || sprite->laserY > MAX_Y) {
		return false;
	} else {
		return true;
	}
}

// checks the WASD keys/tab for player input
void checkWASDInput(MovingSprite * sprite){
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)){
		sprite->ycoord -= PLAYER_SPEED;
	} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
		sprite->ycoord += PLAYER_SPEED;
	} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)){
		sprite->xcoord += PLAYER_SPEED;
	} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
		sprite->xcoord -= PLAYER_SPEED;
	}
	
	//check for laser shot (as long as there's not one already being fired)
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tab) && !sprite->laserActive) {
		sprite->shoot(sprite->xcoord+(sprite->sprite->getGlobalBounds().width)*2/5, sprite->ycoord);
	}
	
	//keep within boundaries
	keepWithinBounds(sprite);
}

// checks the arrow keys/space for player input
void checkArrowInput(MovingSprite * sprite){
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
		sprite->ycoord -= PLAYER_SPEED;
	} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
		sprite->ycoord += PLAYER_SPEED;
	} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
		sprite->xcoord += PLAYER_SPEED;
	} else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
		sprite->xcoord -= PLAYER_SPEED;
	}
	
	//check for laser shot (as long as there's not one already being fired)
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !sprite->laserActive) {
		sprite->shoot(sprite->xcoord+(sprite->sprite->getGlobalBounds().width)*2/5, sprite->ycoord);
	}
	
	//keep within boundaries
	keepWithinBounds(sprite);
}

// checks the joystick (xbox 360 controller) for player input. laser fire with B
void checkJoystickInput(MovingSprite * sprite){
	sf::Vector2f speed = sf::Vector2f(0.f,0.f);
	
	// note that the 0s are hardcoded and refer to the joystick #
	// if more than one joystick is connected to the same computer, this will have to become dynamic
	speed = sf::Vector2f(sf::Joystick::getAxisPosition(0, sf::Joystick::X), sf::Joystick::getAxisPosition(0, sf::Joystick::Y));
	
	//only move if input wasn't part of the control's ghost input
	if (speed.x > 15.f || speed.x < -15.f || speed.y > 15.f || speed.y < -15.f){
		
		// multiply times .2 because otherwise the ∆ is huge
		sprite->ycoord = sprite->ycoord + speed.y * 0.20;
		sprite->xcoord = sprite->xcoord + speed.x * 0.20;
	}
	
	//keep within boundaries
	keepWithinBounds(sprite);
	
	// check for laser shot (as long as there's not one already being fired)
	if (sf::Joystick::isButtonPressed(0, BUTTON_B) && !sprite->laserActive) {
		sprite->shoot(sprite->xcoord+(sprite->sprite->getGlobalBounds().width)*2/5, sprite->ycoord);
	}
	
}

// checks for a collision between two STANDARD sprites
bool collision(sf::Sprite * player, sf::Sprite * enemy){
	return player->getGlobalBounds().intersects(enemy->getGlobalBounds());
}

//updates the laser position for a player
void updateLaser(MovingSprite * player, bool goingUp){
	if (goingUp) { //laser is spaceship's
		if (laserWithinBounds(player)) { //check boundaries
			player->laserY -= LASER_SPEED;
			player->laser->setPosition(player->laserX, player->laserY);
		} else {
			player->laserActive = false; //turn off laser since we're out of bounds
		}
	} else { //laser is alien's
		if (laserWithinBounds(player)) { //check boundaries
			player->laserY += LASER_SPEED;
			player->laser->setPosition(player->laserX, player->laserY);
		} else {
			player->laserActive = false; //turn off laser since we're out of bounds
		}
	}
}

// set up the window, music and icon
int initialSetup(sf::RenderWindow * window, sf::Music * music, sf::Image * icon, sf::Font * font, sf::Text * scoreboard){
	//set framerate limit
	window->setFramerateLimit(30);
	
	// Set the Icon
	if (!icon->loadFromFile(RESOURCE_DIRECTORY + "icon.png")) {
		return EXIT_FAILURE;
	}
	window->setIcon(icon->getSize().x, icon->getSize().y, icon->getPixelsPtr());
	
	//music
	if (!music->openFromFile(RESOURCE_DIRECTORY + "8bitSpace.wav")) {
		return EXIT_FAILURE;
	}
	
	// Play music
	music->setLoop(true);
	music->play();
	
	// set up font
	if (!font->loadFromFile(RESOURCE_DIRECTORY + "Battlestar.TTF")){
		return EXIT_FAILURE;
	}
	
	scoreboard->setString("P1 0 - 0 P2");
	scoreboard->setFont(*font);
	scoreboard->setPosition(500, 500);
	
}

// draws a player and his laser's explosion, if it has collided
void drawPlayer(MovingSprite * player, MovingSprite * opponent, sf::RenderWindow * window, sf::Sprite * explosion){
	if (player->laserActive) { // if laser has been shot...
		updateLaser(player, player->laserDirection); //...update its position...
		window->draw(*player->laser); // ...& draw it.
		if (collision(player->laser, opponent->sprite)){ // If it collided...
			// Send collision message to server
			std::string msg;
			msg += "H::";
            if (player->socket != NULL) player->socket->send(msg.c_str(), msg.size());

			std::cout << player->playerName << "'s laser hit " << opponent->playerName << "!" << std::endl;
			explosion->setPosition(player->laserX-EXPLOSION_OFFSET, player->laserY);
			window->draw(*explosion); // ...draw explosion...


			player->laserActive = false; // ...and turn off laser.
		}
	}
	window->draw(*player->sprite); // draw actual sprite
}

// draws the scoreboard
void drawScoreboard(MovingSprite * player1, MovingSprite * player2, sf::Text * scoreboard, sf::RenderWindow * window){
	std::string sb = player1->playerName + " " + std::to_string(player1->points) + " - " + std::to_string(player2->points) + " " + player2->playerName;
	scoreboard->setString(sb);
	window->draw(*scoreboard);
}
