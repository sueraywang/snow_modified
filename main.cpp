#include "main.h"

using namespace std;

//Old and new time values for each timestep
double old_time, new_time = glfwGetTime();
bool dirty_buffer = true;
int frame_count = 0,
	bsize = 3*WIN_SIZE*WIN_SIZE;
unsigned char* img_buffer;

//Circle drawing
int circle_draw_state = 0;
Vector2f circle_origin;

//Simulation data
bool simulating = false;
int mode = MODE;
float timestep = TIMESTEP;
vector<Shape*> snow_shapes;
int point_size = POINT_SIZE;
PointCloud* snow = NULL;
PointCloud* snow2 = NULL;
PointCloud* snow3 = NULL;
Grid* grid;

int main(int argc, char** argv){
	srand(time(NULL));
	
	
	//Create GLFW window
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	window = glfwCreateWindow(WIN_SIZE, WIN_SIZEY, "Snow Simulator", NULL, NULL);
	if (!window){
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	
	//Center window on screen
	//const GLFWvidmode* monitor = glfwGetVideoMode(glfwGetPrimaryMonitor());
	//glfwSetWindowPos(window, (monitor->width-WIN_SIZE)/2, (monitor->height-WIN_SIZE)/2);
	
	//Setup OpenGL context
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, WIN_SIZE, WIN_SIZEY);
	glOrtho(0, WIN_METERS_X, 0, WIN_METERS_Y, 0, 1);
	
	switch (mode)
	{
	case 1:
		while (!glfwWindowShouldClose(window)){
			if (dirty_buffer){
				redraw();
				dirty_buffer = false;
			}
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		break;
	
	case 2:
		start();
		frame_count = 0;
		while (!glfwWindowShouldClose(window)) {
			update();

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			render();

			glfwSwapBuffers(window);
			glfwPollEvents();
			frame_count++;
		}
		break;
	}

	//Exit
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);

	return 0;
	
}

//Print errors to console
static void error_callback(int error, const char* description){
	printf("\nError: %s",description);
}

//Key listener
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if (action != GLFW_RELEASE)
		return;
    switch (key){
		case GLFW_KEY_F12:
			//Create default simulation loop
			start_simulation();
			break;
		case GLFW_KEY_ESCAPE:
			remove_all_shapes();
			if (snow != NULL) delete snow;
			if (snow2 != NULL) delete snow2;
			if (snow3 != NULL) delete snow3;
			if (simulating)
				simulating = false;
			dirty_buffer = true;
			break;
		case GLFW_KEY_ENTER:
			if (!simulating)
				create_new_shape();
			break;
		case GLFW_KEY_C:
			if (!simulating)
				circle_draw_state = 1;
			break;
		case GLFW_KEY_Q:
			if (!simulating)
				circle_draw_state = 3;
			break;
		case GLFW_KEY_1:
			if (!simulating)
				mode = 1;
			break;
		case GLFW_KEY_2:
			if (!simulating)
				mode = 2;
			break;
	}
}

//Mouse listener
void mouse_callback(GLFWwindow* window, int btn, int action, int mods){
	if (action == GLFW_RELEASE && btn == GLFW_MOUSE_BUTTON_LEFT){
		//Create a snow shape, if none exist
		if (snow_shapes.empty())
			create_new_shape();
		//Add vertex to the shape
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		//Convert screen coordinates to world
		y = (1-y/WIN_SIZE)*WIN_METERS;
		x = x/WIN_SIZE*WIN_METERS;
		//Regular point
		switch (circle_draw_state){
			//Regular point
			case 0:
			{
				snow_shapes.back()->addPoint(x, y);
				break;
			}
			//Circle origin
			case 1:
			{
				circle_origin.setData(x, y);
				circle_draw_state = 2;
				break;
			}
			//Circle radius
			case 2:
			{
				const int segments = 18;
				//circle algorithm: http://slabode.exofire.net/circle_draw.shtml
				float x_dif = circle_origin[0] - x,
					y_dif = circle_origin[1] - y,
					radius = sqrt(x_dif*x_dif + y_dif*y_dif),
					theta = 6.283185307 / (float) segments,
					tan_fac = tan(theta),
					cos_fac = cos(theta),
					x = radius,
					y = 0;
				for (int i=0; i<segments; i++){
					snow_shapes.back()->addPoint(x+circle_origin[0], y+circle_origin[1]);
					float flip_x = -y, flip_y = x;
					x += flip_x*tan_fac;
					y += flip_y*tan_fac;
					x *= cos_fac;
					y *= cos_fac;
				}
				circle_draw_state = 0;
				break;
			}
			case 3:
			{
				circle_origin.setData(x, y);
				const int segments = 18;
				//circle algorithm: http://slabode.exofire.net/circle_draw.shtml
				float radius = 0.1,
					theta = 6.283185307 / (float) segments,
					tan_fac = tan(theta),
					cos_fac = cos(theta),
					x = radius,
					y = 0;
				for (int i=0; i<segments; i++){
					snow_shapes.back()->addPoint(x+circle_origin[0], y+circle_origin[1]);
					float flip_x = -y, flip_y = x;
					x += flip_x*tan_fac;
					y += flip_y*tan_fac;
					x *= cos_fac;
					y *= cos_fac;
				}
				circle_draw_state = 0;
				break;
			}
		}
		dirty_buffer = true;
	}
}

//Creates snow shape for editing
void create_new_shape(){
	snow_shapes.push_back(new Shape());
}

//Removes all snow shapes
void remove_all_shapes(){
	for (int i=0, len=snow_shapes.size(); i<len; i++)
		delete snow_shapes[i];
	snow_shapes.clear();
}

