/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ImWidgets.h"
#include <ctools/FileHelper.h>
#include <Contrib/FontIcons/CustomFont.h>

#include <imgui/imgui.h>

#define STARTING_CUSTOMID 125

#ifdef USE_GLFW3
#include <GLFW/glfw3.h>
#endif // USE_GLFW3

/////////////////////////////////////
/////////////////////////////////////

void ImGui::CustomSameLine(float offset_from_start_x, float spacing_w)
{
	ImGui::SameLine(offset_from_start_x, spacing_w);
}

void ImGui::SetContrastRatio(float vRatio)
{
	CustomStyle::Instance()->puContrastRatio = vRatio;
}

void ImGui::SetContrastedTextColor(ImU32 vColor)
{
	CustomStyle::Instance()->puContrastedTextColor = vColor;
}

void ImGui::DrawContrastWidgets()
{
	ImGui::SliderFloat("Contrast Ratio", &CustomStyle::Instance()->puContrastRatio, 0.0f, 21.0f);

	static ImVec4 contrastedTextColor = ImVec4(0, 0, 0, 1);
	if (ImGui::ColorPicker4("Contrasted Text Color", &contrastedTextColor.x))
	{
		CustomStyle::Instance()->puContrastedTextColor = ImGui::ColorConvertFloat4ToU32(contrastedTextColor);
	}
}

/////////////////////////////////////
/////////////////////////////////////

int ImGui::IncPUSHID()
{
	return ++ImGui::CustomStyle::Instance()->pushId;
}

int ImGui::GetPUSHID()
{
	return ImGui::CustomStyle::Instance()->pushId;
}

void ImGui::SetPUSHID(int vID)
{
	ImGui::CustomStyle::Instance()->pushId = vID;
}

ImVec4 ImGui::GetGoodOrBadColorForUse(bool vUsed)
{
	if (vUsed)
		return ImGui::CustomStyle::Instance()->GoodColor;
	return ImGui::CustomStyle::Instance()->BadColor;
}

#ifdef USE_GLFW3
// viewport mode :
// if not called from a ImGui Window, will return ImVec2(0,0)
// if its your case, you need to set the GLFWwindow
// no issue withotu viewport
ImVec2 ImGui::GetLocalMousePos(GLFWWindow* vWin)
{
#if defined(IMGUI_HAS_VIEWPORT) && defined(GLFW3)
	if (vWin)
	{
		double mouse_x, mouse_y;
		glfwGetCursorPos(vWin, &mouse_x, &mouse_y);
		return ImVec2((float)mouse_x, (float)mouse_y);
	}
	else
	{
		ImGuiContext& g = *GImGui;
		auto viewport = g.CurrentViewport;
		if (!viewport && g.CurrentWindow)
			viewport = g.CurrentWindow->Viewport;
		if (viewport && viewport->PlatformHandle)
		{
			double mouse_x, mouse_y;
			glfwGetCursorPos((GLFWwindow*)viewport->PlatformHandle, &mouse_x, &mouse_y);
			return ImVec2((float)mouse_x, (float)mouse_y);
		}
	}
#else
	return ImGui::GetMousePos();
#endif
	return ImVec2(0.0f, 0.0f);
}
#endif // USE_GLFW3

/////////////////////////////////////
/////////////////////////////////////

// contrast from 1 to 21
// https://www.w3.org/TR/WCAG20/#relativeluminancedef
float ImGui::CalcContrastRatio(const ImU32& backgroundColor, const ImU32& foreGroundColor)
{
	const float sa0 = (float)((backgroundColor >> IM_COL32_A_SHIFT) & 0xFF);
	const float sa1 = (float)((foreGroundColor >> IM_COL32_A_SHIFT) & 0xFF);
	static float sr = 0.2126f / 255.0f;
	static float sg = 0.7152f / 255.0f;
	static float sb = 0.0722f / 255.0f;
	const float contrastRatio =
		(sr * sa0 * ((backgroundColor >> IM_COL32_R_SHIFT) & 0xFF) +
			sg * sa0 * ((backgroundColor >> IM_COL32_G_SHIFT) & 0xFF) +
			sb * sa0 * ((backgroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f) /
		(sr * sa1 * ((foreGroundColor >> IM_COL32_R_SHIFT) & 0xFF) +
			sg * sa1 * ((foreGroundColor >> IM_COL32_G_SHIFT) & 0xFF) +
			sb * sa1 * ((foreGroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f);
	if (contrastRatio < 1.0f)
		return 1.0f / contrastRatio;
	return contrastRatio;
}

bool ImGui::PushStyleColorWithContrast(const ImGuiCol& backGroundColor, const ImGuiCol& foreGroundColor, const ImU32& invertedColor, const float& maxContrastRatio)
{
	const float contrastRatio = CalcContrastRatio(ImGui::GetColorU32(backGroundColor), ImGui::GetColorU32(foreGroundColor));
	if (contrastRatio < maxContrastRatio)
	{
		ImGui::PushStyleColor(foreGroundColor, invertedColor);
		return true;
	}
	return false;
}

bool ImGui::PushStyleColorWithContrast(const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImU32& invertedColor, const float& maxContrastRatio)
{
	const float contrastRatio = CalcContrastRatio(ImGui::GetColorU32(backGroundColor), ImGui::GetColorU32(foreGroundColor));
	if (contrastRatio < maxContrastRatio)
	{
		ImGui::PushStyleColor(foreGroundColor, invertedColor);
		return true;
	}
	return false;
}

bool ImGui::PushStyleColorWithContrast(const ImGuiCol& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio)
{
	const float contrastRatio = CalcContrastRatio(ImGui::GetColorU32(backGroundColor), ImGui::GetColorU32(foreGroundColor));
	if (contrastRatio < maxContrastRatio)
	{
		ImGui::PushStyleColor(foreGroundColor, invertedColor);

		return true;
	}
	return false;
}

bool ImGui::PushStyleColorWithContrast(const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio)
{
	const float contrastRatio = CalcContrastRatio(backGroundColor, ImGui::GetColorU32(foreGroundColor));
	if (contrastRatio < maxContrastRatio)
	{
		ImGui::PushStyleColor(foreGroundColor, invertedColor);
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// https://github.com/ocornut/imgui/issues/3710

inline void PathInvertedRect(ImDrawList* vDrawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, ImDrawFlags rounding_corners)
{
	if (!vDrawList) return;

	rounding = ImMin(rounding, ImFabs(b.x - a.x) *
		(((rounding_corners & ImDrawFlags_RoundCornersTop) == ImDrawFlags_RoundCornersTop) ||
			((rounding_corners & ImDrawFlags_RoundCornersBottom) == ImDrawFlags_RoundCornersBottom) ? 0.5f : 1.0f) - 1.0f);
	rounding = ImMin(rounding, ImFabs(b.y - a.y) *
		(((rounding_corners & ImDrawFlags_RoundCornersLeft) == ImDrawFlags_RoundCornersLeft) ||
			((rounding_corners & ImDrawFlags_RoundCornersRight) == ImDrawFlags_RoundCornersRight) ? 0.5f : 1.0f) - 1.0f);

	if (rounding <= 0.0f || rounding_corners == 0)
	{
		return;
	}
	else
	{
		const float rounding_tl = (rounding_corners & ImDrawFlags_RoundCornersTopLeft) ? rounding : 0.0f;
		vDrawList->PathLineTo(a);
		vDrawList->PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
		vDrawList->PathFillConvex(col);

		const float rounding_tr = (rounding_corners & ImDrawFlags_RoundCornersTopRight) ? rounding : 0.0f;
		vDrawList->PathLineTo(ImVec2(b.x, a.y));
		vDrawList->PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
		vDrawList->PathFillConvex(col);

		const float rounding_br = (rounding_corners & ImDrawFlags_RoundCornersBottomRight) ? rounding : 0.0f;
		vDrawList->PathLineTo(ImVec2(b.x, b.y));
		vDrawList->PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
		vDrawList->PathFillConvex(col);

		const float rounding_bl = (rounding_corners & ImDrawFlags_RoundCornersBottomLeft) ? rounding : 0.0f;
		vDrawList->PathLineTo(ImVec2(a.x, b.y));
		vDrawList->PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
		vDrawList->PathFillConvex(col);
	}
}

void ImGui::AddInvertedRectFilled(ImDrawList* vDrawList, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags rounding_corners)
{
	if (!vDrawList) return;

	if ((col & IM_COL32_A_MASK) == 0) return;
	if (rounding > 0.0f)
		PathInvertedRect(vDrawList, p_min, p_max, col, rounding, rounding_corners);
}

// Render a rectangle shaped with optional rounding and borders
void ImGui::RenderInnerShadowFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, ImU32 fill_col_darker, ImU32 bg_Color, bool border, float rounding)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
#if 0
	window->DrawList->AddRectFilled(p_min, p_max, fill_col, rounding);
#else
	window->DrawList->AddRectFilledMultiColor(p_min, p_max, fill_col, fill_col, fill_col_darker, fill_col_darker);
	AddInvertedRectFilled(window->DrawList, p_min, p_max, bg_Color, rounding, ImDrawFlags_RoundCornersAll);
#endif
	const float border_size = g.Style.FrameBorderSize;
	if (border && border_size > 0.0f)
	{
		window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawFlags_RoundCornersAll, border_size);
		window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, ImDrawFlags_RoundCornersAll, border_size);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ImGui::DrawShadowImage(ImTextureID vShadowImage, const ImVec2& vSize, ImU32 col)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + vSize);
	ItemSize(bb);
	if (!ItemAdd(bb, 0))
		return;

	window->DrawList->AddImage(vShadowImage, bb.Min, bb.Max, ImVec2(0, 0), ImVec2(1, 1), col);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ImRatioX(a) a.x / a.y
#define ImRatioY(a) a.y / a.x

// based on ImGui::ImageButton
bool ImGui::ImageCheckButton(
	ImTextureID user_texture_id, bool* v,
	const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1,
	const ImVec2& vHostTextureSize, int frame_padding,
	float vRectThickNess, ImVec4 vRectColor)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	// Default to using texture ID as ID. User can still push string/integer prefixes.
	// We could hash the size/uv to create a unique ID but that would prevent the user from animating UV.
	PushID((void*)(intptr_t)user_texture_id);
	const ImGuiID id = window->GetID("#image");
	PopID();

	const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
	const ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);
	ItemSize(bb);
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	const bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	if (pressed && v)
		*v = !*v;

	// Render
	const ImU32 col = GetColorU32(((held && hovered) || (v && *v)) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, style.FrameRounding));
	if (vRectThickNess > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(vRectColor), 0.0, ImDrawFlags_RoundCornersAll, vRectThickNess);
	}

	// resize with respect to glyph ratio
	float hostRatioX = 1.0f;
	if (vHostTextureSize.y > 0)
		hostRatioX = ImRatioX(vHostTextureSize);
	const ImVec2 uvSize = uv1 - uv0;
	const float ratioX = ImRatioX(uvSize) * hostRatioX;
	const ImVec2 imgSize = image_bb.GetSize();
	const float newX = imgSize.y * ratioX;
	ImVec2 glyphSize = ImVec2(imgSize.x, imgSize.x / ratioX) * 0.5f;
	if (newX < imgSize.x) glyphSize = ImVec2(newX, imgSize.y) * 0.5f;
	const ImVec2 center = image_bb.GetCenter();
	window->DrawList->AddImage(user_texture_id, center - glyphSize, center + glyphSize, uv0, uv1, GetColorU32(ImGuiCol_Text));

	return pressed;
}

