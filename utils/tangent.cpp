#include <iostream>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <queue>

/** Basic GLFW header */
//#include <GL/glew.h>	// Important - this header must come before glfw3 header
//#include <glad/glad.h>
//#include <GLFW/glfw3.h>

/** GLFW Math */
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

void readvals(string filename, vector<float> & data) {

	string str; 
	ifstream in;
	stringstream ss;

	in.open(filename); 
	if (!in.is_open()) return;

	while (in) {
		float coord;
		in >> coord;
		data.push_back(coord);
		//cout << coord;
	}
}

void calc_tangent_space(vector<float> data) {

	if (data.size() < 20)
		return;

	glm::vec3 pos1, pos2, pos3, pos4;
	glm::vec2 uv1, uv2, uv3, uv4;

	// positions                                             // texture coordinat
	pos1.x = data[ 0]; pos1.y = data[ 1]; pos1.z = data[ 2]; uv1.x = data[ 3]; uv1.y = data[ 4];
	pos2.x = data[ 5]; pos2.y = data[ 6]; pos2.z = data[ 7]; uv2.x = data[ 8]; uv2.y = data[ 9];
	pos3.x = data[10]; pos3.y = data[11]; pos3.z = data[12]; uv3.x = data[13]; uv3.y = data[14];
	pos4.x = data[15]; pos4.y = data[16]; pos4.z = data[17]; uv4.x = data[18]; uv4.y = data[19];
	
	// normal vector
	//glm::vec3 nm(0.0f, 1.0f, 0.0f);

	// calculate tangent/bitangent vectors of both triangles
	glm::vec3 tangent1, bitangent1;
	glm::vec3 tangent2, bitangent2;
	// triangle 1
	// ----------
	glm::vec3 edge1 = pos2 - pos1;
	glm::vec3 edge2 = pos3 - pos1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent1 = glm::normalize(tangent1);

	bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent1 = glm::normalize(bitangent1);

	// triangle 2
	// ----------
	edge1 = pos3 - pos1;
	edge2 = pos4 - pos1;
	deltaUV1 = uv3 - uv1;
	deltaUV2 = uv4 - uv1;

	f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	tangent2 = glm::normalize(tangent2);

	bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
	bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
	bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
	bitangent2 = glm::normalize(bitangent2);
/**
	float quadVertices[] = {
	    // positions            // normal         // texcoords  // tangent                          // bitangent
	    pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
	    pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
	    pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

	    pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
	    pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
	    pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
	};
*/
	printf(" px   py   pz   tx   ty   tz   cx   cy   cz\n");
	printf("%4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f\n", pos1.x, pos1.y, pos1.z, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z);
	printf("%4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f\n", pos2.x, pos2.y, pos2.z, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z);
	printf("%4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f\n", pos3.x, pos3.y, pos3.z, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z);
	printf("%4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f\n", pos1.x, pos1.y, pos1.z, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z);
	printf("%4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f\n", pos3.x, pos3.y, pos3.z, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z);
	printf("%4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f\n", pos4.x, pos4.y, pos4.z, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z);
}

int main(int argc, char **argv) {

	vector<float> data;

	for (int i=1; i<argc; i++) {
		data.clear();
		readvals(string(argv[i]), data);
		while (data.size() >= 20) {
			calc_tangent_space(data);
			data.erase(data.begin(), data.begin() + 20);
		}
	}

	return 0;
}
