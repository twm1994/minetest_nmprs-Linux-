#ifdef _MSC_VER
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(lib, "jthread.lib")
// This would get rid of the console window
//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleep_ms(x) Sleep(x)
#else
#include <unistd.h>
#define sleep_ms(x) usleep(x*1000)
#endif
#include <iostream>
#include <time.h>
#include <jthread/jmutexautolock.h>
using namespace jthread;
#include "common_irrlicht.h"
#include "map.h"
#include "player.h"
#include "npc.h"
#include "main.h"
#include "environment.h"
#include "server.h"
#include "client.h"
#include <string>
//#include "properties.h"

//const char * material_stone = properties::getString("material", "stone",
//		"../data/stone.png").c_str();
//const char * material_grass = properties::getString("material", "grass",
//		"../data/grass.png").c_str();
//const char * material_water = properties::getString("material", "water",
//		"../data/water.png").c_str();
//const char *g_material_filenames[MATERIALS_COUNT] = { material_stone,
//		material_grass, material_water };

const char *g_material_filenames[MATERIALS_COUNT] = { "../data/stone.png",
		"../data/grass.png", "../data/water.png", };

#define FPS_MIN 15
#define FPS_MAX 25

#define VIEWING_RANGE_NODES_MIN MAP_BLOCKSIZE
#define VIEWING_RANGE_NODES_MAX 35

JMutex g_viewing_range_nodes_mutex;
s16 g_viewing_range_nodes = MAP_BLOCKSIZE;

/*
 Random stuff
 */
u16 g_selected_material = 0;

/*
 Debug streams
 - use these to disable or enable outputs of parts of the program
 */

std::ofstream dfile("debug.txt");
//std::ofstream dfilemap("map_debug.txt");
std::ostream dout_con(dfile.rdbuf());
std::ostream dout_server(dfile.rdbuf());
std::ostream dout_client(dfile.rdbuf());
//std::ostream dout_map(dfilemap.rdbuf());
Player *player;
class MyEventReceiver: public IEventReceiver {
public:
	// This is the one method that we have to implement
	virtual bool OnEvent(const SEvent& event) {
		// Remember whether each key is down or up
		if (event.EventType == irr::EET_KEY_INPUT_EVENT) {
			keyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
			if (event.KeyInput.PressedDown) {
				if (event.KeyInput.Key == irr::KEY_KEY_F) {
					if (g_selected_material < USEFUL_MATERIAL_COUNT - 1)
						g_selected_material++;
					else
						g_selected_material = 0;
					std::cout << "Selected material: " << g_selected_material
							<< std::endl;
				}
			}
			if (event.KeyInput.Key == KEY_KEY_W
					|| event.KeyInput.Key == KEY_KEY_A
					|| event.KeyInput.Key == KEY_KEY_S
					|| event.KeyInput.Key == KEY_KEY_D) {

				if ((event.KeyInput.PressedDown) && (!walking)) //this will be done once
						{
					walking = true;
					player->animateMove();
				} else if ((!event.KeyInput.PressedDown) && (walking)) //this will be done on key up
						{
					walking = false;
					player->animateStand();
				}

			}
			if ((event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown)) {
				if (!isPaused) {
					isPaused = true;
				} else {
					isExit = true;
				}
			}
			if ((event.KeyInput.Key == KEY_KEY_Q && isPaused)) {
				isPaused = false;
			}
		}

		if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
			if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
				leftclicked = true;
			}
			if (event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) {
				rightclicked = true;
			}
		}

		return false;
	}

	// This is used to check whether a key is being held down
	virtual bool IsKeyDown(EKEY_CODE keyCode) const {
		return keyIsDown[keyCode];
	}

	MyEventReceiver() {
		for (u32 i = 0; i < KEY_KEY_CODES_COUNT; ++i)
			keyIsDown[i] = false;
		leftclicked = false;
		rightclicked = false;
		isPaused = false;
		isExit = false;
	}

	bool leftclicked;
	bool rightclicked;
	bool isPaused;
	bool isExit;
