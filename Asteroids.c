/*
 * CSCI 3161: Introduction to Computer Graphics and Animation
 * Assignment 1 - Asteroids
 * Connor Foran
 * B00649015
 * Dalhousie University
 * February 2016
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384;
#endif

#define RAD2DEG 180.0/M_PI
#define DEG2RAD M_PI/180.0

#define myTranslate2D(x,y) glTranslated(x, y, 0.0)
#define myScale2D(x,y) glScalef(x, y, 1.0)
#define myRotate2D(angle) glRotatef(RAD2DEG*angle, 0.0, 0.0, 1.0)

#define MAX_PHOTONS		10
#define MAX_ASTEROIDS 	15
#define MAX_VERTICES 	16
#define MAX_PARTICLES	30

typedef struct { //Ship struct (the player)
	double	x, y, phi, dx, dy;
	int anim, active, timer;
} Ship;
typedef struct { //Ship's projectiles struct
	int	active;
	double	x, y, dx, dy;
} Photon;
typedef struct { //Asteroid struct
	int	active, nVertices, size;
	double x, y, phi, dx, dy, dphi;
	double x_coords[MAX_VERTICES];
	double y_coords[MAX_VERTICES];
} Asteroid;
typedef struct { //Particle struct (dust, asteroid fragments)
	double x, y, phi, dx, dy, dphi, fade;
	int active;
} Particle;
typedef struct { //Struct for pieces of the spaceship when it is destroyed
	double x, y, phi, dx, dy, dphi;
	double x1, y1, x2, y2;
	int active;
} Piece;

//Functions
static void start();  //Initialize and begin a level
static void myDisplay(void);  //Display function
static void myTimer(int value);  //Timer callback function
static void	myKey(unsigned char key, int x, int y);  //Keyboard callback function
static void	keyPress(int key, int x, int y);  //Special keyboard callbacks
static void	keyRelease(int key, int x, int y);
static void drawShip(Ship *s);  //Basic draw functions
static void drawPhoton(Photon *p);
static void	drawAsteroid(Asteroid *a);
static void newAsteroid(double size);  //Create an asteroid of the given size if there is room
static void	initAsteroid(Asteroid *a, double x, double y, double size);  //Initialize an asteroid at the given location (called by newAsteroid once a location and slot are chosen)
static void makeParticle(double x, double y);  //Create a particle from the given point
static void drawParticle(Particle *p);  //Draw a particle
static void breakShip();  //Break the ship into pieces (killed)
static void drawShipPieces();  //Draw the pieces of the ship
static int pointAsteroidTest(double x, double y, Asteroid *a);  //Run a point-polygon test with the given asteroid
static double myRandom(double min, double max); //Provided double random function

//Global variables
static int	up=0, down=0, left=0, right=0;	/* state of cursor keys */
static Ship ship; //The ship struct
static Photon photons[MAX_PHOTONS]; //Array of photons
static Asteroid	asteroids[MAX_ASTEROIDS]; //Array of asteroids
static Particle particles[MAX_PARTICLES]; //Array of particles
static Piece pieces[6]; //Array for ship fragments
static double w=800, h=500; //Window size
static int turnspeed = 40, shotspeed = 3, edge = 5; //Turning speed, photon speed, and the thickness of the outside border
static int level = 0, score = 0, lives = 0, win; //Game parameters
static double maxSpeed = 1.8, decel = 74.0/75.0, minvar = 2.5, maxvar = 5.0, theta = 0;
	//max speed, deceleration fraction, minimum/maximum variance of asteroid vertices, and a theta variable for animated text

//Asteroids per level by size, not counting the 'extra' asteroid
static int size1[6] = {1, 3, 1, 2, 6, 3};
static int size2[6] = {2, 2, 3, 3, 0, 3};
static int size3[6] = {0, 0, 1, 3, 4, 5};

int main(int argc, char *argv[]) //Main function
{
	srand((unsigned int) time(NULL)); //Seed the random
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
	glutInitWindowSize(w, h);
	glutCreateWindow("Asteroids!");

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 100*w/h, 0.0, 100, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);

	glutDisplayFunc(myDisplay);
	glutIgnoreKeyRepeat(1);
	glutKeyboardFunc(myKey);
	glutSpecialFunc(keyPress);
	glutSpecialUpFunc(keyRelease);

	glutTimerFunc(33, myTimer, 0);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glColor3f(1.0, 1.0, 1.0);

	glutMainLoop();
	return 0;
}

