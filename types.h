#include "texture.h"

double frand(double fMin, double fMax)
{
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

/*
 * Solves the quadratic equasion and returs roots r0 and r1
 */
bool solveQuadratic(double a, double b, double c, double & r0, double & r1)
{
	double radical = (b * b) - (4 * a * c);

    if (radical < 0) 
    	return false; //imaginary solution

    else if (radical == 0)
        return false;

    else 
    { 
        r0 = (-0.5 * (b - sqrt(radical))) / a;
        r1 = (-0.5 * (b + sqrt(radical))) / a;
    } 
 
    return true; 
}

glm::dvec3 backgroundColor = glm::dvec3(1.0f, 1.0f, 1.0f); //r,g,b
glm::dvec3 ambientLight = glm::dvec3(0.0f, 0.0f, 0.0f); //r,g,b

struct Scene
{
    int n; //number of pixels in w/h
    double d; //distance to the corner of the image plane
};

struct Light
{
    glm::dvec3 position;
    glm::dvec3 color;
};

struct Material 
{
    glm::dvec3 diffuse;
    glm::dvec3 specular;
    glm::dvec3 refraction;

    Texture texture;

    double shininess;
    double refractiveIndex = AIRREFRACTIVEINDEX;

    void print()
    {
        pv3(diffuse);
        pv3(specular);
        pv3(refraction);

        printf("%f -- %f\n\n", shininess, refractiveIndex);
    }
};

struct Ray
{
	glm::dvec4 orgin; //u
	glm::dvec4 direction; //v


    void print()
    {
        printf("o: %f %f %f %f \t", orgin.x, orgin.y, orgin.z, orgin.w);
        printf("d: %f %f %f %f\n", direction.x, direction.y, direction.z, direction.w);
    }
};

struct Sphere
{
    glm::dmat4 T;
	glm::dmat4 IT;

    Material material;

	//intersection done in object space where the center of the sphere is the orgin
	bool intersect(Ray ray, double & t)
	{
		double t0, t1; // the two possible t intersections on either side of the sphere

		glm::dvec4 uPrime = glm::dvec4(IT * ray.orgin);
        glm::dvec4 vPrime = glm::dvec4(IT * ray.direction);

		//calculate the three components for the quadratic equation
        double a = glm::dot(vPrime, vPrime); 
        double b = 2 * glm::dot(vPrime, uPrime); 
        double c = glm::dot(glm::dvec3(uPrime), glm::dvec3(uPrime)) - 1;

        //if the quad eq cant be solved no intersection
        if (!solveQuadratic(a, b, c, t0, t1))
        {
        	return false;
        }

        if(t0 > t1)
        {
        	double temp = t0;
        	t0 = t1;
        	t1 = temp;
        }

        if(t0 < 0)
        {
        	t0 = t1; // if t0 < 0 behind camera so use t1

        	if(t0 < 0) //uh oh big problem 2 neg t's
        		return false;
        }

        t = t0;
        return true;
	}
};

typedef std::vector<Sphere> ObjectArray;
typedef std::vector<Light> LightArray;

void loadScene(char * sceneFile, Scene & s, ObjectArray & objects, LightArray & lights)
{
    FILE * SceneFile;
    char cmd[512];
    char Buff[2048];
    bool first = false;

    char filename[256];

    glm::dmat4 currentTransform[64];
    Material currentMaterial[64];

    double x, y, z, angle, ni, r, g, b;
    char axis;

    int curGroupLevel = 0;
    int curMaterialLevel = 0;

    currentTransform[curGroupLevel] = glm::dmat4(1.0f);

    if ((SceneFile = fopen(sceneFile, "r")) == NULL) 
    {
        printf("Error opening scene file \n");
        exit(1);
    }
    fscanf(SceneFile, "%s", &cmd); 

    while (!feof(SceneFile))
    {
        if (!strcmp(cmd, "view"))
        {
            fscanf(SceneFile, "%d", &s.n);
            fscanf(SceneFile, "%lf", &s.d);
        }
        else if (!strncmp(cmd, "#", 1)) 
        {
            fgets(Buff, 2048, SceneFile);
        }
        else if (!strcmp(cmd, "group")) 
        {
            ++curGroupLevel;
            ++curMaterialLevel;

            currentTransform[curGroupLevel] = currentTransform[curGroupLevel - 1];
            currentMaterial[curMaterialLevel] = currentMaterial[curMaterialLevel - 1];
        }
        else if (!strcmp(cmd, "groupend")) 
        {
            --curGroupLevel;
            --curMaterialLevel;
        }
        else if (!strcmp(cmd, "background"))
        {
            fscanf(SceneFile, "%lf", &backgroundColor.x);
            fscanf(SceneFile, "%lf", &backgroundColor.y);
            fscanf(SceneFile, "%lf", &backgroundColor.z);
        }
        else if (!strcmp(cmd, "ambient"))
        {
            fscanf(SceneFile, "%lf", &ambientLight.x);
            fscanf(SceneFile, "%lf", &ambientLight.y);
            fscanf(SceneFile, "%lf", &ambientLight.z);
        }
        else if (!strcmp(cmd, "light"))
        {
            Light l;

            fscanf(SceneFile,"%lf", &l.color.x);
            fscanf(SceneFile,"%lf", &l.color.y);
            fscanf(SceneFile,"%lf", &l.color.z);

            fscanf(SceneFile,"%lf", &l.position.x);
            fscanf(SceneFile,"%lf", &l.position.y);
            fscanf(SceneFile,"%lf", &l.position.z);

            lights.push_back(l);
        }
        else if (!strcmp(cmd, "sphere"))
        {
            Sphere s;
            s.T = currentTransform[curGroupLevel];
            s.IT = glm::inverse(s.T);
            s.material = currentMaterial[curMaterialLevel];

            objects.push_back(s);
        }
        else if (!strcmp(cmd, "material"))
        {
            Material m = currentMaterial[curMaterialLevel];

            fscanf(SceneFile, "%lf", &m.diffuse.x);
            fscanf(SceneFile, "%lf", &m.diffuse.y);
            fscanf(SceneFile, "%lf", &m.diffuse.z);
            fscanf(SceneFile, "%lf", &m.specular.x);
            fscanf(SceneFile, "%lf", &m.specular.y);
            fscanf(SceneFile, "%lf", &m.specular.z);
            fscanf(SceneFile, "%lf", &m.shininess);

            currentMaterial[curMaterialLevel] = m;
        }
        else if (!strcmp(cmd, "refraction"))
        {
            Material m = currentMaterial[curMaterialLevel];

            fscanf(SceneFile, "%lf", &m.refraction.x);
            fscanf(SceneFile, "%lf", &m.refraction.y);
            fscanf(SceneFile, "%lf", &m.refraction.z);
            fscanf(SceneFile, "%lf", &m.refractiveIndex);

            currentMaterial[curMaterialLevel] = m;
        }
        else if (!strcmp(cmd, "texture"))
        {
            Material m = currentMaterial[curMaterialLevel];

            fscanf(SceneFile, "%s", filename);
            m.texture.readJpegFile(filename);
            m.texture.loadedTex = true;

            currentMaterial[curMaterialLevel] = m;
        }
        else if (!strcmp(cmd, "move"))
        {
            fscanf(SceneFile, "%lf", &x);
            fscanf(SceneFile, "%lf", &y);
            fscanf(SceneFile, "%lf", &z);

            glm::dmat4 t = glm::translate(glm::dmat4(1.0), glm::dvec3(x,y,z));
            currentTransform[curGroupLevel] = currentTransform[curGroupLevel] * t;
        }
        else if (!strcmp(cmd, "scale"))
        {
            fscanf(SceneFile, "%lf", &x);
            fscanf(SceneFile, "%lf", &y);
            fscanf(SceneFile, "%lf", &z);

            glm::dmat4 t = glm::scale(glm::dmat4(1.0), glm::dvec3(x,y,z));
            currentTransform[curGroupLevel] = currentTransform[curGroupLevel] * t;
        }
        else if (!strcmp(cmd, "rotate"))
        {
            fscanf(SceneFile, "%lf", &angle);
            fscanf(SceneFile, "%lf", &x);
            fscanf(SceneFile, "%lf", &y);
            fscanf(SceneFile, "%lf", &z);

            glm::dmat4 t = glm::rotate(glm::dmat4(1.0), glm::radians(angle), glm::dvec3(x,y,z));
            currentTransform[curGroupLevel] = currentTransform[curGroupLevel] * t;
        }
        else
        {
            fprintf(stderr, "Error: Unknown cmd '%s' in description file\n", cmd);
            exit(-1);
        }

        fscanf(SceneFile, "%s", &cmd);
    }

    fclose(SceneFile);
}