bool ImGui::BeginFramedGroup(const char* vLabel, ImVec2 /*vSize*/, ImVec4 /*vCol*/, ImVec4 /*vHoveredCol*/)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	draw_list->ChannelsSplit(2); // split for have 2 layers

	draw_list->ChannelsSetCurrent(1); // Layer ForeGround

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	window->ContentRegionRect.Max.x -= style.FramePadding.x * 3.0f;
	window->WorkRect.Max.x -= style.FramePadding.x * 3.0f;

	//const ImGuiID id = window->GetID(vLabel);
	ImGui::BeginGroup();
	ImGui::Indent();

	FramedGroupText(vLabel);

	return true;
}

void ImGui::EndFramedGroup(ImGuiCol vHoveredIdx, ImGuiCol NormalIdx)
{
	ImGui::Unindent();
	ImGui::Spacing();
	ImGui::EndGroup();

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->ContentRegionRect.Max.x += style.FramePadding.x * 3.0f;
	window->WorkRect.Max.x += style.FramePadding.x * 3.0f;

	draw_list->ChannelsSetCurrent(0); // Layer Background

	ImVec2 p_min = ImGui::GetItemRectMin();
	p_min.x = window->WorkRect.Min.x;

	ImVec2 p_max = ImGui::GetItemRectMax();
	p_max.x = window->WorkRect.Max.x;

	const ImU32 frameCol = ImGui::GetColorU32(ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) ? vHoveredIdx : NormalIdx);
	//const ImU32 frameCol = ImGui::GetColorU32(idx);
	ImGui::RenderFrame(p_min, p_max, frameCol, true, style.FrameRounding);

	draw_list->ChannelsMerge(); // merge layers
}

void ImGui::FramedGroupSeparator()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	ImRect bb;
	bb.Min.x = window->DC.CursorPos.x;
	bb.Min.y = window->DC.CursorPos.y;
	bb.Max.x = window->WorkRect.Max.x;
	bb.Max.y = window->DC.CursorPos.y + style.FramePadding.y;

	ImGui::ItemSize(bb, style.FramePadding.y);
	if (ImGui::ItemAdd(bb, 0))
	{
		const ImU32 lineCol = ImGui::GetColorU32(ImGuiCol_FrameBg);
		window->DrawList->AddLine(
			ImVec2(bb.Min.x, bb.Max.y - style.FramePadding.y * 0.5f),
			ImVec2(bb.Max.x, bb.Max.y - style.FramePadding.y * 0.5f), lineCol);
		if (g.LogEnabled)
			LogRenderedText(&bb.Min, "--------------------------------");
	}
}

void ImGui::FramedGroupText(ImVec4* vTextColor, const char* vHelp, const char* vFmt, va_list vArgs)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	static char TempBuffer[2048] = "\0";
	int w = vsnprintf(TempBuffer, 2046, vFmt, vArgs);
	if (w)
	{
		TempBuffer[w + 1] = '\0'; // 2046 + 1 = 2047 => ok (under array size of 2048 in any case)
		ImGuiContext& g = *GImGui;
		const ImGuiID id = window->GetID(TempBuffer);
		const ImGuiStyle& style = g.Style;
		const ImVec2 label_size = ImGui::CalcTextSize(TempBuffer, nullptr, true);

		const float frame_height =
			ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y),
				label_size.y + style.FramePadding.y);
		ImRect bb;
		bb.Min.x = window->WorkRect.Min.x;
		bb.Min.y = window->DC.CursorPos.y;
		bb.Max.x = window->WorkRect.Max.x;
		bb.Max.y = window->DC.CursorPos.y + frame_height;

		ImGui::ItemSize(bb, 0.0f);
		if (ImGui::ItemAdd(bb, id))
		{
			if (vTextColor)
				ImGui::PushStyleColor(ImGuiCol_Text, *vTextColor);
			ImGui::RenderTextClipped(bb.Min, bb.Max, TempBuffer, nullptr, &label_size, style.ButtonTextAlign, &bb);
			if (vTextColor)
				ImGui::PopStyleColor();
		}
	}

	if (vHelp)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", vHelp);
}

void ImGui::FramedGroupText(const char* vFmt, ...)
{
	va_list args;
	va_start(args, vFmt);
	FramedGroupText(nullptr, 0, vFmt, args);
	va_end(args);
}

// renamed because "FramedGroupTextHelp(const char* vHelp, const char* vFmt"
// can be choosed by the compiler for "FramedGroupText(const char* vFmt"
void ImGui::FramedGroupTextHelp(const char* vHelp, const char* vFmt, ...)
{
	va_list args;
	va_start(args, vFmt);
	FramedGroupText(nullptr, vHelp, vFmt, args);
	va_end(args);
}

void ImGui::FramedGroupText(ImVec4 vTextColor, const char* vFmt, ...)
{
	va_list args;
	va_start(args, vFmt);
	FramedGroupText(&vTextColor, 0, vFmt, args);
	va_end(args);
}

bool ImGui::CheckBoxIcon(const char* vLabel,  const char* vIconTrue, bool* v)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(vLabel);
	const ImVec2 label_size = CalcTextSize(vLabel, NULL, true);

	const float square_sz = GetFrameHeight();
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
	{
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
		return false;
	}

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}

	const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
	RenderNavHighlight(total_bb, id);
	RenderFrame(check_bb.Min, check_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), true, style.FrameRounding);
	ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
	bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
	if (mixed_value)
	{
		// Undocumented tristate/mixed/indeterminate checkbox (#2644)
		// This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
		ImVec2 pad(ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)), ImMax(1.0f, IM_FLOOR(square_sz / 3.6f)));
		window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
	}
	else if (*v)
	{
		const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
		//RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
		//ImGui::PushStyleColor(ImGuiCol_Text, check_col);
		RenderText(check_bb.Min + ImVec2(pad, pad), vIconTrue);
		//ImGui::PopStyleColor();
	}

	ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
	if (g.LogEnabled)
		LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");
	if (label_size.x > 0.0f)
		RenderText(label_pos, vLabel);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	return pressed;
}

bool ImGui::CheckBoxBoolDefault(const char* vName, bool* vVar, bool vDefault, const char* vHelp, ImFont* vLabelFont)
{
	bool change = false;

	change = ImGui::ContrastedButton(ICON_NDP_RESET, "Reset", vLabelFont);
	if (change)
		*vVar = vDefault;

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	ImGui::PushID(++CustomStyle::Instance()->pushId);

	change |= ImGui::Checkbox(vName, vVar);

	ImGui::PopID();

	if (vHelp)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", vHelp);

	return change;
}

bool ImGui::CheckBoxFloat(const char* vName, float* vVar)
{
	bool change = false;

	ImGui::PushID(ImGui::IncPUSHID());
	bool v = (*vVar > 0.5);
	if (ImGui::Checkbox(vName, &v))
	{
		change = true;
		*vVar = v ? 1.0f : 0.0f;
	}
	ImGui::PopID();

	return change;
}

bool ImGui::CheckBoxIntDefault(const char* vName, int* vVar, int vDefault, const char* vHelp, ImFont* vLabelFont)
{
	bool change = false;

	//float padding = ImGui::GetStyle().FramePadding.x;

	change = ImGui::ContrastedButton(ICON_NDP_RESET, "Reset", vLabelFont);
	if (change)
		*vVar = vDefault;

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	ImGui::PushID(++CustomStyle::Instance()->pushId);

	change |= ImGui::CheckBoxInt(vName, vVar);

	ImGui::PopID();

	if (vHelp)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", vHelp);

	return change;
}

bool ImGui::CheckBoxInt(const char* vName, int* vVar)
{
	bool change = false;

	ImGui::PushID(ImGui::IncPUSHID());
	bool v = !!(*vVar);
	if (ImGui::Checkbox(vName, &v))
	{
		change = true;
		*vVar = !!(v);
	}
	ImGui::PopID();

	return change;
}

bool ImGui::CheckBoxFloatDefault(const char* vName, float* vVar, float vDefault, const char* vHelp, ImFont* vLabelFont)
{
	bool change = false;

	change = ImGui::ContrastedButton(ICON_NDP_RESET, "Reset", vLabelFont);
	if (change)
		*vVar = vDefault;

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	change |= CheckBoxFloat(vName, vVar);

	if (vHelp)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", vHelp);

	return change;
}

