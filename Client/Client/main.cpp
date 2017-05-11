// © Alonso Iturbe, Arthur Alves and Jonathan Ginsburg 201705110220
/*
	Alonso Iturbe Sotomayor, Jonathan Ginsburg, Arthur Alves
	Final Project - client
 
	** compile with: g++ *.cpp -o spacewar -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network -std=c++11 -g -lpthread
	** run with: ./spacewar
 */

#include "MovingSprite.hpp"
#include "MainFunctions.hpp"
#include <SFML/Network.hpp>
#include <pthread.h>
#include <stdlib.h>
#include <cstring>
#include <signal.h>
#include "../../AidFunctions.cpp"

#define BUFFER_SIZE 1024
#define DEV_MODE 0

// Struct of message of type S: (shot from enemy)
struct s_msg {
    int id;
    // laser beam origin coordinates
    int x_origin;
    int y_origin;
};

// Struct of message of type P: (update of enemy position)
struct p_msg {
    int id;
    // enemy player coordinates
    int x_pos;
    int y_pos;
};

// Struct message of type U: (update of scoreboard)
struct u_msg {
    // key is user id and element is the corresponding score
    std::map<int, int> scores;
};

// Struct message of type A: (id assignment)
struct a_msg {
    int id;
};

// Struct message of type F: (end game event)
struct f_msg {
    // id of winner
    int id;
};

// Create message buffers
std::vector<s_msg> s_messages;
std::vector<p_msg> p_messages;
std::vector<u_msg> u_messages;
std::vector<a_msg> a_messages;
std::vector<f_msg> f_messages;

// Incoming communication thread id
pthread_t ic_thread_id;
sf::TcpSocket socket;

bool gameRunning = true;

pthread_mutex_t mutex_game_running = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_message_s = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_message_p = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_message_u = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_message_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_message_f = PTHREAD_MUTEX_INITIALIZER;

void * incoming_communication (void *);
void parse_and_add_msgs (char *);

// Parse functions
void parse_and_add_S(std::string);
void parse_and_add_P(std::string);
void parse_and_add_U(std::string);
void parse_and_add_A(std::string);
void parse_and_add_F(std::string);

// Boolean alternator
bool should_send_m_message = false;

