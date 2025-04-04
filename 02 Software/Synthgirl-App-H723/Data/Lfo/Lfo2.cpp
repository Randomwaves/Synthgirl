#include <iostream>
#include <fstream>
#include <cmath>
#include "math.h"

using namespace std;

#define PI 3.14159265

const char targetName[] = "Lfo.rws";

float data[25000];

/* tri1 ---------------------------------------------------- */

void calculateLfoTri1() {
	float inc = 1.0f / 12500;
	
	for (uint16_t i = 0; i < 12500; i++) {
		data[i] = (inc * i);
		// printf("%f, ", (inc * i));
	}
	
	for (uint16_t i = 0; i < 12500; i++) {
		data[i + 12500] = 1.0f - (inc * i);
		// printf("%f, ", 1.0f - (inc * i));
	}
}

/* tri2 ---------------------------------------------------- */

void calculateLfoTri2() {
	float inc = 1.0f / 12500;
	
	for (uint16_t i = 0; i < 12500; i++) {
		data[i] = 1.0f - (inc * i);
		// printf("%f, ", (inc * i));
	}
	
	for (uint16_t i = 0; i < 12500; i++) {
		data[i + 12500] = (inc * i);
		// printf("%f, ", 1.0f - (inc * i));
	}
}

/* ramp1 --------------------------------------------------- */

void calculateLfoRamp1() {
	float inc = 1.0f / 24000;
	
	for (uint16_t i = 0; i < 24000; i++) {
		data[i] = (inc * i);
	}
	
	inc = 1.0f / 1000;
	
	for (uint16_t i = 0; i < 1000; i++) {
		data[24000 + i] = 1.0f - (inc * i);
	}
}

/* ramp2 --------------------------------------------------- */

void calculateLfoRamp2() {
	float inc = 1.0f / 1000;
	
	for (uint16_t i = 0; i < 1000; i++) {
		data[i] = (inc * i);
	}
		
	inc = 1.0f / 24000;
	
		for (uint16_t i = 0; i < 24000; i++) {
			data[1000 + i] = 1.0f - (inc * i);
		}
}

/* dome1 --------------------------------------------------- */

void calculateLfoDome1() {
	for (uint16_t i = 0; i < 25000; i++) {
		float ang = 180.0f / 25000;
		data[i] = sin(ang * i * PI / 180);
	}
}

/* dome2 --------------------------------------------------- */

void calculateLfoDome2() {
	for (uint16_t i = 0; i < 25000; i++) {
		float ang = 180.0f / 25000;
		data[i] = sin((ang * i + 180) * PI / 180) + 1.0;
	}
}

/* sine1 --------------------------------------------------- */

void calculateLfoSine1() {
	for (uint16_t i = 0; i < 25000; i++) {
		float ang = 360.0f / 25000;
		data[i] = (sin(ang * i * PI / 180) * 0.5) + 0.5;
	}
}

/* sine2 --------------------------------------------------- */

void calculateLfoSine2() {
	for (uint16_t i = 0; i < 12500; i++) {
		float ang = 360.0f / 12500;
		data[i] = (sin(ang * i * PI / 180) * 0.5) + 0.5;
	}
	for (uint16_t i = 0; i < 12500; i++) {
		float ang = 720.0f / 12500;
		data[12500 + i] = (sin(ang * i * PI / 180) * 0.5) + 0.5;
	}
}

/* sine3 --------------------------------------------------- */

void calculateLfoSine3() {
	for (uint16_t i = 0; i < 25000; i++) {
		float mul = 1.0f / 25000;
		float ang = 1440.0f / 25000;
		data[i] = (sin(ang * i * PI / 180) * 0.5 * (1.0 - (mul * i))) + 0.5;
	}
}

/* sine4 --------------------------------------------------- */

void calculateLfoSine4() {
	for (uint16_t i = 0; i < 12500; i++) {
		float mul = 1.0f / 12500;
		float ang = 1440.0f / 12500;
		data[i] = (sin(ang * i * PI / 180) * 0.5 * (1.0 - (mul * i))) + 0.5;
	}
	
	for (uint16_t i = 12500; i > 0; i--) {
		float mul = 1.0f / 12500;
		float ang = 1440.0f / 12500;
		data[12500 + 12500 - i] = (sin(ang * i * PI / 180) * 0.5 * (1.0 - (mul * i))) + 0.5;
	}
}

using namespace std;
int main(int argc, char *argv[]) {
	
	ofstream target;
	target.open (targetName);
	target << "RW_SYNTHGIRL_LFO         ";
	
	calculateLfoTri1();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	calculateLfoTri2();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	calculateLfoRamp1();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	
	calculateLfoRamp2();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	calculateLfoDome1();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	calculateLfoDome2();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	calculateLfoSine1();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	calculateLfoSine2();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	calculateLfoSine3();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	calculateLfoSine4();
	for (uint16_t i = 0; i < 25000; i++) {
		target.write(reinterpret_cast<const char*>(&(data[i])), sizeof(float));
	}
	
	target << "EOF";
	target.close();

}