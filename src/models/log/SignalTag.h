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

class SignalTag {
public:
    static SignalTagPtr Create();

private:
    SignalTagWeak m_This;

public:
    SignalEpochTime time_epoch = 0.0;
    SignalDateTime time_date_time;
    SignalTagColor color;
    SignalTagName name;
    SignalTagHelp help;
};