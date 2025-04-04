#include <iostream>
#include <fstream>
#include <cmath>
#include "math.h"

using namespace std;

#define PI 3.14159265

float data[2048];

/* tri1 ---------------------------------------------------- */

void calculateLfoTri1() {
	float inc = 1.0f / 1024;
	
	for (uint16_t i = 0; i < 1024; i++) {
		data[i] = (inc * i);
		// printf("%f, ", (inc * i));
	}
	
	for (uint16_t i = 0; i < 1024; i++) {
		data[i + 1024] = 1.0f - (inc * i);
		// printf("%f, ", 1.0f - (inc * i));
	}
}

/* tri2 ---------------------------------------------------- */

void calculateLfoTri2() {
	float inc = 1.0f / 1024;
	
	for (uint16_t i = 0; i < 1024; i++) {
		data[i] = 1.0f - (inc * i);
		// printf("%f, ", (inc * i));
	}
	
	for (uint16_t i = 0; i < 1024; i++) {
		data[i + 1024] = (inc * i);
		// printf("%f, ", 1.0f - (inc * i));
	}
}

/* ramp1 --------------------------------------------------- */

void calculateLfoRamp1() {
	float inc = 1.0f / 2000;
	
	for (uint16_t i = 0; i < 2000; i++) {
		data[i] = (inc * i);
	}
	
	inc = 1.0f / 48;
	
	for (uint16_t i = 0; i < 48; i++) {
		data[2000 + i] = 1.0f - (inc * i);
	}
}

/* ramp2 --------------------------------------------------- */

void calculateLfoRamp2() {
	float inc = 1.0f / 48;
	
	for (uint16_t i = 0; i < 48; i++) {
		data[i] = (inc * i);
	}
		
	inc = 1.0f / 2000;
	
		for (uint16_t i = 0; i < 2000; i++) {
			data[48 + i] = 1.0f - (inc * i);
		}
}

/* dome1 --------------------------------------------------- */

void calculateLfoDome1() {
	for (uint16_t i = 0; i < 2048; i++) {
		float ang = 180.0f / 2048;
		data[i] = sin(ang * i * PI / 180);
	}
}

/* dome2 --------------------------------------------------- */

void calculateLfoDome2() {
	for (uint16_t i = 0; i < 2048; i++) {
		float ang = 180.0f / 2048;
		data[i] = sin((ang * i + 180) * PI / 180) + 1.0;
	}
}

/* sine1 --------------------------------------------------- */

void calculateLfoSine1() {
	for (uint16_t i = 0; i < 2048; i++) {
		float ang = 360.0f / 2048;
		data[i] = (sin(ang * i * PI / 180) * 0.5) + 0.5;
	}
}

/* sine2 --------------------------------------------------- */

void calculateLfoSine2() {
	for (uint16_t i = 0; i < 1024; i++) {
		float ang = 360.0f / 1024;
		data[i] = (sin(ang * i * PI / 180) * 0.5) + 0.5;
	}
	for (uint16_t i = 0; i < 1024; i++) {
		float ang = 720.0f / 1024;
		data[1024 + i] = (sin(ang * i * PI / 180) * 0.5) + 0.5;
	}
}

/* sine3 --------------------------------------------------- */

void calculateLfoSine3() {
	for (uint16_t i = 0; i < 2048; i++) {
		float mul = 1.0f / 2048;
		float ang = 1440.0f / 2048;
		data[i] = (sin(ang * i * PI / 180) * 0.5 * (1.0 - (mul * i))) + 0.5;
	}
}

/* sine4 --------------------------------------------------- */

void calculateLfoSine4() {
	for (uint16_t i = 0; i < 1024; i++) {
		float mul = 1.0f / 1024;
		float ang = 1440.0f / 1024;
		data[i] = (sin(ang * i * PI / 180) * 0.5 * (1.0 - (mul * i))) + 0.5;
	}
	
	for (uint16_t i = 1024; i > 0; i--) {
		float mul = 1.0f / 1024;
		float ang = 1440.0f / 1024;
		data[1024 + (1024 - i)] = (sin(ang * i * PI / 180) * 0.5 * (1.0 - (mul * i))) + 0.5;
	}
}

/* pulse1 --------------------------------------------------- */

void calculateLfoPulse1() {
	for (int i = 0;i < 512;i++) {
		data[i] = 0.0f;
	}
	for (int i = 0;i < 512;i++) {
		data[512 + i] = 0.5;
	}
	for (int i = 0;i < 512;i++) {
		data[1024 + i] = 0.0;
	}
	for (int i = 0;i < 512;i++) {
		data[1536 + i] = 1.0;
	}
}

/* pulse2 --------------------------------------------------- */

void calculateLfoPulse2() {
	for (int i = 0;i < 256;i++) {
		data[i] = 0.0f;
	}
	for (int i = 0;i < 256;i++) {
		data[256 + i] = 0.25f;
	}
	for (int i = 0;i < 256;i++) {
		data[512 + i] = 0.0f;
	}
	for (int i = 0;i < 256;i++) {
		data[768 + i] = 0.50f;
	}
	for (int i = 0;i < 256;i++) {
		data[1024 + i] = 0.0f;
	}
	for (int i = 0;i < 256;i++) {
		data[1280 + i] = 0.75f;
	}
	for (int i = 0;i < 256;i++) {
		data[1536 + i] = 0.0f;
	}
	for (int i = 0;i < 256;i++) {
		data[1792 + i] = 1.0f;
	}
}