void start() //Initialize and begin a level
{
	int i;
	//Disable all entities
	for(i=0; i<MAX_PHOTONS; i++)
		photons[i].active = 0;
	for(i=0; i<MAX_ASTEROIDS; i++)
		asteroids[i].active = 0;
	for(i=0; i<MAX_PARTICLES; i++)
		particles[i].active = 0;

	//Initialize the ship
	ship.x = 80;
	ship.y = 50;
	ship.dx = 0;
	ship.dy = 0;
	ship.phi = 0;
	ship.active = 1;

	//Increment the level
	level++;
	if(level > 7) //Game ends after level 7
	{
		win = 1;
		lives = 0;
	}
	else
	{
		lives = 3; //Reset lives and win variable
		win = 0;
		if(level == 7) //Special final level
		{
			for(i=0; i<MAX_ASTEROIDS-3; i++)
				newAsteroid(1);
			newAsteroid(10);
		}
		else //Create the asteroids for the normal levels
		{
			newAsteroid(level); //'Extra' asteroid that keeps getting bigger
			for(i=0; i< size1[level-1]; i++)
				newAsteroid(1);
			for(i=0; i < size2[level-1]; i++)
				newAsteroid(2);
			for(i=0; i < size3[level-1]; i++)
				newAsteroid(3);
		}
	}
}

//---------------Timer Callback Function (movement & collision detection)---------------

