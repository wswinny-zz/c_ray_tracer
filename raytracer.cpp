#include "types.h"

Scene scene;
ObjectArray objects;
LightArray lights;

void loadScene(char * sceneFile, Scene & s, ObjectArray & objects, LightArray & lights);
void castRays(char * outfilename);
glm::dvec3 trace(Ray ray, int depth, double currentRefractiveIndex);
bool shadow(glm::dvec3 P, glm::dvec3 L, glm::dvec3 N, Light light);

int main(int argc, char * argv[])
{
    //make sure you have enough args
    if(argc < 3)
    {
        printf("Usage: ./raytracer options scenefile(s)\n");
        printf("Options:\n\t1. n (normal raytracer)\n\t2. ss (super sampleing)\n\t3. dof (depth of field)\n");
        exit(-1);
    }

    //check if they used any options
    if(strcmp (argv[1], "dof") == 0)
        depthOfField = true;
    if(strcmp (argv[1], "ss") == 0)
        supersampleing = true;

    //if you have more than one scene file render them all
    if(argc > 3)
    {
        for(int i = 2; i < argc; i++)
        {
            loadScene(argv[i], scene, objects, lights); //for each file name load each scene

            int len = strlen(argv[i]);
            char * fileOutput = new char[len + 5];
            
            //add the ppm extension to the scene file name
            strcpy (fileOutput,argv[i]); 
            fileOutput[len + 0] = '.';
            fileOutput[len + 1] = 'p';
            fileOutput[len + 2] = 'p';
            fileOutput[len + 3] = 'm';
            fileOutput[len + 4] = '\0';

            castRays(fileOutput); //cast rays for this scene

            //clear objects from previous scenes
            objects.clear();
            lights.clear();

            //print when you finish rendering a scene
            printf("Finished rendering scene: %s\n", fileOutput);

            delete [] fileOutput;
        }
    }
    else //just load the scene normally
    {
        loadScene(argv[2], scene, objects, lights);
        castRays("./out.ppm");
    }

	return 0;
}

