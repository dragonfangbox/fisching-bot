#include <opencv2/opencv.hpp>
#include <windows.h>
#include <iostream>

int main() {

	std::cout << "hello world!!";

	const cv::Mat lineTemplate = cv::imread("fish-line.jpg");
	if(lineTemplate.empty()) {
		std::cout << "Template not found!\n";
	}

	HWND robloxHandle = FindWindowA(NULL, "Roblox");
	if(!robloxHandle) {
		std::cout << "Roblox not found! Trying Again...\n";
		robloxHandle = FindWindowA(NULL, "Roblox Player");
	}

	if(!robloxHandle) {
		std::cout << "Roblox not found! Capturing the whole screen...\n";
		robloxHandle = NULL;
	}

	int running = 1;
	while(running == 1) {
		std::cout << "--- RUNNING ---\n";
		RECT clientRect;
		GetClientRect(robloxHandle, &clientRect);
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;

		POINT topLeft = {0, 0};
		ClientToScreen(robloxHandle, &topLeft);

		HDC hScreenDC = GetDC(NULL);
		HDC hcompatDC = CreateCompatibleDC(hScreenDC);
		HBITMAP hBitMap = CreateCompatibleBitmap(hScreenDC, width, height);
		SelectObject(hcompatDC, hBitMap);

		BitBlt(hcompatDC, 0, 0, width, height, hScreenDC, topLeft.x, topLeft.y, SRCCOPY);
		BITMAPINFOHEADER bi;
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = width;
		bi.biHeight = -height;
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
  		bi.biXPelsPerMeter = 0;
   		bi.biYPelsPerMeter = 0;
  	 	bi.biClrUsed = 0;
 	   	bi.biClrImportant = 0;

		cv::Mat mat(height, width, CV_8UC3);
		GetDIBits(hcompatDC, hBitMap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

		cv::Mat res;
		cv::matchTemplate(mat, lineTemplate, res, cv::TM_CCOEFF_NORMED);

		double thres = 0.8;
		cv::Mat mask;
		cv::threshold(res, mask, thres, 1.0, cv::THRESH_BINARY);
		for (int y = 0; y < res.rows; y++) {
				for (int x = 0; x < res.cols; x++) {
					if (res.at<float>(y, x) >= thres) {
						cv::rectangle(
							mat,
							cv::Point(x, y),
							cv::Point(x + lineTemplate.cols, y + lineTemplate.rows),
							cv::Scalar(0, 255, 0),
							2
						);
					}
				}
			}

		cv::imshow("Matched", mat);
		cv::waitKey(0);

		DeleteObject(hBitMap);
		DeleteDC(hcompatDC);
		ReleaseDC(NULL, hScreenDC);
		Sleep(100);
	}

	return 0;
}
	// RECT clientRect;
	// GetClientRect(robloxHandle, &clientRect);
	// int width = clientRect.right - clientRect.left;
	// int height = clientRect.bottom - clientRect.top;

	// POINT topLeft = {0, 0};
	// ClientToScreen(robloxHandle, &topLeft);

	// HDC hScreenDC = GetDC(NULL);
	// HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	// HBITMAP hBitMap = CreateCompatibleBitmap(hScreenDC, width, height);
	// SelectObject(hMemoryDC, hBitMap);

	// BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, topLeft.x, topLeft.y, SRCCOPY);
