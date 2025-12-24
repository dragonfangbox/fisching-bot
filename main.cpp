#include <opencv2/opencv.hpp>
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>

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


void click() {
	mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	Sleep(5);
	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}


void clickHold(int ms) {
	mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	Sleep(ms);
	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}


int main() {
	std::cout << "Starting program..." << std::endl;

  	cv::Mat leftArrowTemplate = cv::imread("..\\fish-left.png");
	if (leftArrowTemplate.empty()) {
        std::cerr << "Failed to load left arrow template image!" << std::endl;
        return -1;
    }

	cv::Mat rightArrowTemplate = cv::imread("..\\fish-right.png");
   	if (rightArrowTemplate.empty()) {
        std::cerr << "Failed to load right arrow template image!" << std::endl;
        return -1;
	}
	
	cv::Mat fishTemplate = cv::imread("..\\fish.png");
	cv::Mat fishTemplateResized;
	cv::resize(fishTemplate, fishTemplateResized, cv::Size(46 * 0.3125, 49 * 0.3125));
   	if (rightArrowTemplate.empty()) {
        std::cerr << "Failed to load fish template image!" << std::endl;
        return -1;
	}

	cv::Mat leftBoundTemplate = cv::imread("..\\left-bound.png");
   	if (rightArrowTemplate.empty()) {
        std::cerr << "Failed to load leftBound template image!" << std::endl;
        return -1;
	}


	cv::Mat textTemplate = cv::imread("..\\text.png");
   	if (rightArrowTemplate.empty()) {
        std::cerr << "Failed to load mouse template image!" << std::endl;
        return -1;
	}

	cv::Mat leftArrowTemplateGRAY, rightArrowTemplateGRAY, fishTemplateGRAY, leftBoundTemplateGRAY, textTemplateGRAY, fishTemplateResizedGRAY;
	cv::cvtColor(leftArrowTemplate, leftArrowTemplateGRAY, cv::COLOR_BGR2GRAY);
	cv::cvtColor(rightArrowTemplate, rightArrowTemplateGRAY, cv::COLOR_BGR2GRAY);
	cv::cvtColor(fishTemplate, fishTemplateGRAY, cv::COLOR_BGR2GRAY);
	cv::cvtColor(leftBoundTemplate, leftBoundTemplateGRAY, cv::COLOR_BGR2GRAY);
	cv::cvtColor(textTemplate, textTemplateGRAY, cv::COLOR_BGR2GRAY);

	cv::cvtColor(fishTemplateResized, fishTemplateResizedGRAY, cv::COLOR_BGR2GRAY);

	

	State_t state;
	initState(&state);
	
	cv::Point rLoc, lLoc;
	int leftBound = 763 * 0.3125;
	int rightBound = 1796 * 0.3125;
	int barHeight = 541;

	int midPoint, fishLoc;

	Sleep(5000);

	bool running = true;
    while (running) {
		if(GetAsyncKeyState('Q') & 0x8000){
			running = false;
		}

		holdLeftClick(300);
		
		while(!matchTemplateGrayscale(&state, textTemplateGRAY)) {
			std::cout << "Checking if fisching..." << std::endl;
			holdLeftClick(100);
			updateState(&state);
		}

		bool fisching = true;
		while(fisching) {

			updateState(&state);

		
			
			// if(leftBound == 0) {
			// 	if(matchTemplateGrayscale(&state, leftBoundTemplateGRAY)) {
			// 		leftBound = state.matchLocation.x;
			// 	}
			//}

			std::cout << "--- FISCHING ---" << std::endl;

			// cv::imshow("HELlo", state.mat);

			// cv::waitKey(0);
			int fishPos;
			int whiteBarLBound, whiteBarRBound;
			bool first = false;
			for(int i = leftBound; i < rightBound; i++) {
				cv::Vec3b color = getPixelColor(&state, i, barHeight);
				int red = color[2];
				int green = color[1];
				int blue = color[0];
		//		std::cout << "COLOR: " << color << std::endl;
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

			std::cout << "fishPOS: " << fishPos << std::endl;
			std::cout << "POS: " << whiteBarLBound << " : " << whiteBarRBound << std::endl;

			int barCenter = (whiteBarLBound + whiteBarRBound) / 2;

			// distance from fish to nearest bar edge
			int distLeft  = whiteBarLBound - fishPos;
			int distRight = fishPos - whiteBarRBound;

			static int lastBarCenter = barCenter;
			int barVelocity = barCenter - lastBarCenter;
			

			int barWidth = whiteBarRBound - whiteBarLBound - 10;
			int deadZone = barWidth * 0.12 + abs(barVelocity) * 2;

			int error = fishPos - barCenter;

			const int MAX_HOLD = 130;   // ms
			const int MIN_HOLD = 12;
			const float GAIN   = 1.4f;  // sensitivity (tune this)

		//	std::cout << "Error: " << error << std::endl;

			// on the fish
			// --- DEAD ZONE ---
			if (abs(error) < deadZone) {
				holdLeftClick(12); // minimal stabilization
			}

			// --- FISH RIGHT: HOLD ---
			if (error > 0) {
				int holdTime = std::clamp((int)(error * 0.6f), 12, 90);
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