void myTimer(int value) //Timer callback function
{
	if(level == 0 || win)
	{
		theta += M_PI*(1.0/20.0); //Animate text
		if(theta >= 2*M_PI)
			theta -= 2*M_PI;
		if(level == 0) //Title screen
		{
			glutPostRedisplay();
			glutTimerFunc(33, myTimer, value);		/* 30 frames per second */
			return;
		}
	}

	int i,j,a,b,hit;
	double x,y;
	double A, V;

	if(ship.active && win == 0) //Check if all of the asteroids have been destroyed
	{
		for(i = 0; i < MAX_ASTEROIDS; i++)
		{
			if(asteroids[i].active)
				break;
			if(i == MAX_ASTEROIDS-1)
				win = 1; //If they have, the player beat the level
		}
	}

	if(level > 7) //Victory confetti
		makeParticle(ship.x, ship.y);

	//Ship Movement
	if(ship.active)
	{
		//Turn the ship
		if(right)
			ship.phi -= M_PI*(1.0/turnspeed);
		if(left)
			ship.phi += M_PI*(1.0/turnspeed);
		if(ship.phi >= 2*M_PI) //keep ship.phi between 0 and 2p
			ship.phi -= 2*M_PI;
		if(up) //Accelerate the ship
		{
			//Increase movement speed
			ship.dx += cos(ship.phi+M_PI/2.0)/4;
			ship.dy += sin(ship.phi+M_PI/2.0)/4;
			V = sqrt(ship.dx*ship.dx+ship.dy*ship.dy); //Calculate velocity
			if(V >= maxSpeed) //If velocity is greater than the speed cap
			{
				//Check if the horizontal or vertical velocities are negative
				if(ship.dx < 0)
				{
					x = -1;
					ship.dx *= -1;
				}
				else
					x = 1;
				if(ship.dy < 0)
				{
					y = -1;
					ship.dy *= -1;
				}
				else
					y = 1;
				A = atan(ship.dy/ship.dx); //Get the angle of the velocity
				ship.dx = cos(A)*maxSpeed*x; //Calculate the x/y velocities at the maxmimum speed
				ship.dy = sin(A)*maxSpeed*y;
			}
		}
		if(down) //Pressing down decelerates the ship
		{
			ship.dx *= 19.0/20.0;
			ship.dy *= 19.0/20.0;
		}

		//Move the ship
		ship.x += ship.dx;
		ship.y += ship.dy;

		//Decelerate
		if(ship.dx != 0)
			ship.dx *= decel;
		if(ship.dy != 0)
			ship.dy *= decel;

		//Set dx and dy to 0 if they get low enough
		if(ship.dx < 0.06 && ship.dx > -0.06)
			ship.dx = 0;
		if(ship.dy < 0.06 && ship.dy > -0.06)
			ship.dy = 0;

		//Place the ship on the opposite side if it goes beyond the border
		if(ship.x > 100*w/h+edge)
			ship.x = -edge;
		else if(ship.x < -edge)
			ship.x = 100*w/h+edge;
		if(ship.y > 100+edge)
			ship.y = -edge;
		else if(ship.y < -edge)
			ship.y = 100+edge;
	}
	else if(lives > 0) //If the ship is destroyed, but the player still has lives
	{
		ship.timer--; //Decrement the respawn timer
		if(ship.timer <= 0) //If the timer reaches 0
		{
			//Respawn the ship
			ship.x = 80;
			ship.y = 50;
			ship.dx = 0;
			ship.dy = 0;
			ship.phi = 0;
			ship.active = 1;
			ship.timer = 50; //Start the invulnerability timer
		}
	}

	//Photon movement
	for(i=0; i<MAX_PHOTONS; i++)
	{
		if(photons[i].active)
		{
			//Move the photon
			photons[i].x += photons[i].dx;
			photons[i].y += photons[i].dy;
		}
		//Remove the photo if it goes beyond the border
		if(photons[i].x > 100*w/h+edge || photons[i].x < -edge || photons[i].y > 100+edge || photons[i].y < -edge)
			photons[i].active = 0;
	}

	//Asteroid movement
    for(i=0; i<MAX_ASTEROIDS; i++)
    {
    	//Move and rotate the asteroid
    	if(asteroids[i].active)
    	{
    		asteroids[i].x += asteroids[i].dx;
    		asteroids[i].y += asteroids[i].dy;
    		asteroids[i].phi += asteroids[i].dphi;
    	}
    	//Place the asteroid on the opposite side if it goes beyond the border, with some extra space based on the size of the asteroid
    	if(asteroids[i].x > 100*w/h+edge*asteroids[i].size)
    		asteroids[i].x = -edge*asteroids[i].size;
    	else if(asteroids[i].x < -edge*asteroids[i].size)
    		asteroids[i].x = 100*w/h+edge*asteroids[i].size;
    	if(asteroids[i].y > 100+edge*asteroids[i].size)
    		asteroids[i].y = -edge*asteroids[i].size;
    	else if(asteroids[i].y < -edge*asteroids[i].size)
    		asteroids[i].y = 100+edge*asteroids[i].size;
    }

    //Collision detection: Asteroids & Photons
    for(i=0; i<MAX_ASTEROIDS; i++) //For each asteroid...
    {
    	hit=0;
    	if(!asteroids[i].active) //(Skip a slot if the asteroid is not active)
    		continue;
    	for(j=0; j<MAX_PHOTONS && !hit; j++) //...check each photon
    	{
    		if(photons[j].active) //(If the photon is active)
    		{
    			//Calculate the distance between the photon and the asteroid's origin point
    			x = sqrt( (asteroids[i].x-photons[j].x)*(asteroids[i].x-photons[j].x) + (asteroids[i].y-photons[j].y)*(asteroids[i].y-photons[j].y) );
    			if(x < maxvar*asteroids[i].size + 1)  //Check if the distance is within the maximum size of the asteroid before doing advanced collision detection
    			{									  // (Basically a tight, circular bounding box)
    				if(pointAsteroidTest(photons[j].x, photons[j].y, &asteroids[i])) //Collision detection: If the point polygon test returns true...
    				{
        				score += 1000*asteroids[i].size; //Increase the score
        				hit = 1; //Signal to stop checking photons for this asteroid
        				photons[j].active = 0; //Deactivate the photon
        				for(a = (rand() % 3) + 5; a > 0; a--) //Spawn dust particles
        					makeParticle(asteroids[i].x, asteroids[i].y);
        				if(asteroids[i].size > 1) //If the asteroid is large, spawn more asteroids in its place
        				{
        					b = 0;
        					for(a = 0; a < MAX_ASTEROIDS && b < 2; a++) //Find a slot to add the new asteroid (two will be created)
        					{
        						if(!asteroids[a].active)
        						{
        							initAsteroid(&asteroids[a], asteroids[i].x, asteroids[i].y, asteroids[i].size-1); //Create the new asteroid
        							b++;
        						}
        					}
        				}
        				asteroids[i].active = 0; //Deactivate the asteroid
    				}
    			}
    		}
    	}
    }

    //Collision detection: Ship & Asteroids
	int arr[6] = {0.0, 6.0, -2.6, -2, 2.6, -2}; //Ship vertex values for easy reference

	if(ship.active && ship.timer <= 0) //If the ship is alive and invulnerability is not active
	{
		hit = 0;
		for(i=0; i<MAX_ASTEROIDS && hit == 0; i++) //Check each asteroid...
		{
			if(asteroids[i].active) //If asteroid is active...
			{
				//Calculate the distance between the asteroid's and the ship's center points
				x = sqrt( (asteroids[i].x-ship.x)*(asteroids[i].x-ship.x) + (asteroids[i].y-ship.y)*(asteroids[i].y-ship.y) );
				if(x < maxvar*asteroids[i].size + 6) //Check if the distance is lower that the maximum size of the asteroid plus the length of the ship from its center
				{
					for(j=0; j<6 && hit == 0; j+=2)
					{
						//Calculate the one of the three primary points on the ship
						x = arr[j]*cos(-ship.phi)+arr[j+1]*sin(-ship.phi)+ship.x;
						y = -arr[j]*sin(-ship.phi)+arr[j+1]*cos(-ship.phi)+ship.y;
						if(pointAsteroidTest(x, y, &asteroids[i])) //Perform collision detection
						{
							//Disable and break the ship if there is a collision, and decrement the lives.
							ship.active = 0;
							hit = 1;
							breakShip();
							lives -= 1;
							if(lives > 0) //If the player still has more lives, start the respawn timer
								ship.timer = 100;
						}
					}
				}
			}
		}
	}
	glutPostRedisplay();
	glutTimerFunc(33, myTimer, value);		/* 30 frames per second */
}