int main(int argc, char const *argv[]) {
    //Stop for debugger
    //kill(getpid(), SIGSTOP);
    
    int player_id;
    // Create a socket and connect it to 127.0.0.1 on port 55001
    if (argc >= 3)
        socket.connect(argv[1], atoi(argv[2]));
    else {
        std::cout << "Usage:"<< argv[0] <<" <server_ip> <server_port> <input_mode>" << std::endl;
        return 0;
    }
    
    std::cout << "Waiting for other player to connect to server." << std::endl;
    
    if (pthread_create(&ic_thread_id, NULL, &incoming_communication, NULL))
        exit(EXIT_FAILURE);
    
    while (a_messages.size() == 0) {
        // wait
    }
    
    player_id = a_messages.back().id;
    a_messages.pop_back();
    
    std::cout << "Player id from server: " << player_id << std::endl;
    
    // Set up player
    MovingSprite * player = new MovingSprite(RESOURCE_DIRECTORY + "spaceship.png", 0.2, RESOURCE_DIRECTORY + "blue_laser.png", 0.2, "Player", &socket);
    
    // Set up opponent
    MovingSprite * opponent = new MovingSprite(RESOURCE_DIRECTORY + "invader.png", 0.2, RESOURCE_DIRECTORY + "red_laser.png", 0.2, "Opponent", NULL);
    
    // Explosion when needed
    sf::Texture explosionTexture;
    explosionTexture.loadFromFile(RESOURCE_DIRECTORY + "otherExplosion.png");
    sf::Sprite * explosion = new sf::Sprite(explosionTexture);
    explosion->setScale(0.35, 0.35);
    
    // Load background
    sf::Texture texture;
    if (!texture.loadFromFile(RESOURCE_DIRECTORY + "spaceBG.jpg")) {
        return EXIT_FAILURE;
    }
    sf::Sprite background(texture);
    
    std::string window_name = std::to_string(player_id) + "SPACEWAR 2020";
    
    sf::RenderWindow * window = new sf::RenderWindow(sf::VideoMode(MAX_X, MAX_Y), window_name);
    sf::Music * music = new sf::Music();
    sf::Image * icon = new sf::Image();
    sf::Font * font = new sf::Font();
    sf::Text * scoreboard = new sf::Text();
    initialSetup(window, music, icon, font, scoreboard);
    
    // opponent
    opponent->xcoord = 450;
    opponent->ycoord = 50;
    opponent->laserDirection = false;
    
    // player
    player->xcoord = 450;
    player->ycoord = 600;
    player->laserDirection = true;
    
    //main window loop
    while (window->isOpen()) {
        // Check for relevant events
        sf::Event event;
        while (window->pollEvent(event)) {
            // Close window: exit
            if (event.type == sf::Event::Closed) {
                window->close();
            }
            
            // Escape pressed: exit
            if ((event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) || sf::Joystick::isButtonPressed(0, BUTTON_BACK)) { //not sure this works
                window->close();
            }
        }
        
        // check if finished
        if (f_messages.size() > 0) {
            if (f_messages.back().id == player_id)
                std::cout << "Congratulations! You won!" << std::endl;
            else
                std::cout << "You lost." << std::endl;
            window->close();
            pthread_mutex_lock(&mutex_game_running);
            gameRunning = false;
            pthread_mutex_unlock(&mutex_game_running);
            break;
        }
        
        // check for player input
        if (atoi(argv[3]) == 1)
            checkArrowInput(player);
        else if (atoi(argv[3]) == 2)
            checkWASDInput(player);
        else if (atoi(argv[3]) == 3)
            checkJoystickInput(player);
        
        // send coordinates & shoot info
        // std::string message = std::to_string(player->xcoord) + " " + std::to_string(player->ycoord);
        // socket.send(message.c_str(), message.size() + 1);
        
        // Check alternator and send position message
        should_send_m_message = !should_send_m_message;
        if (should_send_m_message) {
            std::string msg;
            msg += "M:";
            msg += std::to_string(player->xcoord) + "_";
            msg += std::to_string(player->ycoord) + ":";
            //std::cout << "Sending M [" << player_id << "] = " << msg << std::endl;
            socket.send(msg.c_str(), msg.size());
        }
        
        // DRAW LOOP
        window->draw(background);
        
        // ALIEN
        // receive enemy positions
        pthread_mutex_lock(&mutex_message_p);
        if (p_messages.size() > 0) {
            opponent->xcoord = p_messages.back().x_pos;
            opponent->ycoord = p_messages.back().y_pos;
            p_messages.clear();
        }
        pthread_mutex_unlock(&mutex_message_p);
        
        // move players accordingly
        opponent->sprite->setPosition(opponent->xcoord, opponent->ycoord);
        player->sprite->setPosition(player->xcoord, player->ycoord);
        
        drawPlayer(opponent, player, window, explosion);
        
        // PLAYER
        drawPlayer(player, opponent, window, explosion);
        
        // SHOTS
        pthread_mutex_lock(&mutex_message_s);
        if (s_messages.size() > 0) {
            for (int i = 0; i < s_messages.size(); i++) {
                s_msg msg = s_messages[i];
                opponent->shoot(msg.x_origin, msg.y_origin);
            }
            s_messages.clear();
        }
        pthread_mutex_unlock(&mutex_message_s);
        
        // SCOREBOARD
        pthread_mutex_lock(&mutex_message_u);
        if (u_messages.size() > 0) {
            std::map<int, int> m = u_messages.back().scores;
            int own_score;
            int oppontent_score;
            for (auto it = m.begin(); it != m.end(); it++) {
                if (it->first == player_id) {
                    own_score = it->second;
                } else {
                    oppontent_score = it->second;
                }
            }
            std::string scorestring = player->playerName + ": ";
            scorestring += std::to_string(own_score);
            scorestring += " - ";
            scorestring += std::to_string(oppontent_score) + " :";
            scorestring += opponent->playerName;
            
            player->points = own_score;
            opponent->points = oppontent_score;
            
            scoreboard->setString(scorestring);
            window->draw(*scoreboard);
            
            u_messages.pop_back();
        }
        else {
            drawScoreboard(opponent, player, scoreboard, window);
        }
        pthread_mutex_unlock(&mutex_message_u);
        
        // DISPLAY
        window->display();
    }
    
    socket.disconnect();
    
    return EXIT_SUCCESS;
}