bool ImGui::RadioFloatDefault(const char* vName, float* vVar, int vCount, float* vDefault, const char* vHelp, ImFont* vLabelFont)
{
	bool change = false;

	//float padding = ImGui::GetStyle().FramePadding.x;

	ImGui::BeginGroup();
	{
		change = ImGui::ContrastedButton(ICON_NDP_RESET, "Reset", vLabelFont);
		if (change)
			*vVar = *vDefault;

		ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

		int radioSelectedId = 0;
		for (int i = 0; i < vCount; ++i)
		{
			if (i > 0) ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::PushID(ImGui::IncPUSHID());
			bool v = (vVar[i] > 0.5f);
			if (ImGui::Checkbox("##radio", &v))
			{
				radioSelectedId = i;
				change = true;
			}
			ImGui::PopID();
		}

		ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

		ImGui::Text("%s", vName);

		if (change)
		{
			for (int j = 0; j < vCount; j++)
			{
				vVar[j] = (radioSelectedId == j ? 1.0f : 0.0f);
			}
		}
	}
	ImGui::EndGroup();

	if (vHelp)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", vHelp);

	return change;
}

void sc_ImFormatStringToTempBufferV(const char** out_buf, const char** out_buf_end, const char* fmt, va_list args)
{
	ImGuiContext& g = *GImGui;
	int buf_len = ImFormatStringV(g.TempBuffer.Data, g.TempBuffer.Size, fmt, args);
	*out_buf = g.TempBuffer.Data;
	if (out_buf_end) { *out_buf_end = g.TempBuffer.Data + buf_len; }
}

bool ImGui::Selectable_FramedText_Selected(const bool& vSelected, const char* fmt, ...)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	va_list args;
	va_start(args, fmt);
	const char* text, * text_end;
	sc_ImFormatStringToTempBufferV(&text, &text_end, fmt, args);
	va_end(args);

	const ImVec2 label_size = CalcTextSize(text, text_end, true);

	const ImGuiID id = window->GetID(text);

	const ImRect check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(label_size.x + style.FramePadding.x * 4 - 1, label_size.y + style.FramePadding.y * 2 - 1));
	ItemSize(check_bb, style.FramePadding.y);

	//frame
	ImRect total_bb = check_bb;
	if (label_size.x > 0)
		ImGui::CustomSameLine(0, style.ItemInnerSpacing.x);
	const ImRect text_bb(window->DC.CursorPos + ImVec2(0, style.FramePadding.y), window->DC.CursorPos + ImVec2(0, style.FramePadding.y) + label_size);
	if (label_size.x > 0)
	{
		ItemSize(ImVec2(total_bb.GetWidth(), total_bb.GetHeight()), style.FramePadding.y);
		total_bb.Add(total_bb);
	}

	if (!ItemAdd(total_bb, 0))
		return false;

	bool hovered, held;
	const bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);

	// frame display
	const ImU32 col = GetColorU32(
		(hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button,
		(hovered && held) ? 1.0f : hovered ? 1.0f : 0.0f);
	window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, col, style.FrameRounding);

	if (label_size.x > 0.0f)
	{
		if (vSelected || hovered)
		{
			PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		}

		RenderText(total_bb.Min + ImVec2(style.FramePadding.x * 2.0f, style.FramePadding.y * 0.5f), text, text_end);

		if (vSelected || hovered)
		{
			PopStyleColor();
		}
	}

	return pressed;
}

bool ImGui::Selectable_FramedText(const char* fmt, ...)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	va_list args;
	va_start(args, fmt);
	const char* text, * text_end;
	sc_ImFormatStringToTempBufferV(&text, &text_end, fmt, args);
	va_end(args);

	const ImVec2 label_size = CalcTextSize(text, text_end, true);

	const ImGuiID id = window->GetID(text);

	const ImRect check_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(label_size.x + style.FramePadding.x * 4 - 1, label_size.y + style.FramePadding.y * 2 - 1));
	ItemSize(check_bb, style.FramePadding.y);

	//frame
	ImRect total_bb = check_bb;
	if (label_size.x > 0)
		ImGui::CustomSameLine(0, style.ItemInnerSpacing.x);
	const ImRect text_bb(window->DC.CursorPos + ImVec2(0, style.FramePadding.y), window->DC.CursorPos + ImVec2(0, style.FramePadding.y) + label_size);
	if (label_size.x > 0)
	{
		ItemSize(ImVec2(total_bb.GetWidth(), total_bb.GetHeight()), style.FramePadding.y);
		total_bb.Add(total_bb);
	}

	if (!ItemAdd(total_bb, 0))
		return false;

	bool hovered, held;
	const bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);

	// frame display
	const ImU32 col = GetColorU32(
		(hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button,
		(hovered && held) ? 1.0f : hovered ? 1.0f : 0.0f);
	window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, col, style.FrameRounding);

	if (label_size.x > 0.0f)
	{
		if (hovered)
		{
			PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		}

		RenderText(total_bb.Min + ImVec2(style.FramePadding.x * 2.0f, style.FramePadding.y * 0.5f), text, text_end);

		if (hovered)
		{
			PopStyleColor();
		}
	}

	return pressed;
}

bool ImGui::RadioButtonLabeled(float vWidth, const char* label, bool active, bool disabled)
{
	using namespace ImGui;

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	float w = vWidth;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, nullptr, true);
	if (w < 0.0f) w = ImGui::GetContentRegionMaxAbs().x - window->DC.CursorPos.x;
	if (IS_FLOAT_EQUAL(w, 0.0f)) w = label_size.x + style.FramePadding.x * 2.0f;
	const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	// check
	bool pressed = false;
	ImGuiCol colUnderText = ImGuiCol_Button;
	if (!disabled)
	{
		bool hovered, held;
		pressed = ButtonBehavior(total_bb, id, &hovered, &held);

		colUnderText = ImGuiCol_FrameBg;
		window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : colUnderText), style.FrameRounding);
		if (active)
		{
			colUnderText = ImGuiCol_Button;
			window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, GetColorU32((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : colUnderText), style.FrameRounding);
		}
	}

	// circle shadow + bg
	if (style.FrameBorderSize > 0.0f)
	{
		window->DrawList->AddRect(total_bb.Min + ImVec2(1, 1), total_bb.Max, GetColorU32(ImGuiCol_BorderShadow), style.FrameRounding);
		window->DrawList->AddRect(total_bb.Min, total_bb.Max, GetColorU32(ImGuiCol_Border), style.FrameRounding);
	}

	if (label_size.x > 0.0f)
	{
		const bool pushed = ImGui::PushStyleColorWithContrast(colUnderText, ImGuiCol_Text, CustomStyle::Instance()->puContrastedTextColor, CustomStyle::Instance()->puContrastRatio);
		RenderTextClipped(total_bb.Min, total_bb.Max, label, nullptr, &label_size, ImVec2(0.5f, 0.5f));
		if (pushed)
			ImGui::PopStyleColor();
	}

	return pressed;
}

bool ImGui::RadioButtonLabeled(float vWidth, const char* label, const char* help, bool active, bool disabled, ImFont* vLabelFont)
{
	if (vLabelFont)	ImGui::PushFont(vLabelFont);
	const bool change = RadioButtonLabeled(vWidth, label, active, disabled);
	if (vLabelFont)	ImGui::PopFont();
	if (help)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", help);
	return change;
}

bool ImGui::RadioButtonLabeled(float vWidth, const char* label, const char* help, bool* active, bool disabled, ImFont* vLabelFont)
{
	if (vLabelFont)	ImGui::PushFont(vLabelFont);
	const bool change = RadioButtonLabeled(vWidth, label, *active, disabled);
	if (vLabelFont)	ImGui::PopFont();
	if (change)
		*active = !*active;
	if (help)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", help);
	return change;
}

bool ImGui::CollapsingHeader_SmallHeight(const char* vName, float vHeightRatio, float vWidth, bool vDefaulExpanded, bool* vIsOpen)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(vName);
	ImVec2 label_size = ImGui::CalcTextSize(vName, nullptr, true);
	label_size.y *= vHeightRatio;

	//label_size.x = ImMin(label_size.x, vWidth);
	ImVec2 padding = style.FramePadding;
	padding.y *= vHeightRatio;

	const ImVec2 pos = window->DC.CursorPos;
	const ImVec2 nsize = ImGui::CalcItemSize(
		ImVec2(vWidth, label_size.y + padding.y * 2.0f),
		label_size.x + padding.x * 2.0f, label_size.y + padding.y * 2.0f);
	//nsize.y *= vHeightRatio;

	ImRect bb(pos, pos + nsize);
	const ImRect bbTrigger = bb;
	ImGui::ItemSize(bb, padding.y);
	ImGui::ItemAdd(bb, id);

	bool is_open = vDefaulExpanded;
	if (vIsOpen && !*vIsOpen) is_open = false;
	is_open = window->DC.StateStorage->GetInt(id, is_open ? 1 : 0) != 0;
	bool hovered, held;
	const bool pressed = ImGui::ButtonBehavior(bbTrigger, id, &hovered, &held, 0);
	if (pressed)
	{
		is_open = !is_open;
		window->DC.StateStorage->SetInt(id, is_open);
		if (vIsOpen)
			*vIsOpen = is_open;
	}

	// Render
	static ImVec4 _ScrollbarGrab(0.5f, 0.0f, 1.0f, 0.50f);
	static ImVec4 _ScrollbarGrabHovered(0.4f, 0.0f, 0.75f, 0.90f);
	static ImVec4 _ScrollbarGrabActive(0.3f, 0.0f, 0.5f, 0.90f);

	const ImU32 col = ImGui::GetColorU32((held && hovered) ? _ScrollbarGrabActive : hovered ? _ScrollbarGrabHovered : _ScrollbarGrab);
	ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
	ImGui::RenderTextClipped(bb.Min, bb.Max - padding, vName, nullptr, &label_size, style.ButtonTextAlign, &bb);
	padding.y *= vHeightRatio;
	RenderArrow(window->DrawList, bb.Min + padding, ImGui::GetColorU32(ImGuiCol_Text),
		(is_open ? ImGuiDir_::ImGuiDir_Down : ImGuiDir_::ImGuiDir_Right), 1.0f);

	return is_open;
}

