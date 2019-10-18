
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <d3dx9tex.h>
#include <dinput.h>
#include <dshow.h>   
#include "Camera.h"


#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

#define WM_GRAPHNOTIFY  WM_APP + 1 


LPDIRECT3D9             directD3D = NULL;
LPDIRECT3DDEVICE9       direct3Device9 = NULL; 
LPDIRECT3DVERTEXBUFFER9 vertexBuffer = NULL;
LPDIRECT3DTEXTURE9		g_pTexture_up = NULL;
LPDIRECT3DTEXTURE9		g_pTexture_down = NULL;
LPDIRECT3DTEXTURE9		g_pTexture_front = NULL;

CXCamera *camera;

LPDIRECTINPUT8			g_pDin;							
LPDIRECTINPUTDEVICE8	g_pDinKeyboard;					
LPDIRECTINPUTDEVICE8	g_pDinmouse;					
BYTE					g_Keystate[256];				
DIMOUSESTATE			g_pMousestate;					
POINT					g_SurfacePosition;

FLOAT dx_CAMERA = 0.05, dy_CAMERA = -0.05, dz_Camera = 0.0;
FLOAT dx_finn = 0.05, dy_finn = -10.05, dz_finn=0.0;

BOOL rotate = false;
FLOAT rotationangle = 0.0;
D3DXMATRIX g_RotateMesh;

IGraphBuilder *graphBuilder = NULL;
IMediaControl *mediaControl = NULL;
IMediaEventEx *mediaEvent = NULL;
IMediaSeeking *mediaSeeking = NULL;

HWND hWnd;
HDC hdc;


D3DXVECTOR3 vEyePt(0.0f, 1.0f, -10.5f);
D3DXVECTOR3 vLookatPt(0.0f, -6.05, 0.0f);
D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);

LPD3DXMESH              Mesh = NULL; 
D3DMATERIAL9*           MeshMaterials = NULL;
LPDIRECT3DTEXTURE9*     MeshTextures = NULL; 
DWORD                   NumMaterials = 0L;   


struct CUSTOMVERTEX
{
	FLOAT x, y, z; 
	FLOAT tu, tv;
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_TEX1)


HRESULT InitD3D(HWND hWnd)
{
	
	if (NULL == (directD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return E_FAIL;


	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;


	if (FAILED(directD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &direct3Device9)))
	{
		if (FAILED(directD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&d3dpp, &direct3Device9)))
			return E_FAIL;
	}


	direct3Device9->SetRenderState(D3DRS_ZENABLE, TRUE);


	direct3Device9->SetRenderState(D3DRS_AMBIENT, 0xffffffff);


	direct3Device9->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);


	direct3Device9->SetRenderState(D3DRS_LIGHTING, FALSE);

	return S_OK;
}

HRESULT InitDirectShow(HWND hWnd)
{

	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL,
		CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&graphBuilder);
 
	hr = graphBuilder->QueryInterface(IID_IMediaControl, (void **)&mediaControl);
	hr = graphBuilder->QueryInterface(IID_IMediaEventEx, (void **)&mediaEvent);
	hr = graphBuilder->QueryInterface(IID_IMediaSeeking, (void**)&mediaSeeking);


	hr = graphBuilder->RenderFile(L"adventure_time.mp3", NULL);

	mediaEvent->SetNotifyWindow((OAHWND)hWnd, WM_GRAPHNOTIFY, 0);


	mediaControl->Stop();


	return S_OK;
}
void HandleGraphEvent()
{
 
	long evCode;
	LONG_PTR param1, param2;

	while (SUCCEEDED(mediaEvent->GetEvent(&evCode, &param1, &param2, 0)))
	{
		mediaEvent->FreeEventParams(evCode, param1, param2);
		switch (evCode)
		{
			case EC_COMPLETE:   
			case EC_USERABORT:   
			case EC_ERRORABORT:
				PostQuitMessage(0);
				return;
		}
	}
}


