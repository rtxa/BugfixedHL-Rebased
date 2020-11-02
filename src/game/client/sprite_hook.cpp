#include <cstdint>
#include <cmath>

#ifdef PLATFORM_WINDOWS
#include <winsani_in.h>
#include <Windows.h>
#include <gl/GL.h>
#include <winsani_out.h>
#else

#endif

#include <tier1/strtools.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include "sprite_hook.h"
#include "cl_util.h"
#include "client_vgui.h"
#include <studio.h>
#include <r_studioint.h>
#include <com_model.h>

extern engine_studio_api_t IEngineStudio;
static CSpriteHook s_Instance;

ConVar hud_sprite_hook("hud_sprite_hook", "1");
ConVar hud_scale("hud_scale", "1.0", FCVAR_BHL_ARCHIVE);

//#define HUD_FONT_NAME "Open Sans"
//#define HUD_FONT_PATH VGUI2_ROOT_DIR "resource/OpenSans-Bold.ttf"

//#define HUD_FONT_NAME "DejaVu Sans"
//#define HUD_FONT_PATH VGUI2_ROOT_DIR "resource/DejaVuSans-Bold.ttf"

#define HUD_FONT_NAME "DejaVu Sans Condensed"
#define HUD_FONT_PATH VGUI2_ROOT_DIR "resource/DejaVuSansCondensed-Bold.ttf"

#define CONSOLE_FONT_NAME "Open Sans"
#define CONSOLE_FONT_PATH VGUI2_ROOT_DIR "resource/OpenSans-SemiBold.ttf"

constexpr int HUD_FONT_TALL = 20;
constexpr int HUD_FONT_WEIGHT = 700;

CSpriteHook &CSpriteHook::Get()
{
	return s_Instance;
}

void CSpriteHook::Init()
{
	/*if (!IEngineStudio.IsHardware())
	{
		// Only works in OpenGL mode
		return;
	}*/

	m_pfnEngineSet = gEngfuncs.pfnSPR_Set;
	m_pfnEngineDraw = gEngfuncs.pfnSPR_Draw;
	m_pfnEngineDrawHoles = gEngfuncs.pfnSPR_DrawHoles;
	m_pfnEngineDrawAdditive = gEngfuncs.pfnSPR_DrawAdditive;
	m_pfnEngineEnableScissor = gEngfuncs.pfnSPR_EnableScissor;
	m_pfnEngineDisableScissor = gEngfuncs.pfnSPR_DisableScissor;
	m_pfnEngineDrawGeneric = gEngfuncs.pfnSPR_DrawGeneric;
	m_pfnEngineFillRGBA = gEngfuncs.pfnFillRGBA;

	m_pfnEngineDrawCharacter = gEngfuncs.pfnDrawCharacter;
	m_pfnEngineDrawConsoleString = gEngfuncs.pfnDrawConsoleString;
	m_pfnEngineDrawSetTextColor = gEngfuncs.pfnDrawSetTextColor;
	m_pfnEngineDrawConsoleStringLen = gEngfuncs.pfnDrawConsoleStringLen;
	m_pfnEngineDrawString = gEngfuncs.pfnDrawString;
	m_pfnEngineDrawStringReverse = gEngfuncs.pfnDrawStringReverse;

	// Add HUD text font
	vgui2::surface()->AddCustomFontFile(HUD_FONT_PATH);
	vgui2::surface()->AddCustomFontFile(CONSOLE_FONT_PATH);

	EnableHook();
}

void CSpriteHook::VidInit()
{
	Think();
	OnHudScaleChanged();
}

void CSpriteHook::RunFrame()
{
	if (hud_sprite_hook.GetBool())
		EnableHook();
	else
		DisableHook();

	if (hud_scale.GetFloat() != m_flHudScaleCvarValue)
	{
		m_flHudScaleCvarValue = hud_scale.GetFloat();

		m_iHudScale = (int)std::round(hud_scale.GetFloat() * 100.0);
		m_iHudScale = std::clamp(m_iHudScale, MIN_HUD_SCALE, MAX_HUD_SCALE);

		m_flHudScale = (float)(m_iHudScale / 100.0);

		Think();
		OnHudScaleChanged();
	}
}

void CSpriteHook::Think()
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	m_iHudWidth = m_scrinfo.iWidth / m_flHudScale;
	m_iHudHeight = m_scrinfo.iHeight / m_flHudScale;
}

int CSpriteHook::GetHudScreenWidth()
{
	return m_iHudWidth;
}

int CSpriteHook::GetHudScreenHeight()
{
	return m_iHudHeight;
}