//---------------Point-Polygon Test Function---------------

int pointAsteroidTest(double x, double y, Asteroid *a) //Run a point-polygon test on a point and an asteroid
{
	//Uses the method shown in class
	int i, count = 0;
	double x1,x2,y1,y2,u;
	for(i = 0; i < a->nVertices; i++) //For each edge of the polygon
	{
		x1 = a->x_coords[i]*cos(-a->phi) + a->y_coords[i]*sin(-a->phi)+a->x;
		if(i == a->nVertices - 1) //x-coordinate of the next vertex (if the current vertex is the last one, use the first vertex
			x2 = a->x_coords[0]*cos(-a->phi) + a->y_coords[0]*sin(-a->phi)+a->x;
		else
			x2 = a->x_coords[i+1]*cos(-a->phi) + a->y_coords[i+1]*sin(-a->phi)+a->x;

		if(x1 >= x || x2 >= x) //If the test point is to the left of either vertex
		{
			y1 = -a->x_coords[i]*sin(-a->phi) + a->y_coords[i]*cos(-a->phi)+a->y; //y-coordinate of the current vertex
			if(i == a->nVertices - 1) //y-coordinate of the next vertex (if the current vertex is the last one, use the first vertex
				y2 = -a->x_coords[0]*sin(-a->phi) + a->y_coords[0]*cos(-a->phi)+a->y;
			else
				y2 = -a->x_coords[i+1]*sin(-a->phi) + a->y_coords[i+1]*cos(-a->phi)+a->y;

			if(y1 > y2) //If y1 is higher than y2, swap the edges (y2 is supposed to be higher for the calculation)
			{
				u = y1;
				y1 = y2;
				y2 = u;
				u = x1;
				x1 = x2;
				x2 = u;
			}
			if( y >= y1 && y <= y2) //If the test point is vertically between y1 and y2
			{
				//Compute the x coordinate of the intersection with a ray cast from the test point in the direction of the x-axis
				u = x2*(y-y1)/(y2-y1) + x1*(y2-y)/(y2-y1);
				if(u >= x) //If the intersection is to the right of the test point, count it
					count++;
			}
		}
	}
	if(count % 2 != 0) //iff the number of intersections with the ray is odd, there is a collision
		return 1;
	else
		return 0;
}

