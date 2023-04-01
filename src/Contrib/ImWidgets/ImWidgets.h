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

#pragma once

#include <imgui/imgui.h>

#include <ctools/cTools.h>

#include <cstdint> // types like uint32_t

#define ImWidgets_VERSION "ImWidgets v1.0"

struct ImGuiWindow;
#ifdef USE_GLFW3
struct GLFWWindow;
#endif // USE_GLFW3
namespace ImGui
{
	IMGUI_API class CustomStyle
	{
	public:
		IMGUI_API static CustomStyle* Instance(CustomStyle* vCopy = nullptr, bool vForce = false)
		{
			static CustomStyle _instance;
			static CustomStyle* _instance_copy = nullptr;
			if (vCopy || vForce)
				_instance_copy = vCopy;
			if (_instance_copy)
				return _instance_copy;
			return &_instance;
		}

	public:
		float puContrastRatio = 3.0f;
		ImU32 puContrastedTextColor = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
		int pushId = 125;
		int minorNumber = 0;
		int majorNumber = 0;
		int buildNumber = 0;
		ImVec4 ImGuiCol_Symbol = ImVec4(0, 0, 0, 1);
		ImVec4 GoodColor = ImVec4(0.2f, 0.8f, 0.2f, 0.8f);
		ImVec4 BadColor = ImVec4(0.8f, 0.2f, 0.2f, 0.8f);
	};

	IMGUI_API void CustomSameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f);

	IMGUI_API int IncPUSHID();
	IMGUI_API int GetPUSHID();
	IMGUI_API void SetPUSHID(int vID);

	IMGUI_API ImVec4 GetGoodOrBadColorForUse(bool vUsed); // return a "good" color if true or "bad" color if false

#ifdef USE_GLFW3
	IMGUI_API ImVec2 GetLocalMousePos(GLFWWindow* vWin = nullptr); // return local window mouse pos