void * incoming_communication (void * arg) {
    while (gameRunning) {
        char buffer[BUFFER_SIZE];
        std::memset(buffer, 0, BUFFER_SIZE);
        // Receive raw data
        std::size_t received = 0;
        socket.receive(buffer, BUFFER_SIZE - 1, received);
        buffer[received] = 0;
//        if (DEV_MODE)
//            std::cout << buffer << std::endl;
        
        // Parse and fill event buffers
        parse_and_add_msgs(buffer);
    }
    pthread_exit(NULL);
}

// Receive raw
void parse_and_add_msgs (char * message) {
    std::string rawString(message);
    std::memset(message, 0, BUFFER_SIZE);
    
    // split message by ':'
    std::vector<std::string> tokens;
    split(rawString, ':', std::back_inserter(tokens));
    
    // loop and parse each tokens
    for (int i = 0; i < tokens.size()/2; i++) {
        if (tokens[i*2] == (std::string)"S") {
            parse_and_add_S(tokens[(i*2)+1]);
        } else if (tokens[i*2] == (std::string)"P") {
            parse_and_add_P(tokens[(i*2)+1]);
        } else if (tokens[i*2] == (std::string)"U") {
            parse_and_add_U(tokens[(i*2)+1]);
        } else if (tokens[i*2] == (std::string)"A") {
            parse_and_add_A(tokens[(i*2)+1]);
        } else if (tokens[i*2] == (std::string)"F") {
            parse_and_add_F(tokens[(i*2)+1]);
        }
    }
}

void parse_and_add_S(std::string content) {
    std::vector<std::string> tokens;
    split(content, '_', std::back_inserter(tokens));
    
    s_msg msg;
    msg.id = atoi(tokens[0].c_str());
    msg.x_origin = atoi(tokens[1].c_str());
    msg.y_origin = atoi(tokens[2].c_str());
    
    pthread_mutex_lock(&mutex_message_s);
    s_messages.push_back(msg);
    pthread_mutex_unlock(&mutex_message_s);
}

void parse_and_add_P(std::string content) {
    std::vector<std::string> tokens;
    split(content, '_', std::back_inserter(tokens));
    
    p_msg msg;
    msg.id = atoi(tokens[0].c_str());
    msg.x_pos = atoi(tokens[1].c_str());
    msg.y_pos = atoi(tokens[2].c_str());
    
    pthread_mutex_lock(&mutex_message_p);
    p_messages.push_back(msg);
    pthread_mutex_unlock(&mutex_message_p);
}

void parse_and_add_U(std::string content) {
    std::vector<std::string> tokens;
    split(content, ';', std::back_inserter(tokens));
    
    std::map<int, int> new_scores;
    for (int i = 0; i < tokens.size(); i++) {
        std::vector<std::string> sub_tokens;
        split(tokens[i], '_', std::back_inserter(sub_tokens));
        
        new_scores[atoi(sub_tokens[0].c_str())] = atoi(sub_tokens[1].c_str());
    }
    
    u_msg msg;
    msg.scores = new_scores;
    
    pthread_mutex_lock(&mutex_message_u);
    u_messages.push_back(msg);
    pthread_mutex_unlock(&mutex_message_u);
}

void parse_and_add_A(std::string content) {
    std::vector<std::string> tokens;
    split(content, '_', std::back_inserter(tokens));
    
    a_msg msg;
    msg.id = atoi(tokens[0].c_str());
    
    pthread_mutex_lock(&mutex_message_a);
    a_messages.push_back(msg);
    pthread_mutex_unlock(&mutex_message_a);
}

void parse_and_add_F(std::string content) {
    std::vector<std::string> tokens;
    split(content, '_', std::back_inserter(tokens));
    
    f_msg msg;
    msg.id = atoi(tokens[0].c_str());
    
    pthread_mutex_lock(&mutex_message_f);
    f_messages.push_back(msg);
    pthread_mutex_unlock(&mutex_message_f);
}
