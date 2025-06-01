#include "link/link.h"


namespace NDebugOverlay
{
	static StaticFuncThunk<void, const Vector&, const Vector&, const Vector&, int, int, int, int, float>                               ft_Box                       ("NDebugOverlay::Box");
	static StaticFuncThunk<void, const Vector&, const Vector&, const Vector&, const Vector&, int, int, int, int, float>                ft_BoxDirection              ("NDebugOverlay::BoxDirection");
	static StaticFuncThunk<void, const Vector&, const Vector&, const Vector&, const QAngle&, int, int, int, int, float>                ft_BoxAngles                 ("NDebugOverlay::BoxAngles");
	static StaticFuncThunk<void, const Vector&, const Vector&, const Vector&, const Vector&, const QAngle&, int, int, int, int, float> ft_SweptBox                  ("NDebugOverlay::SweptBox");
	static StaticFuncThunk<void, const CBaseEntity *, int, int, int, int, float>                                                       ft_EntityBounds              ("NDebugOverlay::EntityBounds");
	static StaticFuncThunk<void, const Vector&, const Vector&, int, int, int, bool, float>                                             ft_Line                      ("NDebugOverlay::Line");
	static StaticFuncThunk<void, const Vector&, const Vector&, const Vector&, int, int, int, int, bool, float>                         ft_Triangle                  ("NDebugOverlay::Triangle");
	static StaticFuncThunk<void, int, int, const char *, float, int, int, int, int>                                                    ft_EntityText                ("NDebugOverlay::EntityText");
	static StaticFuncThunk<void, const Vector&, int, const char *, float, int, int, int, int>                                          ft_EntityTextAtPosition      ("NDebugOverlay::EntityTextAtPosition");
	static StaticFuncThunk<void, const Vector&>                                                                                        ft_Grid                      ("NDebugOverlay::Grid");
	static StaticFuncThunk<void, const Vector&, const char *, bool, float>                                                             ft_Text                      ("NDebugOverlay::Text");
	static StaticFuncThunk<void, float, float, const char *, int, int, int, int, float>                                                ft_ScreenText                ("NDebugOverlay::ScreenText");
	static StaticFuncThunk<void, const Vector&, const Vector&, const Vector&, int, int, int, bool, float>                              ft_Cross3D_ext               ("NDebugOverlay::Cross3D_ext");
	static StaticFuncThunk<void, const Vector&, float, int, int, int, bool, float>                                                     ft_Cross3D_size              ("NDebugOverlay::Cross3D_size");
	static StaticFuncThunk<void, const Vector&, const QAngle&, float, int, int, int, bool, float>                                      ft_Cross3DOriented_ang       ("NDebugOverlay::Cross3DOriented_ang");
	static StaticFuncThunk<void, const matrix3x4_t&, float, int, bool, float>                                                          ft_Cross3DOriented_mat       ("NDebugOverlay::Cross3DOriented_mat");
	//static StaticFuncThunk<void>                                                                                                       ft_DrawOverlayLines          ("NDebugOverlay::DrawOverlayLines");
	static StaticFuncThunk<void, const Vector&, const Vector&, float, int, int, int, int, bool, float>                                 ft_DrawTickMarkedLine        ("NDebugOverlay::DrawTickMarkedLine");
	static StaticFuncThunk<void>                                                                                                       ft_DrawGroundCrossHairOverlay("NDebugOverlay::DrawGroundCrossHairOverlay");
	static StaticFuncThunk<void, const Vector&, const Vector&, float, int, int, int, int, bool, float>                                 ft_HorzArrow                 ("NDebugOverlay::HorzArrow");
	static StaticFuncThunk<void, const Vector&, float, float, float, int, int, int, int, bool, float>                                  ft_YawArrow                  ("NDebugOverlay::YawArrow");
	static StaticFuncThunk<void, const Vector&, const Vector&, float, int, int, int, int, bool, float>                                 ft_VertArrow                 ("NDebugOverlay::VertArrow");
	static StaticFuncThunk<void, const Vector&, const QAngle&, float, bool, float>                                                     ft_Axis                      ("NDebugOverlay::Axis");
	static StaticFuncThunk<void, const Vector&, float, int, int, int, bool, float>                                                     ft_Sphere                    ("NDebugOverlay::Sphere");
	static StaticFuncThunk<void, const Vector&, float, int, int, int, int, bool, float>                                                ft_Circle                    ("NDebugOverlay::Circle");
	static StaticFuncThunk<void, const Vector&, const QAngle&, float, int, int, int, int, bool, float>                                 ft_Circle_ang                ("NDebugOverlay::Circle_ang");
	static StaticFuncThunk<void, const Vector&, const Vector&, const Vector&, float, int, int, int, int, bool, float>                  ft_Circle_axes               ("NDebugOverlay::Circle_axes");
	static StaticFuncThunk<void, const Vector&, const QAngle&, float, int, int, int, int, bool, float>                                 ft_Sphere_ang                ("NDebugOverlay::Sphere_ang");
	
	
	void Box(const Vector& origin, const Vector& mins, const Vector& maxs, int r, int g, int b, int a, float flDuration)                                              { ft_Box(origin, mins, maxs, r, g, b, a, flDuration); }
	void BoxDirection(const Vector& origin, const Vector& mins, const Vector& maxs, const Vector& forward, int r, int g, int b, int a, float flDuration)              { ft_BoxDirection(origin, mins, maxs, forward, r, g, b, a, flDuration); }
	void BoxAngles(const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)                  { ft_BoxAngles(origin, mins, maxs, angles, r, g, b, a, flDuration); }
	void SweptBox(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration) { ft_SweptBox(start, end, mins, maxs, angles, r, g, b, a, flDuration); }
	void EntityBounds(const CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration)                                                                       { ft_EntityBounds(pEntity, r, g, b, a, flDuration); }
	void Line(const Vector& origin, const Vector& target, int r, int g, int b, bool noDepthTest, float duration)                                                      { ft_Line(origin, target, r, g, b, noDepthTest, duration); }
	void Triangle(const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration)                                 { ft_Triangle(p1, p2, p3, r, g, b, a, noDepthTest, duration); }
	void EntityText(int entityID, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)                                                    { ft_EntityText(entityID, text_offset, text, flDuration, r, g, b, a); }
	void EntityTextAtPosition(const Vector& origin, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)                                  { ft_EntityTextAtPosition(origin, text_offset, text, flDuration, r, g, b, a); }
	void Grid(const Vector& vPosition)                                                                                                                                { ft_Grid(vPosition); }
	void Text(const Vector& origin, const char *text, bool bViewCheck, float flDuration)                                                                              { ft_Text(origin, text, bViewCheck, flDuration); }
	void ScreenText(float flXpos, float flYpos, const char *text, int r, int g, int b, int a, float flDuration)                                                       { ft_ScreenText(flXpos, flYpos, text, r, g, b, a, flDuration); }
	void Cross3D(const Vector& position, const Vector& mins, const Vector& maxs, int r, int g, int b, bool noDepthTest, float flDuration)                             { ft_Cross3D_ext(position, mins, maxs, r, g, b, noDepthTest, flDuration); }
	void Cross3D(const Vector& position, float size, int r, int g, int b, bool noDepthTest, float flDuration)                                                         { ft_Cross3D_size(position, size, r, g, b, noDepthTest, flDuration); }
	void Cross3DOriented(const Vector& position, const QAngle& angles, float size, int r, int g, int b, bool noDepthTest, float flDuration)                           { ft_Cross3DOriented_ang(position, angles, size, r, g, b, noDepthTest, flDuration); }
	void Cross3DOriented(const matrix3x4_t& m, float size, int c, bool noDepthTest, float flDuration)                                                                 { ft_Cross3DOriented_mat(m, size, c, noDepthTest, flDuration); }
	//void DrawOverlayLines()                                                                                                                                           { ft_DrawOverlayLines(); }
	void DrawTickMarkedLine(const Vector& startPos, const Vector& endPos, float tickDist, int tickTextDist, int r, int g, int b, bool noDepthTest, float flDuration)  { ft_DrawTickMarkedLine(startPos, endPos, tickDist, tickTextDist, r, g, b, noDepthTest, flDuration); }
	void DrawGroundCrossHairOverlay()                                                                                                                                 { ft_DrawGroundCrossHairOverlay(); }
	void HorzArrow(const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)                         { ft_HorzArrow(startPos, endPos, width, r, g, b, a, noDepthTest, flDuration); }
	void YawArrow(const Vector& startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)                       { ft_YawArrow(startPos, yaw, length, width, r, g, b, a, noDepthTest, flDuration); }
	void VertArrow(const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)                         { ft_VertArrow(startPos, endPos, width, r, g, b, a, noDepthTest, flDuration); }
	void Axis(const Vector& position, const QAngle& angles, float size, bool noDepthTest, float flDuration)                                                           { ft_Axis(position, angles, size, noDepthTest, flDuration); }
	void Sphere(const Vector& center, float radius, int r, int g, int b, bool noDepthTest, float flDuration)                                                          { ft_Sphere(center, radius, r, g, b, noDepthTest, flDuration); }
	void Circle(const Vector& position, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)                                                { ft_Circle(position, radius, r, g, b, a, bNoDepthTest, flDuration); }
	void Circle(const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)                          { ft_Circle_ang(position, angles, radius, r, g, b, a, bNoDepthTest, flDuration); }
	void Circle(const Vector& position, const Vector& xAxis, const Vector& yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)      { ft_Circle_axes(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration); }
	void Sphere(const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)                          { ft_Sphere_ang(position, angles, radius, r, g, b, a, bNoDepthTest, flDuration); }
}