bool ImGui::CollapsingHeader_CheckBox(const char* vName, float vWidth, bool vDefaulExpanded, bool vShowCheckBox, bool* vCheckCatched)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(vName);
	ImVec2 label_size = ImGui::CalcTextSize(vName, nullptr, true);
	//label_size.x = ImMin(label_size.x, vWidth);
	const ImVec2 padding = ImGui::GetStyle().FramePadding;
	const float text_base_offset_y = ImMax(0.0f, window->DC.CurrLineTextBaseOffset - padding.y); // Latch before ItemSize changes it

	const ImVec2 pos = window->DC.CursorPos;
	const ImVec2 nsize = ImGui::CalcItemSize(ImVec2(vWidth, label_size.y + style.FramePadding.y * 2.0f), label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	ImRect bb(pos, pos + nsize);
	ImRect bbTrigger = bb;
	if (vShowCheckBox && vCheckCatched != nullptr)
		bbTrigger.Max.x -= nsize.y;
	ImGui::ItemSize(bb, style.FramePadding.y);
	ImGui::ItemAdd(bb, id);

	bool is_open = window->DC.StateStorage->GetInt(id, vDefaulExpanded ? 1 : 0) != 0;
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bbTrigger, id, &hovered, &held, 0);
	//ImGui::SetItemAllowOverlap();
	if (pressed)
	{
		is_open = !is_open;
		window->DC.StateStorage->SetInt(id, is_open);
	}

	// Render
	const ImU32 colArrow = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
	ImGui::RenderFrame(bb.Min, bb.Max, colArrow, true, style.FrameRounding);
	ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, vName, nullptr, &label_size, style.ButtonTextAlign, &bb);
	RenderArrow(window->DrawList, bb.Min + padding + ImVec2(0.0f, text_base_offset_y), ImGui::GetColorU32(ImGuiCol_Text),
		(is_open ? ImGuiDir_::ImGuiDir_Down : ImGuiDir_::ImGuiDir_Right), 1.0f);

	// menu
	if (vShowCheckBox && vCheckCatched)
	{
		// item
		const ImGuiID extraId = window->GetID((void*)(intptr_t)(id + 1));
		ImRect bbMenu(ImVec2(bb.Max.x - nsize.y, bb.Min.y), bb.Max);
		bbMenu.Min.y += nsize.y * 0.1f;
		bbMenu.Max.x -= nsize.y * 0.1f;
		bbMenu.Max.y -= nsize.y * 0.1f;

		// detection
		pressed = ImGui::ButtonBehavior(bbMenu, extraId, &hovered, &held, 0);
		if (pressed)
		{
			*vCheckCatched = !(*vCheckCatched);
			ImGui::MarkItemEdited(extraId);
		}

		// render
		const ImU32 colFrame = ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
		ImGui::RenderFrame(bbMenu.Min, bbMenu.Max, colFrame, true, style.FrameRounding);
		//ImVec2 iconPos = ImVec2(pos.x + nsize.x - nsize.y * 0.5f, pos.y + nsize.y * 0.5f);
		if (*vCheckCatched)
		{
			const float sizey = nsize.y;
			const float pad = ImMax(1.0f, (float)(int)(sizey / 6.0f));
			ImGui::RenderCheckMark(window->DrawList, bbMenu.Min + ImVec2(pad, pad - 0.1f * sizey), ImGui::GetColorU32(ImGuiCol_CheckMark), sizey - pad * 2.0f);
		}
	}

	return is_open;
}

bool ImGui::CollapsingHeader_Button(const char* vName, float vWidth, bool vDefaulExpanded, 
	const char* vLabelButton, bool vShowButton, bool* vButtonPressed, ImFont* /*vButtonFont*/)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(vName);
	ImVec2 label_size = ImGui::CalcTextSize(vName, nullptr, true);
	//label_size.x = ImMin(label_size.x, vWidth);
	const ImVec2 padding = ImGui::GetStyle().FramePadding;
	const float text_base_offset_y = ImMax(0.0f, window->DC.CurrLineTextBaseOffset - padding.y); // Latch before ItemSize changes it

	const ImVec2 pos = window->DC.CursorPos;
	const ImVec2 nsize = ImGui::CalcItemSize(ImVec2(vWidth, label_size.y + style.FramePadding.y * 2.0f), label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	ImRect bb(pos, pos + nsize);
	ImRect bbTrigger = bb;
	if (vButtonPressed && vShowButton)
		bbTrigger.Max.x -= nsize.y;
	ImGui::ItemSize(bb, style.FramePadding.y);
	ImGui::ItemAdd(bb, id);

	bool is_open = window->DC.StateStorage->GetInt(id, vDefaulExpanded ? 1 : 0) != 0;
	bool hovered, held;
	const bool pressed = ImGui::ButtonBehavior(bbTrigger, id, &hovered, &held, 0);
	//ImGui::SetItemAllowOverlap();
	if (pressed)
	{
		is_open = !is_open;
		window->DC.StateStorage->SetInt(id, is_open);
	}

	// Render
	const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
	ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
	ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, vName, nullptr, &label_size, style.ButtonTextAlign, &bb);
	RenderArrow(window->DrawList, bb.Min + padding + ImVec2(0.0f, text_base_offset_y), ImGui::GetColorU32(ImGuiCol_Text),
		(is_open ? ImGuiDir_::ImGuiDir_Down : ImGuiDir_::ImGuiDir_Right), 1.0f);

	// menu
	if (vButtonPressed && vShowButton && vLabelButton)
	{
		const ImVec2 button_label_size = ImGui::CalcTextSize(vLabelButton, nullptr, true);
		
		// item
		const float button_width = ImMax(button_label_size.x + padding.x, bb.Max.y - bb.Min.y);

		const ImGuiID extraId = window->GetID((void*)(intptr_t)(id + 1));
		const ImRect bbMenu(ImVec2(bb.Max.x - button_width, bb.Min.y), bb.Max);
		const ImVec2 offset = (bbMenu.GetSize() - button_label_size) * 0.5f;

		// detection
		bool menuHovered, menuHeld;
		*vButtonPressed |= ImGui::ButtonBehavior(bbMenu, extraId, &menuHovered, &menuHeld, 0);
		const ImU32 menuCol = ImGui::GetColorU32((menuHovered) ? ImGuiCol_ButtonHovered : menuHovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);

		// render
		ImGui::RenderFrame(bbMenu.Min, bbMenu.Max, menuCol, true, style.FrameRounding);
		ImGui::RenderTextClipped(bbMenu.Min + offset, bbMenu.Max, vLabelButton, nullptr, &button_label_size);
	}

	return is_open;
}

bool ImGui::ButtonNoFrame(const char* vLabel, ImVec2 size, ImVec4 vColor, const char* vHelp, ImFont* vLabelFont)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	//ImGuiContext& g = *GImGui;
	ImGui::PushID(++CustomStyle::Instance()->pushId);
	const ImGuiID id = window->GetID(vLabel);
	ImGui::PopID();
	const float h = ImGui::GetFrameHeight();
	const ImVec2 sz = ImMax(ImVec2(h, h), size);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + sz);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	bool hovered, held;
	const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

	if (vLabelFont) ImGui::PushFont(vLabelFont);
	ImGui::PushStyleColor(ImGuiCol_Text, vColor);
	RenderTextClipped(bb.Min, bb.Max, vLabel, nullptr, nullptr, ImVec2(0.5f, 0.5f));
	ImGui::PopStyleColor();
	if (vLabelFont)	ImGui::PopFont();

	if (vHelp)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", vHelp);

	return pressed;
}

bool ImGui::SmallContrastedButton(const char* label)
{
	ImGuiContext& g = *GImGui;
	float backup_padding_y = g.Style.FramePadding.y;
	g.Style.FramePadding.y = 0.0f;
	const bool pushed = ImGui::PushStyleColorWithContrast(ImGuiCol_Button, ImGuiCol_Text, CustomStyle::Instance()->puContrastedTextColor, CustomStyle::Instance()->puContrastRatio);
	ImGui::PushID(++CustomStyle::Instance()->pushId);
	bool pressed = ButtonEx(label, ImVec2(0, 0), ImGuiButtonFlags_AlignTextBaseLine);
	ImGui::PopID();
	if (pushed)
		ImGui::PopStyleColor();
	g.Style.FramePadding.y = backup_padding_y;
	return pressed;
}

bool ImGui::ClickableTextUrl(const char* label, const char* url, bool vOnlined)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
	const ImVec2 pos = window->DC.CursorPos;
	const ImVec2 size = ImGui::CalcItemSize(ImVec2(0.0f, 0.0f), label_size.x + style.FramePadding.x * 1.0f, label_size.y);
	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(bb, 0.0f);
	if (!ImGui::ItemAdd(bb, id))
		return false;
	const ImGuiButtonFlags flags = 0;
	bool hovered, held;
	const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);
	if (held || (g.HoveredId == id && g.HoveredIdPreviousFrame == id))
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	ImGui::RenderNavHighlight(bb, id);
	ImVec4 defColor = ImGui::GetStyleColorVec4(ImGuiCol_Text); defColor.w = 0.5f;
	ImVec4 hovColor = defColor; hovColor.w = 0.75f;
	ImVec4 actColor = defColor; actColor.w = 1.0f;
	const ImVec4 col = (hovered && held) ? actColor : hovered ? hovColor : defColor;
	ImVec2 p0 = bb.Min;
	ImVec2 p1 = bb.Max;
	if (hovered && held)
	{
		p0 += ImVec2(1, 1);
		p1 += ImVec2(1, 1);
	}
	if (vOnlined)
		window->DrawList->AddLine(ImVec2(p0.x + style.FramePadding.x, p1.y), ImVec2(p1.x - style.FramePadding.x, p1.y), ImGui::GetColorU32(col));
	ImGui::PushStyleColor(ImGuiCol_Text, col);
	ImGui::RenderTextClipped(p0, p1, label, nullptr, &label_size, style.ButtonTextAlign, &bb);
	ImGui::PopStyleColor();

	if (hovered)
	{
		ImGui::SetTooltip("%s", url);
	}

	if (pressed)
	{
		FileHelper::Instance()->OpenUrl(url);
	}

	return pressed;
}

