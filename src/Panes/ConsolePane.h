#pragma once

#include <ImGuiPack.h>
#include <cstdint>
#include <memory>
#include <string>

class ProjectFile;
class ConsolePane : public AbstractPane {
public:
    bool Init() override;
    void Unit() override;
    bool DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened = nullptr, ImGuiContext* vContextPtr = nullptr, void* vUserDatas = nullptr) override;

public:  // singleton
    static std::shared_ptr<ConsolePane> Instance() {
        static std::shared_ptr<ConsolePane> _instance = std::make_shared<ConsolePane>();
        return _instance;
    }

public:
    ConsolePane();                              // Prevent construction
    ConsolePane(const ConsolePane&) = default;  // Prevent construction by copying
    ConsolePane& operator=(const ConsolePane&) {
        return *this;
    };                       // Prevent assignment
    virtual ~ConsolePane();  // Prevent unwanted destruction};
};
