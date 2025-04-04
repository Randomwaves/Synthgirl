#include <iostream>
#include <cmath>
#include "math.h"

using namespace std;

#define PI 3.14159265

/* tri1 ---------------------------------------------------- */
void drawLfoTri1() {
	for (int i = 0;i < 512;i++) {
		cout << (128 * i) << ", ";
	}
	for (int i = 0;i < 512;i++) {
		cout << 65536 - (128 * i) << ", ";
	}
}

/* tri2 ---------------------------------------------------- */
void drawLfoTri2() {
	for (int i = 0;i < 512;i++) {
		cout << 65536 - (128 * i) << ", ";
	}
	for (int i = 0;i < 512;i++) {
		cout << (128 * i) << ", ";
	}
}

/* ramp1 --------------------------------------------------- */
void drawLfoRamp1() {
	for (int i = 0;i < 2048;i++) {
		cout << i * 32 << ", ";
	}
}

/* ramp2 --------------------------------------------------- */
void drawLfoRamp2() {
	for (int i = 1;i <= 2048;i++) {
		cout << 65536 - (i * 32) << ", ";
	}
}

/* dome1 --------------------------------------------------- */
void drawLfoDome1() {
	for (int i = 0;i < 1024;i++) {
		float ang = float(180) / 1024;
		cout << round(sin(ang * i * PI / 180) * 65536) << ", ";
	}
}

/* dome2 --------------------------------------------------- */
void drawLfoDome2() {
	for (int i = 0;i < 1024;i++) {
		float ang = float(180) / 1024;
		cout << round(sin((ang * i + 180) * PI / 180) * 65536) + 65536 << ", ";
	}
}


/* sine1 --------------------------------------------------- */
void drawLfoSine1() {
	for (int i = 0;i < 1024;i++) {
		float ang = float(360) / 1024;
		cout << round(sin(ang * i * PI / 180) * 32768) + 32768 << ", ";
	}
}

/* sine2 --------------------------------------------------- */
void drawLfoSine2() {
	for (int i = 0;i < 512;i++) {
		float ang = float(360) / 512;
		cout << round(sin((ang * i) * PI / 180) * 32768) + 32768 << ", ";
	}
	for (int i = 0;i < 512;i++) {
		float ang = float(720) / 512;
		cout << round(sin((ang * i) * PI / 180) * 32768) + 32768 << ", ";
	}
}

/* sine3 --------------------------------------------------- */
void drawLfoSine3() {
	for (int i = 0;i < 1024;i++) {
		float mul = float(1) / 1024;
		float ang = float(1440) / 1024;
		cout << round(sin(ang * i * PI / 180) * 32768 * (1 - (mul * i))) + 32768 << ", ";
	}
}

/* sine4 --------------------------------------------------- */
void drawLfoSine4() {
	for (int i = 0;i < 512;i++) {
		float mul = float(1) / 512;
		float ang = float(1440) / 512;
		cout << round(sin(ang * i * PI / 180) * 32768 * (1 - (mul * i))) + 32768 << ", ";
	}

	for (int i = 511;i >= 0;i--) {
		float mul = float(1) / 512;
		float ang = float(1440) / 512;
		cout << round(sin(ang * i * PI / 180) * 32768 * (1 - (mul * i))) + 32768 << ", ";
	}
}

/* pulse1 --------------------------------------------------- */
void drawLfoPulse1() {
		for (int i = 0;i < 256;i++) {
			cout << 65535 << ", ";
		}
		for (int i = 0;i < 256;i++) {
			cout << 0 << ", ";
		}
		for (int i = 0;i < 256;i++) {
			cout << 32768 << ", ";
		}
		for (int i = 0;i < 256;i++) {
			cout << 0 << ", ";
		}
}

/* pulse2 --------------------------------------------------- */
void drawLfoPulse2() {
	for (int i = 0;i < 128;i++) {
		cout << 0 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 16375 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 32750 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 49125 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 65500 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 49125 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 32750 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 16375 << ", ";
	}
}

/* pulse3 --------------------------------------------------- */
void drawLfoPulse3() {
	for (int i = 0;i < 128;i++) {
		cout << 16384 * 4 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 0 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 16384 * 3 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 0 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 16384 * 2 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 0 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 16384 * 1 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 0 << ", ";
	}
}