void CSpriteHook::EnableHook()
{
	// Sprites
	gEngfuncs.pfnSPR_Set = [](HSPRITE hPic, int r, int g, int b) {
		CSpriteHook::Get().SPR_Set(hPic, r, g, b);
	};

	gEngfuncs.pfnSPR_Draw = [](int frame, int x, int y, const wrect_t *prc) {
		CSpriteHook::Get().SPR_Draw(frame, x, y, prc);
	};

	gEngfuncs.pfnSPR_DrawHoles = [](int frame, int x, int y, const wrect_t *prc) {
		CSpriteHook::Get().SPR_DrawHoles(frame, x, y, prc);
	};

	gEngfuncs.pfnSPR_DrawAdditive = [](int frame, int x, int y, const wrect_t *prc) {
		CSpriteHook::Get().SPR_DrawAdditive(frame, x, y, prc);
	};

	gEngfuncs.pfnSPR_DrawGeneric = [](int frame, int x, int y, const wrect_t *prc, int src, int dest, int w, int h) {
		CSpriteHook::Get().SPR_DrawGeneric(frame, x, y, prc, src, dest, w, h);
	};

	gEngfuncs.pfnSPR_EnableScissor = [](int x, int y, int width, int height) {
		CSpriteHook::Get().SPR_EnableScissor(x, y, width, height);
	};

	gEngfuncs.pfnSPR_DisableScissor = []() {
		CSpriteHook::Get().SPR_DisableScissor();
	};

	gEngfuncs.pfnFillRGBA = [](int x, int y, int width, int height, int r, int g, int b, int a) {
		CSpriteHook::Get().FillRGBA(x, y, width, height, r, g, b, a);
	};

	// Text
	gEngfuncs.pfnDrawCharacter = [](int x, int y, int number, int r, int g, int b) {
		return CSpriteHook::Get().DrawCharacter(x, y, number, r, g, b);
	};

	gEngfuncs.pfnDrawConsoleString = [](int x, int y, const char *const pszString) {
		return CSpriteHook::Get().DrawConsoleString(x, y, pszString);
	};

	gEngfuncs.pfnDrawSetTextColor = [](float r, float g, float b) {
		CSpriteHook::Get().DrawSetTextColor(r, g, b);
	};

	gEngfuncs.pfnDrawConsoleStringLen = [](const char *const pszString, int *piLength, int *piHeight) {
		CSpriteHook::Get().DrawConsoleStringLen(pszString, piLength, piHeight);
	};

	gEngfuncs.pfnDrawString = [](int x, int y, const char *const pszString, int r, int g, int b) {
		return CSpriteHook::Get().DrawString(x, y, pszString, r, g, b);
	};

	gEngfuncs.pfnDrawStringReverse = [](int x, int y, const char *const pszString, int r, int g, int b) {
		return CSpriteHook::Get().DrawStringReverse(x, y, pszString, r, g, b);
	};

	gEngfuncs.pfnSPR_DisableScissor = []() {
		CSpriteHook::Get().SPR_DisableScissor();
	};

	gEngfuncs.pfnSPR_DisableScissor = []() {
		CSpriteHook::Get().SPR_DisableScissor();
	};

	gEngfuncs.pfnSPR_DisableScissor = []() {
		CSpriteHook::Get().SPR_DisableScissor();
	};
}

void CSpriteHook::DisableHook()
{
	gEngfuncs.pfnSPR_Set = m_pfnEngineSet;
	gEngfuncs.pfnSPR_Draw = m_pfnEngineDraw;
	gEngfuncs.pfnSPR_DrawHoles = m_pfnEngineDrawHoles;
	gEngfuncs.pfnSPR_DrawAdditive = m_pfnEngineDrawAdditive;
	gEngfuncs.pfnSPR_DrawGeneric = m_pfnEngineDrawGeneric;
	gEngfuncs.pfnSPR_EnableScissor = m_pfnEngineEnableScissor;
	gEngfuncs.pfnSPR_DisableScissor = m_pfnEngineDisableScissor;
	gEngfuncs.pfnFillRGBA = m_pfnEngineFillRGBA;

	gEngfuncs.pfnDrawCharacter = m_pfnEngineDrawCharacter;
	gEngfuncs.pfnDrawConsoleString = m_pfnEngineDrawConsoleString;
	gEngfuncs.pfnDrawSetTextColor = m_pfnEngineDrawSetTextColor;
	gEngfuncs.pfnDrawConsoleStringLen = m_pfnEngineDrawConsoleStringLen;
	gEngfuncs.pfnDrawString = m_pfnEngineDrawString;
	gEngfuncs.pfnDrawStringReverse = m_pfnEngineDrawStringReverse;
}

