// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SourceFile.h"

#include <ctools/FileHelper.h>

SourceFilePtr SourceFile::Create()
{
	auto res = std::make_shared<SourceFile>();
	res->m_This = res;
	return res;
}

SourceFilePtr SourceFile::Create(const SourceFileName& vSourceFileName)
{
	auto res = std::make_shared<SourceFile>();
	res->m_This = res;
	res->SetSourceFilePathName(vSourceFileName);
	return res;
}

SourceFile::SourceFile() = default;

SourceFile::~SourceFile() = default;

void SourceFile::SetSourceFilePathName(const SourceFileName& vSourceFileName)
{
	m_SourceFileName = m_SourceFilePathName = vSourceFileName;

	auto ps = FileHelper::Instance()->ParsePathFileName(m_SourceFileName);
	if (ps.isOk)
	{
		m_SourceFileName = ps.GetFPNE_WithPath("");
	}
}

SourceFileName SourceFile::GetSourceFilePathName() const
{
	return m_SourceFilePathName;
}

SourceFileName SourceFile::GetSourceFileName() const
{
	return m_SourceFileName;
}

ImGuiLabel SourceFile::GetImGuiLabel() const
{
	return m_SourceFileName.c_str();
}

void SourceFile::SetEpochOffset(const EpochOffset& vEpohOffset)
{
	m_EpochOffset = vEpohOffset;
}

EpochOffset SourceFile::GetEpochOffset() const
{
	return m_EpochOffset;
}

std::string SourceFile::getXml(const std::string& /*vOffset*/, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool SourceFile::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
{
	// The value of this child identifies the name of this element
	std::string strName;
	std::string strValue;
	std::string strParentName;

	strName = vElem->Value();
	if (vElem->GetText())
		strValue = vElem->GetText();
	if (vParent != nullptr)
		strParentName = vParent->Value();

	return true;
}