bool ImGui::ClickableTextFile(const char* label, const char* file, bool vOnlined)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
	const ImVec2 pos = window->DC.CursorPos;
	const ImVec2 size = ImGui::CalcItemSize(ImVec2(0.0f, 0.0f), label_size.x + style.FramePadding.x * 1.0f, label_size.y);
	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(bb, 0.0f);
	if (!ImGui::ItemAdd(bb, id))
		return false;
	const ImGuiButtonFlags flags = 0;
	bool hovered, held;
	const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);
	if (held || (g.HoveredId == id && g.HoveredIdPreviousFrame == id))
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	ImGui::RenderNavHighlight(bb, id);
	ImVec4 defColor = ImGui::GetStyleColorVec4(ImGuiCol_Text); defColor.w = 0.5f;
	ImVec4 hovColor = defColor; hovColor.w = 0.75f;
	ImVec4 actColor = defColor; actColor.w = 1.0f;
	const ImVec4 col = (hovered && held) ? actColor : hovered ? hovColor : defColor;
	ImVec2 p0 = bb.Min;
	ImVec2 p1 = bb.Max;
	if (hovered && held)
	{
		p0 += ImVec2(1, 1);
		p1 += ImVec2(1, 1);
	}
	if (vOnlined)
		window->DrawList->AddLine(ImVec2(p0.x + style.FramePadding.x, p1.y), ImVec2(p1.x - style.FramePadding.x, p1.y), ImGui::GetColorU32(col));
	ImGui::PushStyleColor(ImGuiCol_Text, col);
	ImGui::RenderTextClipped(p0, p1, label, nullptr, &label_size, style.ButtonTextAlign, &bb);
	ImGui::PopStyleColor();

	if (hovered)
	{
		ImGui::SetTooltip("%s", file);
	}

	if (pressed)
	{
		FileHelper::Instance()->OpenFile(file);
	}

	return pressed;
}

void ImGui::Header(const char* vName, float width)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(vName);
	const ImVec2 label_size = ImGui::CalcTextSize(vName, NULL, true);

	float w = width;
	if (width <= 0.0f)
	{
		w = ImGui::GetContentRegionAvail().x;
	}

	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id, &frame_bb))
		return;

	bool hovered, held;
	/*bool pressed = */ImGui::ButtonBehavior(frame_bb, id, &hovered, &held, 0);
	ImGui::SetItemAllowOverlap();

	// Render
	const ImU32 col = ImGui::GetColorU32(hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
	ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, col, true, style.FrameRounding);
	ImGui::RenderTextClipped(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding, vName, NULL, &label_size, style.ButtonTextAlign, &frame_bb);
}

void ImGui::ImageRect(ImTextureID user_texture_id, const ImVec2& pos, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + pos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ItemSize(bb);
	if (!ItemAdd(bb, 0))
		return;

	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
		window->DrawList->AddImage(user_texture_id, bb.Min + pos + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, GetColorU32(tint_col));
	}
	else
	{
		window->DrawList->AddImage(user_texture_id, bb.Min + pos, bb.Max, uv0, uv1, GetColorU32(tint_col));
	}
}

bool ImGui::ColorEdit3Default(float vWidth, const char* vName, float* vCol, float* vDefault)
{
	bool change = false;

	float padding = ImGui::GetStyle().FramePadding.x;

	if (!ImGui::GetCurrentWindow()->ScrollbarY)
	{
		vWidth -= ImGui::GetStyle().ScrollbarSize;
	}

	ImGui::PushID(ImGui::IncPUSHID());
	change = ImGui::ContrastedButton(ICON_NDP_RESET);
	ImGui::PopID();
	float w = vWidth - ImGui::GetItemRectSize().x - padding * 2.0f;
	if (change)
	{
		vCol[0] = vDefault[0];
		vCol[1] = vDefault[1];
		vCol[2] = vDefault[2];
	}

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	ImGui::PushID(ImGui::IncPUSHID());
	ImGui::PushItemWidth(w);
	change |= ImGui::ColorEdit3(vName, vCol, ImGuiColorEditFlags_Float);
	ImGui::PopItemWidth();
	ImGui::PopID();

	return change;
}

bool ImGui::ColorEdit4Default(float vWidth, const char* vName, float* vCol, float* vDefault)
{
	bool change = false;

	float padding = ImGui::GetStyle().FramePadding.x;

	if (!ImGui::GetCurrentWindow()->ScrollbarY)
	{
		vWidth -= ImGui::GetStyle().ScrollbarSize;
	}

	ImGui::PushID(ImGui::IncPUSHID());
	change = ImGui::ContrastedButton(ICON_NDP_RESET);
	ImGui::PopID();
	float w = vWidth - ImGui::GetItemRectSize().x - padding * 2.0f;
	if (change)
	{
		vCol[0] = vDefault[0];
		vCol[1] = vDefault[1];
		vCol[2] = vDefault[2];
		vCol[3] = vDefault[3];
	}

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	ImGui::PushID(ImGui::IncPUSHID());
	ImGui::PushItemWidth(w);
	change |= ImGui::ColorEdit4(vName, vCol, 
		ImGuiColorEditFlags_Float | 
		ImGuiColorEditFlags_AlphaBar | 
		ImGuiColorEditFlags_AlphaPreview);
	ImGui::PopItemWidth();
	ImGui::PopID();

	return change;
}

void ImGui::Spacing(float vSpace)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;
	ItemSize(ImVec2(vSpace, 0));
}

ImGuiWindow* ImGui::GetHoveredWindow()
{
	ImGuiContext& g = *GImGui;
	return g.HoveredWindow;
}

bool ImGui::BeginMainStatusBar()
{
	ImGuiContext& g = *GImGui;
	ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)GetMainViewport();

	// Notify of viewport change so GetFrameHeight() can be accurate in case of DPI change
	SetCurrentViewport(NULL, viewport);

	// For the main menu bar, which cannot be moved, we honor g.Style.DisplaySafeAreaPadding to ensure text can be visible on a TV set.
	// FIXME: This could be generalized as an opt-in way to clamp window->DC.CursorStartPos to avoid SafeArea?
	// FIXME: Consider removing support for safe area down the line... it's messy. Nowadays consoles have support for TV calibration in OS settings.
	g.NextWindowData.MenuBarOffsetMinVal = ImVec2(g.Style.DisplaySafeAreaPadding.x, ImMax(g.Style.DisplaySafeAreaPadding.y - g.Style.FramePadding.y, 0.0f));
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = GetFrameHeight();
	bool is_open = BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height, window_flags);
	g.NextWindowData.MenuBarOffsetMinVal = ImVec2(0.0f, 0.0f);

	if (is_open)
		BeginMenuBar();
	else
		End();
	return is_open;
}

void ImGui::EndMainStatusBar()
{
	EndMenuBar();

	// When the user has left the menu layer (typically: closed menus through activation of an item), we restore focus to the previous window
	// FIXME: With this strategy we won't be able to restore a NULL focus.
	ImGuiContext& g = *GImGui;
	if (g.CurrentWindow == g.NavWindow && g.NavLayer == ImGuiNavLayer_Main && !g.NavAnyRequest)
		FocusTopMostWindowUnderOne(g.NavWindow, NULL);

	End();
}

bool ImGui::BeginLeftToolBar(float vWidth)
{
	//ImGuiContext& g = *GImGui;
	ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)GetMainViewport();

	// Notify of viewport change so GetFrameHeight() can be accurate in case of DPI change
	SetCurrentViewport(NULL, viewport);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	bool is_open = BeginViewportSideBar("##LeftToolBar", viewport, ImGuiDir_Left, vWidth, window_flags);
	
	if (!is_open)
		End();

	return is_open;
}

void ImGui::EndLeftToolBar()
{
	End();
}

bool ImGui::ContrastedButton_For_Dialogs(const char* label, const ImVec2& size_arg)
{
	return ContrastedButton(label, nullptr, nullptr, 0.0f, size_arg, ImGuiButtonFlags_None);
}

bool ImGui::ContrastedButton(const char* label, const char* help, ImFont* imfont, float vWidth, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
	const bool pushed = ImGui::PushStyleColorWithContrast(ImGuiCol_Button, ImGuiCol_Text, CustomStyle::Instance()->puContrastedTextColor, CustomStyle::Instance()->puContrastRatio);

	if (imfont)
		ImGui::PushFont(imfont);

	ImGui::PushID(++CustomStyle::Instance()->pushId);

	const bool res = ImGui::ButtonEx(label, ImVec2(ImMax(vWidth, size_arg.x), size_arg.y), flags);

	ImGui::PopID();

	if (imfont)
		ImGui::PopFont();

	if (pushed)
		ImGui::PopStyleColor();

	if (help)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", help);

	return res;
}

bool ImGui::ToggleContrastedButton(const char* vLabelTrue, const char* vLabelFalse, bool* vValue, const char* vHelp, ImFont* vImfont)
{
	bool res = false;

	assert(vValue);

	const auto pushed = ImGui::PushStyleColorWithContrast(ImGuiCol_Button, ImGuiCol_Text, CustomStyle::Instance()->puContrastedTextColor, CustomStyle::Instance()->puContrastRatio);

	if (vImfont)
		ImGui::PushFont(vImfont);

	ImGui::PushID(++CustomStyle::Instance()->pushId);

	if (ImGui::Button(*vValue ? vLabelTrue : vLabelFalse))
	{
		*vValue = !*vValue;
		res = true;
	}

	ImGui::PopID();

	if (vImfont)
		ImGui::PopFont();

	if (pushed)
		ImGui::PopStyleColor();

	if (vHelp)
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", vHelp);

	return res;
}

