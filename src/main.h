#ifndef MAIN_HEADER
#define MAIN_HEADER
#include <jthread/jmutex.h>
using namespace jthread;
#define PI 3.14159
#define FOV_ANGLE (PI/2.5)
// =====For map=====
#define MAP_BOTTOM 0
#define MAP_LENGTH 35
#define MAP_WIDTH 32
#define MAP_HEIGHT 4
#define MAP_BOTTOM 0
#define SERVER_MAP_FILE "server_nodes.json"
#define CLIENT_MAP_FILE "client_nodes.json"
// Change to struct settings or something
extern s16 g_viewing_range_nodes;
extern JMutex g_viewing_range_nodes_mutex;
#include <fstream>
#include <json/json.h>
// Debug streams
extern std::ostream dout_con;
extern std::ostream dout_client;
extern std::ostream dout_server;
//extern std::ostream dout_map_gen;
#endif

