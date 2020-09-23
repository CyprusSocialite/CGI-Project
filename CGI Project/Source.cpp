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


/*----Global Parameters----*/

int				NSamples = 50,				//Noise reduction. Low: 100, Medium: 1000, High: 10000
				PathTracingBounces = 10;	//Low:2 Medium:5 High:10
const double	sceneSize = 5.0,			//Visible span of the X- and Y-axis
				fov = 30,					//Degrees
				RR = 1,						//Russian Roulette probability of continuing. 1 to disable
				pi = 4.0*std::atan(1.0);
const Vec3		cam = { 0.0, 0.0, -sceneSize / (2.0 * std::tan(fov*pi / 360.0)) }, //Camera position
				bg	= { 0.0, 0.0, 0.0 };	//Background colour


/*----Classes----*/

class Object
{
public:
	int type;
	Vec3 origin;
	Vec3 emit;
	Vec3 col;
	Vec3 lightPos;

	Object(	int type,
			Vec3 xyz = { 0.0, 0.0, 0.0 },
			Vec3 rgb = { 1.0, 1.0, 1.0 }, //RGB 0-1
			Vec3 L_e = { 0.0, 0.0, 0.0 })
	{
		origin = xyz;
		col = rgb;// / pi;
		emit = L_e;
		lightPos = {};
	};

	virtual Vec3 BRDF(Vec3 destDir, Vec3 srcDir)
	{
		return col/pi;		//Lambertian BRDF
	};

	virtual bool Intersect(Vec3 sourcePos, Vec3 targetDir) = 0;

	virtual Vec3 SurfaceNormal() = 0;
};

class Sphere : public Object
{
public:
	double rad;

	Sphere(	Vec3 xyz = { 0.0, 0.0, 0.0 }, double r = 1.0,
			Vec3 rgb = { 1.0, 1.0, 1.0 },
			Vec3 L_e = { 0.0, 0.0, 0.0 }) : Object(0, xyz, rgb, L_e)
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

			//Both points of intersection are BEHIND the ray start point
			if (root1 <= 0 && root2 <= 0)
			{
				lightPos = {};
				return false;
			}

			//Are ~both~ points of intersection ahead the ray start point?
			//If yes, choose the closest. Else, the ray start point is inside the object, choose the intersection point ahead
			/*if (root1 >= 0 && root2 >= 0)
			{
				lightPos = srcPos + (std::min(root1, root2))*destDir;
				return true;
			}
			lightPos = srcPos + (std::max(root1, root2))*destDir;*/
			lightPos = srcPos + destDir*((root1 >= 0 && root2 >= 0) ? std::min(root1, root2) : std::max(root1, root2));
			return true;
		}

		lightPos = {};
		return false;
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
	Vec3 dim = { 1.0, 1.0, 1.0 };

	Rectangle(	Vec3 xyz = { 0.0, 0.0, 0.0 },
				Vec3 hlw = { 1.0, 1.0, 1.0 },
				Vec3 rgb = { 1.0, 1.0, 1.0 },
				Vec3 L_e = { 0.0, 0.0, 0.0 }) : Object(1, xyz, rgb, L_e)
	{
		dim = hlw;
	};

	bool Intersect(Vec3 srcPos, Vec3 destDir) override
	{};
};


/*----Utility Functions----*/

std::random_device rand_dev; // Set up a "random device" that generates a new random number each time the program is run
std::mt19937 rnd(rand_dev()); // Set up a pseudo-random number generater "rnd", seeded with a random number
std::uniform_real_distribution<> dis(0, 1);

Vec3 alignVec(const Vec3 &v, const Vec3 &n)		//Change of baseeeees
{
	Vec3 a, b;

	a = cross(Vec3(1.0, 0.0, 0.0), n);
	if (a.norm() < 1e-4)
	{
		a = cross(Vec3(0.0, 1.0, 0.0), n);
	}
	a.normalise();
	b = cross(n, a);

	return v.x*a + v.y*n + v.z*b;
}

Vec3 randVec()
{
	double 	z = dis(rnd),
			phi = 2 * pi * dis(rnd),
			r = std::sqrt(1.0 - z * z);

	return { r * std::cos(phi), z, r * std::sin(phi) };
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
		if (obj->Intersect(destPos, srcDir)
			&& (!surface || (obj->origin - destPos).norm() < (surface->origin - destPos).norm()))
			surface = obj;
	};
	return surface;
};