void ImGui::PlotFVec4Histo(const char* vLabel, ct::fvec4* vDatas, int vDataCount, bool* vShowChannel, ImVec2 frame_size, ct::fvec4 scale_min, ct::fvec4 scale_max, int* vHoveredIdx)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(vLabel);

	const ImVec2 label_size = ImGui::CalcTextSize(vLabel, NULL, true);
	if (IS_FLOAT_EQUAL(frame_size.x, 0.0f))
		frame_size.x = ImGui::CalcItemWidth();
	if (IS_FLOAT_EQUAL(frame_size.y, 0.0f))
		frame_size.y = label_size.y + (style.FramePadding.y * 2);

	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
	const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, 0, &frame_bb))
		return;

	ImGui::PushID(++CustomStyle::Instance()->pushId);

	const bool hovered = ImGui::ItemHoverable(frame_bb, id);

	// Determine scale from values if not specified
	for (int chan = 0; chan < 4; chan++)
	{
		if (IS_FLOAT_EQUAL(scale_min[chan], FLT_MAX) || IS_FLOAT_EQUAL(scale_max[chan], FLT_MAX))
		{
			float v_min = FLT_MAX;
			float v_max = -FLT_MAX;
			for (int i = 0; i < vDataCount; ++i)
			{
				const float v = vDatas[i][chan];
				v_min = ct::mini(v_min, v);
				v_max = ct::maxi(v_max, v);
			}
			if (scale_min[chan] == FLT_MAX)
				scale_min[chan] = v_min;
			if (scale_max[chan] == FLT_MAX)
				scale_max[chan] = v_max;
		}
	}

	ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

	const int values_count_min = 2;
	if (vDataCount >= values_count_min)
	{
		int res_w = ct::mini((int)frame_size.x, (int)vDataCount) - 1;
		int item_count = vDataCount - 1;

		// Tooltip on hover
		int v_hovered = -1;
		if (vHoveredIdx)
			*vHoveredIdx = v_hovered;

		if (hovered && inner_bb.Contains(g.IO.MousePos))
		{
			const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
			const int v_idx = (int)(t * item_count);
			IM_ASSERT(v_idx >= 0 && v_idx < vDataCount);

			const ct::fvec4 v0 = vDatas[v_idx % vDataCount];
			ImGui::BeginTooltip();
			if (!vShowChannel || vShowChannel[0])
			{
				ImGui::Text("r %d: %8.4g", v_idx, v0.x);
			}
			if (!vShowChannel || vShowChannel[1])
			{
				ImGui::Text("g %d: %8.4g", v_idx, v0.y);
			}
			if (!vShowChannel || vShowChannel[2])
			{
				ImGui::Text("b %d: %8.4g", v_idx, v0.z);
			}
			if (!vShowChannel || vShowChannel[3])
			{
				ImGui::Text("a %d: %8.4g", v_idx, v0.w);
			}
			ImGui::EndTooltip();
			v_hovered = v_idx;

			if (vHoveredIdx)
				*vHoveredIdx = v_hovered;
		}

		float t_step = 1.0f / (float)res_w;
		ct::fvec4 inv_scale;
		inv_scale.x = (scale_min.x == scale_max.x) ? 0.0f : (1.0f / (scale_max.x - scale_min.x));
		inv_scale.y = (scale_min.y == scale_max.y) ? 0.0f : (1.0f / (scale_max.y - scale_min.y));
		inv_scale.z = (scale_min.z == scale_max.z) ? 0.0f : (1.0f / (scale_max.z - scale_min.z));
		inv_scale.w = (scale_min.w == scale_max.w) ? 0.0f : (1.0f / (scale_max.w - scale_min.w));

		const ct::fvec4 v0 = (vDatas[0] - scale_min) * inv_scale;
		ct::fvec4 yt0;
		yt0.x = 1.0f - ct::clamp(v0.x);
		yt0.y = 1.0f - ct::clamp(v0.y);
		yt0.z = 1.0f - ct::clamp(v0.z);
		yt0.w = 1.0f - ct::clamp(v0.w);
		float xt0 = 0.0f;

		ct::fvec4 histogram_zero_line_t;
		histogram_zero_line_t.x = (scale_min.x * scale_max.x < 0.0f) ? (-scale_min.x * inv_scale.x) : (scale_min.x < 0.0f ? 0.0f : 1.0f); // Where does the zero line stands
		histogram_zero_line_t.y = (scale_min.y * scale_max.y < 0.0f) ? (-scale_min.y * inv_scale.y) : (scale_min.y < 0.0f ? 0.0f : 1.0f); // Where does the zero line stands
		histogram_zero_line_t.z = (scale_min.z * scale_max.z < 0.0f) ? (-scale_min.z * inv_scale.z) : (scale_min.z < 0.0f ? 0.0f : 1.0f); // Where does the zero line stands
		histogram_zero_line_t.w = (scale_min.w * scale_max.w < 0.0f) ? (-scale_min.w * inv_scale.w) : (scale_min.w < 0.0f ? 0.0f : 1.0f); // Where does the zero line stands

		for (int n = 0; n < res_w; n++)
		{
			float xt1 = xt0 + t_step;
			int v1_idx = (int)(xt0 * item_count + 0.5f);
			IM_ASSERT(v1_idx >= 0 && v1_idx < vDataCount);
			const ct::fvec4 v1 = (vDatas[(v1_idx + 1) % vDataCount] - scale_min) * inv_scale;
			ct::fvec4 yt1;// = 1.0f - ct::clamp(v1);
			yt1.x = 1.0f - ct::clamp(v1.x);
			yt1.y = 1.0f - ct::clamp(v1.y);
			yt1.z = 1.0f - ct::clamp(v1.z);
			yt1.w = 1.0f - ct::clamp(v1.w);

			if (!vShowChannel || vShowChannel[0])
			{
				ImVec2 p0x = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(xt0, yt0.x));
				ImVec2 p1x = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(xt1, yt1.x));
				window->DrawList->AddLine(p0x, p1x, ImGui::GetColorU32(ImVec4(1, 0, 0, 1)));
			}

			if (!vShowChannel || vShowChannel[1])
			{
				ImVec2 p0y = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(xt0, yt0.y));
				ImVec2 p1y = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(xt1, yt1.y));
				window->DrawList->AddLine(p0y, p1y, ImGui::GetColorU32(ImVec4(0, 1, 0, 1)));
			}

			if (!vShowChannel || vShowChannel[2])
			{
				ImVec2 p0z = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(xt0, yt0.z));
				ImVec2 p1z = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(xt1, yt1.z));
				window->DrawList->AddLine(p0z, p1z, ImGui::GetColorU32(ImVec4(0, 0, 1, 1)));
			}

			if (!vShowChannel || vShowChannel[3])
			{
				ImVec2 p0w = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(xt0, yt0.w));
				ImVec2 p1w = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(xt1, yt1.w));
				window->DrawList->AddLine(p0w, p1w, ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
			}

			xt0 = xt1;
			yt0 = yt1;
		}

		if (!vShowChannel || vShowChannel[0])
		{
			float y = ImLerp(inner_bb.Min.y, inner_bb.Max.y, yt0.x);
			char buf[20]; snprintf(buf, 20, "r %.3f", yt0.x);
			window->DrawList->AddText(ImVec2(inner_bb.Max.x + style.FramePadding.x, y + style.FramePadding.y), ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), buf);
		}

		if (!vShowChannel || vShowChannel[1])
		{
			float y = ImLerp(inner_bb.Min.y, inner_bb.Max.y, yt0.y);
			char buf[20]; snprintf(buf, 20, "g %.3f", yt0.y);
			window->DrawList->AddText(ImVec2(inner_bb.Max.x + style.FramePadding.x, y + style.FramePadding.y), ImGui::GetColorU32(ImVec4(0, 1, 0, 1)), buf);
		}

		if (!vShowChannel || vShowChannel[2])
		{
			float y = ImLerp(inner_bb.Min.y, inner_bb.Max.y, yt0.z);
			char buf[20]; snprintf(buf, 20, "b %.3f", yt0.z);
			window->DrawList->AddText(ImVec2(inner_bb.Max.x + style.FramePadding.x, y + style.FramePadding.y), ImGui::GetColorU32(ImVec4(0, 0, 1, 1)), buf);
		}

		if (!vShowChannel || vShowChannel[3])
		{
			float y = ImLerp(inner_bb.Min.y, inner_bb.Max.y, yt0.w);
			char buf[20]; snprintf(buf, 20, "a %.3f", yt0.w);
			window->DrawList->AddText(ImVec2(inner_bb.Max.x + style.FramePadding.x, y + style.FramePadding.y), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), buf);
		}
	}

	ImGui::PopID();
}

void ImGui::ImageZoomPoint(ImTextureID vUserTextureId, const float vWidth, const ImVec2& vCenter, const ImVec2& vPoint, const ImVec2& vRadiusInPixels)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;
	ImVec2 size = ImVec2(vWidth, vWidth);

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;

	ImVec2 rp = ImVec2(vRadiusInPixels.x + 1.0f, vRadiusInPixels.y + 1.0f);

	ImVec2 uv0 = vCenter - vRadiusInPixels * vPoint;
	ImVec2 uv1 = vCenter + rp * vPoint;

	window->DrawList->AddImage(vUserTextureId, bb.Min, bb.Max, uv0, uv1);

	ImVec2 s = size * vRadiusInPixels / (rp + vRadiusInPixels);

	ImVec2 a = bb.Min + s;
	ImVec2 b = bb.Max - s;

	window->DrawList->AddRect(a, b, ImGui::GetColorU32(ImVec4(1, 1, 0, 1)));
}

/*
void ImGui::ImageZoomLine(ImTextureID vUserTextureId, const float vWidth, const ImVec2& vStart, const ImVec2& vEnd)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;
	ImVec2 size = ImVec2(vWidth, vWidth);
	float w = std::fabs(vEnd.x - vStart.x);
	float h = std::fabs((1.0f - vEnd.y) - (1.0f - vStart.y));
	float ratioX = (float)h / (float)w;
	float y = size.x * ratioX;
	if (y > size.y)
		size.x = size.y / ratioX;
	else
		size.y = y;
	size.x = ct::clamp(size.x, 1.0f, vWidth);
	size.y = ct::clamp(size.y, 1.0f, vWidth);

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;

	window->DrawList->AddImage(vUserTextureId, bb.Min, bb.Max, vStart, vEnd);

	//ImVec2 a = bb.Min + s;
	//ImVec2 b = bb.Max - s;
	//window->DrawList->AddLine(a, b, ImGui::GetColorU32(ImVec4(1, 1, 0, 1)));
}
*/

bool ImGui::InputText_Validation(const char* label, char* buf, size_t buf_size,
	const bool* vValidation, const char* vValidationHelp,
	ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	if (vValidation)
	{
		if (*vValidation)
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
		else
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
	}
	const bool res = ImGui::InputText(label, buf, buf_size, flags, callback, user_data);
	if (vValidation)
	{
		ImGui::PopStyleColor();
		if (!*vValidation && ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", vValidationHelp);
	}
	return res;
}

//from imgui_demo.h
void ImGui::HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

#ifdef USE_GRADIENT

void ImGui::RenderGradFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_start_col, ImU32 fill_end_col, bool border, float rounding)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	window->DrawList->AddRectFilledMultiColor(p_min, p_max, fill_end_col, fill_end_col, fill_start_col, fill_start_col);
	const float border_size = g.Style.FrameBorderSize;
	if (border && border_size > 0.0f)
	{
		window->DrawList->AddRect(p_min + ImVec2(1, 1), p_max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawFlags_RoundCornersAll, border_size);
		window->DrawList->AddRect(p_min, p_max, GetColorU32(ImGuiCol_Border), rounding, ImDrawFlags_RoundCornersAll, border_size);
	}
}

