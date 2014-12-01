#include <stdio.h>
#include <stdlib.h>

//--- OpenGL ---//
#include "gl/glew.h" 
#include "gl/gl.h"

#pragma comment(lib, "gl/OpenGL32.Lib")
#pragma comment(lib, "gl/glew64.lib")

//-- Wintab をコントロールするクラス
#include "Tablet/CTablet.h" 

#define WIDTH 640 // キャンバスの解像度
#define HEIGHT 480
#define SIZE 10 // 描画する線の太さ

int * canvas; // 描画を行うキャンバス。int型配列にて実装

CTablet tablet; // wintabの初期化等を行うオブジェクト
TABLETPACKET p;
TABLETPACKET p_bk;

//--- 線の入力を行う関数---//
//マウスまたはタブレットからの入力
//に応じて、描き始め、描いている途中、描き終わった瞬間ごとに
//処理を行う
bool mousepressed = false;
void LDown(int x, int y);
void LUp(int x, int y);
void MMove(int x, int y);

void draw(int x, int y); //キャンバスに点を描画する
void getClientPos(int &x, int &y);

//---OpenGL の初期化,後処理---//
HWND hWnd;
HDC hDC;
HGLRC hRC;
static void OnCreate(HWND hWnd, HDC&, HGLRC&);
static void OnDestroy(HWND hWnd, HDC, HGLRC);
bool initGL();
void cleanUp();
void display();

LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

int main(int argc, char ** argv) {
	HINSTANCE hCurrInstance = GetModuleHandle(NULL);
	int nWinMode = SW_SHOW;
	WNDCLASSEX wc;

	wc.cbSize = sizeof (WNDCLASSEX);
	wc.hInstance = hCurrInstance;
	wc.lpszClassName = "Opengl Tablet";
	wc.lpfnWndProc = WindowProc;
	wc.style = 0;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	if (RegisterClassEx(&wc) == 0) {
		return 0;
	}
	hWnd = CreateWindow("Opengl Tablet",
		"Opengl Tablet",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0,
		0,
		WIDTH,
		HEIGHT,
		NULL,
		NULL,
		hCurrInstance,
		NULL);

	// OpenGLの設定．
	OnCreate(hWnd, hDC, hRC);
	if (!initGL())
		return 1;

	//タブレットの初期化
	tablet.Init(hWnd);
	ZeroMemory(&p_bk, sizeof(TABLETPACKET));

	//キャンバスの初期化
	canvas = (int*)malloc(sizeof(int)* (WIDTH * HEIGHT)); 
	memset(canvas, 0xFFFFFFFF, sizeof(int)* WIDTH * HEIGHT);

	ShowWindow(hWnd, nWinMode);
	while (TRUE) {
		MSG msg;
		if (GetMessage(&msg, 0, NULL, NULL)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) {
			cleanUp();
			OnDestroy(hWnd, hDC, hRC);
			break;
		}

		else {
			display();
			SwapBuffers(hDC); //バックバッファとフロントバッファの切り替えを行う。
							  //この関数を使うと、WT_PACKETの到着に遅延が生じるようになります。
							  //Tabletの上に入力をしてから、そのパケットがGetMessageを通じて
							  //届くまでに余計な時間がかかってしまいます。
		}
	}
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam) {
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);
	getClientPos(x, y);

	switch (nMessage) {
		//タブレットによる線の描画
		case WT_PACKET:
			if (tablet.Packet(wParam, lParam, &p)) { //Tabletからのパケットを取得。ここで既に処理を終えたはずのパケットが再び(wParamシリアル番号の値が既に来たもの)来てしまう。

				POINT pt;
				int x;
				int y;

				tablet.GetSysPos(p.x, p.y, &x, &y);

				pt.x = pt.y = 0;
				ClientToScreen(hWnd, &pt);

				x = x - pt.x;
				y = y - pt.y;

				getClientPos(x, y);

				if ((p.button & 1 ) && !(p_bk.button & 1)) {
					LDown(x, y);
				}
				else if(!(p.button & 1) && (p_bk.button & 1)){
					LUp(x, y);
				}

				if (p.x != p_bk.x || p.y != p_bk.y) {
					MMove(x, y);
				}
				p_bk = p;
			}
			break;
		//マウスによる線の描画
		case WM_LBUTTONDOWN:
			LDown(x, y);
			break;
		case WM_LBUTTONUP:
			LUp(x, y);
			break;
		case WM_MOUSEMOVE:
			MMove(x, y);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, nMessage, wParam, lParam);
	}
	return 0;
}

void LDown(int x, int y) {
	if (mousepressed)
		return;
	mousepressed = true;
	draw(x, y);
}

void LUp(int x, int y) {
	if (!mousepressed)
		return;
	mousepressed = false;
	draw(x, y);
}

void MMove(int x, int y) {
	if (mousepressed)
		draw(x, y);
}

void draw(int px, int py) {
	int x, y, index;
	for (int tip_y = 0; tip_y < SIZE; tip_y++) {
		for (int tip_x = 0; tip_x < SIZE; tip_x++) {
			x = tip_x + px - SIZE / 2;
			y = tip_y + py - SIZE / 2;
			//if (x < 0 || x > WIDTH || y < 0 || y > HEIGHT)
			//continue;
			index = x + y * WIDTH;
			if (index > 0 && index < WIDTH * HEIGHT)
				canvas[index] = 0;//c;
		}
	}
}

void getClientPos(int &x, int &y) {
	RECT rw, rc;
	GetWindowRect(hWnd, &rw);
	GetClientRect(hWnd, &rc);
	int c = (rw.bottom - rw.top) - (rc.bottom - rc.top);
	y = HEIGHT - y - c;
}

static void OnCreate(HWND hWnd, HDC &hDC, HGLRC &hRC)
{
	hDC = GetDC(hWnd);

	const PIXELFORMATDESCRIPTOR pfd = {
		sizeof (PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0,
		0, 0, 0,
		0, 0,
		0, 0, 0, 0, 0,
		32,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0,
		0,
		0
	};


	int nPfdID = ChoosePixelFormat(hDC, &pfd);
	if (nPfdID == 0) {
		return;
	}

	SetPixelFormat(hDC, nPfdID, &pfd);

	hRC = wglCreateContext(hDC);

	wglMakeCurrent(hDC, hRC);
}

static void OnDestroy(HWND hWnd, HDC hDC, HGLRC hRC)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hWnd, hDC);
}

// OpenGL関係の初期設定．
bool initGL(void)
{
	// glewの初期化．
	glewInit();
	if (!glewIsSupported("GL_VERSION_2_0")) {
		fprintf(stderr, "ERROR: Support for necessary OpenGL extensions missing.");
		return false;
	}

	return true;
}

void cleanUp() {
	printf("cleanUp\n");
	free(canvas);
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT);
	glRasterPos2i(-1, -1);
	glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, canvas);
	glFlush();
}
