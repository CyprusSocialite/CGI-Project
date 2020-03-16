/*----Includes----*/

#include <cmath>
#include "file_loading.h"
#include <fstream>
#include "Image.h"
#include <iostream>
#include <random>
#include <string>
#include "Vec3.h"
#include <vector>

using namespace std;


/*----Global Parameters----*/

const int NSamples = 1;
const int PathTracingBounces = 5;
const float sceneSize = 10; //X
const Vec3 cam = { 0, 0, -5 }; //Camera position, z-1 away from the 'screen'


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
		double x = 0.0, double y = 0.0, double z = 0.0,
		double colr = 1.0, double colg = 1.0, double colb = 1.0,
		Vec3 L_e = { 0.0, 0.0, 0.0 })
	{
		origin = { x, y, z };
		col = { colr, colg, colb }; 	//RGB 0-1
		emit = L_e;
		lightPos = {};
	};

	virtual Vec3 BRDF(Vec3 destDir, Vec3 srcDir)
	{
		return col;		//Lambertian BRDF
	};

	virtual bool Intersect(Vec3 sourcePos, Vec3 targetDir) = 0;

	virtual Vec3 SurfaceNormal(Vec3 dir) = 0;

};

class Sphere : public Object
{
public:
	double rad;

	Sphere(double x = 0.0, double y = 0.0, double z = 0.0, double r = 1.0,
		double colr = 1.0, double colg = 1.0, double colb = 1.0,
		Vec3 L_e = { 0.0, 0.0, 0.0 }) : Object(0, x, y, z, colr, colg, colb, L_e)
	{
		rad = r;
	};

	bool Intersect(Vec3 srcPos, Vec3 destDir) override
	{
		std::cout << "In Intersect, srcPos=" << srcPos << ", destDir=" << destDir << std::endl;
		double	a = destDir.norm2(),
			b = 2 * dot(destDir, srcPos - origin),
			c = (srcPos - origin).norm2() - pow(rad, 2.0),
			det = b * b - 4 * a*c,
			root1, root2;
		//std::cout << origin << " " << rad << std::endl;
		//	std::cout << a << " " <<  b << " " << c << std::endl;
		//std::cout << det << std::endl;
		if (det >= 0)
		{
			root1 = (-b + sqrt(det)) / (2 * a);
			root2 = (-b - sqrt(det)) / (2 * a);
			//	std::cout << root1 << " " << root2 << std::endl;
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

	Vec3 SurfaceNormal(Vec3 dir = { 0,0,0 }) override
	{
		if (dir.norm2() == 0) dir = lightPos;
		return (dir - origin);
	};
};	vector<Sphere*> objects;		//TODO: fix

class Rectangle : public Object
{
public:
	double height, length, width;		// Perhaps better as Vec3?

	Rectangle(double x = 0.0, double y = 0.0, double z = 0.0,
		double h = 1.0, double l = 1.0, double w = 1.0,
		double colr = 1.0, double colg = 1.0, double colb = 1.0,
		Vec3 L_e = { 0.0, 0.0, 0.0 }) : Object(1, x, y, z, colr, colg, colb, L_e)
	{
		height = h; length = l; width = w;
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
	//

	v = Vec3(0, 0, 1);

	//	std::cout << "In alignVec, v="<<v << " n="<<n<<std::endl;
	double 	xz = std::atan(n.z / n.x),		//rotation about y-axis
		xy = std::atan(n.y / n.x);		//rotation about z-axis
	std::cout << n.z / n.x << " " << xz << " " << xy << std::endl;
	Vec3 retval = {
			v.x * std::cos(xy) * std::cos(xz) + v.y * std::sin(xy) + v.z * std::cos(xy) * std::sin(xz),
			v.x * std::cos(xz) * std::sin(xy) + v.y * std::cos(xy) + v.z * std::sin(xy) * std::sin(xz),
			-v.x * std::sin(xz) + v.z*std::cos(xz)
	};
	/*
		Vec3 retval= {
				v.x * (n.x/n.y) * (n.x/n.z)		+ v.y * (n.y/n.x) 	+ v.z * (n.x/n.y) * (n.z/n.x),
				v.x * (n.x/n.z) * (n.y/n.x)		+ v.y * (n.x/n.y)	+ v.z * (n.y/n.x) * (n.z/n.x),
				-v.x * (n.z/n.x) 									+ v.z * (n.x/n.z)
				};
	*/
	std::cout << "retval=" << retval << std::endl;
	if (dot(retval, n) < 0)
		throw;

	return retval;
}

Vec3 randVec()
{
	double 	z = dis(rnd),
		phi = 2 * pi * dis(rnd),
		r = std::sqrt(1.0 - z * z);
	//	std::cout << "in randvec, r="<<r<< " phi="<<phi<<std::endl;
	return { r * std::cos(phi), r * std::sin(phi), z };	// Z vs Y???
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

	Vec3 objNorm = sourceObject->SurfaceNormal(destDir), srcDir = alignVec(randVec(), objNorm);

	return (
		sourceObject->emit +
		(sourceObject->BRDF(destDir, srcDir) / ProbDist(destDir))*
		(srcDir*objNorm)*
		IncomingLight(sourceObject->lightPos, srcDir, bounce++)
		);
};

Vec3 IncomingLight(Vec3 destPos, Vec3 srcDir, int bounce)	//(4)L_i(x, w_i)
{
	std::cout << "In IncomingLight, " << destPos << " " << srcDir << " " << bounce << std::endl;
	Object* srcObj = SourceSurface(destPos, srcDir);

	return (srcObj ? OutgoingLight(srcObj, -srcDir, bounce) : Vec3(0, 0, 0));
};

Vec3 PixVal(Image &img, double x, double y)		//(6)I_xy
{
	x -= double(img.Width()) / 2.0; y = double(img.Height()) / 2.0 - y;		//Convert pixel coordinates to 3D scene coordinates
	x *= sceneSize / img.Width(); y *= sceneSize / img.Height();
	Vec3 d = Vec3(x, y, cam.z + 5) - cam; d.normalise();

	std::cout << "d=" << d << std::endl;
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
		new Sphere(0.0, 0.0, 5.0, 3.0,
			1.0, 1.0, 1.0,
			Vec3(1.0, 1.0, 1.0))
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

	Image img(101, 101);		//DEBUG: Which way do the axis point?
	/*
	for (int x = 0; x <= img.Width() - 1; x++)
	{
		for (int y = 0; y <= img.Height() - 1; y++)
		{
			img(x, y) = PixVal(img, x, y);	//DEBUG: accessing works fine
		}
	}*/

	img(51, 51) = PixVal(img, 51, 51);

	img.Save("output.png");

	cout << "Done";
	cin.get();
}