bool ImGui::GradButton(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	// Render
	//const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	//RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

	const ImVec4 col = GetStyleColorVec4((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	ImVec4 col_darker = ImVec4(col.x * 0.5f, col.y * 0.5f, col.z * 0.5f, col.w);
	RenderGradFrame(bb.Min, bb.Max, GetColorU32(col_darker), GetColorU32(col), true, g.Style.FrameRounding);

	RenderNavHighlight(bb, id);
	RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

	// Automatically close popups
	//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
	//    CloseCurrentPopup();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
	return pressed;
}

#endif

bool ImGui::TransparentButton(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, nullptr, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	const ImVec2 size = CalcItemSize(size_arg, label_size.x, label_size.y);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.CurrentItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	const bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	// Render
	//const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	//RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
	RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, nullptr, &label_size, style.ButtonTextAlign, &bb);

	return pressed;
}

void ImGui::PlainImageWithBG(ImTextureID vTexId, const ImVec2& size, const ImVec4& bg_col, const ImVec4& tint_col)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	ItemSize(bb);
	if (!ItemAdd(bb, 0))
		return;

	window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(bg_col), 0.0f);
	window->DrawList->AddImage(vTexId, bb.Min, bb.Max, ImVec2(0, 0), ImVec2(1, 1), GetColorU32(tint_col));
}

void ImGui::ImageRatio(ImTextureID vTexId, float vRatioX, float vWidth, ImVec4 vColor, float /*vBorderThick*/)
{
	if (vTexId == 0)
		return;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	if (!window->ScrollbarY)
	{
		vWidth -= ImGui::GetStyle().ScrollbarSize;
	}

	const ImVec2 uv0 = ImVec2(0, 0);
	const ImVec2 uv1 = ImVec2(1, 1);

	ImVec2 size = ImVec2(vWidth, vWidth);

	const float ratioX = vRatioX;
	const float y = size.x * ratioX;
	if (y > size.y)
		size.x = size.y / ratioX;
	else
		size.y = y;

	size.x = ct::clamp(size.x, 1.0f, vWidth);
	size.y = ct::clamp(size.y, 1.0f, vWidth);

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (vColor.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0))
		return;

	if (vColor.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(vColor), 0.0f);
		window->DrawList->AddImage(vTexId, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
	}
	else
	{
		window->DrawList->AddImage(vTexId, bb.Min, bb.Max, uv0, uv1, ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
	}

#ifdef _DEBUG
	if (ImGui::IsItemHovered())
	{
		char arr[3];
		if (snprintf(arr, 3, "%i", (int)(size_t)vTexId))
		{
			ImGui::SetTooltip("%s", arr);
		}
	}
#endif
}

#ifdef USE_OPENGL
bool ImGui::TextureOverLay(float vWidth, std::shared_ptr<ct::texture> vTex, ImVec4 vBorderColor, const char* vOverlayText, ImVec4
	vOverLayTextColor, ImVec4 vOverLayBgColor, const ImVec2& vUV0, const ImVec2& vUV1)
{
	if (vTex == nullptr)
		return false;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	if (!window->ScrollbarY)
	{
		vWidth -= ImGui::GetStyle().ScrollbarSize;
	}

	ImVec2 size = ImVec2(vWidth, vWidth);

	const float ratioX = (float)vTex->h / (float)vTex->w;
	const float y = size.x * ratioX;
	if (y > size.y)
		size.x = size.y / ratioX;
	else
		size.y = y;

	size.x = ct::clamp(size.x, 1.0f, vWidth);
	size.y = ct::clamp(size.y, 1.0f, vWidth);

	const ImGuiID id = window->GetID(vTex.get());

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (vBorderColor.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	bool hovered, held;
	const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

	//if (pressed)
		//CTOOL_DEBUG_BREAK;

	if (vBorderColor.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(vBorderColor), 0.0f, ImDrawFlags_RoundCornersAll, vBorderColor.w);
		window->DrawList->AddImage(
			(ImTextureID)(size_t)vTex->glTex, 
			bb.Min + ImVec2(1, 1), 
			bb.Max - ImVec2(1, 1), 
			vUV0, 
			vUV1, 
			ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
	}
	else
	{
		window->DrawList->AddImage(
			(ImTextureID)(size_t)vTex->glTex, bb.Min, 
			bb.Max, 
			vUV0, 
			vUV1, 
			ImGui::GetColorU32(ImVec4(1, 1, 1, 1)));
	}

	ImVec2 center = (bb.Max + bb.Min) * 0.5f;
	ImGuiContext& g = *GImGui;
	const float fontSize = 40.0f;
	const ImVec2 ls = ImGui::CalcTextSize(vOverlayText, nullptr, 0) * fontSize / g.Font->FontSize;
	center = ImVec2(center.x - ls.x * 0.5f, center.y - ls.y * 0.5f);

	if (hovered)
	{
		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(vOverLayBgColor));
		window->DrawList->AddText(nullptr, fontSize, center, ImGui::GetColorU32(vOverLayTextColor), vOverlayText);
		//ImGui::RenderTextClipped(bb.Min, bb.Max, vOverlayText, 0, 0, ImVec2(0.5f, 0.5f), &bb);
	}
	else
	{
		// bug fix de type flickering https://github.com/ocornut/imgui/issues/2506
		//window->DrawList->AddText(0, fontSize, center, ImGui::GetColorU32(ImVec4(0, 0, 0, 1)), ".");
	}

#ifdef _DEBUG
	/*if (ImGui::IsItemHovered())
	{
		char arr[3];
		if (snprintf(arr, 3, "%i", (int)vTex->glTex))
		{
			ImGui::SetTooltip(arr);
		}
	}*/
#endif

	return pressed;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// COMBO ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float inCalcMaxPopupHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool ImGui::BeginContrastedCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
	// Always consume the SetNextWindowSizeConstraint() call in our early return paths
	ImGuiContext& g = *GImGui;
	bool has_window_size_constraint = (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint) != 0;
	g.NextWindowData.Flags &= ~ImGuiNextWindowDataFlags_HasSizeConstraint;

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const ImGuiStyle& style = g.Style;

	PushID(++ImGui::CustomStyle::Instance()->pushId);
	const ImGuiID id = window->GetID(label);
	PopID();

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const float expected_w = CalcItemWidth();
	const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : expected_w;
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
	bool popup_open = IsPopupOpen(id, ImGuiPopupFlags_None);

	const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	const float value_x2 = ImMax(frame_bb.Min.x, frame_bb.Max.x - arrow_size);
	RenderNavHighlight(frame_bb, id);
	if (!(flags & ImGuiComboFlags_NoPreview))
		window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(value_x2, frame_bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
	if (!(flags & ImGuiComboFlags_NoArrowButton))
	{
		const bool pushed = ImGui::PushStyleColorWithContrast(ImGuiCol_Button, ImGuiCol_Text, CustomStyle::Instance()->puContrastedTextColor, CustomStyle::Instance()->puContrastRatio);
		ImU32 bg_col = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		ImU32 text_col = GetColorU32(ImGuiCol_Text);
		window->DrawList->AddRectFilled(ImVec2(value_x2, frame_bb.Min.y), frame_bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
		if (value_x2 + arrow_size - style.FramePadding.x <= frame_bb.Max.x)
			RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, frame_bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);
		if (pushed)
			ImGui::PopStyleColor();
	}
	RenderFrameBorder(frame_bb.Min, frame_bb.Max, style.FrameRounding);
	if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
	{
		ImVec2 preview_pos = frame_bb.Min + style.FramePadding;
		if (g.LogEnabled)
			LogSetNextTextDecoration("{", "}");
		RenderTextClipped(preview_pos, ImVec2(value_x2, frame_bb.Max.y), preview_value, NULL, NULL, ImVec2(0.0f, 0.0f));
	}
	if (label_size.x > 0)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	if ((pressed || g.NavActivateId == id) && !popup_open)
	{
		if (window->DC.NavLayerCurrent == 0)
			window->NavLastIds[0] = id;
		OpenPopupEx(id, ImGuiPopupFlags_None);
		popup_open = true;
	}

	if (!popup_open)
		return false;

	if (has_window_size_constraint)
	{
		g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasSizeConstraint;
		g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
	}
	else
	{
		if ((flags & ImGuiComboFlags_HeightMask_) == 0)
			flags |= ImGuiComboFlags_HeightRegular;
		IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_));    // Only one
		int popup_max_height_in_items = -1;
		if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
		else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
		else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
		SetNextWindowSizeConstraints(ImVec2(w, 0.0f), ImVec2(FLT_MAX, inCalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));
	}

	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

	// Position the window given a custom constraint (peak into expected window size so we can position it)
	// This might be easier to express with an hypothetical SetNextWindowPosConstraints() function.
	if (ImGuiWindow* popup_window = FindWindowByName(name))
		if (popup_window->WasActive)
		{
			// Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
			ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
			if (flags & ImGuiComboFlags_PopupAlignLeft)
				popup_window->AutoPosLastDirection = ImGuiDir_Left; // "Below, Toward Left"
			else
				popup_window->AutoPosLastDirection = ImGuiDir_Down; // "Below, Toward Right (default)"
			ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
			ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
			SetNextWindowPos(pos);
		}

	// We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

	// Horizontally align ourselves with the framed text
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
	bool ret = Begin(name, NULL, window_flags);
	PopStyleVar();
	if (!ret)
	{
		EndPopup();
		IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
		return false;
	}
	return true;
}

