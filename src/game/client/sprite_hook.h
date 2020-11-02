#ifndef SPRITE_HOOK_H
#define SPRITE_HOOK_H
#include <vgui/VGUI2.h>
#include <map>
#include "hud.h"

typedef struct model_s model_t;

class CSpriteHook
{
public:
	static constexpr int MIN_HUD_SCALE = 25;
	static constexpr int MAX_HUD_SCALE = 400;
	
	/**
	 * When upscaling sprites (scale > 1), borders become visible.
	 * To counteract this, sprite texture bounds are shifted (TEX_OFFSET * (scale - 1)) pixels inwards.
	 */
	static constexpr float TEX_OFFSET = 0.2;

	static CSpriteHook &Get();
	
	/**
	 * Initializes sprite hook.
	 */
	void Init();
	
	/**
	 * Called on VidInit.
	 */
	void VidInit();

	/**
	 * Polls for update of hud_scale.
	 */
	void RunFrame();

	/**
	 * Updates screen resolution.
	 */
	void Think();

	/**
	 * Returns adjusted HUD screen width.
	 */
	int GetHudScreenWidth();

	/**
	 * Returns adjusted HUD screen height.
	 */
	int GetHudScreenHeight();

private:
	enum spriteframetype_t
	{
		SPR_SINGLE,
		SPR_GROUP
	};

	struct mspriteframe_t
	{
		int width;
		int height;
		float up;
		float down;
		float left;
		float right;
		int gl_texturenum;
	};

	struct mspriteframedesc_t
	{
		spriteframetype_t type;
		mspriteframe_t *frameptr;
	};

	struct msprite_t
	{
		short type;
		short texFormat;
		int maxwidth;
		int maxheight;
		int numframes;
		int paloffset;
		float beamlength;
		void *cachespot;
		mspriteframedesc_t frames[1];
	};

	struct Scissor
	{
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
		bool test = false;
	};

	struct ScaleData
	{
		vgui2::HFont textFont = NULL_HANDLE;
		vgui2::HFont consoleFont = NULL_HANDLE;
	};

	// Sprite functions
	decltype(gEngfuncs.pfnSPR_Set) m_pfnEngineSet = nullptr;
	decltype(gEngfuncs.pfnSPR_Draw) m_pfnEngineDraw = nullptr;
	decltype(gEngfuncs.pfnSPR_DrawHoles) m_pfnEngineDrawHoles = nullptr;
	decltype(gEngfuncs.pfnSPR_DrawAdditive) m_pfnEngineDrawAdditive = nullptr;
	decltype(gEngfuncs.pfnSPR_EnableScissor) m_pfnEngineEnableScissor = nullptr;
	decltype(gEngfuncs.pfnSPR_DisableScissor) m_pfnEngineDisableScissor = nullptr;
	decltype(gEngfuncs.pfnSPR_DrawGeneric) m_pfnEngineDrawGeneric = nullptr;
	decltype(gEngfuncs.pfnFillRGBA) m_pfnEngineFillRGBA = nullptr;

	// Text functions
	decltype(gEngfuncs.pfnDrawCharacter) m_pfnEngineDrawCharacter = nullptr;
	decltype(gEngfuncs.pfnDrawConsoleString) m_pfnEngineDrawConsoleString = nullptr;
	decltype(gEngfuncs.pfnDrawSetTextColor) m_pfnEngineDrawSetTextColor = nullptr;
	decltype(gEngfuncs.pfnDrawConsoleStringLen) m_pfnEngineDrawConsoleStringLen = nullptr;
	decltype(gEngfuncs.pfnDrawString) m_pfnEngineDrawString = nullptr;
	decltype(gEngfuncs.pfnDrawStringReverse) m_pfnEngineDrawStringReverse = nullptr;


	std::map<int, ScaleData> m_ScaleData;

	float m_flHudScaleCvarValue = 1.0f;
	float m_flHudScale = 1.0f;
	int m_iHudScale = 100;

	// Screen info
	SCREENINFO m_scrinfo;
	int m_iHudWidth = 0;
	int m_iHudHeight = 0;
	vgui2::HFont m_CurTextFont = NULL_HANDLE;
	vgui2::HFont m_CurConsoleFont = NULL_HANDLE;

	// Current sprite info
	HSPRITE m_hSprite = 0;
	const model_t *m_pSpriteModel = nullptr;
	const msprite_t *m_pSprite = nullptr;
	Color m_SpriteColor;
	Scissor m_SpriteScissor;

	// Current font info
	Color m_CurTextColor;

	/**
	 * Replaces gEngfuncs members with hooks.
	 */
	void EnableHook();

	/**
	 * Restores gEngfuncs.
	 */
	void DisableHook();

	/**
	 * Returns HUD scale as a multiplier [0.25; 4].
	 */
	float GetHudScale();

	/**
	 * Returns HUD scale in percents [25; 400].
	 */
	int GetHudScaleInt();

	/**
	 * Coverts HUD pixels into screen pixels.
	 */
	inline int ScaleValue(int val)
	{
		return (int)(val * m_flHudScale);
	}

	/**
	 * Coverts screen pixels into HUD pixels.
	 */
	inline int UnscaleValue(int val)
	{
		return (int)(val / m_flHudScale);
	}

	/**
	 * HUD scale was updated.
	 */
	void OnHudScaleChanged();

	// Sprite hooks
	void SPR_Set(HSPRITE hPic, int r, int g, int b);
	void SPR_Draw(int frame, int x, int y, const wrect_t *prc);
	void SPR_DrawHoles(int frame, int x, int y, const wrect_t *prc);
	void SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc);
	void SPR_DrawGeneric(int frame, int x, int y, const wrect_t *prc, int src, int dest, int w, int h);
	void SPR_EnableScissor(int x, int y, int width, int height);
	void SPR_DisableScissor();
	void FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a);

	// Text hooks
	int DrawCharacter(int x, int y, int number, int r, int g, int b);
	int DrawConsoleString(int x, int y, const char *const pszString);
	void DrawSetTextColor(float r, float g, float b);
	void DrawConsoleStringLen(const char *const pszString, int *piLength, int *piHeight);
	int DrawString(int x, int y, const char *const pszString, int r, int g, int b);
	int DrawStringReverse(int x, int y, const char *const pszString, int r, int g, int b);

	// Sprite methods
	void SPR_DrawInternal(int frame, float x, float y, float width, float height, const wrect_t *prc);
	bool SPR_Scissor(float &x, float &y, float &width, float &height, float &u0, float &v0, float &u1, float &v1);
	void GL_SetRenderMode(int mode);

	// Text methods
	/**
	 * Draws a string. Coordinates in HUD pixels.
	 * @returns Width of the string in HUD pixels.
	 */
	int DrawString(const char *pszString, int x, int y, vgui2::HFont font);

	/**
	 * Draws a string but it will end at (x,y). Coordinates in HUD pixels.
	 * @returns Width of the string in HUD pixels.
	 */
	int DrawStringReverse(const char *pszString, int x, int y, vgui2::HFont font);

	/**
	 * @returns Lnfth of the string in HUD pixels.
	 */
	int GetStringLength(const char *pszString, vgui2::HFont font);

	const mspriteframe_t *R_GetSpriteFrame(int frame);
};

#endif