private:
	// We use this array to store the current state of each key
	bool keyIsDown[KEY_KEY_CODES_COUNT];
	bool walking = false;
	//s32 mouseX;
	//s32 mouseY;
};

void updateViewingRange(f32 frametime) {
#if 1
	static f32 counter = 0;
	if (counter > 0) {
		counter -= frametime;
		return;
	}
	counter = 5.0; //seconds

	g_viewing_range_nodes_mutex.Lock();
	bool changed = false;
	if (frametime > 1.0 / FPS_MIN
			|| g_viewing_range_nodes > VIEWING_RANGE_NODES_MAX) {
		if (g_viewing_range_nodes > VIEWING_RANGE_NODES_MIN) {
			g_viewing_range_nodes -= MAP_BLOCKSIZE / 2;
			changed = true;
		}
	} else if (frametime < 1.0 / FPS_MAX
			|| g_viewing_range_nodes < VIEWING_RANGE_NODES_MIN) {
		if (g_viewing_range_nodes < VIEWING_RANGE_NODES_MAX) {
			g_viewing_range_nodes += MAP_BLOCKSIZE / 2;
			changed = true;
		}
	}
	if (changed) {
		std::cout << "g_viewing_range_nodes = " << g_viewing_range_nodes
				<< std::endl;
	}
	g_viewing_range_nodes_mutex.Unlock();
#endif
}