bool ImGui::ContrastedCombo(float vWidth, const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items)
{
	ImGuiContext& g = *GImGui;

	if (vWidth > -1)
		ImGui::PushItemWidth((float)vWidth);

	// Call the getter to obtain the preview string which is a parameter to BeginCombo()
	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(data, *current_item, &preview_value);

	// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
	if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, inCalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	if (!BeginContrastedCombo(label, preview_value, ImGuiComboFlags_None))
		return false;

	//nd::Suspend();

	// Display items
	// FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
	bool value_changed = false;
	for (int i = 0; i < items_count; ++i)
	{
		PushID((void*)(intptr_t)i);
		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(data, i, &item_text))
			item_text = "*Unknown item*";
		if (Selectable(item_text, item_selected))
		{
			value_changed = true;
			*current_item = i;
		}
		if (item_selected)
			SetItemDefaultFocus();
		PopID();
	}

	//nd::Resume();

	EndCombo();
	if (value_changed)
		MarkItemEdited(g.LastItemData.ID);

	if (vWidth > -1)
		ImGui::PopItemWidth();

	return value_changed;
}

// Getter for the old Combo() API: const char*[]
inline bool inItems_ArrayGetter(void* data, int idx, const char** out_text)
{
	const char* const* items = (const char* const*)data;
	if (out_text)
		*out_text = items[idx];
	return true;
}

// Getter for the old Combo() API: "item1\0item2\0item3\0"
inline bool inItems_SingleStringGetter(void* data, int idx, const char** out_text)
{
	// FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
	const char* items_separated_by_zeros = (const char*)data;
	int items_count = 0;
	const char* p = items_separated_by_zeros;
	while (*p)
	{
		if (idx == items_count)
			break;
		p += strlen(p) + 1;
		items_count++;
	}
	if (!*p)
		return false;
	if (out_text)
		*out_text = p;
	return true;
}

// Combo box helper allowing to pass an array of strings.
bool ImGui::ContrastedCombo(float vWidth, const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
	PushID(++ImGui::CustomStyle::Instance()->pushId);

	const bool value_changed = ContrastedCombo(vWidth, label, current_item, inItems_ArrayGetter, (void*)items, items_count, height_in_items);

	PopID();

	return value_changed;
}

// Combo box helper allowing to pass all items in a single string literal holding multiple zero-terminated items "item1\0item2\0"
bool ImGui::ContrastedCombo(float vWidth, const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
	PushID(++ImGui::CustomStyle::Instance()->pushId);

	int items_count = 0;
	const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
	while (*p)
	{
		p += strlen(p) + 1;
		items_count++;
	}
	bool value_changed = ContrastedCombo(vWidth, label, current_item, inItems_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);

	PopID();

	return value_changed;
}

bool ImGui::ContrastedComboVectorDefault(float vWidth, const char* label, int* current_item, const std::vector<std::string>& items, int vDefault, int height_in_items)
{
	bool change = false;

	if (!items.empty())
	{
		float px = ImGui::GetCursorPosX();

		ImGui::PushID(++CustomStyle::Instance()->pushId);

		change = ImGui::ContrastedButton(ICON_NDP_RESET);
		if (change)
			*current_item = vDefault;

		ImGui::CustomSameLine();

		if (vWidth > 0.0f)
		{
			vWidth -= ImGui::GetCursorPosX() - px;
		}

		change |= ContrastedCombo(vWidth, label, current_item, [](void* data, int idx, const char** out_text)
			{
				*out_text = ((const std::vector<std::string>*)data)->at(idx).c_str();
				return true;
			}, (void*)&items, (int)items.size(), height_in_items);

		ImGui::PopID();
	}

	return change;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// INPUT ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMGUI_API bool ImGui::InputFloatDefault(float vWidth, const char* vName, float* vVar, float vDefault, const char* vInputPrec, const char* vPopupPrec, bool vShowResetButton, float vStep, float vStepFast)
{
	bool change = false;

	const float padding = ImGui::GetStyle().FramePadding.x;

	/*if (!ImGui::GetCurrentWindow()->ScrollbarY)
	{
		vWidth -= ImGui::GetStyle().ScrollbarSize;
	}*/

	float w = vWidth - padding * 2.0f;// -24;

	if (vShowResetButton)
	{
		change = ImGui::ContrastedButton(ICON_NDP_RESET);
		w -= ImGui::GetItemRectSize().x;
		if (change)
			*vVar = vDefault;
	}

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	ImGui::PushID(++CustomStyle::Instance()->pushId);
	ImGui::PushItemWidth(w);
	change |= ImGui::InputFloat(vName, vVar, vStep, vStepFast, vInputPrec);
	ImGui::PopItemWidth();
	ImGui::PopID();

	if (ImGui::IsItemActive() || ImGui::IsItemHovered())
		ImGui::SetTooltip(vPopupPrec, *vVar);

	return change;
}

bool ImGui::InputFloatDefaultStepper(float vWidth, const char* vName, float* vVar, float vDefault, float vStep, float vStepFast, const char* vInputPrec, const char* vPopupPrec, bool vShowResetButton)
{
	bool change = false;

	const float padding = ImGui::GetStyle().FramePadding.x;

	float w = vWidth - padding * 2.0f;// -24;

	if (vShowResetButton)
	{
		change = ImGui::ContrastedButton(ICON_NDP_RESET);
		w -= ImGui::GetItemRectSize().x;
		if (change)
			*vVar = vDefault;
	}

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	ImGui::PushID(++CustomStyle::Instance()->pushId);
	ImGui::PushItemWidth(w);
	change |= ImGui::InputFloat("##InputFloat", vVar, 0.0f, 0.0f, vInputPrec, ImGuiInputTextFlags_CharsScientific);
	ImGui::PopItemWidth();
	ImGui::PopID();

	if (vStep > 0.0f)
	{
		ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGuiButtonFlags button_flags = ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups;
		if (ImGui::ContrastedButton(ICON_NDP_ADD, nullptr, nullptr, 0.0f, ImVec2(0.0f, 0.0f), button_flags))
		{
			*vVar += ImGui::GetIO().KeyCtrl ? vStepFast : vStep;
		}
		ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		if (ImGui::ContrastedButton(ICON_NDP_REMOVE, nullptr, nullptr, 0.0f, ImVec2(0.0f, 0.0f), button_flags))
		{
			*vVar -= ImGui::GetIO().KeyCtrl ? vStepFast : vStep;
		}
		ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::Text("%s", vName);
	}
	else
	{
		ImGui::Text("%s", vName);
	}

	if (ImGui::IsItemActive() || ImGui::IsItemHovered())
		ImGui::SetTooltip(vPopupPrec, *vVar);

	return change;
}

IMGUI_API bool ImGui::InputIntDefault(float vWidth, const char* vName, int* vVar, int step, int step_fast, int vDefault)
{
	bool change = false;

	const float padding = ImGui::GetStyle().FramePadding.x;

	if (!ImGui::GetCurrentWindow()->ScrollbarY)
	{
		vWidth -= ImGui::GetStyle().ScrollbarSize;
	}

	change = ImGui::ContrastedButton(ICON_NDP_RESET);
	const float w = vWidth - ImGui::GetItemRectSize().x - padding * 2.0f - 24;
	if (change)
		*vVar = vDefault;

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	ImGui::PushID(++CustomStyle::Instance()->pushId);
	ImGui::PushItemWidth(w);
	change |= ImGui::InputInt(vName, vVar, step, step_fast);
	ImGui::PopItemWidth();
	ImGui::PopID();

	if (ImGui::IsItemActive() || ImGui::IsItemHovered())
		ImGui::SetTooltip("%i", *vVar);

	return change;
}

IMGUI_API bool ImGui::InputUIntDefault(float vWidth, const char* vName, uint32_t* vVar, uint32_t step, uint32_t step_fast, uint32_t vDefault)
{
	bool change = false;

	const float padding = ImGui::GetStyle().FramePadding.x;

	if (!ImGui::GetCurrentWindow()->ScrollbarY)
	{
		vWidth -= ImGui::GetStyle().ScrollbarSize;
	}

	change = ImGui::ContrastedButton(ICON_NDP_RESET);
	const float w = vWidth - ImGui::GetItemRectSize().x - padding * 2.0f - 24;
	if (change)
		*vVar = vDefault;

	ImGui::CustomSameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

	ImGui::PushID(++CustomStyle::Instance()->pushId);
	ImGui::PushItemWidth(w);
	change |= InputScalar(vName, ImGuiDataType_U32, (void*)vVar, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), "%d", 0);
	ImGui::PopItemWidth();
	ImGui::PopID();

	if (ImGui::IsItemActive() || ImGui::IsItemHovered())
		ImGui::SetTooltip("%u", *vVar);

	return change;
}

IMGUI_API bool ImWidgets::InputText::DisplayInputText(const float& vWidth, const std::string& vLabel, const std::string& vDefaultText)
{
	bool res = false;
	float px = ImGui::GetCursorPosX();
	ImGui::Text("%s", vLabel.c_str());
	ImGui::SameLine();
	const float w = vWidth - (ImGui::GetCursorPosX() - px);
	ImGui::PushID(++ImGui::CustomStyle::Instance()->pushId);
	ImGui::PushItemWidth(w);
	if (buffer[0] == '\0') // default text
	{
		SetText(vDefaultText);
	}
	if (ImGui::InputText("##ImWidgets_InputText_DisplayInputText", buffer, 512))
	{
		m_Text = std::string(buffer, strlen(buffer));
		res = true;
	}
	ImGui::PopItemWidth();
	ImGui::PopID();
	return res;
}

IMGUI_API void ImWidgets::InputText::SetText(const std::string& vText)
{
	m_Text = vText;
	size_t len = m_Len;
	if (vText.size() < len)
		len = vText.size();
#ifdef _MSC_VER
	strncpy_s(buffer, vText.c_str(), len);
#else
	strncpy(buffer, vText.c_str(), len);
#endif
}

IMGUI_API std::string ImWidgets::InputText::GetText(const std::string& vNumericType)
{ 
	if (vNumericType == "float")
	{
		auto pos = m_Text.find('.');
		if (pos == std::string::npos)
		{
			return m_Text + ".0f";
		}
		else
		{
			pos = m_Text.find('f');
			if (pos == std::string::npos)
			{
				return m_Text + "f";
			}
			else
			{
				return m_Text;
			}
		}
	}
	else if (vNumericType == "uint")
	{
		return m_Text + "U";
	}

	return m_Text; 
}