//Simulation
//float TIMESTEP;
void start_simulation(){
	//Multiple snow shapes
	/*
	snow_shapes.push_back(generateSnowball(Vector2f(1.1,1.1), .27));
	snow = PointCloud::createShape(snow_shapes, Vector2f(10, 10));
	//remove_all_shapes();
	snow_shapes.push_back(generateSnowball(Vector2f(1.1,.9), .25));
	PointCloud* snow2 = PointCloud::createShape(snow_shapes, Vector2f(11, -2));
	//remove_all_shapes();
	snow_shapes.push_back(generateSnowball(Vector2f(.87,1.05), .28));
	PointCloud* snow3 = PointCloud::createShape(snow_shapes, Vector2f(-11, 5));
	//snow->merge(*snow2);
	//snow->merge(*snow3);
	*/
	

	//Convert drawn shapes to snow particles
	snow = PointCloud::createShape(snow_shapes, Vector2f(INITIAL_VELOCITY_X, INITIAL_VELOCITY_Y));
	//If there are no shapes, we can't do a simulation
	if (snow == NULL) return;
	
	//Computational grid
	grid = new Grid(Vector2f(0), Vector2f(WIN_METERS, WIN_METERS), Vector2f(128), snow);
	//We need to estimate particle volumes before we start
	grid->initializeMass();	
	grid->initializeVelocities();
	grid->calculateVolumes();
	
	pthread_t sim_thread;
	
	pthread_create(&sim_thread, NULL, simulate, NULL);
}
void *simulate(void *args){
	simulating = true;
	struct timespec delay;
	delay.tv_sec = 0;
	clock_t start = clock(), end;
	std::cout << "Starting simulation..." << endl;
	Vector2f gravity = Vector2f(0, GRAVITY);
	
	float cum_sum = 0;
	int iter = 0;
	while (simulating && ++iter > 0){
		timestep = adaptive_timestep();
		//TIMESTEP = 1e-5;
		cum_sum += timestep;
		
		//Initialize FEM grid
		grid->initializeMass();
		grid->initializeVelocities();
		//Compute grid velocities
		grid->explicitVelocities(gravity);

#if ENABLE_IMPLICIT
		if (IMPLICIT_RATIO > 0) {
			grid->implicitVelocities();
		}
#endif

		//Map back to particles
		grid->updateVelocities();
		//Update particle data
		snow->update();
		
		//Redraw snow
		if (!LIMIT_FPS || cum_sum >= FRAMERATE){
			dirty_buffer = true;
			cum_sum -= FRAMERATE;
		}
		//Realtime visualization (approximate)
	}

	std::cout << "Simulation complete: " << (clock()-start)/(float) CLOCKS_PER_SEC << " seconds\n" << endl;
	simulating = false;
	pthread_exit(NULL);
}
float adaptive_timestep(){
	float max_vel = snow->max_velocity, f;
	if (max_vel > 1e-8){
		//We should really take the min(cellsize) I think, if the grid is not square
		float dt = CFL * grid->cellsize[0]/sqrt(max_vel);
		f = dt > FRAMERATE ? FRAMERATE : dt;
	}
	else f = FRAMERATE;
	return f > MAX_TIMESTEP ? MAX_TIMESTEP : f;
}

void redraw(){
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	if (simulating){
		//Grid nodes
                /*
		glPointSize(1);
		glColor3f(0, .7, 1);
		glBegin(GL_POINTS);
		for (int i=0; i<grid->size[0]; i++){
			for (int j=0; j<grid->size[1]; j++)
				glVertex2fv((grid->origin+grid->cellsize*Vector2f(i, j)).data);
		}
		glEnd();
                 */

		//Snow particles
		if (SUPPORTS_POINT_SMOOTH)
			glEnable(GL_POINT_SMOOTH);
		glPointSize(point_size);
		glBegin(GL_POINTS);
		for (int i=0; i<snow->size; i++){
			Particle& p = snow->particles[i];
			//We can use the particle's density to vary color
			float contrast = 0.6;
			float density = p.density/DENSITY*contrast;
			density += 1-contrast;
			glColor3f(density, density, density);
			glVertex2fv(p.position.data);
		}
		glEnd();
		if (SUPPORTS_POINT_SMOOTH)
			glDisable(GL_POINT_SMOOTH);
	}
	else{
		if (circle_draw_state == 2){
			glPointSize(10);
			glColor3f(1, 0, 0);
			glBegin(GL_POINTS);
				glVertex2fv(circle_origin.data);
			glEnd();
		}
		for (int i=0, l=snow_shapes.size(); i<l; i++)
			snow_shapes[i]->draw();
	}
}

Shape* generateSnowball(Vector2f origin, float radius){
	Shape* snowball = new Shape();
	const int segments = 18;
	//Cool circle algorithm: http://slabode.exofire.net/circle_draw.shtml
	float theta = 6.283185307 / (float) segments,
		tan_fac = tan(theta),
		cos_fac = cos(theta),
		x = radius,
		y = 0;
	for (int i=0; i<segments; i++){
		snowball->addPoint(x+origin[0], y+origin[1]);
		float flip_x = -y, flip_y = x;
		x += flip_x*tan_fac;
		y += flip_y*tan_fac;
		x *= cos_fac;
		y *= cos_fac;
	}
	return snowball;
}

void start() {
	//Scene* scene = Scene::snowman();
	Scene* scene = Scene::snowballSmash();

	snow_simulator = new SnowSimulation(scene);
}

void update() {
	cout << "Frame: " << frame_count << endl;
	snow_simulator->update();
}

void render() {
	// Draw grid
	//snow_simulator->grid->draw();

	// Draw snow
	snow_simulator->snow->draw();
}