int main() {
	sockets_init();
	atexit(sockets_cleanup);
	/*
	 Initialization
	 */
	srand(time(0));
	g_viewing_range_nodes_mutex.Init();
	assert(g_viewing_range_nodes_mutex.IsInitialized());
	MyEventReceiver receiver;
	// create device and exit if creation failed
	/*
	 Host selection
	 */
	char connect_name[100];
	// std::cout<<std::endl<<std::endl;
	// std::cout<<"Address to connect to [empty = host a game]: ";
	// std::cin.getline(connect_name, 100);
	bool hosting = false;
	// if(connect_name[0] == 0){
	// 	snprintf(connect_name, 100, "127.0.0.1");
	// 	hosting = true;
	// }
	// std::cout<<"-> "<<connect_name<<std::endl;
	// std::cout<<"Port [empty=30000]: ";
	// char templine[100];
	// std::cin.getline(templine, 100);
	unsigned short port;
	// if(templine[0] == 0)
	// 	port = 30000;
	// else
	// 	port = atoi(templine);
	std::cout << "Address to connect to [empty = host a game]: ";
	snprintf(connect_name, 100, "127.0.0.1");
	std::cout << "-> " << connect_name << std::endl;
	port = 30000;
	hosting = true;
	/*
	 Resolution selection
	 */
	u16 screenW = 800;
	u16 screenH = 600;
	video::E_DRIVER_TYPE driverType;
#ifdef _WIN32
	//driverType = video::EDT_DIRECT3D9; // Doesn't seem to work
	driverType = video::EDT_OPENGL;
#else
	driverType = video::EDT_OPENGL;
#endif
	IrrlichtDevice *device;
	device = createDevice(driverType, core::dimension2d<u32>(screenW, screenH),
			16, false, false, false, &receiver);
	if (device == 0)
		return 1; // could not create selected driver.
	/*
	 Continue initialization
	 */
	video::IVideoDriver* driver = device->getVideoDriver();
	scene::ISceneManager* smgr = device->getSceneManager();
	gui::IGUIEnvironment* guienv = device->getGUIEnvironment();

	// -----Pause menu-----
	video::ITexture* image = driver->getTexture("../data/pause.png");
	u16 imgWidth = 600;
	u16 imgHeight = 600;
	gui::IGUIImage* pauseOverlay = guienv->addImage(image,
			core::position2d<int>(screenW / 2 - imgWidth / 2,
					screenH / 2 - imgHeight / 2));
	pauseOverlay->setVisible(false);
	gui::IGUISkin* skin = guienv->getSkin();
	gui::IGUIFont* font = guienv->getFont("../data/fontlucida.png");
	if (font)
		skin->setFont(font);
	skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255, 255, 255, 255));
	skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(255, 0, 0, 0));
	skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(255, 0, 0, 0));
	const wchar_t *text = L"Loading...";
	core::vector2d<s32> center(screenW / 2, screenH / 2);
	core::dimension2d<u32> textd = font->getDimension(text);
	std::cout << "Text w=" << textd.Width << " h=" << textd.Height << std::endl;
	// Have to add a bit to disable the text from word wrapping
	//core::vector2d<s32> textsize(textd.Width+4, textd.Height);
	core::vector2d<s32> textsize(300, textd.Height);
	core::rect<s32> textrect(center - textsize / 2, center + textsize / 2);
	gui::IGUIStaticText *gui_loadingtext = guienv->addStaticText(text, textrect,
			false, false);
	gui_loadingtext->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_UPPERLEFT);
	driver->beginScene(true, true, video::SColor(255, 255, 255, 255));
	guienv->drawAll();
	driver->endScene();
	video::SMaterial materials[MATERIALS_COUNT];
	for (u16 i = 0; i < MATERIALS_COUNT; i++) {
		materials[i].Lighting = false;
		materials[i].BackfaceCulling = false;
		const char *filename = g_material_filenames[i];
		if (filename != NULL) {
			video::ITexture *t = driver->getTexture(filename);
			if (t == NULL) {
				std::cout << "Texture could not be loaded: \"" << filename
						<< "\"" << std::endl;
				return 1;
			}
			materials[i].setTexture(0, driver->getTexture(filename));
		}
		//materials[i].setFlag(video::EMF_TEXTURE_WRAP, video::ETC_REPEAT);
		materials[i].setFlag(video::EMF_BILINEAR_FILTER, false);
		//materials[i].setFlag(video::EMF_ANISOTROPIC_FILTER, false);
	}
	// Make a scope here for the client so that it gets removed
	// before the irrlicht device
	{
		std::cout << "Creating server and client" << std::endl;
		Server *server = NULL;
		if (hosting) {
			//server inits m_env, m_env inits MasterMap
			//MasterMap inheritates Map, Map inits with camera_position,camera_direction,updater,m_heightmap,m_sector_cache,m_hwrapper,drawoffset
			server = new Server();
			server->start(port);
		}
		Client client(smgr, materials); //this will create local player
		Address connect_address(0, 0, 0, 0, port);
		try {
			connect_address.Resolve(connect_name);
		} catch (ResolveError &e) {
			std::cout << "Couldn't resolve address" << std::endl;
			return 0;
		}
		client.connect(connect_address);
		player = client.getLocalPlayer();
		player->animateStand();
		// Create the camera node
		scene::ICameraSceneNode* camera = smgr->addCameraSceneNode(0, // Camera parent
				v3f(BS * 100, BS * 2, BS * 100), // Look from
				v3f(BS * 100 + 1, BS * 2, BS * 100), // Look to
				-1 // Camera ID
				);
		if (camera == NULL)
			return 1;

		camera->setFOV(FOV_ANGLE);
		// Just so big a value that everything rendered is visible
		camera->setFarValue(BS * 1000);

#define ZOOM_MAX 1.5
#define ZOOM_MIN (-5.0*BS)
#define ZOOM_SPEED (0.02*BS)
#define ROTATE_SPEED 1

		f32 camera_yaw = 0; // "right/left"
		f32 camera_pitch = 0; // "up/down"

		// camera zoom distance control
		f32 zoom_max = ZOOM_MAX;
		f32 zoom_min = ZOOM_MIN;
		f32 zoom_speed = ZOOM_SPEED;
		f32 camera_zoom = 0;

		// camera rotate control
		f64 rotate_speed = ROTATE_SPEED;
		f64 camera_rotate = 0;

		// Random constants
#define WALK_ACCELERATION (4.0 * BS)
#define WALKSPEED_MAX (4.0 * BS)

//#define WALKSPEED_MAX (20.0 * BS)
		f32 walk_acceleration = WALK_ACCELERATION;
		f32 walkspeed_max = WALKSPEED_MAX;

		/*
		 The mouse cursor needs not be visible, so we hide it via the
		 irr::IrrlichtDevice::ICursorControl.
		 */
		device->getCursorControl()->setVisible(false);

		gui_loadingtext->remove();

		gui::IGUIStaticText *guitext = guienv->addStaticText(L"Minetest-c55",
				core::rect<s32>(5, 5, 5 + 300, 5 + textsize.Y), false, false);
		gui::IGUIStaticText *positiontextgui = guienv->addStaticText(
				L"Pointing at",
				core::rect<s32>(5, 5 + textsize.Y, 5 + 300, 5 + 2 * textsize.Y),
				false, false);
		/*
		 Main loop
		 */

		bool first_loop_after_window_activation = true;

		s32 lastFPS = -1;

		// Time is in milliseconds
		u32 lasttime = device->getTimer()->getTime();

		while (device->run()) {
			// game pause check
			if (receiver.isPaused) {
				if (!pauseOverlay->isVisible()) {
					device->getTimer()->stop();
					pauseOverlay->setVisible(true);
					device->getCursorControl()->setVisible(true);
				}
				if (device->isWindowActive()) {
					driver->beginScene(true, true,
							video::SColor(0, 200, 200, 200));

					guienv->drawAll();

					driver->endScene();
				}
				if (receiver.isExit) {
					break;
				}
			} else {
				if (pauseOverlay->isVisible()) {
					pauseOverlay->setVisible(false);
					device->getCursorControl()->setVisible(false);
					device->getTimer()->start();
				}
				/*
				 Time difference calculation
				 */
				u32 time = device->getTimer()->getTime();
				f32 dtime; // in seconds
				if (time > lasttime)
					dtime = (time - lasttime) / 1000.0;
				else
					dtime = 0;
				lasttime = time;

				updateViewingRange(dtime);

				// Collected during the loop and displayed
//				core::list<core::aabbox3d<f32> > hilightboxes;

				v3f zoom_direction = v3f(0, 0, 1);
				zoom_direction.rotateXZBy(camera_yaw);
				/*Camera zoom*/
				if (receiver.IsKeyDown(irr::KEY_UP)) {
					camera_zoom += zoom_speed;
				}

				if (receiver.IsKeyDown(irr::KEY_DOWN)) {
					camera_zoom -= zoom_speed;
				}
				if (camera_zoom < zoom_min) {
					camera_zoom = zoom_min;
				} else if (camera_zoom > zoom_max) {
					camera_zoom = zoom_max;
				}

				/*Camera rotate*/
				if (receiver.IsKeyDown(irr::KEY_LEFT)) {
					camera_rotate -= rotate_speed;
				}

				if (receiver.IsKeyDown(irr::KEY_RIGHT)) {
					camera_rotate += rotate_speed;
				}
				/*
				 Player speed control
				 */

				v3f move_direction = v3f(0, 0, 1);
				move_direction.rotateXZBy(camera_yaw);

				v3f speed = v3f(0, 0, 0);
				if (receiver.IsKeyDown(irr::KEY_KEY_W)) {
					speed += move_direction;
				}
				if (receiver.IsKeyDown(irr::KEY_KEY_S)) {
					speed -= move_direction;
				}
				if (receiver.IsKeyDown(irr::KEY_KEY_A)) {
					speed += move_direction.crossProduct(v3f(0, 1, 0));
				}
				if (receiver.IsKeyDown(irr::KEY_KEY_D)) {
					speed += move_direction.crossProduct(v3f(0, -1, 0));
				}
				if (receiver.IsKeyDown(irr::KEY_SPACE)) {
					if (player->touching_ground) {
						//player_speed.Y = 30*BS;
						//player.speed.Y = 5*BS;
						player->speed.Y = 6.5 * BS;
					}
				}

				// The speed of the player (Y is ignored)
				speed = speed.normalize() * walkspeed_max;

				f32 inc = walk_acceleration * BS * dtime;
				if (player->speed.X < speed.X - inc)
					player->speed.X += inc;
				else if (player->speed.X > speed.X + inc)
					player->speed.X -= inc;
				else if (player->speed.X < speed.X)
					player->speed.X = speed.X;
				else if (player->speed.X > speed.X)
					player->speed.X = speed.X;

				if (player->speed.Z < speed.Z - inc)
					player->speed.Z += inc;
				else if (player->speed.Z > speed.Z + inc)
					player->speed.Z -= inc;
				else if (player->speed.Z < speed.Z)
					player->speed.Z = speed.Z;
				else if (player->speed.Z > speed.Z)
					player->speed.Z = speed.Z;

				/*
				 Process environment
				 */

				{
					//TimeTaker("client.step(dtime)", device);
					client.step(dtime);
				}

				if (server != NULL) {
					//TimeTaker("server->step(dtime)", device);
					server->step(dtime);
				}

				/*
				 Mouse and camera control
				 */

				if (device->isWindowActive()) {
					if (first_loop_after_window_activation) {
						first_loop_after_window_activation = false;
					} else {
						s32 dx = device->getCursorControl()->getPosition().X
								- screenW / 2;
						s32 dy = device->getCursorControl()->getPosition().Y
								- screenH / 2;
						camera_yaw -= dx * 0.2;
						camera_pitch += dy * 0.2;
						if (camera_pitch < -89.9)
							camera_pitch = -89.9; // look up
						if (camera_pitch > 89.9)
							camera_pitch = 89.9; // look down
					}
					device->getCursorControl()->setPosition(screenW / 2,
							screenH / 2);
				} else {
					first_loop_after_window_activation = true;
				}

				v3f camera_direction = v3f(0, 0, 1);
				camera_direction.rotateYZBy(camera_pitch);
				camera_direction.rotateXZBy(camera_yaw);

				v3f p = player->getPosition();

				// Adjust camera if pressing left/right arrow
				zoom_direction.rotateXZBy(camera_rotate);
				camera_direction.rotateXZBy(camera_rotate);
				player->setRotation(v3f(0, -1 * camera_yaw, 0));

				//dout_dummy << "-Local player location at time " << time << ": (" << p.X << "," << p.Y << "," << p.Z << ")" << std::endl;

				// BS*1.7 is the value of PLAYER_HEIGHT in player.cpp
				v3f camera_position = p + v3f(0, BS * 1.65, zoom_max)
						+ zoom_direction * camera_zoom;

				camera->setPosition(camera_position);
				camera->updateAbsolutePosition();
				camera->setTarget(camera_position + camera_direction);
				client.updateCamera(camera_position, camera_direction);

				/*
				 Calculate what block is the crosshair pointing to
				 */

				//u32 t1 = device->getTimer()->getTime();
				f32 d = 4; // max. distance
				core::line3d<f32> shootline(camera_position,
						camera_position + camera_direction * BS * (d + 1));
				bool nodefound = false;
				v3s16 nodepos;
				v3s16 neighbourpos;
				core::aabbox3d<f32> nodefacebox;
				f32 mindistance = BS * 1001;

				v3s16 pos_i = Map::floatToInt(player->getPosition());
				//dout_dummy << "---Map::floatToInt(player->getPosition()): (" << pos_i.X << "," << pos_i.Y << "," << pos_i.Z << ")" << std::endl;
				s16 a = d;
				s16 ystart = pos_i.Y + 0 - (camera_direction.Y < 0 ? a : 1);
				s16 zstart = pos_i.Z - (camera_direction.Z < 0 ? a : 1);
				s16 xstart = pos_i.X - (camera_direction.X < 0 ? a : 1);
				s16 yend = pos_i.Y + 1 + (camera_direction.Y > 0 ? a : 1);
				s16 zend = pos_i.Z + (camera_direction.Z > 0 ? a : 1);
				s16 xend = pos_i.X + (camera_direction.X > 0 ? a : 1);

				for (s16 y = ystart; y <= yend; y++) {
					for (s16 z = zstart; z <= zend; z++) {
						for (s16 x = xstart; x <= xend; x++) {
							//dout_dummy << "---Client::getNode at (" << x << "," << y << "," << z << ")" << std::endl;
							try {
								//if(client.m_env.getMap().getNode(x,y,z).d == MATERIAL_AIR){
								if (client.getNode(v3s16(x, y, z)).d
										== MATERIAL_AIR) {
									continue;
								}
							} catch (InvalidPositionException &e) {
								continue;
							}

							v3s16 np(x, y, z);
							v3f npf = Map::intToFloat(np);

							f32 d = 0.01;

							v3s16 directions[6] = { v3s16(0, 0, 1), // back
							v3s16(0, 1, 0), // top
							v3s16(1, 0, 0), // right
							v3s16(0, 0, -1), v3s16(0, -1, 0), v3s16(-1, 0, 0), };

							for (u16 i = 0; i < 6; i++) {
								//{u16 i=3;
								v3f dir_f = v3f(directions[i].X,
										directions[i].Y, directions[i].Z);
								v3f centerpoint = npf + dir_f * BS / 2;
								f32 distance =
										(centerpoint - camera_position).getLength();

								if (distance < mindistance) {
									core::CMatrix4<f32> m;
									m.buildRotateFromTo(v3f(0, 0, 1), dir_f);

									// This is the back face
									v3f corners[2] = { v3f(BS / 2, BS / 2,
									BS / 2), v3f(-BS / 2, -BS / 2,
									BS / 2 + d) };

									for (u16 j = 0; j < 2; j++) {
										m.rotateVect(corners[j]);
										corners[j] += npf;
									}

									//core::aabbox3d<f32> facebox(corners[0],corners[1]);
									core::aabbox3d<f32> facebox(corners[0]);
									facebox.addInternalPoint(corners[1]);

									if (facebox.intersectsWithLine(shootline)) {
										nodefound = true;
										nodepos = np;
										neighbourpos = np + directions[i];
										mindistance = distance;
										nodefacebox = facebox;
									}
								}
							}
						}
					}
				}

				if (nodefound) {
					static v3s16 nodepos_old(-1, -1, -1);
					if (nodepos != nodepos_old) {
//						std::cout << "Pointing at (" << nodepos.X << ","
//								<< nodepos.Y << "," << nodepos.Z << ")"
//								<< std::endl;
						wchar_t positiontext[30];
						swprintf(positiontext, 30, L"Pointing at (%i,%i,%i)",
								nodepos.X, nodepos.Y, nodepos.Z);
						positiontextgui->setText(positiontext);
						nodepos_old = nodepos;
					}
//					hilightboxes.push_back(Map::getNodeBox(nodepos));
					if (receiver.leftclicked) {
						std::cout << "Removing block (MapNode)" << std::endl;
						u32 time1 = device->getTimer()->getRealTime();

						//client.m_env.getMap().removeNodeAndUpdate(nodepos);
						client.removeNode(nodepos);

						u32 time2 = device->getTimer()->getRealTime();
						u32 dtime = time2 - time1;
						std::cout << "Took " << dtime << "ms" << std::endl;
					}
					if (receiver.rightclicked) {
						std::cout << "Placing block (MapNode)" << std::endl;
						u32 time1 = device->getTimer()->getRealTime();

						/*f32 light = client.m_env.getMap().getNode(neighbourpos).light;
						 MapNode n;
						 n.d = g_selected_material;
						 client.m_env.getMap().setNode(neighbourpos, n);
						 client.m_env.getMap().nodeAddedUpdate(neighbourpos, light);*/
						MapNode n;
						n.d = g_selected_material;
						client.addNode(neighbourpos, n);

						u32 time2 = device->getTimer()->getRealTime();
						u32 dtime = time2 - time1;
						std::cout << "Took " << dtime << "ms" << std::endl;
					}
				} else {
					//std::cout<<"nodefound == false"<<std::endl;
					//positiontextgui->setText(L"");
				}

				receiver.leftclicked = false;
				receiver.rightclicked = false;

				/*
				 Update gui stuff
				 */

				static u8 old_selected_material = MATERIAL_AIR;
				if (g_selected_material != old_selected_material) {
					old_selected_material = g_selected_material;
					wchar_t temptext[50];
					swprintf(temptext, 50, L"Minetest-c55 (F: material=%i)",
							g_selected_material);
					guitext->setText(temptext);
				}

				/*
				 Drawing begins
				 */

				/*
				 Background color is choosen based on whether the player is
				 much beyond the initial ground level
				 */
				/*video::SColor bgcolor;
				 v3s16 p0 = Map::floatToInt(player->position);
				 s16 gy = client.m_env.getMap().getGroundHeight(v2s16(p0.X, p0.Z));
				 if(p0.Y > gy - MAP_BLOCKSIZE)
				 bgcolor = video::SColor(255,90,140,200);
				 else
				 bgcolor = video::SColor(255,0,0,0);*/
				video::SColor bgcolor = video::SColor(255, 90, 140, 200);
//				driver->draw3DLine(camera->getAbsolutePosition(),
//						camera->getAbsolutePosition()
//								+ camera_direction * BS * (d + 1),
//						video::SColor(255, 255, 255, 255));
				driver->beginScene(true, true, bgcolor);
				smgr->drawAll();

				core::vector2d<s32> displaycenter(screenW / 2, screenH / 2);
				driver->draw2DLine(displaycenter - core::vector2d<s32>(10, 0),
						displaycenter + core::vector2d<s32>(10, 0),
						video::SColor(255, 255, 255, 255));
				driver->draw2DLine(displaycenter - core::vector2d<s32>(0, 10),
						displaycenter + core::vector2d<s32>(0, 10),
						video::SColor(255, 255, 255, 255));

				video::SMaterial m;
				m.Thickness = 5;
				m.Lighting = false;
				driver->setMaterial(m);

//				for (core::list<core::aabbox3d<f32> >::Iterator i =
//						hilightboxes.begin(); i != hilightboxes.end(); i++) {
//					driver->draw3DBox(*i, video::SColor(255, 0, 0, 0));
//				}

//				driver->draw3DBox(player->getBoundingBox(),
//						video::SColor(255, 0, 0, 0));

				guienv->drawAll();

				driver->endScene();

				/*
				 Drawing ends
				 */

				u16 fps = driver->getFPS();

				if (lastFPS != fps) {
					core::stringw str = L"Minetest [";
					str += driver->getName();
					str += "] FPS:";
					str += fps;
					str += " (";
					str += camera->getPosition().X;
					str += ",";
					str += camera->getPosition().Y;
					str += ",";
					str += camera->getPosition().Z;
					str += ")";

					device->setWindowCaption(str.c_str());
					lastFPS = fps;
				}
				/*}
				 else
				 device->yield();*/
			} // if (receiver.isPaused) {} else
		}
//		//=====Save the map at exit=====
		client.saveMap();
		server->saveMap();
		if (server != NULL)
			delete server;
	} // client is deleted at this point
	/*
	 In the end, delete the Irrlicht device.
	 */
	device->drop();
	return 0;
}

//END
