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

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <headers/DatasDef.h>

class SourceFile {
public:
    static SourceFilePtr Create();
    static SourceFilePtr Create(const SourceFileName& vSourceFileName);

private:
    SourceFileWeak m_This;
    SourceFilePathName m_SourceFilePathName;
    SourceFileName m_SourceFileName;
    EpochOffset m_EpochOffset = 0.0;

public:
    void SetSourceFilePathName(const SourceFileName& vSourceFileName);
    SourceFilePathName GetSourceFilePathName() const;
    SourceFileName GetSourceFileName() const;
    ImGuiLabel GetImGuiLabel() const;

    void SetEpochOffset(const EpochOffset& vEpohOffset);
    EpochOffset GetEpochOffset() const;
};