//---------------Display Callback Function---------------
//Code for text displays is a bit messy...

void myDisplay()
{
	int i, x;
	double d;
	glClear(GL_COLOR_BUFFER_BIT);
	if(level == 0) //Title screen
	{
		char title[] = "ASTEROIDS!"; //Draw the title
		d = 0.4;
		for(x = 0; x < 4; x++)
		{
		    glColor3f(d, d, d);
			for(i = 0; i < sizeof(title)/sizeof(title[0]); i++)
			{
				glRasterPos2d(55+i*5,80+sin(theta+(i+0.9*x)/M_PI)*2);
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
			}
			d += 0.2;
		}

		char string[] = "Press space to start!"; //Draw the start message
		for(i = 0; i < sizeof(string)/sizeof(string[0]); i++)
		{
			glRasterPos2d(30+i*5,40+sin(theta)*2);
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, string[i]);
		}
	}
	else //In-game
	{
		//Draw particles
	    for (i=0; i<MAX_PARTICLES; i++)
	        if (particles[i].active)
	        	drawParticle(&particles[i]);
	    glColor3f(1.0, 1.0, 1.0); //Reset colour, since particles can fade and change it

	    if(ship.active) //Draw the ship if it is active
	    {
	    	if(ship.timer >= 0) //Makes the ship flash during invulnerability after respawning
	    	{
		    	if(ship.timer % 2 == 0)
		    		drawShip(&ship);
		    	ship.timer--;
	    	}
	    	else
	    		drawShip(&ship);
	    }
	    else
	    	drawShipPieces(); //Draw the pieces of the destroyed ship

	    //Draw photons
		for(i=0; i<MAX_PHOTONS; i++)
			if(photons[i].active)
				drawPhoton(&photons[i]);

		//Draw asteroids
	    for (i=0; i<MAX_ASTEROIDS; i++)
	    	if (asteroids[i].active)
	            drawAsteroid(&asteroids[i]);

	    glLoadIdentity(); //Reset the identity for drawing text

	    //Draw the score in the top left
	    char printScore[10];
	    sprintf(printScore, "%d", score);
		for(i = 0; i < sizeof(printScore)/sizeof(printScore[0]) && printScore[i] != '\0'; i++)
		{
			glColor3f(0.0, 0.0, 0.0);
			glRasterPos2d(2.2+2*i,95.8);
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, printScore[i]);
			glColor3f(1.0, 1.0, 1.0);
			glRasterPos2d(2+2*i,96);
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, printScore[i]);
		}

		if(level < 8)
		{
			//Draw the current level in the bottom right
			//"LEVEL" label
			char stage[] = "LEVEL";
			for(i = 0; i < sizeof(stage)/sizeof(stage[0]); i++)
			{
				glColor3f(0.0, 0.0, 0.0);
				glRasterPos2d(135.2+i*3, 5.3);
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, stage[i]);
				glColor3f(1.0, 1.0, 1.0);
				glRasterPos2d(135+i*3, 5.5);
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, stage[i]);
			}
			//Level number
			glColor3f(0.0, 0.0, 0.0);
			glRasterPos2d(151.2,5.4);
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, level+48);
			glColor3f(1.0, 1.0, 1.0);
			glRasterPos2d(151,5.6);
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, level+48);
		}

		if(lives > 0)
		{
			//Print the lives counter
			//"LIVES" label
			char label[] = "LIVES: ";
			for(i = 0; i < sizeof(label)/sizeof(label[0]); i++)
			{
				glColor3f(0.0, 0.0, 0.0);
				if(i >= 2)
					glRasterPos2d(129.2+i*3, 94.3);
				else
					glRasterPos2d(130.2+i*3, 94.3);
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, label[i]);
				glColor3f(1.0, 1.0, 1.0);
				if(i >= 2)
					glRasterPos2d(129+i*3, 94.5);
				else
					glRasterPos2d(130+i*3, 94.5);
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, label[i]);
			}
			//Extra life icons
			for(i = 0; i < lives; i++)
			{
				glColor3f(0.0, 0.0, 0.0);
				glLoadIdentity();
				myTranslate2D(155, 95);
				glBegin(GL_POLYGON);
				glVertex2f( -i*3, 3.2 );
				glVertex2f( -1.5-i*3, -1.2 );
				glVertex2f( -i*3, -0.5 );
				glVertex2f( 1.5-i*3, -1.2 );
				glEnd();
				glColor3f(1.0, 1.0, 1.0);
				glLoadIdentity();
				myTranslate2D(155, 95);
				glBegin(GL_POLYGON);
				glVertex2f( -i*3, 3 );
				glVertex2f( -1.3-i*3, -1 );
				glVertex2f( -i*3, -0.3 );
				glVertex2f( 1.3-i*3, -1 );
				glEnd();
			}
			glLoadIdentity();

			if(win) //Level clear message
			{
				char won[] = "Level Complete!";
				for(i = 0; i < sizeof(won)/sizeof(won[0]); i++)
				{
					glRasterPos2d(44+i*5,60+sin(theta+i/M_PI)*2);
					glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, won[i]);
				}
				char cont[] = "(Press space to continue)";
				for(i = 0; i < sizeof(cont)/sizeof(cont[0]); i++)
				{
					glColor3f(1.0, 1.0, 1.0);
					glRasterPos2d(43+i*3,40);
					glutBitmapCharacter(GLUT_BITMAP_9_BY_15, cont[i]);
				}
			}
		}
		else //Game over/end message
		{
			char gameover[] = "GAME OVER";
			char victory[] = " YOU WIN ";
			char *title;
			if(level <= 7)
				title = &gameover[0];
			else
				title = &victory[0];
			for(i = 0; i < 9; i++)
			{
				glColor3f(0.0, 0.0, 0.0);
				glRasterPos2d(59.2+i*5,49.8);
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
				glColor3f(1.0, 1.0, 1.0);
				glRasterPos2d(59+i*5,50);
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
			}
			if(level <= 7)
			{
				char restart[] = "(Press R to restart)";
				for(i = 0; i < sizeof(restart)/sizeof(restart[0]); i++)
				{
					glColor3f(0.0, 0.0, 0.0);
					glRasterPos2d(51.2+i*3,39.8);
					glutBitmapCharacter(GLUT_BITMAP_9_BY_15, restart[i]);
					glColor3f(1.0, 1.0, 1.0);
					glRasterPos2d(51+i*3,40);
					glutBitmapCharacter(GLUT_BITMAP_9_BY_15, restart[i]);
				}
			}
			else
			{
				char end[] = "(That's it. That's the end. Goodbye.)";
				for(i = 0; i < sizeof(end)/sizeof(end[0]); i++)
				{
					glRasterPos2d(25+i*3,40);
					glutBitmapCharacter(GLUT_BITMAP_9_BY_15, end[i]);
				}
			}
		}
	}

	glutSwapBuffers();
}

