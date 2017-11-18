#include "windows/timer.h"
#include "windows/window.h"
#include "vulkan/basic.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char * layerPrefix,
	const char * msg,
	void * userData
	)
{
	printf("[VAL] %s\n", msg);
	return VK_FALSE;
}

void resizeCallback(void * pUserData, int width, int height)
{
	if (pUserData)
	{
		reinterpret_cast<vulkan::Wrapper *>(pUserData)->onWindowResize(width, height);
	}
}
void chageFocusCallback(void * pUserData, bool isInFocus)
{
}
void keyStateCallback(void * pUserData, windows::KeyCode keyCode, windows::KeyState keyState)
{
	using namespace windows;

#define DBG_KEY_TESTING 0

#if (DBG_KEY_TESTING == 1)
	char * keyStatus = "UNKNOWN";
	if (keyState == KeyState::ePressed)
	{
		keyStatus = "pressed";
	}
	else if (keyState == KeyState::eHeldDown)
	{
		keyStatus = "held down";
	}
	else if (keyState == KeyState::eReleased)
	{
		keyStatus = "released";
	}
#endif

	switch (keyCode)
	{
		case KeyCode::eEscape:
		{
			if (pUserData)
			{
				reinterpret_cast<vulkan::Wrapper *>(pUserData)->setIsExitting(true);
			}
			break;
		}
#if (DBG_KEY_TESTING == 1)
		case KeyCode::eEnter:
		{
			printf("Enter %s\n", keyStatus);
			break;
		}
		case KeyCode::eLCtrl:
		{
			printf("LCtrl %s\n", keyStatus);
			break;
		}
		case KeyCode::eRCtrl:
		{
			printf("RCtrl %s\n", keyStatus);
			break;
		}
		case KeyCode::eLShift:
		{
			printf("LShift %s\n", keyStatus);
			break;
		}
		case KeyCode::eRShift:
		{
			printf("RShift %s\n", keyStatus);
			break;
		}
		case KeyCode::eLAlt:
		{
			printf("LAlt %s\n", keyStatus);
			break;
		}
		case KeyCode::eRAlt:
		{
			printf("RAlt %s\n", keyStatus);
			break;
		}
		default:
		{
			printf("Key %d %s\n", (int)keyCode, keyStatus);
			break;
		}
#endif
	}
}

int main()
{
	using namespace windows;

	Timer perfTimer;

	Window window;
	window.setParameters(800, 600, Window::Kind::eWindowed);
	window.init();

	MSG msg;

	vulkan::Wrapper testApp;
	setUserDataPointer(&testApp);

	testApp.setDebugCallback(debugCallback);
	testApp.init(window.getHWnd(), window.getWidth(), window.getHeight());

	setResizeCallback(resizeCallback);
	setChangeFocusCallback(chageFocusCallback);
	setKeyStateCallback(keyStateCallback);

	double accumTime = 0.0;
	int accumFrames = 0;
	perfTimer.start();
	double dtMS = 0.0;

	bool isRunning = true;
	while (isRunning)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				isRunning = false;
				break;
			}
		}

		if (testApp.getIsExitting())
		{
			isRunning = false;
		}

		if (!isRunning)
		{
			break;
		}

		testApp.update(dtMS);
		testApp.render();
		dtMS = perfTimer.time();
		accumTime += dtMS;
		++accumFrames;
		if (accumTime > 500.0)
		{
			testApp.setDTime(accumTime / (float)accumFrames);
			accumTime = 0.0;
			accumFrames = 0;
		}
		perfTimer.start();
	}

	testApp.deinit();

	window.deinit();

	return 0;
}