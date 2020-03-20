/*----Includes----*/

#include <cmath>
#include "file_loading.h"
#include <fstream>
#include "Image.h"
#include <iostream>
#include "matrix.h"
#include <random>
#include <string>
#include "Vec3.h"
#include <vector>

using namespace std;


/*----Global Parameters----*/

const int NSamples = 1;
const int PathTracingBounces = 5;
const float sceneSize = 10; //X
const Vec3 cam = {0.0, 0.0, -5.0}; //Camera position, z-1 away from the 'screen'


/*----Classes----*/

class Object
{
public:
	int type;
	Vec3 origin;
	Vec3 col;
	Vec3 emit;
	Vec3 lightPos;

	Object(int type,
		Vec3 xyz = {0.0, 0.0, 0.0},
		Vec3 rgb = {1.0, 1.0, 1.0}, //RGB 0-1
		Vec3 L_e = {0.0, 0.0, 0.0})
	{
		origin = xyz;
		col = rgb; 	
		emit = L_e;
		lightPos = {};
	};

	virtual Vec3 BRDF(Vec3 destDir, Vec3 srcDir)
	{
		return col;		//Lambertian BRDF
	};

	virtual bool Intersect(Vec3 sourcePos, Vec3 targetDir) = 0;

	virtual Vec3 SurfaceNormal() = 0;

};

class Sphere : public Object
{
public:
	double rad;

	Sphere(	Vec3 xyz = {0.0, 0.0, 0.0}, double r = 1.0,
			Vec3 rgb = {1.0, 1.0, 1.0},
			Vec3 L_e = {0.0, 0.0, 0.0}) : Object(0, xyz, rgb, L_e)
	{
		rad = r;
	};

	bool Intersect(Vec3 srcPos, Vec3 destDir) override
	{
		double	a = destDir.norm2(),
				b = 2 * dot(destDir, srcPos - origin),
				c = (srcPos - origin).norm2() - pow(rad, 2.0),
				det = b * b - 4 * a*c,
				root1, root2;

		if (det >= 0)
		{
			root1 = (-b + sqrt(det)) / (2 * a);
			root2 = (-b - sqrt(det)) / (2 * a);
			if (root1 >= 0 && root2 >= 0)	//They will have the same sign but better safe than sorry
			{
				lightPos = srcPos + min(root1, root2)*destDir;
			};
		}
		else
		{
			lightPos = {};
			return false;
		};
	};

	Vec3 SurfaceNormal() override
	{
		Vec3 n = lightPos - origin; n.normalise();
		return n;
	};
};	vector<Sphere*> objects;		//TODO: fix

class Rectangle : public Object
{
public:
	Vec3 dim = {1.0, 1.0, 1.0};

	Rectangle(	Vec3 xyz = {0.0, 0.0, 0.0},
				Vec3 hlw = {1.0, 1.0, 1.0},
				Vec3 rgb = {1.0, 1.0, 1.0},
				Vec3 L_e = {0.0, 0.0, 0.0}) : Object(1, xyz, rgb, L_e)
	{
		dim = hlw;
	};

	bool Intersect(Vec3 srcPos, Vec3 destDir) override
	{};
};


/*----Utility Functions----*/

const double pi = 4.0*atan(1.0);
std::random_device rand_dev; // Set up a "random device" that generates a new random number each time the program is run
std::mt19937 rnd(rand_dev()); // Set up a pseudo-random number generater "rnd", seeded with a random number
std::uniform_real_distribution<> dis(0, 1);

Vec3 alignVec(const Vec3 &v, const Vec3 &n)		//Change of baseeeees
{
	Matrix<double> _v(3, 1), _n(3, 3);

	_n.put(0, 0, n.x); _n.put(1, 1, n.y); _n.put(2, 2, n.z); _n.invert();
	_v.put(0, 0, v.x); _v.put(1, 0, v.y); _v.put(2, 0, v.z);
	_n *= _v;

	return Vec3(_n.get(0, 0), _n.get(1, 0), _n.get(2, 0));
}