//------------Keyboard callback functions------------

void myKey(unsigned char key, int x, int y) //Normal keyboard callbacks
{
	int i;
    if(key == ' ') //Spacebar
    {
    	if(level == 0 || win) //Start/continue/restart when game is not active
    	{
    		if(level > 7)
    		{
    			level = 0;
    			score = 0;
    		}
    		start();
    	}
    	else if(ship.active) //Fire photon
    	{
			for(i=0; i<MAX_PHOTONS; i++) //Find an empty photon index
			{
				if(!photons[i].active) //Create the photon at an empty slot
				{
					photons[i].active = 1;
					//Spawn at the position of the ship, towards the front
					photons[i].x = ship.x+cos(ship.phi+M_PI/2.0)*2.5;
					photons[i].y = ship.y+sin(ship.phi+M_PI/2.0)*2.5;
					//Move in the direction that the ship is pointing
					photons[i].dx = shotspeed*cos(ship.phi+M_PI/2.0);
					photons[i].dy = shotspeed*sin(ship.phi+M_PI/2.0);
					break;
				}
			}
    	}
    }
    if(key == 'w') //Instant win button
    {
    	win = 1;
    }
    //Spawn asteroids
    if(key == '1')
    	newAsteroid(1);
    if(key == '2')
    	newAsteroid(2);
    if(key == '3')
        newAsteroid(3);
    if(key == 'r' && level > 0 && lives <= 0) //Restart after dying
    {
    	score = 0;
    	level = 0;
    	start();
    }
}
//Provided special keyboard callbacks, for arrow keys
void keyPress(int key, int x, int y)
{
     switch (key)
    {
        case 100:
            left = 1; break;
        case 101:
            up = 1; break;
        case 102:
            right = 1; break;
        case 103:
            down = 1; break;
    }
}
void keyRelease(int key, int x, int y)
{
	switch (key)
    {
        case 100:
            left = 0; break;
        case 101:
            up = 0; break;
        case 102:
            right = 0; break;
        case 103:
            down = 0; break;
    }
}

