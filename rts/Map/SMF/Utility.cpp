//
// Utility.cpp
//Added to Spring by Peter Sarkozy (mysterme AT gmail DOT com)
// Billion thanks to Bryan Turner (Jan, 2000)
//                    brturn@bellsouth.net
//
//

//#include <windows.h>
//#include <stdio.h>
//#include <stdlib.h>
#include "FastMath.h"
//#include <gl/gl.h>		// OpenGL
//#include <gl/glu.h>		// GLU library

#include "Utility.h"
//#include "landscape.h"

// Observer and Follower modes
#define FOLLOW_MODE (0)
#define OBSERVE_MODE (1)
#define DRIVE_MODE (2)
#define FLY_MODE (3)

// Perspective & Window defines
#define FOV_ANGLE     (90.0f)
#define NEAR_CLIP     (1.0f)
#define FAR_CLIP      (2500.0f)

// --------------------------------------
// GLOBALS
// --------------------------------------
//Landscape gLand;


// Misc. Globals
int gAnimating  = 0;
int gRotating  = 0;
int gDrawFrustum = 1;
int gCameraMode = OBSERVE_MODE;
//int gDrawMode   = DRAW_USE_TEXTURE;
int gStartX=-1, gStartY;
int gNumTrisRendered;
long gStartTime, gEndTime;
unsigned char *gHeightMap;
unsigned char *gHeightMaster;
int gNumFrames;
float gFovX = 90.0f;

// Beginning frame varience (should be high, it will adjust automatically)
float gFrameVariance = 50;

// Desired number of Binary Triangle tessellations per frame.
// This is not the desired number of triangles rendered!
// There are usually twice as many Binary Triangle structures as there are rendered triangles.
int gDesiredTris = 100000;

// ---------------------------------------------------------------------
// Reduces a normal vector specified as a set of three coordinates,
// to a unit normal vector of length one.
//
void ReduceToUnit(float vector[3])
	{
	float length;
	
	// Calculate the length of the vector		
	length = sqrt((vector[0]*vector[0]) + 
				   (vector[1]*vector[1]) +
				   (vector[2]*vector[2]));

	// Keep the program from blowing up by providing an exceptable
	// value for vectors that may calculated too close to zero.
	if(length == 0.0f)
		length = 1.0f;

	// Dividing each element by the length will result in a
	// unit normal vector.
	vector[0] /= length;
	vector[1] /= length;
	vector[2] /= length;
	}

// ---------------------------------------------------------------------
// Points p1, p2, & p3 specified in counter clock-wise order
//
void calcNormal(float v[3][3], float out[3])
	{
	float v1[3],v2[3];
	static const int x = 0;
	static const int y = 1;
	static const int z = 2;

	// Calculate two vectors from the three points
	v1[x] = v[0][x] - v[1][x];
	v1[y] = v[0][y] - v[1][y];
	v1[z] = v[0][z] - v[1][z];

	v2[x] = v[1][x] - v[2][x];
	v2[y] = v[1][y] - v[2][y];
	v2[z] = v[1][z] - v[2][z];

	// Take the cross product of the two vectors to get
	// the normal vector which will be stored in out
	out[x] = v1[y]*v2[z] - v1[z]*v2[y];
	out[y] = v1[z]*v2[x] - v1[x]*v2[z];
	out[z] = v1[x]*v2[y] - v1[y]*v2[x];

	// Normalize the vector (shorten length to one)
	ReduceToUnit(out);
	}

// ---------------------------------------------------------------------
// Load the Height Field from a data file
//
/*
void loadTerrain(int size, unsigned char **dest)
{
	FILE *fp;
	char fileName[30];

	// Optimization:  Add an extra row above and below the height map.
	//   - The extra top row contains a copy of the last row in the height map.
	//   - The extra bottom row contains a copy of the first row in the height map.
	// This simplifies the wrapping of height values to a trivial case. 
	gHeightMaster = (unsigned char *)malloc( size * size * sizeof(unsigned char) + size * 2 );
	
	// Give the rest of the application a pointer to the actual start of the height map.
	*dest = gHeightMaster + size;

	sprintf( fileName, "Height%d.raw", size );
	fp = fopen(fileName, "rb");

	// TESTING: READ A TREAD MARKS MAP...
	if ( fp == NULL )
	{
		sprintf( fileName, "Map.ved", size );
		fp = fopen(fileName, "rb");
		if ( fp != NULL )
			fseek( fp, 40, SEEK_SET );	// Skip to the goods...
	}

	if (fp == NULL)
	{
		// Oops!  Couldn't find the file.
		
		// Clear the board.
		memset( gHeightMaster, 0, size * size + size * 2 );
		return;
	}
	fread(gHeightMaster + size, 1, (size * size), fp);
	fclose(fp);

	// Copy the last row of the height map into the extra first row.
	memcpy( gHeightMaster, gHeightMaster + size * size, size );

	// Copy the first row of the height map into the extra last row.
	memcpy( gHeightMaster + size * size + size, gHeightMaster + size, size );
}
*/
// ---------------------------------------------------------------------
// Free the Height Field array
//



// ---------------------------------------------------------------------
// Initialize the ROAM implementation
//


// ---------------------------------------------------------------------
// Call all functions needed to draw a frame of the landscape
//

// ---------------------------------------------------------------------
// Draw a simplistic frustum for debug purposes.
//

// ---------------------------------------------------------------------
// Key Binding Functions
//
// ---------------------------------------------------------------------
// Called when the window has changed size
//


// ---------------------------------------------------------------------
// Called to update the window
//


// ---------------------------------------------------------------------
// Called when user moves the mouse
//

// ---------------------------------------------------------------------
// Called when application is idle
//
// ---------------------------------------------------------------------
// This function does any needed initialization on the rendering
// context.  Here it sets up and initializes the lighting for
// the scene.
