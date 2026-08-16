#pragma once
#include "imgui.h"
#include <windows.h>
#include <vector>
#include <functional>
#include <time.h>

extern enum class eGameVer;
extern eGameVer gGameVer;

struct FontInfo {
    bool m_bFontLoaded = false;
    ImFont *m_pFont = nullptr;
    std::string m_Path;
    float m_nSize = 14.0f;
    size_t m_nStart, m_nEnd;
};

class ImGuiFrame {
public:
    ImGuiContext *m_pContext = nullptr;

    // Scaling related
    ImVec2 m_vecScaling = ImVec2(1, 1);
    bool m_bWasScalingUpdatedThisFrame;
    bool m_bNeedToUpdateScaling; 
    long long m_nLastScriptCallMS; 

    // Render buffers
    bool m_bIsBackBufferReady;
    std::vector<std::function<void()>> m_RenderBuffer, m_BackBuffer; 
    
    // for ImGui::ImageButton()
    ImVec4 m_vecImgTint = ImVec4(1, 1, 1, 1);
    ImVec4 m_vecImgBgCol = ImVec4(0, 0, 0, 0);   // was white - painted a
                                                 // solid square behind
                                                 // every transparent PNG

    // Fonts
    std::vector<std::pair<size_t, size_t>> m_FontGlyphRange;
    std::vector<FontInfo> m_FontTable;

    ImGuiFrame() {
        // m_pContext = ImGui::CreateContext();
    }

    ImGuiFrame& operator+=(std::function<void()> f) {
        if (!m_bIsBackBufferReady) {
            m_BackBuffer.push_back(f);
        }
        return *this;
    }   

    void BeforeRender() {
        // bool buildRequired = false;
        // for (auto& e: m_FontTable) {
        //     if (!e.m_bFontLoaded) {
        //         ImWchar ranges[] = { 
        //             e.m_nStart, e.m_nEnd, 0
        //         };
        //         ImGui::GetIO().Fonts->AddFontFromFileTTF(e.m_Path.c_str(), e.m_nSize, NULL, ranges);
        //         buildRequired = true;
        //     }
        // }

        // if (buildRequired) {
        //     ImGui::GetIO().Fonts->Build();
        // }
    }

    void OnRender() {
        for (auto func : m_RenderBuffer) {
            func();
        }

        // if back buffer is render ready switch the buffer and reset render state
        if (m_bIsBackBufferReady) {
            m_RenderBuffer = std::move(m_BackBuffer);
            m_bIsBackBufferReady = false;
        }

        // Clear the frame as soon as the owning script stops feeding it -
        // e.g. the game is paused (scripts freeze) or the script ended. The
        // old check was 2 whole SECONDS plus 32-bit-only memory reads, so the
        // overlay lingered over the pause menu. 400 ms in real milliseconds.
        if ((long long)GetTickCount64() - m_nLastScriptCallMS > 400) {
            OnClear();
        }

        if (m_bWasScalingUpdatedThisFrame) {
            m_bNeedToUpdateScaling = false;
            m_bWasScalingUpdatedThisFrame = false;
        }
    }

    void OnClear() {
        m_RenderBuffer.clear();
    }
};