float CSpriteHook::GetHudScale()
{
	return m_flHudScale;
}

int CSpriteHook::GetHudScaleInt()
{
	return m_iHudScale;
}

void CSpriteHook::OnHudScaleChanged()
{
	auto it = m_ScaleData.find(GetHudScaleInt());

	if (it == m_ScaleData.end())
	{
		ScaleData data;
		data.textFont = vgui2::surface()->CreateFont();
		data.consoleFont = vgui2::surface()->CreateFont();
		
		// HUD
		int tall = ScaleValue(HUD_FONT_TALL);
		int flags = tall >= 20 ? vgui2::ISurface::FONTFLAG_ANTIALIAS : 0;
		vgui2::surface()->AddGlyphSetToFont(data.textFont, HUD_FONT_NAME, tall, HUD_FONT_WEIGHT, 0, 0, flags, 0, 0);

		// Console
		// TODO:
		tall = ScaleValue(14);
		flags = 0 | vgui2::ISurface::FONTFLAG_DROPSHADOW;
		vgui2::surface()->AddGlyphSetToFont(data.consoleFont, CONSOLE_FONT_PATH, tall, 600, 0, 0, flags, 0, 0);

		m_ScaleData.insert({ GetHudScaleInt(), data });

		m_CurTextFont = data.textFont;
		m_CurConsoleFont = data.consoleFont;
	}
	else
	{
		m_CurTextFont = it->second.textFont;
		m_CurConsoleFont = it->second.consoleFont;
	}

	gHUD.ClearCharWidths();
}

void CSpriteHook::SPR_Set(HSPRITE hPic, int r, int g, int b)
{
	m_hSprite = hPic;
	m_pSpriteModel = gEngfuncs.GetSpritePointer(hPic);
	m_SpriteColor[0] = r;
	m_SpriteColor[1] = g;
	m_SpriteColor[2] = b;
	m_SpriteColor[3] = 255;

	if (m_pSpriteModel)
	{
		m_pSprite = (msprite_t *)m_pSpriteModel->cache.data;
	}

	// set default state
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void CSpriteHook::SPR_Draw(int frame, int x, int y, const wrect_t *prc)
{
	glEnable(GL_ALPHA_TEST);
	SPR_DrawInternal(frame, x, y, -1, -1, prc);
}

void CSpriteHook::SPR_DrawHoles(int frame, int x, int y, const wrect_t *prc)
{
	GL_SetRenderMode(kRenderTransAlpha);
	SPR_DrawInternal(frame, x, y, -1, -1, prc);
}

void CSpriteHook::SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc)
{
	GL_SetRenderMode(kRenderTransAdd);
	SPR_DrawInternal(frame, x, y, -1, -1, prc);
}

void CSpriteHook::SPR_DrawGeneric(int frame, int x, int y, const wrect_t *prc, int src, int dest, int w, int h)
{
	glEnable(GL_BLEND);
	glBlendFunc(src, dest); // g-cont. are params is valid?
	SPR_DrawInternal(frame, x, y, w, h, prc);
}

void CSpriteHook::SPR_EnableScissor(int x, int y, int width, int height)
{
	// check bounds
	//x = bound(0, x, clgame.scrInfo.iWidth);
	//y = bound(0, y, clgame.scrInfo.iHeight);
	//width = bound(0, width, clgame.scrInfo.iWidth - x);
	//height = bound(0, height, clgame.scrInfo.iHeight - y);

	m_SpriteScissor.x = x;
	m_SpriteScissor.w = width;
	m_SpriteScissor.y = y;
	m_SpriteScissor.h = height;
	m_SpriteScissor.test = true;
}

void CSpriteHook::SPR_DisableScissor()
{
	m_SpriteScissor.x = 0;
	m_SpriteScissor.w = 0;
	m_SpriteScissor.y = 0;
	m_SpriteScissor.h = 0;
	m_SpriteScissor.test = false;
}

void CSpriteHook::FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a)
{
	m_pfnEngineFillRGBA(ScaleValue(x), ScaleValue(y), ScaleValue(width), ScaleValue(height), r, g, b, a);
}

int CSpriteHook::DrawCharacter(int x, int y, int number, int r, int g, int b)
{
	vgui2::surface()->DrawSetTextColor(r, g, b, 255);
	vgui2::surface()->DrawSetTextPos(ScaleValue(x), ScaleValue(y));
	vgui2::surface()->DrawSetTextFont(m_CurTextFont);
	vgui2::surface()->DrawUnicodeCharAdd(number);
	return vgui2::surface()->GetCharacterWidth(m_CurTextFont, number);
}

