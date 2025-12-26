#include <opencv2/opencv.hpp>
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>

//#define DEBUG

int leftBound = 763 * 0.3125;
int rightBound = 1796 * 0.3125;
int barHeight = 541;

const int MAX_HOLD = 140;   // ms (original value = 90)
const int MIN_HOLD = 12;    // (original value = 12)
const float GAIN   = 0.75f;  // sensitivity (tune this) (original value = 0.6f)

typedef struct {
	cv::Point matchLocation;

	int width, height;
	HWND robloxHandle;
	cv::Mat mat;
	cv::Mat matGray;
} State_t;


void initState(State_t* state) {
	HWND robloxHandle = FindWindowW(NULL, L"Roblox");

	RECT clientRect;
	if(!GetClientRect(robloxHandle, &clientRect)) {
		std::cerr << "COULDNT GET GAME AREA" << std::endl;
	}
	int width, height;
	width = clientRect.right - clientRect.left;
	height = clientRect.bottom - clientRect.top;
	std::cout << "WIDTH: " << width << " HEIGHT: " << height << std::endl;


	HDC robloxDC = GetWindowDC(robloxHandle);
 	HDC compatDC = CreateCompatibleDC(robloxDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(robloxDC, width, height);
    SelectObject(compatDC, hBitmap);

    BitBlt(compatDC, 0, 0, width, height, robloxDC, 0, 0, SRCCOPY);

    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    cv::Mat mat(height, width, CV_8UC3);
    GetDIBits(compatDC, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DeleteObject(hBitmap);
    DeleteDC(compatDC);
    ReleaseDC(NULL, robloxDC);

	state->robloxHandle = robloxHandle;
	state->width = width;
	state->height = width;

	state->mat = mat;
	cv::cvtColor(mat, state->matGray, cv::COLOR_BGR2GRAY);
}


void updateState(State_t* state) {
	HDC robloxDC = GetWindowDC(state->robloxHandle);
 	HDC compatDC = CreateCompatibleDC(robloxDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(robloxDC, state->width, state->height);
    SelectObject(compatDC, hBitmap);

    BitBlt(compatDC, 0, 0, state->width, state->height, robloxDC, 0, 0, SRCCOPY);

    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = state->width;
    bi.biHeight = -state->height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;

    cv::Mat mat(state->height, state->width, CV_8UC3);
	if(mat.empty()) {
		std::cerr << "MAT IS EMPTY\n";
	}

    GetDIBits(compatDC, hBitmap, 0, state->height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DeleteObject(hBitmap);
    DeleteDC(compatDC);
    ReleaseDC(NULL, robloxDC);

	state->mat = mat;
	cv::cvtColor(mat, state->matGray, cv::COLOR_BGR2GRAY);
}


bool matchTemplate(State_t* state, cv::Mat templ) {

    cv::Mat res;
    cv::matchTemplate(state->mat, templ, res, cv::TM_CCOEFF_NORMED);

	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(res, &minVal, &maxVal, &minLoc, &maxLoc);

	//std::cout << "Best match score: " << maxVal << std::endl;

	double threshold = 0.9;
	if (maxVal >= threshold) {
		state->matchLocation = maxLoc;
		//std::cout << "Template matched at: (" << maxLoc.x << ", " << maxLoc.y << ")" << std::endl;
		return true;
	} else {
		return false;
	}
	
}


bool matchTemplateGrayscale(State_t* state, cv::Mat templ) {

    cv::Mat res;
    cv::matchTemplate(state->matGray, templ, res, cv::TM_CCOEFF_NORMED);

	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(res, &minVal, &maxVal, &minLoc, &maxLoc);

	std::cout << "Best match score: " << maxVal << std::endl;

	double threshold = 0.65;
	if (maxVal >= threshold) {
		state->matchLocation = maxLoc;
		//std::cout << "Template matched at: (" << maxLoc.x << ", " << maxLoc.y << ")" << std::endl;
		return true;
	} else {
		return false;
	}
	
}

cv::Vec3b getPixelColor(State_t* state, int x, int y) {
	cv::Vec3b color = state->mat.at<cv::Vec3b>(cv::Point(x,y));
	
	return color;
}

void MouseLeftDown() {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

void MouseLeftUp() {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void precisionDelay(int ms) {
	auto start = std::chrono::high_resolution_clock::now();
	while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() < ms) {
		// nothing
	}
}

void holdLeftClick(int ms) {
	MouseLeftDown();
	
	precisionDelay(ms);

	MouseLeftUp();
}

int main() {
	std::cout << "Starting program..." << std::endl;

	cv::Mat textTemplate = cv::imread("..\\text.png");
   	if (textTemplate.empty()) {
        std::cerr << "Failed to load mouse template image!" << std::endl;
        return -1;
	}

	cv::Mat textTemplateGRAY;
	cv::cvtColor(textTemplate, textTemplateGRAY, cv::COLOR_BGR2GRAY);

	State_t state;
	initState(&state);
	

	int midPoint, fishLoc;

	Sleep(5000);

	bool running = true;
    while (running) {
		if(GetAsyncKeyState('q') & 0x8000){
			running = false;
			break;
		}

		Sleep(400);
		
		while(!matchTemplateGrayscale(&state, textTemplateGRAY)) {
			std::cout << "Checking if fisching..." << std::endl;
			holdLeftClick(200);
			Sleep(200);
			updateState(&state);
		}

		bool fisching = true;
		while(fisching) {

			updateState(&state);

			std::cout << "--- FISCHING ---" << std::endl;

			int fishPos;
			int whiteBarLBound, whiteBarRBound;
			bool first = false;
			for(int i = leftBound; i < rightBound; i++) {
				cv::Vec3b color = getPixelColor(&state, i, barHeight);
				int red = color[2];
				int green = color[1];
				int blue = color[0];

				if(red > 150 && green > 150 && blue > 150) {
					if(!first) {
						whiteBarLBound = i;
						first = true;
					} else {
						whiteBarRBound = i;
					}
				}

				// fish is inside the bar
				if(red == 67 && green == 75 && blue == 91) {
					fishPos = i;
				}
			}

			#ifdef DEBUG
			std::cout << "fishPOS: " << fishPos << std::endl;
			std::cout << "POS: " << whiteBarLBound << " : " << whiteBarRBound << std::endl;
			#endif

			int barCenter = (whiteBarLBound + whiteBarRBound) / 2;

			// distance from fish to nearest bar edge
			int distLeft  = whiteBarLBound - fishPos;
			int distRight = fishPos - whiteBarRBound;

			static int lastBarCenter = barCenter;
			int barVelocity = barCenter - lastBarCenter;
			

			int barWidth = whiteBarRBound - whiteBarLBound - 10;
			int deadZone = barWidth * 0.12 + abs(barVelocity) * 2;

			int error = fishPos - barCenter;

			#ifdef DEBUG
			std::cout << "Error: " << error << std::endl;
			#endif

			// on the fish
			// --- DEAD ZONE ---
			if (abs(error) < deadZone) {
				holdLeftClick(12); // minimal stabilization
			}

			// --- FISH RIGHT: HOLD ---
			if (error > 0) {
				int holdTime = std::clamp((int)(error * GAIN), MIN_HOLD, MAX_HOLD);
				holdLeftClick(holdTime);
			}

			// --- FISH LEFT: RELEASE ---
			if (error < 0) {
				int releaseTime = std::clamp((int)(-error * 1.1f), 8, 70);
				Sleep(releaseTime);
			}

			lastBarCenter = barCenter;

			if(!matchTemplateGrayscale(&state, textTemplateGRAY)) {
				fisching = false;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