/* pulse4 --------------------------------------------------- */
void drawLfoPulse4() {
	for (int i = 0;i < 128;i++) {
		cout << 32768 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 49152 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 8192 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 40960 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 65535 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 16384 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 32768 << ", ";
	}
	for (int i = 0;i < 128;i++) {
		cout << 8192 << ", ";
	}
}

/* shot1 ---------------------------------------------------- */
void drawLfoShot1() {
	for (int i = 0;i < 128;i++) {
		float ang = float(180) / 128;
		cout << round(sin(ang * i * PI / 180) * 65536) << ", ";
	}
	for (int i = 0;i < 768;i++) {
		float ang = float(90) / 768;
		cout << round(sin((ang * i + 270) * PI / 180) * 65536 * 0.75) + (65536 * 0.75) << ", ";
	}
	for (int i = 0;i < 128;i++) {
		float ang = float(90) / 128;
		cout << round(sin((ang * i +90) * PI / 180) * 65536 * 0.75) << ", ";
	}
}

/* shot2 ---------------------------------------------------- */
void drawLfoShot2() {
	for (int i = 0;i < 64;i++) {
		float ang = float(180) / 64;
		cout << round(sin(ang * i * PI / 180) * 65536) << ", ";
	}
	for (int i = 0;i < 128;i++) {
		float ang = float(180) / 128;
		cout << round(sin(ang * i * PI / 180) * 65536 * 0.80)  << ", ";
	}
	for (int i = 0;i < 256;i++) {
		float ang = float(180) / 256;
		cout << round(sin(ang * i * PI / 180) * 65536 * 0.60) << ", ";
	}
	for (int i = 0;i < 576;i++) {
		float ang = float(180) / 576;
		cout << round(sin(ang * i * PI / 180) * 65536 * 0.40) << ", ";
	}
}

/* misc1 --------------------------------------------------- */
void drawLfoMisc1() {
	for (int i = 0;i < 128;i++) {
		float ang = float(180) / 512;
		cout << round(sin((ang * i + 315) * PI / 180) * 65536) + 65536 << ", ";
	}
	for (int i = 0;i < 512;i++) {
		float ang = float(180) / 512;
		cout << round(sin((ang * i + 180) * PI / 180) * 65536) + 65536 << ", ";
	}
	for (int i = 0;i < 384;i++) {
		float ang = float(180) / 512;
		cout << round(sin((ang * i + 180) * PI / 180) * 65536) + 65536 << ", ";
	}
}

/* misc2 --------------------------------------------------- */
void drawLfoMisc2() {
	for (int i = 0;i < 128;i++) {
		float ang = float(180) / 512;
		cout << round(sin((ang * i + 135) * PI / 180) * 65536) << ", ";
	}
	for (int i = 0;i < 512;i++) {
		float ang = float(180) / 512;
		cout << round(sin((ang * i) * PI / 180) * 65536) << ", ";
	}
	for (int i = 0;i < 384;i++) {
		float ang = float(180) / 512;
		cout << round(sin((ang * i) * PI / 180) * 65536) << ", ";
	}
}

/* misc3 --------------------------------------------------- */
void drawLfoMisc3() {
	for (int i = 0;i < 512;i++) {
		int num = (64 * i);
		((i / 64) % 2 != 0) ? num = -num : num = num;
		cout << num + 32768 << ", ";
	}
	for (int i = 0;i < 512;i++) {
		int num = 32768 - (64 * i);
		((i / 64) % 2 != 0) ? num = -num : num = num;
		cout << num + 32768 << ", ";
	}
}

/* misc4 --------------------------------------------------- */
void drawLfoMisc4() {
	for (int i = 0;i < 1024;i++) {
		float ang = float(360) / 1024;
		int num;
		((i / 64) % 2 != 0) ? num = round(sin((ang * i) * PI / 180) * 32768) : num = round(sin((ang * i + 180) * PI / 180) * 32768);
		cout << num + 32768 << ", ";
	}
}


int main(int argc, char *argv[]) {
	drawLfoRamp2();
}