int CSpriteHook::DrawConsoleString(int x, int y, const char *const pszString)
{
	vgui2::surface()->DrawSetTextColor(m_CurTextColor);
	return x + DrawString(pszString, x, y, m_CurConsoleFont);
}

void CSpriteHook::DrawSetTextColor(float r, float g, float b)
{
	m_CurTextColor[0] = r * 255;
	m_CurTextColor[1] = g * 255;
	m_CurTextColor[2] = b * 255;
	m_CurTextColor[3] = 255;
}

void CSpriteHook::DrawConsoleStringLen(const char *const pszString, int *piLength, int *piHeight)
{
	if (piLength)
		*piLength = GetStringLength(pszString, m_CurConsoleFont);
	
	if (piHeight)
		*piHeight = vgui2::surface()->GetFontTall(m_CurConsoleFont);
}

int CSpriteHook::DrawString(int x, int y, const char *const pszString, int r, int g, int b)
{
	vgui2::surface()->DrawSetTextColor(r, g, b, 255);
	return DrawString(pszString, x, y, m_CurTextFont);
}

int CSpriteHook::DrawStringReverse(int x, int y, const char *const pszString, int r, int g, int b)
{
	vgui2::surface()->DrawSetTextColor(r, g, b, 255);
	return DrawStringReverse(pszString, x, y, m_CurTextFont);
}

void CSpriteHook::SPR_DrawInternal(int frame, float x, float y, float width, float height, const wrect_t *prc)
{
	if (!m_pSprite)
		return;

	float s1, s2, t1, t2;
	const mspriteframe_t *pFrame = R_GetSpriteFrame(frame);

	if (width == -1 && height == -1)
	{
		// assume we get sizes from image
		width = pFrame->width;
		height = pFrame->height;
	}

	if (prc)
	{
		wrect_t rc;

		rc = *prc;

		// Sigh! some stupid modmakers set wrong rectangles in hud.txt
		if (rc.left <= 0 || rc.left >= width)
			rc.left = 0;
		if (rc.top <= 0 || rc.top >= height)
			rc.top = 0;
		if (rc.right <= 0 || rc.right > width)
			rc.right = width;
		if (rc.bottom <= 0 || rc.bottom > height)
			rc.bottom = height;

		// Hide sprite borders
		float offset = 0;
		if (m_flHudScale > 1)
		{
			offset = TEX_OFFSET * (m_flHudScale - 1);
		}

		// calc user-defined rectangle
		s1 = (rc.left + offset) / width;
		t1 = (rc.top + offset) / height;
		s2 = (rc.right - offset) / width;
		t2 = (rc.bottom - offset) / height;
		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
	}
	else
	{
		s1 = t1 = 0.0f;
		s2 = t2 = 1.0f;
	}

	// pass scissor test if supposed
	if (m_SpriteScissor.test && !SPR_Scissor(x, y, width, height, s1, t1, s2, t2))
		return;

	//vgui2::surface()->DrawSetColor(255, 255, 255, 255);
	//vgui2::surface()->DrawSetTextColor(255, 255, 255, 255);
	vgui2::surface()->DrawSetTexture(pFrame->gl_texturenum);
	//glColor4f(1, 1, 1, 1);
	glColor4ub(m_SpriteColor[0], m_SpriteColor[1], m_SpriteColor[2], m_SpriteColor[3]);

	// Scale values
	x = ScaleValue(x);
	y = ScaleValue(y);
	width = ScaleValue(width);
	height = ScaleValue(height);

	glBegin(GL_QUADS);
	{
		glTexCoord2f(s1, t1);
		glVertex2f(x, y);

		glTexCoord2f(s2, t1);
		glVertex2f(x + width, y);

		glTexCoord2f(s2, t2);
		glVertex2f(x + width, y + height);

		glTexCoord2f(s1, t2);
		glVertex2f(x, y + height);
	}
	glEnd();
}

