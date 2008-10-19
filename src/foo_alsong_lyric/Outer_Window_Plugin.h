class Outer_Window_Plugin
{
private:
	HWND m_hWnd;

public:
	Outer_Window_Plugin();
	void Show();
	void Hide();
	HWND Create();
	void Destroy();
	void OnContextMenu(HWND hWndFrom);

	HWND GetHWND() {return m_hWnd;}

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
};

extern Outer_Window_Plugin g_OuterWindow;