/*----Path Tracing Algorithm----*/

Vec3 IncomingLight(Vec3 destPos, Vec3 srcDir, int bounce = 0);	//Forward declaration

Vec3 OutgoingLight(Object* sourceObject, Vec3 destDir, int bounce)		//(9)L_o(x_0, w_0)
{
	if (dis(rnd) >= RR) return { 0.0, 0.0, 0.0 };
	if (bounce >= PathTracingBounces) return sourceObject->emit;

	Vec3 objNorm = sourceObject->SurfaceNormal(),
		 srcDir = randVec();

	if (dot(destDir, objNorm) < 0) objNorm *= -1.0;	//Ray from inside the sphere, hence inverted normal
	srcDir = alignVec(srcDir, objNorm);

	Vec3 returnval = sourceObject->emit +
		 (sourceObject->BRDF(destDir, srcDir) / ProbDist(destDir))*
		 dot(srcDir, objNorm)*
		 IncomingLight(sourceObject->lightPos + 1e-6 * objNorm, srcDir, bounce + 1);
		 //*exp(-distance*absorptionPerDistance);

	return (returnval / RR);
};

Vec3 IncomingLight(Vec3 destPos, Vec3 srcDir, int bounce)	//(4)L_i(x, w_i)
{
	Object* srcObj = SourceSurface(destPos, srcDir);
	return (srcObj ? OutgoingLight(srcObj, -srcDir, bounce) : bg);
};

Vec3 PixVal(Image &img, double x, double y)		//(6)I_xy
{
	x -= double(img.Width()) / 2.0; y = double(img.Height()) / 2.0 - y;						//Convert pixel coordinates to 3D scene coordinates
	x *= 2.0*sceneSize / double(img.Width()); y *= 2.0*sceneSize / double(img.Height());	//Convert from image size to scene size
	Vec3 d = Vec3(x, y, 0.0) - cam; d.normalise();											//Camera to pixel direction vector

	Vec3 PixelValue = { 0, 0, 0 };
	for (int i = 0; i < NSamples; i++)
	{
		PixelValue += IncomingLight(cam, d);
	}
	return PixelValue / NSamples;
};


/*----Main----*/
int main_Image(int h = 1, int w = 1)
{
	Image img(h, w);
	for (int y = 0; y <= h - 1; y++)
	{
		for (int x = 0; x <= w - 1; x++)
		{
			img(x, y) = PixVal(img, x, y);
		}
	}
	img.Save("output.png");
	return 0;
}

int main_SinglePixel(int n = 1)
{
	Image img(1, 1);
	for (int i = 0; i <= n - 1; i++)
	{
		std::cout << PixVal(img, 0, 0).x << std::endl;
	}
	return 0;
}

int main_VariedSampling(double n = 1, double k = 1, int count = 1)
{
	/*
	Image img(1, 1);
	for (double i = n; i <= n*pow(k, count); i *= k)
	{
		NSamples = i;
		std::cout << i << ", " << PixVal(img, 0, 0).x << std::endl;
	}*/
	for (double i = n; i <= n * pow(k, count); i *= k)
	{
		NSamples = i;
		std::cout << i << std::endl;
		main_SinglePixel(100);
	}
	return 0;
}

