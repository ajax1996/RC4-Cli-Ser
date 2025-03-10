#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#define MAX_LEN 1024

using namespace std;

const int MAX_CLIENTS = 5;
//Structure to define a client.
struct terminal
{
	int id;
	string name;
	int socket;
	thread th;
};

//Creating vector of clients.
vector<terminal> clients;

int seed = 0;
mutex cout_mtx, clients_mtx;

void set_name(int id, char name[]);
void broadcast_message(string message, string clientMessage, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);

int main()
{
	int server_socket;
	//Creating socket from server.
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket: ");
		exit(-1);
	}
	
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(50201);
	server.sin_addr.s_addr = INADDR_ANY;
	
	//Binding server socket.
	if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
	{
		perror("bind error: ");
		exit(-1);
	}
	
	//Listening on the assigned port.
	if ((listen(server_socket, MAX_CLIENTS)) == -1)
	{
		perror("listen error: ");
		exit(-1);
	}
	cout << "Server running..." << endl;

	struct sockaddr_in client;
	int client_socket;
	unsigned int len = sizeof(sockaddr_in);
	
	//Running and infinite loop to take connections from different clients.
	while (1)
	{
		if ((client_socket = accept(server_socket, NULL, NULL)) < 0)
		{
			perror("accept error: ");
			exit(-1);
		}
		seed++;
		thread t(handle_client, client_socket, seed);
		lock_guard<mutex> guard(clients_mtx);
		clients.push_back({seed, string("newUser"), client_socket, (move(t))});
	}
	
	// Joining different client's threads.
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].th.joinable())
			clients[i].th.join();
	}

	close(server_socket);
	return 0;
}

// Helping function to set the name of client by first finding if the
// the current client structure by id and assigning its name attribute with the provided name.
void set_name(int id, char name[])
{	
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].id == id)
		{
			clients[i].name = string(name);
		}
	}
}

// Function to send message to the specific client with the name provided as the receiver.
void broadcast_message(string message, string clientName, int sender_id)
{
	char temp[MAX_LEN];
	strcpy(temp, message.c_str());
	if (clientName.length() > 0)
	{	
		// Finding the name of the client same as that provided as receiver and sending the input message.
		for (auto &i : clients)
		{
			if (i.name == clientName)
			{
				send(i.socket, temp, sizeof(temp), 0);
				return;
			}
		}
		
	}
	// Else broadcasting to all the clients in the system.
	else
	{

		for (int i = 0; i < clients.size(); i++)
		{

			if (clients[i].id != sender_id)
			{
				send(clients[i].socket, temp, sizeof(temp), 0);
			}
		}
	}
}


void handle_client(int client_socket, int id)
{
	char name[MAX_LEN], str[MAX_LEN], clientName[MAX_LEN];
	
	//Receiving sender's name in the variabe 'name'.
	recv(client_socket, name, sizeof(name), 0);
	set_name(id, name);

	
	string welcome_message = string(name) + string(" has joined");

	while (1)
	{	
		// Receiving receiver's name in the variable 'clientName' and ciphertext from sender 
		// in the variable str. 
		int name_received = recv(client_socket, clientName, sizeof(clientName), 0);
		int bytes_received = recv(client_socket, str, sizeof(str), 0);
		if (bytes_received <= 0)
			return;
			
		// Printing sender's name, receiver's name and the ciphertext sent by sender to receiver.
		cout<<"From "<<name<<" to "<<clientName<<" Cipher-text: ";
		for (int i = 0; i < strlen(str); i++)
       		 {
                  std::cout << std::hex << (int)str[i];
       		 }
       		 cout<<endl;
		if (strcmp(str, "#exit") == 0)
		{
			string message = string(name) + string(" has left");
			end_connection(id);
			return;
		}
		// Sending sender's name to the receiver.
		broadcast_message(string(name), string(clientName), id);
		
		// Sending ciphertext received from sender to the receiver.
		broadcast_message(string(str), string(clientName), id);
	}
}

// Ending connection and detaching the threads of the client.
void end_connection(int id)
{
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].id == id)
		{
			lock_guard<mutex> guard(clients_mtx);
			clients[i].th.detach();
			clients.erase(clients.begin() + i);
			close(clients[i].socket);
			break;
		}
	}
}
