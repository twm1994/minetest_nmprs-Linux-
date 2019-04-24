#ifndef MAIN_HEADER
#define MAIN_HEADER
#include <jmutex.h>
using namespace jthread;
#define PI 3.14159
#define FOV_ANGLE (PI/2.5)
// =====For map=====
#define MAP_BOTTOM 0
// Change to struct settings or something
extern s16 g_viewing_range_nodes;
extern JMutex g_viewing_range_nodes_mutex;
#include <fstream>
// Debug streams
extern std::ostream dout_con;
extern std::ostream dout_client;
extern std::ostream dout_server;
//extern std::ostream dout_map_gen;
#endif