void castRays(char * outfilename)
{
	glm::dvec3 ** image = new glm::dvec3 *[scene.n]; //declare the image array

    //make the pixel varibles
	double pixelWidth = (2 * scene.d) / scene.n;
    double halfPixWidth = pixelWidth / 2.0;
    int x, y;

    //enable mutiple cores for faster rendering :D
    #pragma omp parallel for private(x)
	for(y = 0; y < scene.n; ++y)
	{
        image[y] = new glm::dvec3[scene.n]; //allocate memory :(

        #pragma omp parallel for
		for(x = 0; x < scene.n; ++x)
		{
            //find the pixel through which we will shoot the ray
            double pixX = (-scene.d + (pixelWidth / 2)) + (x * pixelWidth);
            double pixY = (scene.d - (pixelWidth / 2)) - (y * pixelWidth);

            //create the ray we will shoot
            Ray r;
            r.orgin = CAMERA;
            r.direction = glm::dvec4(pixX, pixY, 0.0f, POINT) - CAMERA;
            r.direction = glm::normalize(r.direction);

            //fire away!!
            int rDepth = 0;
            glm::dvec3 colorCenter = trace(r, rDepth, AIRREFRACTIVEINDEX);

            if(depthOfField)
            {
                //find the aperature radius for later
                glm::dvec4 xAperatureRadius = glm::dvec4(1, 0, 0, 0) * aperatureRad;
                glm::dvec4 yAperatureRadius = glm::dvec4(0, 1, 0, 0) * aperatureRad;

                //for loop to generate random points on the eye to fire more rays to calc depth of field
                for(int i = 0; i < NUM_SAMPLES; ++i)
                {
                    double R1 = frand(-1.0, 1.0);
                    double R2 = frand(-1.0, 1.0);

                    r.orgin = CAMERA + (R1 * xAperatureRadius) + (R2 * yAperatureRadius);
                    r.direction = glm::normalize(glm::dvec4(pixX, pixY, 0.0f, POINT) - r.orgin);

                    //add color to existing color
                    colorCenter += trace(r, rDepth, AIRREFRACTIVEINDEX);
                }

                //divide by numsamples + 1
                image[y][x] = colorCenter / (double)(NUM_SAMPLES + 1);
            }

            else if(supersampleing)
            {
                //make 4 new rays to assist with super sampling they will shoot at the corners of the pixel
                r.direction = glm::dvec4(pixX - halfPixWidth, pixY + halfPixWidth, 0.0f, POINT) - CAMERA;
                r.direction = glm::normalize(r.direction);
                glm::dvec3 colortl = trace(r, rDepth, AIRREFRACTIVEINDEX);

                r.direction = glm::dvec4(pixX + halfPixWidth, pixY + halfPixWidth, 0.0f, POINT) - CAMERA;
                r.direction = glm::normalize(r.direction);
                glm::dvec3 colortr = trace(r, rDepth, AIRREFRACTIVEINDEX);

                r.direction = glm::dvec4(pixX - halfPixWidth, pixY - halfPixWidth, 0.0f, POINT) - CAMERA;
                r.direction = glm::normalize(r.direction);
                glm::dvec3 colorbl = trace(r, rDepth, AIRREFRACTIVEINDEX);

                r.direction = glm::dvec4(pixX + halfPixWidth, pixY - halfPixWidth, 0.0f, POINT) - CAMERA;
                r.direction = glm::normalize(r.direction);
                glm::dvec3 colorbr = trace(r, rDepth, AIRREFRACTIVEINDEX);

                //calculate weighted color
                image[y][x] = (colorCenter * SS_WEIGHT_CENTER) + 
                (colortl * SS_WEIGHT_CORNER) + 
                (colortr * SS_WEIGHT_CORNER) + 
                (colorbl * SS_WEIGHT_CORNER) + 
                (colorbr * SS_WEIGHT_CORNER);
            }

            else //just use the regular color
                image[y][x] = colorCenter;
		}
	}

    //output ppm file
	std::ofstream ofs(outfilename, std::ios::out | std::ios::binary);
    ofs << "P6\n" << scene.n << " " << scene.n << "\n255\n";

    for(int y = 0; y < scene.n; ++y)
    {
        for(int x = 0; x < scene.n; ++x)
        {
            ofs << (unsigned char)(std::min(1.0, image[y][x].x) * 255) <<
                   (unsigned char)(std::min(1.0, image[y][x].y) * 255) <<
                   (unsigned char)(std::min(1.0, image[y][x].z) * 255);
        }
    }
    ofs.close();
}