Vec3 randVec()
{
	double 	z = dis(rnd),
		phi = 2 * pi * dis(rnd),
		r = std::sqrt(1.0 - z * z);

	return { r * std::cos(phi), r * std::sin(phi), z };
};

double ProbDist(Vec3 dir)
{
	return 1.0 / (2.0*pi);
};

Object* SourceSurface(Vec3 destPos, Vec3 srcDir)		//(3)x_M(x, w_i)
{
	Object* surface = nullptr;
	for (auto& obj : objects)
	{
		if (obj->Intersect(destPos, srcDir) && (!surface || (obj->origin - destPos).norm() < (surface->origin - destPos).norm()))
			surface = obj;
	};
	return surface;
};


/*----Path Tracing Algorithm----*/

Vec3 IncomingLight(Vec3 destPos, Vec3 srcDir, int bounce = 0);

Vec3 OutgoingLight(Object* sourceObject, Vec3 destDir, int bounce)		//(9)L_o(x_0, w_0)
{
	if (bounce >= PathTracingBounces) return sourceObject->emit;

	Vec3	objNorm = sourceObject->SurfaceNormal(), 
			srcDir = alignVec(randVec(), objNorm);

	return (
		sourceObject->emit +
		(sourceObject->BRDF(destDir, srcDir) / ProbDist(destDir))*
		(srcDir*objNorm)*
		IncomingLight(sourceObject->lightPos, srcDir, bounce++)
		);
};

Vec3 IncomingLight(Vec3 destPos, Vec3 srcDir, int bounce)	//(4)L_i(x, w_i)
{
	Object* srcObj = SourceSurface(destPos, srcDir);

	return (srcObj ? OutgoingLight(srcObj, -srcDir, bounce) : Vec3(0.0, 0.0, 0.0));
};

Vec3 PixVal(Image &img, double x, double y)		//(6)I_xy
{
	x -= double(img.Width()) / 2.0; y = double(img.Height()) / 2.0 - y;		//Convert pixel coordinates to 3D scene coordinates
	x *= sceneSize / img.Width(); y *= sceneSize / img.Height();
	Vec3 d = Vec3(x, y, cam.z + 5) - cam; d.normalise();

	Vec3 PixelValue = { 0, 0, 0 };
	for (int x = 0; x < NSamples; x++)
	{
		PixelValue += IncomingLight(cam, d);
	}
	return PixelValue / NSamples;
};


/*----Main----*/

int main()
{
	/*
	ifstream ifs("objects.txt");
	if (ifs.is_open())
	{
		DoubleCSVFile inputFile;
		inputFile.Load(ifs);

		for (const auto& line in inputFile)
		{
			//Access line data?
		};

		inputFile.DisplayValues();
		ifs.close();
	};*/

	objects.push_back(
						new Sphere(	{0.0, 0.0, 5.0}, 3.0,
									{0.0, 0.0, 1.0})
					);

	objects.push_back(
						new Sphere(	{3.0, 3.0, 3.0}, 1.0,
									{1.0, 1.0, 1.0},
									{1.0, 1.0, 1.0})
					);

	/*objects.push_back(
						new Sphere(0.0, 0.0, 5.0, 1.0,
						1.0, 1.0, 1.0,
						Vec3(1.0, 1.0, 1.0))
					);

	objects.push_back(
						new Sphere(-2.0, 0.0, 3.0, 2.0,
						0.5, 0.0, 0.0)
					);

	objects.push_back(
						new Sphere(-2.0, 0.0, 3.0, 2.0,
						0.0, 0.0, 0.5)
					);*/

					//DEBUG: Object type values are weird

	Image img(101, 101);
	for (int y = 0; y <= img.Height() - 1; y++)
	{
		for (int x = 0; x <= img.Width() - 1; x++)
			img(x, y) = PixVal(img, x, y);
	}
	img.Save("output.png");

	cout << "Done";
	cin.get();
}