bool CSpriteHook::SPR_Scissor(float &x, float &y, float &width, float &height, float &u0, float &v0, float &u1, float &v1)
{
	// clip sub rect to sprite
	if (width == 0 || height == 0)
		return false;

	if (x + width <= m_SpriteScissor.x)
		return false;
	if (x >= m_SpriteScissor.x + m_SpriteScissor.w)
		return false;
	if (y + height <= m_SpriteScissor.y)
		return false;
	if (y >= m_SpriteScissor.y + m_SpriteScissor.h)
		return false;

	float dudx = (u1 - u0) / width;
	float dvdy = (v1 - v0) / height;

	if (x < m_SpriteScissor.x)
	{
		u0 += (m_SpriteScissor.x - x) * dudx;
		width -= m_SpriteScissor.x - x;
		x = m_SpriteScissor.x;
	}

	if (x + width > m_SpriteScissor.x + m_SpriteScissor.w)
	{
		u1 -= (x + width - (m_SpriteScissor.x + m_SpriteScissor.w)) * dudx;
		width = m_SpriteScissor.x + m_SpriteScissor.w - x;
	}

	if (y < m_SpriteScissor.y)
	{
		v0 += (m_SpriteScissor.y - y) * dvdy;
		height -= m_SpriteScissor.y - y;
		y = m_SpriteScissor.y;
	}

	if (y + height > m_SpriteScissor.y + m_SpriteScissor.h)
	{
		v1 -= (y + height - (m_SpriteScissor.y + m_SpriteScissor.h)) * dvdy;
		height = m_SpriteScissor.y + m_SpriteScissor.h - y;
	}

	return true;
}

void CSpriteHook::GL_SetRenderMode(int mode)
{
	// TODO: Does IEngineStudio.GL_SetRenderMode work properly?
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	switch (mode)
	{
	case kRenderNormal:
	default:
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		break;
	case kRenderTransColor:
	case kRenderTransTexture:
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case kRenderTransAlpha:
		glDisable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
		break;
	case kRenderGlow:
	case kRenderTransAdd:
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	}
}

int CSpriteHook::DrawString(const char *pszString, int x, int y, vgui2::HFont font)
{
	x = ScaleValue(x);
	y = ScaleValue(y);

	wchar_t wbuf[1024];
	int width = 0;
	Q_UTF8ToWString(pszString, wbuf, sizeof(wbuf), STRINGCONVERT_REPLACE);
	int len = wcslen(wbuf);

	vgui2::surface()->DrawSetTextFont(font);

	for (int i = 0; i < len; i++)
	{
		int w = vgui2::surface()->GetCharacterWidth(font, wbuf[i]);

		vgui2::surface()->DrawSetTextPos(x, y);
		vgui2::surface()->DrawUnicodeChar(wbuf[i]);

		width += w;
		x += w;
	}

	vgui2::surface()->DrawFlushText();

	return UnscaleValue(width);
}

int CSpriteHook::DrawStringReverse(const char *pszString, int x, int y, vgui2::HFont font)
{
	x = ScaleValue(x);
	y = ScaleValue(y);

	wchar_t wbuf[1024];
	int width = 0;
	Q_UTF8ToWString(pszString, wbuf, sizeof(wbuf), STRINGCONVERT_REPLACE);
	int len = wcslen(wbuf);

	vgui2::surface()->DrawSetTextFont(font);

	for (int i = len - 1; i >= 0; i--)
	{
		int w = vgui2::surface()->GetCharacterWidth(font, wbuf[i]);
		width += w;
		x -= w;

		vgui2::surface()->DrawSetTextPos(x, y);
		vgui2::surface()->DrawUnicodeCharAdd(wbuf[i]);
	}

	vgui2::surface()->DrawFlushText();

	return UnscaleValue(width);
}

int CSpriteHook::GetStringLength(const char *pszString, vgui2::HFont font)
{
	wchar_t wbuf[1024];
	int width = 0;
	Q_UTF8ToWString(pszString, wbuf, sizeof(wbuf), STRINGCONVERT_REPLACE);
	int len = wcslen(wbuf);

	for (int i = 0; i < len; i++)
	{
		int w = vgui2::surface()->GetCharacterWidth(font, wbuf[i]);
		width += w;
	}

	return UnscaleValue(width);
}

const CSpriteHook::mspriteframe_t *CSpriteHook::R_GetSpriteFrame(int frame)
{
	Assert(m_pSprite);

	if (m_pSprite->numframes == 0)
	{
		ConPrintf(ConColor::Red, "Sprite Hook: sprite has no frames\n");
		return nullptr;
	}

	if (frame < 0 || frame >= m_pSprite->numframes)
	{
		ConPrintf(ConColor::Red, "Sprite Hook: frame %d outside of [0; %d)\n", frame, m_pSprite->numframes);
		return nullptr;
	}

	const uint8_t *data = (const uint8_t *)m_pSprite + sizeof(msprite_t) - 4;	// magic?
	data += frame * 8;	// magic?
	return *reinterpret_cast<mspriteframe_t * const *>(data);
}