//------------Draw The Ship------------

void drawShip(Ship *s) //Draw the ship
{
	glLoadIdentity();
	myTranslate2D(s->x, s->y);
	myRotate2D(s->phi);
	myScale2D(2, 2);
	glBegin(GL_LINE_LOOP);
	glVertex2f( 0, 3 );
	glVertex2f( -1.3, -1 );
	glVertex2f( 0, -0.3 );
	glVertex2f( 1.3, -1 );
	glEnd();
	if(up) //Draw the animated thruster
	{
		glBegin(GL_LINE_LOOP);
		glVertex2f( -0.65, -0.65 );
		glVertex2f( 0, -0.3 );
		glVertex2f( 0.65, -0.65 );
		if(s->anim > 3) //Switches every three frames
			glVertex2f( 0, -3 );
		else
			glVertex2f( 0, -2 );
		s->anim++;
		if(s->anim > 6)
			s->anim = 0;
		glEnd();
	}
}

//------------Draw Photons------------

void drawPhoton(Photon *p) //Draw a photon
{
	glLoadIdentity();
	myTranslate2D(p->x, p->y);
	glBegin(GL_POLYGON);
	glVertex2f( 0, 1 );
	glVertex2f( 1, 0 );
	glVertex2f( 0, -1 );
	glVertex2f( -1, 0 );
	glEnd();
}

//------------Asteroid creation/initialization functions------------

void newAsteroid(double size) //Create an asteroid of given size at a random, appropriate location
{
	int i,r;
	double x,y;
	for(i = 0; i < MAX_ASTEROIDS; i++)
	{
		if(!asteroids[i].active)
		{
			r = rand() % 4; //Picks a random wall
			if(r == 0) //Spawn along the bottom
			{
				x = myRandom(0, 100*w/h); //at a random position
				y = 0;
			}
			else if(r == 1) //...left side
			{
				x = 0;
				y = myRandom(0, 100);
			}
			else if(r == 2) //...top
			{
				x = myRandom(0, 100*w/h);
				y = 100;
			}
			else if(r == 3) //...right side
			{
				x = 100*w/h;
				y = myRandom(0, 100);
			}
			initAsteroid(&asteroids[i], x, y, size); //Create the asteroid at this point
			break;
		}
	}
}
void initAsteroid(Asteroid *a, double x, double y, double size) //Initialize the given asteroid with the given parameters
{
	double theta, r;
    int i;

    //Set values
    a->x = x;
    a->y = y;
    a->phi = 0.0;
    a->size = size;
    //Velocities (Bigger asteroids move/rotate slower)
    a->dx = myRandom(-0.8+(size/10), 0.8-(size/10)); //Random horizontal movement speed
    a->dy = myRandom(-0.8+(size/10), 0.8-(size/10)); //Random vertical movement speed
    a->dphi = myRandom(-0.2/size, 0.2/size); //Random rotation speed

    a->nVertices = 6+rand()%(MAX_VERTICES-6); //Random number of vertices (at least 6)

    for (i=0; i<a->nVertices; i++) //Create vertices with random variance
    {
		theta = 2.0*M_PI*i/a->nVertices;
		r = size*myRandom(minvar, maxvar);
		a->x_coords[i] = -r*sin(theta);
		r = size*myRandom(minvar, maxvar);
		a->y_coords[i] = r*cos(theta);
    }

    a->active = 1; //Allow the asteroid to be drawn
}