/* pulse3 --------------------------------------------------- */

void calculateLfoPulse3() {
	for (int i = 0;i < 256;i++) {
		data[i] = 1.0f;
	}
	for (int i = 0;i < 256;i++) {
		data[256 + i] = 0.0f;
	}
	for (int i = 0;i < 256;i++) {
		data[512 + i] = 0.75f;
	}
	for (int i = 0;i < 256;i++) {
		data[768 + i] = 0.0f;
	}
	for (int i = 0;i < 256;i++) {
		data[1024 + i] = 0.50f;
	}
	for (int i = 0;i < 256;i++) {
		data[1280 + i] = 0.0f;
	}
	for (int i = 0;i < 256;i++) {
		data[1536 + i] = 0.25f;
	}
	for (int i = 0;i < 256;i++) {
		data[1792 + i] = 0.0f;
	}
}

/* pulse4 --------------------------------------------------- */

void calculateLfoPulse4() {
	for (int i = 0;i < 256;i++) {
		data[i] = 0.20f;
	}
	for (int i = 0;i < 256;i++) {
		data[256 + i] = 0.50f;
	}
	for (int i = 0;i < 256;i++) {
		data[512 + i] = 0.30f;
	}
	for (int i = 0;i < 256;i++) {
		data[768 + i] = 1.00f;
	}
	for (int i = 0;i < 256;i++) {
		data[1024 + i] = 0.60f;
	}
	for (int i = 0;i < 256;i++) {
		data[1280 + i] = 0.20f;
	}
	for (int i = 0;i < 256;i++) {
		data[1536 + i] = 0.80f;
	}
	for (int i = 0;i < 256;i++) {
		data[1792 + i] = 0.50f;
	}
}

/* shot1 ---------------------------------------------------- */

void calculateLfoShot1() {
	for (int i = 0;i < 256; i++) {
		float ang = float(180) / 256;
		data[i] = sin(ang * i * PI / 180);
	}
	for (int i = 0;i < 1536; i++) {
		float ang = float(90) / 1536;
		data[256 + i] = sin((ang * i + 270) * PI / 180) * 0.75 + 0.75;
	}
	for (int i = 0;i < 256; i++) {
		float ang = float(90) / 256;
		data[1792 + i] = sin((ang * i + 90) * PI / 180) * 0.75;
	}
}

/* shot2 ---------------------------------------------------- */

void calculateLfoShot2() {
	for (int i = 0;i < 128;i++) {
		float ang = float(180) / 128;
		data[i] = sin(ang * i * PI / 180) * 1.00;
	}
	for (int i = 0;i < 256;i++) {
		float ang = float(180) / 256;
		data[128 + i] = sin(ang * i * PI / 180) * 0.80;
	}
	for (int i = 0;i < 512;i++) {
		float ang = float(180) / 512;
		data[384 + i] = sin(ang * i * PI / 180) * 0.60;
	}
	for (int i = 0;i < 1152;i++) {
		float ang = float(180) / 1152;
		data[896 + i] = sin(ang * i * PI / 180) * 0.40;
	}
}

/* half1 --------------------------------------------------- */

void calculateLfoHalf1() {
	for (uint16_t i = 0; i < 1024; i++) {
		float ang = 360.0f / 1024;
		data[i] = (sin(ang * i * PI / 180) * 0.5) + 0.5;
	}
	for (uint16_t i = 0; i < 1024; i++) {
		data[1024 + i] = 0.5;
	}
}

/* half2 --------------------------------------------------- */

void calculateLfoHalf2() {
	for (uint16_t i = 0; i < 2048; i++) {
		float mul = 1.0f / 2048;
		float ang = 1440.0f / 2048;
		if (i < 1024) {
			data[i] = 0.5;
		} else {
			data[i] = (sin(ang * i * PI / 180) * 0.5 * (mul * i)) + 0.5;
		}
	}
}

/* half3 --------------------------------------------------- */

void calculateLfoHalf3() {
	for (uint16_t i = 0; i < 1024; i++) {
		float mul = 1.0f / 1024;
		float ang = 1440.0f / 1024;
		data[i] = (sin(ang * i * PI / 180) * 0.5 * (1.0 - (mul * i))) + 0.5;
	}
	
	for (uint16_t i = 0; i < 1024; i++) {
		data[1024 + i] = 0.5;
}
}

/* half4 --------------------------------------------------- */

void calculateLfoHalf4() {
	for (uint16_t i = 0; i < 2048; i++) {
		float ang = 180.0f / 2048;
		data[i] = sin((ang * i + 180) * PI / 180) + 1.0;
		if (data[i] > 0.5) data[i] = 0.5;
	}
}


using namespace std;
int main(int argc, char *argv[]) {
		
	calculateLfoHalf4();
	
	for (uint16_t i = 0; i < 2048; i++) {
		printf("%f, ", data[i]);
	}

}