#endif // USE_GLFW3

	IMGUI_API void SetContrastRatio(float vRatio);
	IMGUI_API void SetContrastedTextColor(ImU32 vColor);
	IMGUI_API void DrawContrastWidgets();

	IMGUI_API float CalcContrastRatio(const ImU32& backGroundColor, const ImU32& foreGroundColor);
	IMGUI_API bool PushStyleColorWithContrast(const ImGuiCol& backGroundColor, const ImGuiCol& foreGroundColor, const ImU32& invertedColor, const float& minContrastRatio);
	IMGUI_API bool PushStyleColorWithContrast(const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImU32& invertedColor, const float& maxContrastRatio);
	IMGUI_API bool PushStyleColorWithContrast(const ImGuiCol& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio);
	IMGUI_API bool PushStyleColorWithContrast(const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio);

	IMGUI_API void AddInvertedRectFilled(ImDrawList* vDrawList, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawFlags rounding_corners);
	IMGUI_API void RenderInnerShadowFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, ImU32 fill_col_darker, ImU32 bg_Color, bool border, float rounding);
	IMGUI_API void DrawShadowImage(ImTextureID vShadowImage, const ImVec2& vSize, ImU32 col);

	IMGUI_API bool ImageCheckButton(ImTextureID user_texture_id, bool* v, const ImVec2& size, const ImVec2& uv0 = ImVec2(0.0f, 0.0f), const ImVec2& uv1 = ImVec2(1.0f, 1.0f), const ImVec2& vHostTextureSize = ImVec2(0.0f, 0.0f), int frame_padding = -1, float vRectThickNess = 0.0f, ImVec4 vRectColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

	IMGUI_API bool BeginFramedGroup(const char* vLabel, ImVec2 vSize = ImVec2(0, 0), ImVec4 vCol = ImVec4(0.0f, 0.0f, 0.0f, 0.5f), ImVec4 vHoveredCol = ImVec4(0.15f, 0.15f, 0.15f, 0.5f));
	IMGUI_API void EndFramedGroup(ImGuiCol vHoveredIdx = ImGuiCol_TabUnfocusedActive, ImGuiCol NormalIdx = ImGuiCol_TabUnfocused);
	IMGUI_API void FramedGroupSeparator();
	IMGUI_API void FramedGroupText(ImVec4* vTextColor, const char* vHelp, const char* vFmt, va_list vArgs);
	IMGUI_API void FramedGroupText(const char* vFmt, ...);
	IMGUI_API void FramedGroupTextHelp(const char* vHelp, const char* vFmt, ...);
	IMGUI_API void FramedGroupText(ImVec4 vTextColor, const char* vFmt, ...);

	IMGUI_API bool CollapsingHeader_SmallHeight(const char* vName, float vHeightRatio, float vWidth, bool vDefaulExpanded, bool* vIsOpen = nullptr);
	IMGUI_API bool CollapsingHeader_CheckBox(const char* vName, float vWidth = -1, bool vDefaulExpanded = false, bool vShowCheckBox = false, bool* vCheckCatched = 0);
	IMGUI_API bool CollapsingHeader_Button(const char* vName, float vWidth = -1, bool vDefaulExpanded = false, const char* vLabelButton = 0, bool vShowButton = false, bool* vButtonPressed = 0, ImFont* vButtonFont = nullptr);

	IMGUI_API bool CheckBoxIcon(const char* vLabel, const char* vIconTrue, bool* v);
	IMGUI_API bool CheckBoxBoolDefault(const char* vName, bool* vVar, bool vDefault, const char* vHelp = 0, ImFont* vLabelFont = nullptr);
	IMGUI_API bool CheckBoxInt(const char* vName, int* vVar);
	IMGUI_API bool CheckBoxIntDefault(const char* vName, int* vVar, int vDefault, const char* vHelp = 0, ImFont* vLabelFont = nullptr);
	IMGUI_API bool CheckBoxFloat(const char* vName, float* vVar);
	IMGUI_API bool CheckBoxFloatDefault(const char* vName, float* vVar, float vDefault, const char* vHelp = 0, ImFont* vLabelFont = nullptr);
	IMGUI_API bool RadioFloatDefault(const char* vName, float* vVar, int vCount, float* vDefault, const char* vHelp = 0, ImFont* vLabelFont = nullptr);

	IMGUI_API bool Selectable_FramedText_Selected(const bool& vSelected, const char* fmt, ...);
	IMGUI_API bool Selectable_FramedText(const char* fmt, ...);

	IMGUI_API bool RadioButtonLabeled(float vWidth, const char* label, bool active, bool disabled);
	IMGUI_API bool RadioButtonLabeled(float vWidth, const char* label, const char* help, bool active, bool disabled = false, ImFont* vLabelFont = nullptr);
	IMGUI_API bool RadioButtonLabeled(float vWidth, const char* label, const char* help, bool* active, bool disabled = false, ImFont* vLabelFont = nullptr);
	template<typename T>
	IMGUI_API bool RadioButtonLabeled_BitWize(
		float vWidth,
		const char* vLabel, const char* vHelp, T* vContainer, T vFlag,
		bool vOneOrZeroAtTime = false, //only one selcted at a time
		bool vAlwaysOne = true, // radio behavior, always one selected
		T vFlagsToTakeIntoAccount = (T)0,
		bool vDisableSelection = false,
		ImFont* vLabelFont = nullptr) // radio witl use only theses flags
	{
		bool selected = *vContainer & vFlag;
		const bool res = RadioButtonLabeled(vWidth, vLabel, vHelp, &selected, vDisableSelection, vLabelFont);
		if (res) {
			if (selected) {
				if (vOneOrZeroAtTime) {
					if (vFlagsToTakeIntoAccount) {
						if (vFlag & vFlagsToTakeIntoAccount) {
							*vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount); // remove these flags
							*vContainer = (T)(*vContainer | vFlag); // add
						}
					}
					else *vContainer = vFlag; // set
				}
				else {
					if (vFlagsToTakeIntoAccount) {
						if (vFlag & vFlagsToTakeIntoAccount) {
							*vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount); // remove these flags
							*vContainer = (T)(*vContainer | vFlag); // add
						}
					}
					else *vContainer = (T)(*vContainer | vFlag); // add
				}
			}
			else {
				if (vOneOrZeroAtTime) {
					if (!vAlwaysOne) *vContainer = (T)(0); // remove all
				}
				else *vContainer = (T)(*vContainer & ~vFlag); // remove one
			}
		}
		return res;
	}
	template<typename T>
	IMGUI_API bool RadioButtonLabeled_BitWize(
		float vWidth,
		const char* vLabelOK, const char* vLabelNOK, const char* vHelp, T* vContainer, T vFlag,
		bool vOneOrZeroAtTime = false, //only one selcted at a time
		bool vAlwaysOne = true, // radio behavior, always one selected
		T vFlagsToTakeIntoAccount = (T)0,
		bool vDisableSelection = false,
		ImFont* vLabelFont = nullptr) // radio witl use only theses flags
	{
		bool selected = *vContainer & vFlag;
		const char* label = (selected ? vLabelOK : vLabelNOK);
		const bool res = RadioButtonLabeled(vWidth, label, vHelp, &selected, vDisableSelection, vLabelFont);
		if (res) {
			if (selected) {
				if (vOneOrZeroAtTime) {
					if (vFlagsToTakeIntoAccount) {
						if (vFlag & vFlagsToTakeIntoAccount) {
							*vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount); // remove these flags
							*vContainer = (T)(*vContainer | vFlag); // add
						}
					}
					else *vContainer = vFlag; // set
				}
				else {
					if (vFlagsToTakeIntoAccount) {
						if (vFlag & vFlagsToTakeIntoAccount) {
							*vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount); // remove these flags
							*vContainer = (T)(*vContainer | vFlag); // add
						}
					}
					else *vContainer = (T)(*vContainer | vFlag); // add
				}
			}
			else {
				if (vOneOrZeroAtTime) {
					if (!vAlwaysOne) *vContainer = (T)(0); // remove all
				}
				else *vContainer = (T)(*vContainer & ~vFlag); // remove one
			}
		}
		return res;
	}

	template<typename T>
	IMGUI_API bool MenuItem(const char* label, const char* shortcut, T* vContainer, T vFlag, bool vOnlyOneSameTime = false)
	{
		bool selected = *vContainer & vFlag;
		const bool res = MenuItem(label, shortcut, &selected, true);
		if (res) {
			if (selected) {
				if (vOnlyOneSameTime) {
					*vContainer = vFlag; // set
				}
				else {
					*vContainer = (T)(*vContainer | vFlag);// add
				}
			}
			else {
				if (!vOnlyOneSameTime) {
					*vContainer = (T)(*vContainer & ~vFlag); // remove
				}
			}
		}
		return res;
	}

	template<typename T>
	IMGUI_API bool Begin(const char* name, T* vContainer, T vFlag, ImGuiWindowFlags flags)
	{
		bool check = *vContainer & vFlag;
		const bool res = Begin(name, &check, flags);
		if (check) *vContainer = (T)(*vContainer | vFlag); // add
		else *vContainer = (T)(*vContainer & ~vFlag); // remove
		return res;
	}

	template<typename T>
	IMGUI_API bool Begin(const std::string& name, T* vContainer, T vFlag, ImGuiWindowFlags flags)
	{
		bool check = *vContainer & vFlag;
		const bool res = Begin(name.c_str(), &check, flags);
		if (check) *vContainer = (T)(*vContainer | vFlag); // add
		else *vContainer = (T)(*vContainer & ~vFlag); // remove
		return res;
	}

	IMGUI_API bool ClickableTextUrl(const char* label, const char* url, bool vOnlined = true);
	IMGUI_API bool ClickableTextFile(const char* label, const char* file, bool vOnlined = true);

	IMGUI_API void Header(const char* vName, float width = -1);
	/*template<typename T>
	IMGUI_API bool CheckBoxBitWize(const char* vLabel, const char* vHelp, T* vContainer, T vFlag, bool vDef)
	{
		bool check = *vContainer & vFlag;
		bool res = CheckBoxBoolDefault(vLabel, &check, vDef, vHelp);
		if (res)
		{
			if (check)
			{
				// add
				*vContainer = (T)(*vContainer | vFlag);
			}
			else
			{
				// remove
				*vContainer = (T)(*vContainer & ~vFlag);
			}
		}
		return res;
	}*/
	IMGUI_API void ImageRect(
		ImTextureID user_texture_id, const ImVec2& pos, const ImVec2& size,
		const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1),
		const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	IMGUI_API bool ColorEdit3Default(float vWidth, const char* vName, float* vCol, float* vDefault);
	IMGUI_API bool ColorEdit4Default(float vWidth, const char* vName, float* vCol, float* vDefault);

	IMGUI_API void Spacing(float vSpace);
	IMGUI_API ImGuiWindow* GetHoveredWindow();

	IMGUI_API bool BeginMainStatusBar();
	IMGUI_API void EndMainStatusBar();

	IMGUI_API bool BeginLeftToolBar(float vWidth);
	IMGUI_API void EndLeftToolBar();

	IMGUI_API bool ContrastedButton_For_Dialogs(const char* label, const ImVec2& size_arg = ImVec2(0, 0));
	IMGUI_API bool ContrastedButton(const char* label, const char* help = nullptr, ImFont* imfont = nullptr, float vWidth = 0.0f, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = 0);
	IMGUI_API bool ToggleContrastedButton(const char* vLabelTrue, const char* vLabelFalse, bool* vValue, const char* vHelp = nullptr, ImFont* vImfont = nullptr);
	IMGUI_API bool ButtonNoFrame(const char* vName, ImVec2 size = ImVec2(-1, -1), ImVec4 vColor = ImVec4(1, 1, 1, 1), const char* vHelp = 0, ImFont* vLabelFont = nullptr);
	IMGUI_API bool SmallContrastedButton(const char* label);

	IMGUI_API bool Selectable_FramedText_Selected(const bool& vSelected, const char* fmt, ...);
	IMGUI_API bool Selectable_FramedText(const char* fmt, ...);

	void PlotFVec4Histo(
		const char* vLabel, ct::fvec4* vDatas, int vDataCount,
		bool* vShowChannel = 0,
		ImVec2 frame_size = ImVec2(0, 0),
		ct::fvec4 scale_min = FLT_MAX,
		ct::fvec4 scale_max = FLT_MAX,
		int* vHoveredIdx = 0);

	void ImageZoomPoint(ImTextureID vUserTextureId, const float vWidth, const ImVec2& vCenter, const ImVec2& vPoint, const ImVec2& vRadiusInPixels);
	//void ImageZoomLine(ImTextureID vUserTextureId, const float vWidth, const ImVec2& vStart, const ImVec2& vEnd);

	IMGUI_API bool InputText_Validation(const char* label, char* buf, size_t buf_size,
		const bool* vValidation = nullptr, const char* vValidationHelp = nullptr,
		ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

	IMGUI_API void HelpMarker(const char* desc);

#ifdef USE_GRADIENT
	IMGUI_API void RenderGradFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_start_col, ImU32 fill_end_col, bool border, float rounding);
	IMGUI_API bool GradButton(const char* label, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = ImGuiButtonFlags_None);