int main()
{
	//Reading file
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

// Two coloured lights with sphere in center
	objects.push_back(
		new Sphere(	{0.0, 0.0, 5.0}, 3.0,
					{0.6, 0.6, 1.0})
	);

	objects.push_back(
		new Sphere(	{3.0, 3.0, 0.0}, 2.0,
					{1.0, 1.0, 1.0},
					{1.0, 1.0, 1.0})
	);

	objects.push_back(
		new Sphere(	{-3.0, -3.0, 0.0}, 2.0,
					{1.0, 1.0, 1.0},
					{1.0, 0.3, 0.3})
	);
	
	//Camera inside sphere 
	/*
	objects.push_back(
		new Sphere(	{ 0.0, 0.0, 0.0 }, 10.0,
					{ 0.5, 0.5, 0.5 },
					{ 0.5, 0.5, 0.5 })
	);*/

	//Surrounding emitting sphere, spheres inside; Sounds good, doesn't work
	/*objects.push_back(
		new Sphere(	{ 0.0, 0.0, 5.0 }, 3.0,
					{ 0.0, 0.0, 1.0 },
					{ 0.0, 0.0, 1.0 })
	);
	objects.push_back(
		new Sphere(	{ 5.0, 0.0, 3.0 }, 3.0,
					{ 0.0, 0.0, 1.0 },
					{ 1.0, 0.0, 0.0 })
	);
	objects.push_back(
		new Sphere(	{ 0.0, 0.0, 1.0 }, 50.0,
					{ 0.0, 0.0, 0.0 },
					{ 0.7, 0.7, 0.7 })
	);*/

	//Emitting sphere, line of spheres below
	/*objects.push_back(
		new Sphere(	{ 0.0, 2.0, 0.0 }, 0.5,
					{ 1.0, 1.0, 1.0 },
					{ 1.0, 1.0, 1.0 })
	);
	objects.push_back(
		new Sphere(	{ 0.0, 1.0, 0.0 }, 0.3,
					{ 1.0, 0.0, 0.0 })
	);
	objects.push_back(
		new Sphere(	{ 0.0, 0.0, 0.0 }, 0.4,
					{ 0.0, 1.0, 0.0 })
	);
	objects.push_back(
		new Sphere(	{ 0.0, -1.0, 0.0 }, 0.5,
					{ 0.0, 0.0, 1.0 })
	);*/

	//Project logo
	/*
	objects.push_back(
		new Sphere(	{ 0.0, 2.0, 0.0 }, 1.0,
					{ 1.0, 1.0, 1.0 },
					{ 1.0, 1.0, 1.0 })
	);
	objects.push_back(
		new Sphere(	{ -2.0, 1.0, 0.0 }, 0.4,
					{ 1.0, 0.0, 0.0 })
	);
	objects.push_back(
		new Sphere(	{ -1.1, 0.3, 0.0 }, 0.4,
					{ 1.0, 1.0, 0.0 })
	);
	objects.push_back(
		new Sphere(	{ 0.0, 0.0, 0.0 }, 0.4,
					{ 0.0, 1.0, 0.0 })
	);
	objects.push_back(
		new Sphere(	{ 1.1, 0.3, 0.0 }, 0.4,
					{ 0.0, 1.0, 0.1 })
	);
	objects.push_back(
		new Sphere(	{ 2.0, 1.0, 0.0 }, 0.4,
					{ 0.0, 0.0, 0.1 })
	);*/

	//Cornell box
	/*
	objects.push_back(
		new Sphere(	{ 0.0, -10.0, 5.0 }, 7.1,
					{ 1.0, 1.0, 1.0 })
	);//Floor
	objects.push_back(
		new Sphere(	{ 0.0, 10.0, 5.0 }, 5.0,
					{ 1.0, 1.0, 1.0 },
					{ 1.0, 1.0, 1.0 })
	);//Ceiling lamp
	objects.push_back(
		new Sphere(	{ 0.0, 0.0, 15.0 }, 7.1,
					{ 1.0, 1.0, 1.0 })
	);//Back wall
	objects.push_back(
		new Sphere(	{ -10.0, 0.0, 5.0 }, 7.1,
					{ 1.0, 0.0, 0.0 })
	);//Left wall, red
	objects.push_back(
		new Sphere(	{ 10.0, 0.0, 5.0 }, 7.1,
					{ 0.0, 1.0, 0.0 })
	);//Right wall, green
	objects.push_back(
		new Sphere(	{ 1.0, -1.0, 3.5 }, 0.9,
					{ 1.0, 1.0, 1.0 })
	);//Right sphere, small
	objects.push_back(
		new Sphere(	{ -1.0, 0.0, 6.0 }, 1.2,
					{ 1.0, 1.0, 1.0 })
	);//Left sphere, larger
	*/

	//main_Samples(1, 4, 100);
	main_Image(200, 200);
	//main_SinglePixel(100);
	//main_VariedSampling(1, 2, 15);
	std::cout << "Done" << std::endl; cin.get();	
	return 0;
}