glm::dvec3 trace(Ray ray, int depth, double currentRefractiveIndex)
{
    if(depth > MAX_RECURSIVE_DEPTH) //return black if you are past your max depth
        return glm::dvec3(0.0, 0.0, 0.0);

	double tMin = INFINITY; //create infinity so we know when we have left the scene
	Sphere * curSphereIntersect = NULL; //the sphere where tMin is at

	//finds the closest intersection
	for (int i = 0; i < objects.size(); ++i)
	{
        double t = INFINITY; //need infinity to see which t is closer
        if (objects[i].intersect(ray, t)) 
        {
            //intersection YAY!
            if (t < tMin) //the t is less than the current t
            {
                tMin = t;
                curSphereIntersect = &objects[i]; //set the intersected sphere
            }
        }
    }

    //if we hit nothing return the background color
    if(tMin == INFINITY || curSphereIntersect == NULL)
        return backgroundColor;

    //calculate the object space ray
    glm::dvec4 uPrime = glm::dvec4(curSphereIntersect->IT * ray.orgin);
    glm::dvec4 vPrime = glm::dvec4(curSphereIntersect->IT * ray.direction);

    //calculate the normal and pobj and pwor
    glm::dvec3 pWorld = glm::dvec3(ray.orgin) + (glm::dvec3(ray.direction) * tMin);
    glm::dvec3 p = glm::dvec3(uPrime) + (glm::dvec3(vPrime) * tMin);
    glm::dvec3 N = glm::normalize(glm::dvec3(glm::transpose(curSphereIntersect->IT) * glm::dvec4(p, VEC)));

    //declare all colors
    glm::dvec3 color = ambientLight * curSphereIntersect->material.diffuse;
    glm::dvec3 diffuse;
    glm::dvec3 specular;
    glm::dvec3 reflection;
    glm::dvec3 refraction;

    //return glm::abs(N); //for debugging purposes

    glm::dvec3 V = glm::normalize(-glm::dvec3(ray.direction)); //to eye

    //PHONG ILLUMINATION
    for(int i = 0; i < lights.size(); ++i)
    {
        Light light = lights[i];

        glm::dvec3 L = glm::normalize(light.position - pWorld); //to light
        //glm::dvec3 R = glm::normalize(L - (2.0 * glm::dot(L, N) * N)); //reflection from L
        glm::dvec3 H = glm::normalize(V + L); //half vector

        if(shadow(pWorld, L, N, light))
            continue;

        //I kd N dot L
        double diff = std::max(glm::dot(N, L), 0.0);
        glm::dvec3 diffuseC = light.color * diff * curSphereIntersect->material.diffuse;

        //I ks (R dot V)^p
        //double spec = pow(std::max(glm::dot(R, V), 0.0), curSphereIntersect->material.shininess);
        double spec = pow(std::max(glm::dot(H, N), 0.0), curSphereIntersect->material.shininess * BLINNEXPONETMULTIPLIER); //half vector
        glm::dvec3 specularC = light.color * spec * curSphereIntersect->material.specular;

        diffuse += diffuseC;
        specular += specularC;
    }

    //REFLECTION
    if(curSphereIntersect->material.specular != glm::dvec3(0.0, 0.0, 0.0))
    {
        Ray reflect;
        reflect.orgin = glm::dvec4(pWorld + (CANCERDELTA * N), POINT);
        glm::dvec3 reflectDir = glm::normalize(V - (2 * glm::dot(V, N) * N));
        reflect.direction = -glm::dvec4(reflectDir, VEC);

        reflection += curSphereIntersect->material.specular * trace(reflect, depth + 1, curSphereIntersect->material.refractiveIndex);
    }

    //REFRACTION
    if(curSphereIntersect->material.refraction != glm::dvec3(0.0, 0.0, 0.0))
    {
        double nr;
        if(glm::dot(glm::dvec3(ray.direction), N) > 0) //are we inside the sphere
        {
            nr = currentRefractiveIndex / AIRREFRACTIVEINDEX;
            currentRefractiveIndex = AIRREFRACTIVEINDEX;
            N = -N; //make the normal point the other direction
        }
        else //to another material
        {
            nr = currentRefractiveIndex / curSphereIntersect->material.refractiveIndex;
            currentRefractiveIndex = curSphereIntersect->material.refractiveIndex;
        }  

        double ni = glm::dot(N, glm::dvec3(-ray.direction)); //n dot i
        double sqrtInside = 1 - nr * nr * (1 - (ni * ni)); //inside the sqrt

        if(sqrtInside <= 0)
        {
            //total internal refraction
        }
        else //FIRE REFRACTIVE RAY!
        {
            Ray refract;
            refract.orgin = glm::dvec4(pWorld - (CANCERDELTA * N), POINT);

            //find the refractive direction
            refract.direction = glm::normalize(glm::dvec4((nr * ni - sqrt(sqrtInside)) * N - (nr * glm::dvec3(-ray.direction)), VEC)); 

            glm::dvec3 refractColor = trace(refract, depth + 1, currentRefractiveIndex);

            refraction += curSphereIntersect->material.refraction * refractColor; 
        }
    }

    if(curSphereIntersect->material.texture.loadedTex) //if there is a texture get the color of the pixel now
        return color + specular + reflection + refraction + curSphereIntersect->material.texture.getColor(glm::dvec3(glm::transpose(curSphereIntersect->IT) * glm::dvec4(p, VEC)));
    
    return color + diffuse + specular + reflection + refraction; //otherwise just return normal color
}

//are we in a shadow
bool shadow(glm::dvec3 P, glm::dvec3 L, glm::dvec3 N, Light light)
{
    double tMin = INFINITY;
    Sphere * curSphereIntersect = NULL; //the sphere where tMin is at

    Ray ray;
    ray.orgin = glm::dvec4(P + (CANCERDELTA * N), POINT);
    ray.direction = glm::normalize(glm::dvec4(L, VEC));

    //finds the closest intersection
    for (int i = 0; i < objects.size(); ++i)
    {
        double t = INFINITY;
        if (objects[i].intersect(ray, t)) 
            return true;
    }

    return false;
}