#endif

	IMGUI_API bool TransparentButton(const char* label, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = 0);

	IMGUI_API void PlainImageWithBG(ImTextureID vTexId, const ImVec2& size, const ImVec4& bg_col, const ImVec4& tint_col);

	IMGUI_API void ImageRatio(ImTextureID vTexId, float vRatioX, float vWidth, ImVec4 vColor, float /*vBorderThick*/);

#ifdef USE_OPENGL
	// show overlay text on mousehover // l'epaisseur du cadre vient de BorderColor.w
	IMGUI_API bool TextureOverLay(float vWidth, std::shared_ptr<ct::texture> vTex, ImVec4 vBorderColor, const char* vOverlayText, ImVec4 vOverLayTextColor, ImVec4 vOverLayBgColor, const ImVec2& vUV0 = ImVec2(0,0), const ImVec2& vUV1 = ImVec2(1, 1));
#endif
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// SLIDERS //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMGUI_API bool SliderScalarCompact(float width, const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const void* p_step, const char* format = nullptr);
IMGUI_API bool SliderInt32Compact(float width, const char* label, int32_t* v, int32_t v_min, int32_t v_max, int32_t v_step, const char* format = "%d");
IMGUI_API bool SliderInt64Compact(float width, const char* label, int64_t* v, int64_t v_min, int64_t v_max, int64_t v_step, const char* format = "%d");
IMGUI_API bool SliderUIntCompact(float width, const char* label, uint32_t* v, uint32_t v_min, uint32_t v_max, uint32_t v_step, const char* format = "%d");
IMGUI_API bool SliderUInt64Compact(float width, const char* label, uint64_t* v, uint64_t v_min, uint64_t v_max, uint64_t v_step, const char* format = "%d");
IMGUI_API bool SliderSizeTCompact(float width, const char* label, size_t* v, size_t v_min, size_t v_max, size_t v_step, const char* format = "%d");
IMGUI_API bool SliderFloatCompact(float width, const char* label, float* v, float v_min, float v_max, float v_step, const char* format = "%.3f");
IMGUI_API bool SliderDoubleCompact(float width, const char* label, double* v, double v_min, double v_max, double v_step, const char* format = "%.3f");

