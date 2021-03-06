#include "common/PolyModel.h"
#include <stdio.h>
#include "MarchingCubes.h"
#include "fparser.h"

#define NFUNS 15

enum Axis { X = 0, Y = 1, Z = 2 };

// original/topological MC switch
int originalMC = 0;

// grid extension
float xmin = -1.0f, xmax = 1.0f, ymin = -1.0f, ymax = 1.0f, zmin = -1.0f,
zmax = 1.0f;

// implicit formula
char formula[1024];

// implicit functions
const char *fun_list[NFUNS] = {
	"Type Formula", "Sphere",    "Ellipsoid", "Hyperboloid", "Plane",
	"Cubic",        "Cushin",    "Cassini",   "Blooby",      "Chair",
	"Cyclide",      "2 Spheres", "2 Torii",   "Heart",       "Helio" };

// implicit functions
const char *fun_def[NFUNS] = {
	"f(x,y,z, c,i)", "x^2+y^2+z^2-0.49", "2*x^2+y^2+z^2-0.49",
	"2*x^2-y^2-z^2-0.49", "x+y+z", "4*y^2-8*x^3+2*x",
	"(1.5*z)^2*(1.5*x)^2 - (1.5*z)^4 - 2*(1.5*z)*(1.5*x)^2 + 2*(1.5*z)^3 + "
	"(1.5*x)^2 - (1.5*z)^2 - ((1.5*x)^2 - (1.5*z))*((1.5*x)^2 - (1.5*z)) - "
	"(1.5*y)^4 - 2*(1.5*x)^2*(1.5*y)^2 - (1.5*y)^2*(1.5*z)^2 + "
	"2*(1.5*y)^2*(1.5*z) + (1.5*y)^2",
	"((1.7*x)^2 + (1.7*y)^2 + (1.7*z)^2 + 0.45^2)*((1.7*x)^2 + (1.7*y)^2 + "
	"(1.7*z)^2 + 0.45^2) - 16*0.45^2*((1.7*x)^2 + (1.7*z)^2) - 0.25",
	"(3*x)^4 - 45*x^2+ (3*y)^4 - 45*y^2 + (3*z)^4 - 45*z^2 + 11.8",
	"((5*x)^2+(5*y)^2+(5*z)^2-0.95*25)*((5*x)^2+(5*y)^2+(5*z)^2-0.95*25)-0.8*(("
	"(5*z)-5)^2-2*(5*x)^2)*(((5*z)+5)^2-2*(5*y)^2)",
	"(25 - (6.9)^2)*(25 - (2.9)^2)*((10*x+4)^4+(10*y)^4+(10*z)^4)+ 2*((25 - "
	"(6.9)^2 )*(25 - (2.9)^2) * "
	"((10*x+4)^2*(10*y)^2+(10*x+4)^2*(10*z)^2+(10*y)^2*(10*z)^2))+ "
	"18*((21+4.9^2)* (4*(10*x+4)+9))*((10*x+4)^2+(10*y)^2+(10*z)^2)+ "
	"4*3^4*(2*(10*x+4))*(-9+2*(10*x+4))+4*3^4*4.9^2*(10*y)^2+3^8",
	"((x-0.31)^2+(y-0.31)^2+(z-0.31)^2-0.263) * "
	"((x+0.3)^2+(y+0.3)^2+(z+0.3)^2-0.263)",
	"( ( (8*x)^2 + (8*y-2)^2 + (8*z)^2 + 16 - 1.85*1.85 ) * ( (8*x)^2 + "
	"(8*y-2)^2 + (8*z)^2 + 16 - 1.85*1.85 ) - 64 * ( (8*x)^2 + (8*y-2)^2 ) ) * "
	"( ( (8*x)^2 + ((8*y-2)+4)*((8*y-2)+4) + (8*z)^2 + 16 - 1.85*1.85 ) * ( "
	"(8*x)^2 + ((8*y-2)+4)*((8*y-2)+4) + (8*z)^2 + 16 - 1.85*1.85 ) - 64 * ( "
	"((8*y-2)+4)*((8*y-2)+4) + (8*z)^2 ) ) + 1025",
	"(2*(1.3*x)^2+(1.3*y)^2+(1.3*z)^2-1)^3-(1/"
	"10)*(1.3*x)^2*(1.3*z)^3-(1.3*y)^2*(1.3*z)^3",
	"4*y^2-8*x^3+2*x",
};

// chosen implicit function
int curr_string = -1;

bool run(MarchingCubes &mc, float isoval)
{
	strcpy(formula, fun_def[9]);
	if (strlen(formula) <= 0) return false;

	// Parse formula
	FunctionParser fparser;
	fparser.Parse((const char *)formula, "x,y,z,c,i");
	if (fparser.EvalError()) {
		printf("parse error\n");
		return false;
	}

	float rx = (xmax - xmin) / (mc.size().x - 1);
	float ry = (ymax - ymin) / (mc.size().y - 1);
	float rz = (zmax - zmin) / (mc.size().z - 1);
	glm::vec3 min_pos(xmin, ymin, zmin);
	glm::vec3 range(rx, ry, rz);
	for (int i = 0; i < mc.size().x; i++) {
		float val[5];
		val[X] = (float)i * rx + xmin;
		for (int j = 0; j < mc.size().y; j++) {
			val[Y] = (float)j * ry + ymin;
			for (int k = 0; k < mc.size().z; k++) {
				val[Z] = (float)k * rz + zmin;

				auto w = fparser.Eval(val) - isoval;
				mc.set_data(w, glm::ivec3(i, j, k));
			}
		}
	}

	// Run MC
	mc.SetAlgorithm(MarchingCubes::TopologicalMarchingCubes);
	mc.run();

	// Rescale positions
	for (int i = 0; i < mc.nverts(); ++i) {
		Vertex &v = mc.vertices()[i];
		v.pos = range * v.pos + min_pos;
		v.n = glm::normalize(v.n);
	}

	return true;
}

int main() {

	MarchingCubes mc(glm::ivec3(50));
	mc.Setup();
	run(mc, 100);

	auto verts = std::vector<glm::vec3>();
	verts.reserve(mc.ntrigs() * 3);

	rk9::PolyModel m;

	for (int i = 0; i < mc.ntrigs(); ++i) {
		auto tri = mc.trig(i);
		rk9::Point p[3];
		for (int t = 0; t < 3; ++t) {
			auto vertex = mc.vert(tri.ids[t]).pos;
			p[t].X = vertex.x;
			p[t].Y = vertex.y;
			p[t].Z = vertex.z;
		}
		m.AddTriangle(p[0], p[1], p[2]);
	}

	m.WriteToSTLFile("output.stl");

	return 0;
}