#include <string.h>
#include <sstream>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <math.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <time.h>

#include "glm/glm.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define POINT 1
#define VEC   0

#define PI 3.14159265

#define MAX_RECURSIVE_DEPTH 5
#define INFINITY 1e9

#define CANCERDELTA 0.0001
#define BLINNEXPONETMULTIPLIER 4.0
#define AIRREFRACTIVEINDEX 1.0002926

#define SS_WEIGHT_CENTER 0.6
#define SS_WEIGHT_CORNER 0.1

#define NUM_SAMPLES 5
#define FOCAL_DISTANCE 1

bool supersampleing = false;
bool depthOfField = false;

double aperatureRad = FOCAL_DISTANCE / 256.0;

glm::dvec4 CAMERA = glm::dvec4(0.0, 0.0, FOCAL_DISTANCE, POINT);

void pv3(glm::dvec3 v) 
{
	printf("%-10lf %-10lf %-10lf \n", v.x, v.y, v.z); 
}