IMGUI_API bool SliderScalarDefaultCompact(float width, const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const void* p_default, const void* p_step = nullptr, const char* format = nullptr);
IMGUI_API bool SliderIntDefaultCompact(float width, const char* label, int* v, int v_min, int v_max, int v_default, int v_step = 0, const char* format = "%d");
IMGUI_API bool SliderUIntDefaultCompact(float width, const char* label, uint32_t* v, uint32_t v_min, uint32_t v_max, uint32_t v_default, uint32_t v_step = 0, const char* format = "%d");
IMGUI_API bool SliderSizeTDefaultCompact(float width, const char* label, size_t* v, size_t v_min, size_t v_max, size_t v_default, size_t v_step = 0, const char* format = "%d");
IMGUI_API bool SliderFloatDefaultCompact(float width, const char* label, float* v, float v_min, float v_max, float v_default, float v_step = 0.0f, const char* format = "%.3f");

IMGUI_API bool SliderScalar(float width, const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const void* p_step = nullptr, const char* format = nullptr, ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderInt(float width, const char* label, int* v, int v_min, int v_max, int v_step = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderUInt(float width, const char* label, uint32_t* v, uint32_t v_min, uint32_t v_max, uint32_t v_step = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderSizeT(float width, const char* label, size_t* v, size_t v_min, size_t v_max, size_t v_step = 0, const char* format = "%zu", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderFloat(float width, const char* label, float* v, float v_min, float v_max, float v_step = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display.

IMGUI_API bool SliderScalarDefault(float width, const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const void* p_default, const void* p_step, const char* format = nullptr, ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderIntDefault(float width, const char* label, int* v, int v_min, int v_max, int v_default, int v_step = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderUIntDefault(float width, const char* label, uint32_t* v, uint32_t v_min, uint32_t v_max, uint32_t v_default, uint32_t v_step = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderSizeTDefault(float width, const char* label, size_t* v, size_t v_min, size_t v_max, size_t v_default, size_t v_step = 0, const char* format = "%zu", ImGuiSliderFlags flags = 0);
IMGUI_API bool SliderFloatDefault(float width, const char* label, float* v, float v_min, float v_max, float v_default, float v_step = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);     // adjust format to decorate the value with a prefix or a suffix for in-slider labels or unit display.

template<typename T>
IMGUI_API bool SliderDefault(float vWidth, const char* vName, T* vVar, T vInf, T vSup, T vDefault, T vStep = (T)0, bool vForNode = false)
{
	UNUSED(vForNode);

	bool change = false;

	if (std::is_same<T, int>::value)
	{
		change |= SliderScalarDefault(vWidth, vName, ImGuiDataType_S32, (void*)vVar, (void*)&vInf, (void*)&vSup, (void*)&vDefault, (void*)&vStep);
	}
	else if (std::is_same<T, float>::value)
	{
		change |= SliderScalarDefault(vWidth, vName, ImGuiDataType_Float, (void*)vVar, (void*)&vInf, (void*)&vSup, (void*)&vDefault, (void*)&vStep);
	}

	return change;
}

template<typename T>
IMGUI_API bool SliderDefaultCompact(float vWidth, const char* vName, T* vVar, T vInf, T vSup, T vDefault, T vStep = (T)0, bool vForNode = false)
{
	UNUSED(vForNode);

	bool change = false;

	if (std::is_same<T, int>::value)
	{
		change |= SliderScalarDefaultCompact(vWidth, vName, ImGuiDataType_S32, (void*)vVar, (void*)&vInf, (void*)&vSup, (void*)&vDefault, (void*)&vStep);
	}
	else if (std::is_same<T, float>::value)
	{
		change |= SliderScalarDefaultCompact(vWidth, vName, ImGuiDataType_Float, (void*)vVar, (void*)&vInf, (void*)&vSup, (void*)&vDefault, (void*)&vStep);
	}

	return change;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// COMBO ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMGUI_API bool BeginContrastedCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);
IMGUI_API bool ContrastedCombo(float vWidth, const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
IMGUI_API bool ContrastedCombo(float vWidth, const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1);
IMGUI_API bool ContrastedCombo(float vWidth, const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items = -1);
IMGUI_API bool ContrastedComboVectorDefault(float vWidth, const char* label, int* current_item, const std::vector<std::string>& items, int vDefault, int height_in_items = -1);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// INPUT ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMGUI_API bool InputFloatDefault(float vWidth, const char* vName, float* vVar, float vDefault, const char* vInputPrec = "%.3f", const char* vPopupPrec = "%.3f", bool vShowResetButton = true, float vStep = 0.0f, float vStepFast = 0.0f);
IMGUI_API bool InputFloatDefaultStepper(float vWidth, const char* vName, float* vVar, float vDefault, float vStep, float vStepFast, const char* vInputPrec = "%.3f", const char* vPopupPrec = "%.3f", bool vShowResetButton = true);
IMGUI_API bool InputIntDefault(float vWidth, const char* vName, int* vVar, int step, int step_fast, int vDefault);
IMGUI_API bool InputUIntDefault(float vWidth, const char* vName, uint32_t* vVar, uint32_t step, uint32_t step_fast, uint32_t vDefault);
}

namespace ImWidgets
{
	IMGUI_API class InputText
	{
	private:
		static constexpr size_t m_Len = 512U;
		char buffer[m_Len + 1] = "";
		std::string m_Text;

	public:
		IMGUI_API bool DisplayInputText(const float& vWidth, const std::string& vLabel, const std::string& vDefaultText);
		IMGUI_API void SetText(const std::string& vText);
		IMGUI_API std::string GetText(const std::string& vNumericType = "");
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// SLIDER TEMPLATES /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Convert a parametric position on a slider into a value v in the output space (the logical opposite of ScaleRatioFromValueT)
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
inline TYPE inScaleValueFromRatioT(ImGuiDataType data_type, float t, TYPE v_min, TYPE v_max, bool is_logarithmic, float logarithmic_zero_epsilon, float zero_deadzone_halfsize)
{
	// We special-case the extents because otherwise our logarithmic fudging can lead to "mathematically correct"
	// but non-intuitive behaviors like a fully-left slider not actually reaching the minimum value. Also generally simpler.
	if (t <= 0.0f || v_min == v_max)
		return v_min;
	if (t >= 1.0f)
		return v_max;

	TYPE result = (TYPE)0;
	if (is_logarithmic)
	{
		// Fudge min/max to avoid getting silly results close to zero
		FLOATTYPE v_min_fudged = (ImAbs((FLOATTYPE)v_min) < logarithmic_zero_epsilon) ? ((v_min < 0.0f) ? -logarithmic_zero_epsilon : logarithmic_zero_epsilon) : (FLOATTYPE)v_min;
		FLOATTYPE v_max_fudged = (ImAbs((FLOATTYPE)v_max) < logarithmic_zero_epsilon) ? ((v_max < 0.0f) ? -logarithmic_zero_epsilon : logarithmic_zero_epsilon) : (FLOATTYPE)v_max;

		const bool flipped = v_max < v_min; // Check if range is "backwards"
		if (flipped)
			ImSwap(v_min_fudged, v_max_fudged);

		// Awkward special case - we need ranges of the form (-100 .. 0) to convert to (-100 .. -epsilon), not (-100 .. epsilon)
		if ((v_max == 0.0f) && (v_min < 0.0f))
			v_max_fudged = -logarithmic_zero_epsilon;

		float t_with_flip = flipped ? (1.0f - t) : t; // t, but flipped if necessary to account for us flipping the range

		if ((v_min * v_max) < 0.0f) // Range crosses zero, so we have to do this in two parts
		{
			float zero_point_center = (-(float)ImMin(v_min, v_max)) / ImAbs((float)v_max - (float)v_min); // The zero point in parametric space
			float zero_point_snap_L = zero_point_center - zero_deadzone_halfsize;
			float zero_point_snap_R = zero_point_center + zero_deadzone_halfsize;
			if (t_with_flip >= zero_point_snap_L && t_with_flip <= zero_point_snap_R)
				result = (TYPE)0.0f; // Special case to make getting exactly zero possible (the epsilon prevents it otherwise)
			else if (t_with_flip < zero_point_center)
				result = (TYPE)-(logarithmic_zero_epsilon * ImPow(-v_min_fudged / logarithmic_zero_epsilon, (FLOATTYPE)(1.0f - (t_with_flip / zero_point_snap_L))));
			else
				result = (TYPE)(logarithmic_zero_epsilon * ImPow(v_max_fudged / logarithmic_zero_epsilon, (FLOATTYPE)((t_with_flip - zero_point_snap_R) / (1.0f - zero_point_snap_R))));
		}
		else if ((v_min < 0.0f) || (v_max < 0.0f)) // Entirely negative slider
			result = (TYPE)-(-v_max_fudged * ImPow(-v_min_fudged / -v_max_fudged, (FLOATTYPE)(1.0f - t_with_flip)));
		else
			result = (TYPE)(v_min_fudged * ImPow(v_max_fudged / v_min_fudged, (FLOATTYPE)t_with_flip));
	}
	else
	{
		// Linear slider
		const bool is_floating_point = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
		if (is_floating_point)
		{
			result = ImLerp(v_min, v_max, t);
		}
		else if (t < 1.0)
		{
			// - For integer values we want the clicking position to match the grab box so we round above
			//   This code is carefully tuned to work with large values (e.g. high ranges of U64) while preserving this property..
			// - Not doing a *1.0 multiply at the end of a range as it tends to be lossy. While absolute aiming at a large s64/u64
			//   range is going to be imprecise anyway, with this check we at least make the edge values matches expected limits.
			FLOATTYPE v_new_off_f = (SIGNEDTYPE)(v_max - v_min) * t;
			result = (TYPE)((SIGNEDTYPE)v_min + (SIGNEDTYPE)(v_new_off_f + (FLOATTYPE)(v_min > v_max ? -0.5 : 0.5)));
		}
	}

	return result;
}

// FIXME: Move more of the code into SliderBehavior()
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
inline bool inSliderBehaviorStepperT(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, TYPE* v, const TYPE v_min, const TYPE v_max, const TYPE v_step, const char* format, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
	using namespace ImGui;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
	const bool is_logarithmic = (flags & ImGuiSliderFlags_Logarithmic) != 0;
	const bool is_floating_point = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);

	const float grab_padding = 2.0f;
	const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
	float grab_sz = style.GrabMinSize;
	SIGNEDTYPE v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
	if (!is_floating_point && v_range >= 0)                                             // v_range < 0 may happen on integer overflows
		grab_sz = ImMax((float)(slider_sz / (v_range + 1)), style.GrabMinSize);  // For integer sliders: if possible have the grab size represent 1 unit
	grab_sz = ImMin(grab_sz, slider_sz);
	const float slider_usable_sz = slider_sz - grab_sz;
	const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
	const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

	float logarithmic_zero_epsilon = 0.0f; // Only valid when is_logarithmic is true
	float zero_deadzone_halfsize = 0.0f; // Only valid when is_logarithmic is true
	if (is_logarithmic)
	{
		// When using logarithmic sliders, we need to clamp to avoid hitting zero, but our choice of clamp value greatly affects slider precision. We attempt to use the specified precision to estimate a good lower bound.
		const int decimal_precision = is_floating_point ? ImParseFormatPrecision(format, 3) : 1;
		logarithmic_zero_epsilon = ImPow(0.1f, (float)decimal_precision);
		zero_deadzone_halfsize = (style.LogSliderDeadzone * 0.5f) / ImMax(slider_usable_sz, 1.0f);
	}

	// Process interacting with the slider
	bool value_changed = false;
	if (g.ActiveId == id)
	{
		bool set_new_value = false;
		float clicked_t = 0.0f;
		if (g.ActiveIdSource == ImGuiInputSource_Mouse)
		{
			if (!g.IO.MouseDown[0])
			{
				ClearActiveID();
			}
			else
			{
				const float mouse_abs_pos = g.IO.MousePos[axis];
				clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
				if (axis == ImGuiAxis_Y)
					clicked_t = 1.0f - clicked_t;
				set_new_value = true;
			}
		}
		/*else if (g.ActiveIdSource == ImGuiInputSource_Nav)
		{
			if (g.ActiveIdIsJustActivated)
			{
				g.SliderCurrentAccum = 0.0f; // Reset any stored nav delta upon activation
				g.SliderCurrentAccumDirty = false;
			}

			const ImVec2 input_delta2 = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 0.0f, 0.0f);
			float input_delta = (axis == ImGuiAxis_X) ? input_delta2.x : -input_delta2.y;
			if (input_delta != 0.0f)
			{
				const int decimal_precision = is_floating_point ? ImParseFormatPrecision(format, 3) : 0;
				if (decimal_precision > 0)
				{
					input_delta /= 100.0f;    // Gamepad/keyboard tweak speeds in % of slider bounds
					if (IsNavInputDown(ImGuiNavInput_TweakSlow))
						input_delta /= 10.0f;
				}
				else
				{
					if ((v_range >= -100.0f && v_range <= 100.0f) || IsNavInputDown(ImGuiNavInput_TweakSlow))
						input_delta = ((input_delta < 0.0f) ? -1.0f : +1.0f) / (float)v_range; // Gamepad/keyboard tweak speeds in integer steps
					else
						input_delta /= 100.0f;
				}
				if (IsNavInputDown(ImGuiNavInput_TweakFast))
					input_delta *= 10.0f;

				g.SliderCurrentAccum += input_delta;
				g.SliderCurrentAccumDirty = true;
			}

			float delta = g.SliderCurrentAccum;
			if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
			{
				ClearActiveID();
			}
			else if (g.SliderCurrentAccumDirty)
			{
				clicked_t = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);

				if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f)) // This is to avoid applying the saturation when already past the limits
				{
					set_new_value = false;
					g.SliderCurrentAccum = 0.0f; // If pushing up against the limits, don't continue to accumulate
				}
				else
				{
					set_new_value = true;
					float old_clicked_t = clicked_t;
					clicked_t = ImSaturate(clicked_t + delta);

					// Calculate what our "new" clicked_t will be, and thus how far we actually moved the slider, and subtract this from the accumulator
					TYPE v_new = ScaleValueFromRatioT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, clicked_t, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
					if (!(flags & ImGuiSliderFlags_NoRoundToFormat))
						v_new = RoundScalarWithFormatT<TYPE>(format, data_type, v_new);

					float new_clicked_t = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_new, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);

					if (delta > 0)
						g.SliderCurrentAccum -= ImMin(new_clicked_t - old_clicked_t, delta);
					else
						g.SliderCurrentAccum -= ImMax(new_clicked_t - old_clicked_t, delta);
				}

				g.SliderCurrentAccumDirty = false;
			}
		}*/

		if (set_new_value)
		{
			TYPE v_new = inScaleValueFromRatioT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, clicked_t, v_min, v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);

			if (v_step)
			{
				FLOATTYPE v_new_off_f = (FLOATTYPE)v_new;
				TYPE v_new_off_floor = (TYPE)((int)(v_new_off_f / v_step) * v_step);
				//TYPE v_new_off_round = v_new_off_floor + v_step;
				v_new = v_new_off_floor;
			}

			// Round to user desired precision based on format string
			if (!(flags & ImGuiSliderFlags_NoRoundToFormat) && (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double))
				v_new = RoundScalarWithFormatT<TYPE>(format, data_type, v_new);

			// Apply result
			if (*v != v_new)
			{
				*v = v_new;
				value_changed = true;
			}
		}
	}

	if (slider_sz < 1.0f)
	{
		*out_grab_bb = ImRect(bb.Min, bb.Min);
	}
	else
	{
		// Output grab position so it can be displayed by the caller
		float grab_t = (float)inScaleValueFromRatioT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, (float)*v, (TYPE)v_min, (TYPE)v_max, is_logarithmic, logarithmic_zero_epsilon, zero_deadzone_halfsize);
		if (axis == ImGuiAxis_Y)
			grab_t = 1.0f - grab_t;
		const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
		if (axis == ImGuiAxis_X)
			*out_grab_bb = ImRect(grab_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
		else
			*out_grab_bb = ImRect(bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f);
	}

	return value_changed;
}