//------------Draw Asteroids------------

void drawAsteroid(Asteroid *a)
{
	glLoadIdentity();
	myTranslate2D(a->x, a->y);
	myRotate2D(a->phi);
	glBegin(GL_POLYGON);
	int i;
	for(i=0; i<a->nVertices; i++) //Draw each vertex
		glVertex2f( a->x_coords[i], a->y_coords[i] );
	glEnd();
}

//------------Generate/Draw Particles------------

void makeParticle(double x, double y) //Generate a particle at this position
{
	int i;
	for(i=0; i<MAX_PARTICLES; i++) //Find an open slot
	{
		if(!particles[i].active)
			break;
	}
	if(i == MAX_PARTICLES)
		return;
	particles[i].x = x;
	particles[i].y = y;
	particles[i].phi = 0;
	//Random movement/rotational speeds
	particles[i].dx = myRandom(-1.0, 1.0);
	particles[i].dy = myRandom(-1.0, 1.0);
	particles[i].dphi = myRandom(-0.2, 0.2);
	particles[i].fade = 3.0; //Fade value decreases and causes the particle to fade after passing 1.0
	particles[i].active = 1;
}
void drawParticle(Particle *p) //Draw a particle
{
	//Move/rotate the particle
	p->x += p->dx;
	p->y += p->dy;
	p->phi += p->dphi;
	//Draw it
	glLoadIdentity();
	myTranslate2D(p->x, p->y);
	myRotate2D(p->phi);
	p->fade -= 0.05;
	if(p->fade < 1.0) //Fade out after a bit
		glColor3f(p->fade, p->fade, p->fade);
	else
		glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_POLYGON);
	glVertex2f( 0, 0.5 );
	glVertex2f( 0.5, 0 );
	glVertex2f( 0, -0.5 );
	glVertex2f( -0.5, 0 );
	glEnd();
	if(p->fade <= 0) //Disable once the particle becomes black
		p->active = 0;
}

//------------Create/Draw the pieces of the destroyed ship------------

void breakShip() //Destroy the ship
{
	//The ship pieces are lines created from the geometry used from the ship, with the long side split into two pieces
	int i;
	double arrX[6] = {0, 0.65, -2.6, 0, 2.6, -0.65}; //Line x-values
	double arrY[6] = {6, 2, -2, -0.6, -2, 2}; //Line y-values
	for(i=0; i<6; i++)
	{
		//Initial position (Center point of ship at time of destruction)
		pieces[i].x = ship.x;
		pieces[i].y = ship.y;
		//First vertex of the line
		pieces[i].x1 = arrX[i];
		pieces[i].y1 = arrY[i];
		if(i == 5) //Second vertex of the line
		{
			pieces[i].x2 = arrX[0];
			pieces[i].y2 = arrY[0];
		}
		else //(uses the first coordinates if the first vertex was the final one)
		{
			pieces[i].x2 = arrX[i+1];
			pieces[i].y2 = arrY[i+1];
		}
		pieces[i].phi = ship.phi; //Rotate to match the ship's rotation
		//Random movement/rotation speeds
		pieces[i].dx = myRandom(-1.0, 1.0);
		pieces[i].dy = myRandom(-1.0, 1.0);
		pieces[i].dphi = myRandom(-0.2, 0.2);
	}
}
void drawShipPieces() //Draw/move the ship pieces
{
	int i;
	for(i=0; i<6; i++) //For each piece (line)
	{
		//Move the piece
		pieces[i].x += pieces[i].dx;
		pieces[i].y += pieces[i].dy;
		//Rotate it
		pieces[i].phi += pieces[i].dphi;
		//Draw it
		glLoadIdentity();
		myTranslate2D(pieces[i].x, pieces[i].y);
		myRotate2D(pieces[i].phi);
		glBegin(GL_LINES);
		glVertex2f(pieces[i].x1, pieces[i].y1);
		glVertex2f(pieces[i].x2, pieces[i].y2);
		glEnd();
	}
}


//Provided random double function
double myRandom(double min, double max)
{
	double	d;

	/* return a random number uniformly draw from [min,max] */
	d = min+(max-min)*(rand()%0x7fff)/32767.0;

	return d;
}
