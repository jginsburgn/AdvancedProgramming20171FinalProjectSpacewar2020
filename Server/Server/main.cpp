// © Alonso Iturbe, Arthur Alves and Jonathan Ginsburg 201705110220
/*
	Alonso Iturbe Sotomayor, Jonathan Ginsburg, Arthur Alves
	Final Project - examen
	
	** compile with: g++ server.cpp -o spaceserver -lsfml-network -lpthread -g -std=c++11
	** run with: ./server
 */

#include <iostream>
#include <SFML/Network.hpp>
#include <pthread.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include "../../AidFunctions.cpp"

#define PLAYER_COUNT 2
#define BUFFER_SIZE 1024
#define MAX_SCORE 5
#define DEV_MODE 0
#define MAX_Y 700

struct s_msg {
    // shooter id
    int id;
    int x_origin;
    int y_origin;
};

struct m_msg {
    // mover id
    int id;
    int x_pos;
    int y_pos;
};

struct h_msg {
    // hit originator (the one who gets the point)
    int id;
};

pthread_mutex_t mutex_comm = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_message_s = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_message_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_message_h = PTHREAD_MUTEX_INITIALIZER;
std::vector<sf::TcpSocket *> clients;
std::vector<int> clientScores;
bool gameRunning;

std::vector <s_msg> s_messages;
std::vector <m_msg> m_messages;
std::vector <h_msg> h_messages;

void parse_and_add_msgs (char *, int);

// Parse functions
void parse_and_add_S(std::string, int);
void parse_and_add_M(std::string, int);
void parse_and_add_H(std::string, int);

void* incoming_client_communication (void *);
int invertX(int, int);
int invertY(int, int);

int main(int argc, char const *argv[])
{
    sf::TcpListener listener;
    pthread_t thread_tid[PLAYER_COUNT];
    int width, height;
    
    // check the amount of parameters sent to the program
    if (argc != 2) {
        std::cout << "A port number must me provided with:\n\tusage: " << argv[0] << " <port number>" << std::endl;
        return EXIT_FAILURE;
    } else {
        // Create a listener to wait for incoming connections on port 55001
        listener.listen(atoi(argv[1]));
    }
    
    // Wait for a connection
    while (clients.size() < PLAYER_COUNT) {
        sf::TcpSocket* newConnection = new sf::TcpSocket;
        if(listener.accept(*newConnection) == sf::Socket::Done) {
            // If a new connection is received, add the new socket to vector
            clients.push_back(newConnection);
            clientScores.push_back(0);
            std::cout << "Player Joined! Players Online: " << clients.size() << "\n";
        } else {
            delete newConnection;
        }
    }
    
    // Inform each player of their id
    for (int i = 0; i < PLAYER_COUNT; i++) {
        std::string msg = (std::string)"A:" + std::to_string(i) + ":";
        pthread_mutex_lock(&mutex_comm);
        clients[i]->send(msg.c_str(), msg.size());
        if (DEV_MODE) std::cout << "Outgoing communication: " << msg << std::endl;
        pthread_mutex_unlock(&mutex_comm);
    }
    
    gameRunning = true;
    
    // Start communication between players
    for (int i = 0; i < PLAYER_COUNT; i++)  {
        if (pthread_create(&thread_tid[i], NULL, &incoming_client_communication , new int(i)) )
            exit(EXIT_FAILURE);
    }
    
    while (gameRunning) {
        
        // Check for queued messages and forward them to the corresponding clients
        pthread_mutex_lock(&mutex_message_s);
        if (s_messages.size() > 0) {
            int originalSize = s_messages.size();
            for (int i = 0; i < originalSize; i++) {
                // Create message
                std::string msg;
                msg += "S:";
                msg += std::to_string(s_messages.back().id);
                msg += "_";
                msg += std::to_string(s_messages.back().x_origin);
                msg += "_";
                msg += std::to_string(s_messages.back().y_origin);
                msg += ":";
                
                // transmit message to all users except the originator
                for (int j = 0; j < PLAYER_COUNT; j++) {
                    if (j == s_messages.back().id) continue;
                    pthread_mutex_lock(&mutex_comm);
                    clients[j]->send(msg.c_str(), msg.size());
                    if (DEV_MODE) std::cout << "Outgoing communication: " << msg << std::endl;
                    pthread_mutex_unlock(&mutex_comm);
                }
                
                // Remove original message from queue
                s_messages.pop_back();
            }
        }
        pthread_mutex_unlock(&mutex_message_s);
        pthread_mutex_lock(&mutex_message_m);
        if (m_messages.size() > 0) {
            int originalSize = m_messages.size();
            for (int i = 0; i < originalSize; i++) {
                std::string msg;
                msg += "P:";
                msg += std::to_string(m_messages.back().id);
                msg += "_";
                msg += std::to_string(m_messages.back().x_pos);
                msg += "_";
                msg += std::to_string(m_messages.back().y_pos);
                msg += ":";
                
                for (int j = 0; j < PLAYER_COUNT; j++) {
                    if (j == m_messages.back().id) continue;
                    pthread_mutex_lock(&mutex_comm);
                    clients[j]->send(msg.c_str(), msg.size());
                    if (DEV_MODE) std::cout << "Outgoing communication: " << msg << std::endl;
                    pthread_mutex_unlock(&mutex_comm);
                }
                m_messages.pop_back();
            }
        }
        pthread_mutex_unlock(&mutex_message_m);
        pthread_mutex_lock(&mutex_message_h);
        if (h_messages.size() > 0) {
            // Increment score for originator of h_message,
            // check if client has reached the maximum score and decide if end game
            if (++clientScores[h_messages.back().id] >= MAX_SCORE) {
                // send F message to all clients
                std::string msg;
                msg += "F:";
                msg += std::to_string(h_messages.back().id);
                msg += ":";
                
                for (int j = 0; j < PLAYER_COUNT; j++) {
                    pthread_mutex_lock(&mutex_comm);
                    clients[j]->send(msg.c_str(), msg.size());
                    if (DEV_MODE) std::cout << "Outgoing communication: " << msg << std::endl;
                    pthread_mutex_unlock(&mutex_comm);
                }
                
                gameRunning = false;
                continue;
            } else {
                // Create messages of type U and send
                int originalSize = h_messages.size();
                for (int i = 0; i < originalSize; i++) {
                    std::string msg;
                    msg += "U:";
                    for (int j = 0; j < PLAYER_COUNT; j++)
                    {
                        msg += std::to_string(j);
                        msg += "_";
                        msg += std::to_string(clientScores[j]);
                        
                        if (j != PLAYER_COUNT-1) msg += ";";
                    }
                    msg += ":";
                    
                    for (int j = 0; j < PLAYER_COUNT; j++) {
                        pthread_mutex_lock(&mutex_comm);
                        clients[j]->send(msg.c_str(), msg.size());
                        if (DEV_MODE) std::cout << "Outgoing communication: " << msg << std::endl;
                        pthread_mutex_unlock(&mutex_comm);
                    }
                    h_messages.pop_back();
                }
            }
        }
        pthread_mutex_unlock(&mutex_message_h);
    }
    
    // Wait for threads to finish
    for (int i = 0; i < PLAYER_COUNT; i++) {
        if (pthread_join(thread_tid[i], NULL))
            exit(EXIT_FAILURE);
    }
    
    if (!gameRunning) {
        for (int i = 0; i < clients.size(); i++)
            clients[i]->disconnect();
    }
    
    listener.close();
    
    return 0;
}