HRESULT InitGeometry()
{

	if (FAILED(D3DXCreateTextureFromFile(direct3Device9, "sky.jpg", &g_pTexture_up)))
	{

		if (FAILED(D3DXCreateTextureFromFile(direct3Device9, "..\\sky.v", &g_pTexture_up)))
		{
			MessageBox(NULL, "Could not find sky.jpg", "Proj.exe", MB_OK);
			return E_FAIL;
		}
	}

	if (FAILED(D3DXCreateTextureFromFile(direct3Device9, "grass.jpg", &g_pTexture_down)))
	{
	
		if (FAILED(D3DXCreateTextureFromFile(direct3Device9, "..\\grass.jpg", &g_pTexture_down)))
		{
			MessageBox(NULL, "Could not find grass.jpg", "Proj.exe", MB_OK);
			return E_FAIL;
		}
	}

	if (FAILED(D3DXCreateTextureFromFile(direct3Device9, "front.jpg", &g_pTexture_front)))
	{
		
		if (FAILED(D3DXCreateTextureFromFile(direct3Device9, "..\\front.jpg", &g_pTexture_front)))
		{
			MessageBox(NULL, "Could not find front.jpg", "Proj.exe", MB_OK);
			return E_FAIL;
		}
	}

	CUSTOMVERTEX g_Vertices_Cub[] =
	{
		
		{ -10, -10, 10, 0.0f, 1.0f },
		{ -10, 10, 10, 0.0f, 0.0f },
		{ 10, -10, 10, 1.0f, 1.0f },
		{ 10, 10, 10, 1.0f, 0.0f },

		
		{ 10, -10, -10, 0.0f, 1.0f },
		{ 10, 10, -10, 0.0f, 0.0f },
		{ -10, -10, -10, 1.0f, 1.0f },
		{ -10, 10, -10, 1.0f, 0.0f },

		
		{ -10, -10, -10, 0.0f, 1.0f },
		{ -10, 10, -10, 0.0f, 0.0f },
		{ -10, -10, 10, 1.0f, 1.0f },
		{ -10, 10, 10, 1.0f, 0.0f },

		
		{ 10, -10, 10, 0.0f, 1.0f },
		{ 10, 10, 10, 0.0f, 0.0f },
		{ 10, -10, -10, 1.0f, 1.0f },
		{ 10, 10, -10, 1.0f, 0.0f },

		
		{ -10, 10, 10, 0.0f, 1.0f },
		{ -10, 10, -10, 0.0f, 0.0f },
		{ 10, 10, 10, 1.0f, 1.0f },
		{ 10, 10, -10, 1.0f, 0.0f },

		
		{ -10, -10, -10, 0.0f, 1.0f },
		{ -10, -10, 10, 0.0f, 0.0f },
		{ 10, -10, -10, 1.0f, 1.0f },
		{ 10, -10, 10, 1.0f, 0.0f }
	};


	if (FAILED(direct3Device9->CreateVertexBuffer(100 * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_MANAGED, &vertexBuffer, NULL)))
	{
		return E_FAIL;
	}


	VOID* pVertices;
	if (FAILED(vertexBuffer->Lock(0, sizeof(g_Vertices_Cub), (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, g_Vertices_Cub, sizeof(g_Vertices_Cub));
	vertexBuffer->Unlock();
	
	

	LPD3DXBUFFER pD3DXMtrlBuffer;

	
	if (FAILED(D3DXLoadMeshFromX("finn.x", D3DXMESH_SYSTEMMEM,
		direct3Device9, NULL,
		&pD3DXMtrlBuffer, NULL, &NumMaterials,
		&Mesh)))
	{
	
		if (FAILED(D3DXLoadMeshFromX("..\\finn.x", D3DXMESH_SYSTEMMEM,
			direct3Device9, NULL,
			&pD3DXMtrlBuffer, NULL, &NumMaterials,
			&Mesh)))
		{
			MessageBox(NULL, "Could not find finn.x", "Meshes.exe", MB_OK);
			return E_FAIL;
		}
	}

	
	D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
	MeshMaterials = new D3DMATERIAL9[NumMaterials];
	MeshTextures = new LPDIRECT3DTEXTURE9[NumMaterials];

	for (DWORD i = 0; i<NumMaterials; i++)
	{
		
		MeshMaterials[i] = d3dxMaterials[i].MatD3D;

		
		MeshMaterials[i].Ambient = MeshMaterials[i].Diffuse;

		MeshTextures[i] = NULL;
		if (d3dxMaterials[i].pTextureFilename != NULL &&
			lstrlen(d3dxMaterials[i].pTextureFilename) > 0)
		{
			
			if (FAILED(D3DXCreateTextureFromFile(direct3Device9,
				d3dxMaterials[i].pTextureFilename,
				&MeshTextures[i])))
			{
				
				const TCHAR* strPrefix = TEXT("..\\");
				const int lenPrefix = lstrlen(strPrefix);
				TCHAR strTexture[MAX_PATH];
				lstrcpyn(strTexture, strPrefix, MAX_PATH);
				lstrcpyn(strTexture + lenPrefix, d3dxMaterials[i].pTextureFilename, MAX_PATH - lenPrefix);
				
				if (FAILED(D3DXCreateTextureFromFile(direct3Device9,
					strTexture,
					&MeshTextures[i])))
				{
					MessageBox(NULL, "Could not find texture map", "Meshes.exe", MB_OK);
				}
			}
		}
	}

	
	pD3DXMtrlBuffer->Release();


	LPDIRECT3DVERTEXBUFFER9 VertexBuffer = NULL;
	BYTE* Vertices = NULL;

	DWORD FVFVertexSize = D3DXGetFVFVertexSize(Mesh->GetFVF());

	Mesh->GetVertexBuffer(&VertexBuffer);

	VertexBuffer->Lock(0, 0, (VOID**)&Vertices, D3DLOCK_DISCARD);


	VertexBuffer->Unlock();
	VertexBuffer->Release();


	camera = new CXCamera(direct3Device9);
	camera->LookAtPos(&vEyePt, &vLookatPt, &vUpVec);

	D3DXMatrixIdentity(&g_RotateMesh);

	return S_OK;
}

HRESULT InitDInput(HINSTANCE hInstance, HWND hWnd)
{
	
	DirectInput8Create(hInstance,  
		DIRECTINPUT_VERSION,   
		IID_IDirectInput8,    
		(void**)&g_pDin,   
		NULL);    

				  
	g_pDin->CreateDevice(GUID_SysKeyboard,  
		&g_pDinKeyboard, 
		NULL);    

				 
	g_pDin->CreateDevice(GUID_SysMouse,
		&g_pDinmouse, 
		NULL); 

			   
	g_pDinKeyboard->SetDataFormat(&c_dfDIKeyboard);

	
	g_pDinmouse->SetDataFormat(&c_dfDIMouse);

	
	g_pDinKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

	
	g_pDinmouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);

	return S_OK;
}

VOID Cleanup()
{
	if (vertexBuffer)
		vertexBuffer->Release();

	if (direct3Device9 != NULL)
		direct3Device9->Release();

	if (directD3D != NULL)
		directD3D->Release();

	if (graphBuilder)
		graphBuilder->Release();

	if (mediaControl)
		mediaControl->Release();

	if (mediaEvent)
		mediaEvent->Release();

	if (MeshMaterials != NULL)
		delete[] MeshMaterials;

	if (MeshTextures)
	{
		for (DWORD i = 0; i < NumMaterials; i++)
		{
			if (MeshTextures[i])
				MeshTextures[i]->Release();
		}
		delete[] MeshTextures;
	}
	if (Mesh != NULL)
		Mesh->Release();
}

VOID DetectInput()
{
	
	g_pDinKeyboard->Acquire();
	g_pDinmouse->Acquire();

	
	g_pDinKeyboard->GetDeviceState(256, (LPVOID)g_Keystate);
	g_pDinmouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&g_pMousestate);

}

VOID CleanDInput()
{
	g_pDinKeyboard->Unacquire();
	g_pDin->Release();   
}

VOID SetupMatrices()
{
	D3DXMATRIX g_Transform;
	D3DXMatrixIdentity(&g_Transform);
	D3DXMatrixTranslation(&g_Transform, 0, 0, 0);
	direct3Device9->SetTransform(D3DTS_WORLD, &g_Transform);

	camera->Update();

	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
	direct3Device9->SetTransform(D3DTS_PROJECTION, &matProj);
}

VOID Render()
{

	direct3Device9->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

	D3DXMATRIX g_Translate;
	

	if (SUCCEEDED(direct3Device9->BeginScene()))
	{

		SetupMatrices();

		direct3Device9->SetStreamSource(0, vertexBuffer, 0, sizeof(CUSTOMVERTEX));
		direct3Device9->SetFVF(D3DFVF_CUSTOMVERTEX);


		D3DXMATRIX g_TranslateP;
		D3DXMatrixTranslation(&g_TranslateP, 0, 0, 0);
		
		direct3Device9->SetTransform(D3DTS_WORLD, &(g_TranslateP));

		direct3Device9->SetTexture(0, g_pTexture_front);
		direct3Device9->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

		direct3Device9->SetTexture(0, g_pTexture_front);
		direct3Device9->DrawPrimitive(D3DPT_TRIANGLESTRIP, 4, 2);

		direct3Device9->SetTexture(0, g_pTexture_front);
		direct3Device9->DrawPrimitive(D3DPT_TRIANGLESTRIP, 8, 2);

		direct3Device9->SetTexture(0, g_pTexture_front);
		direct3Device9->DrawPrimitive(D3DPT_TRIANGLESTRIP, 12, 2);

		direct3Device9->SetTexture(0, g_pTexture_up);
		direct3Device9->DrawPrimitive(D3DPT_TRIANGLESTRIP, 16, 2);

		direct3Device9->SetTexture(0, g_pTexture_down);
		direct3Device9->DrawPrimitive(D3DPT_TRIANGLESTRIP, 20, 2);



		D3DXMATRIX g_ScaleMesh, g_TranslateMesh;
		
		D3DXMatrixScaling(&g_ScaleMesh, 2, 2, 2);
		D3DXMatrixTranslation(&g_TranslateMesh, dx_finn, dy_finn, dz_finn);
		
		if (rotate == true)
		{
			D3DXMatrixRotationY(&g_RotateMesh, rotationangle);
			rotate = false;
		}

		direct3Device9->SetTransform(D3DTS_WORLD, &(g_ScaleMesh * g_TranslateMesh * g_RotateMesh));
		for (DWORD i = 0; i < NumMaterials; i++)
		{

			direct3Device9->SetMaterial(&MeshMaterials[i]);
			direct3Device9->SetTexture(0, MeshTextures[i]);


			Mesh->DrawSubset(i);
		}


		direct3Device9->EndScene();
	}


	direct3Device9->Present(NULL, NULL, NULL, NULL);
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		Cleanup();
		CleanDInput();
		PostQuitMessage(0);
		return 0;
	case WM_GRAPHNOTIFY:
		HandleGraphEvent();
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		"Proiect Finn", NULL };
	RegisterClassEx(&wc);

	
	HWND hWnd = CreateWindow("Proiect Finn", "Proiect Finn",
		WS_OVERLAPPEDWINDOW, 100, 100, 1800, 900,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	
	HRESULT hr = CoInitialize(NULL);

	hdc = GetDC(hWnd);

	
	if (SUCCEEDED(InitD3D(hWnd)))
	{
		if (FAILED(InitDirectShow(hWnd)))
			return 0;

		InitDInput(hInst, hWnd);

	
		if (SUCCEEDED(InitGeometry()))
		{
			ShowWindow(hWnd, SW_SHOWDEFAULT);
			UpdateWindow(hWnd);

			MSG msg;
			ZeroMemory(&msg, sizeof(msg));
			while (msg.message != WM_QUIT)
			{
				if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					DetectInput();   
					Render();

					g_SurfacePosition.x += g_pMousestate.lX;
					g_SurfacePosition.y += g_pMousestate.lY;

					if (g_pMousestate.lX != 0)
					{

						D3DXVECTOR3 vEyePt = *camera->GetPosition();


						D3DXMATRIX camera_rotate;
						D3DXMatrixRotationAxis(&camera_rotate, &D3DXVECTOR3(0.0f, 1.0f, 0.0f), D3DXToRadian(g_pMousestate.lX));
						D3DXVec3TransformCoord(&vEyePt, &vEyePt, &camera_rotate);

						camera->LookAtPos(&vEyePt, &vLookatPt, &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
					}

					if (g_pMousestate.lY != 0)
					{

						D3DXVECTOR3 vEyePt = *camera->GetPosition();


						D3DXMATRIX camera_rotate;
						D3DXMatrixRotationAxis(&camera_rotate, &D3DXVECTOR3(1.0f, 0.0f, 0.0f), D3DXToRadian(g_pMousestate.lY));


						D3DXVec3TransformCoord(&vEyePt, &vEyePt, &camera_rotate);

						camera->LookAtPos(&vEyePt, &vLookatPt, &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
					}

					if (g_Keystate[DIK_ESCAPE] & 0x80) {
						PostMessage(hWnd, WM_DESTROY, 0, 0);
					}

					if (g_Keystate[DIK_LEFT] & 0x80) {
						dx_CAMERA += 0.001;
						camera->MoveRight(dx_CAMERA);
					}

					if (g_Keystate[DIK_RIGHT] & 0x80) {
						dx_CAMERA -= 0.001;
						camera->MoveRight(-dx_CAMERA);
					}

					if (g_Keystate[DIK_DOWN] & 0x80) {
						dy_CAMERA += 0.001;
						camera->MoveForward(dy_CAMERA);
					}

					if (g_Keystate[DIK_UP] & 0x80) {
						dy_CAMERA -= 0.001;
						camera->MoveForward(-dy_CAMERA);
					}

					if (g_Keystate[DIK_A] & 0x80) {
						dx_finn -= 0.1;
					}

					if (g_Keystate[DIK_D] & 0x80) {
						dx_finn += 0.1;
					}

					if (g_Keystate[DIK_W] & 0x80) {
						dy_finn += 0.1;
					}

					if (g_Keystate[DIK_S] & 0x80) {
						dy_finn -= 0.1;
					}

					if (g_Keystate[DIK_P] & 0x80)
					{
						mediaControl->Run();
					}

					if (g_Keystate[DIK_O] & 0x80)
					{
						mediaControl->Stop();
					}

					if (g_Keystate[DIK_Q] & 0x80)
					{
						rotationangle += 0.01;
						rotate = true;
					}

					if (g_Keystate[DIK_E] & 0x80)
					{
						rotationangle -=0.01;
						rotate = true;
					}
					if (g_Keystate[DIK_Z] & 0x80)
					{
						dz_finn += 0.1;
					}

					if (g_Keystate[DIK_C] & 0x80)
					{
						dz_finn -= 0.1;
					}
				}
			}
		}
	}

	UnregisterClass("D3D Tutorial", wc.hInstance);
	return 0;
}



