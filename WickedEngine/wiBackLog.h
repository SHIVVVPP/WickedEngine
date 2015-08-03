#ifndef WIBACKLOG
#define WIBACKLOG
#include "CommonInclude.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiLua.h"

class wiBackLog
{
private:
	static deque<string> stream;
	static unsigned int deletefromline;
	static mutex logMutex;
	static const float speed;
	static float pos;
	static int scroll;
	static stringstream inputArea;
	enum State{
		DISABLED,
		IDLE,
		ACTIVATING,
		DEACTIVATING,
	};
	static State state;
	static ID3D11ShaderResourceView* backgroundTex;
	static float fontSize;
public:
	static void Initialize();
	static void CleanUp();
	static void Toggle();
	static void Scroll(int direction);
	static void Update();
	static void Draw();

	static string getText();
	static void clear();
	static void post(const char* input);
	static void input(const char& input);
	static void acceptInput();
	static void deletefromInput();
	static void save(ofstream& file);

	static deque<string> history;
	static int historyPos;
	static void historyPrev();
	static void historyNext();

	static bool isActive(){return state==IDLE;}

	static void setBackground(ID3D11ShaderResourceView* texture);

	static int ClearLua(lua_State* L);
	static int PostLua(lua_State* L);
	static int FontSizeLua(lua_State* L);
};

#endif