void * incoming_client_communication (void * arg) {
    int index = *((int *) arg);
    delete (int *) arg;
    char buffer[1024];
    
    // std::cout << index << std::endl;
    
    while (gameRunning) {
        // Receive a message from the client
        std::size_t received = 0;
        clients[index]->receive(buffer, sizeof(buffer), received);
        
        pthread_mutex_lock(&mutex_comm);
        if (DEV_MODE && strlen(buffer) > 5) std::cout << "Incoming communication: " << buffer << std::endl;
        pthread_mutex_unlock(&mutex_comm);
        
        parse_and_add_msgs(buffer, index);
    }
    
    pthread_exit(NULL);
}

// Receive raw
void parse_and_add_msgs (char * message, int id) {
    std::string rawString(message);
    
//    if (id == 0) {
//        std::cout << "Raw message: " << message << std::endl;
//    }
    
    std::memset(message, 0, BUFFER_SIZE);
    
    // split message by ':'
    std::vector<std::string> tokens;
    split(rawString, ':', std::back_inserter(tokens));
    
    // loop and parse each token
    for (int i = 0; i < tokens.size()/2; i++) {
        if (tokens[i*2] == (std::string)"S") {
            parse_and_add_S(tokens[(i*2)+1], id);
        } else if (tokens[i*2] == (std::string)"M") {
            parse_and_add_M(tokens[(i*2)+1], id);
        } else if (tokens[i*2] == (std::string)"H") {
            parse_and_add_H(tokens[(i*2)+1], id);
        }
    }
}

void parse_and_add_S(std::string content, int id) {
    std::vector<std::string> tokens;
    split(content, '_', std::back_inserter(tokens));
    
    s_msg msg;
    msg.id = id;
    msg.x_origin = atoi(tokens[0].c_str());
    msg.y_origin = invertY(atoi(tokens[1].c_str()), MAX_Y);
    
    pthread_mutex_lock(&mutex_message_s);
    s_messages.push_back(msg);
    pthread_mutex_unlock(&mutex_message_s);
}

void parse_and_add_M(std::string content, int id) {
    std::vector<std::string> tokens;
    split(content, '_', std::back_inserter(tokens));
    
    m_msg msg;
    msg.id = id;
    msg.x_pos = atoi(tokens[0].c_str());
    msg.y_pos = invertY(atoi(tokens[1].c_str()), MAX_Y);
    
    pthread_mutex_lock(&mutex_message_m);
    if (id == 0) {
        //std::cout << "Received M [" << id << "] = " << tokens << ", " << msg.y_pos << std::endl;
    }
    m_messages.push_back(msg);
    pthread_mutex_unlock(&mutex_message_m);
}

void parse_and_add_H(std::string content, int id) {
    std::vector<std::string> tokens;
    split(content, '_', std::back_inserter(tokens));
    
    h_msg msg;
    msg.id = id;
    
    pthread_mutex_lock(&mutex_message_h);
    h_messages.push_back(msg);
    pthread_mutex_unlock(&mutex_message_h);
}

// inverts the coordinates for opponent position
int invertX(int xcoord, int width) { return width-xcoord; }
int invertY(int ycoord, int height) { return height-ycoord; }
