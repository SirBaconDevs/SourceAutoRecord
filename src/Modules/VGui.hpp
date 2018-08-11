#pragma once
#include "Console.hpp"
#include "Engine.hpp"
#include "Scheme.hpp"
#include "Server.hpp"
#include "Surface.hpp"

#include "Features/Hud/InputHud.hpp"
#include "Features/Hud/SpeedrunHud.hpp"
#include "Features/Routing.hpp"
#include "Features/Session.hpp"
#include "Features/Stats.hpp"
#include "Features/StepCounter.hpp"
#include "Features/Timer.hpp"
#include "Features/TimerAverage.hpp"
#include "Features/TimerCheckPoints.hpp"

#include "Cheats.hpp"
#include "Game.hpp"
#include "Interface.hpp"
#include "Offsets.hpp"
#include "Utils.hpp"

namespace VGui {

Interface* enginevgui;

using _Paint = int(__func*)(void* thisptr, int mode);

bool RespectClShowPos = true;

// CEngineVGui::Paint
DETOUR(Paint, int mode)
{
    surface->StartDrawing(surface->matsurface->ThisPtr());

    auto elements = 0;
    auto xPadding = sar_hud_default_padding_x.GetInt();
    auto yPadding = sar_hud_default_padding_y.GetInt();
    auto spacing = sar_hud_default_spacing.GetInt();

    auto font = scheme->GetDefaultFont() + sar_hud_default_font_index.GetInt();
    auto fontSize = surface->GetFontHeight(font);

    int r, g, b, a;
    sscanf(sar_hud_default_font_color.GetString(), "%i%i%i%i", &r, &g, &b, &a);
    Color textColor(r, g, b, a);

    if (RespectClShowPos && cl_showpos.GetBool()) {
        elements += 4;
        yPadding += spacing;
    }

    auto DrawElement = [font, xPadding, yPadding, fontSize, spacing, textColor, &elements](const char* fmt, ...) {
        va_list argptr;
        va_start(argptr, fmt);
        char data[1024];
        vsnprintf(data, sizeof(data), fmt, argptr);
        va_end(argptr);

        surface->DrawTxt(font,
            xPadding,
            yPadding + elements * (fontSize + spacing),
            textColor,
            data);

        ++elements;
    };

    // cl_showpos replacement
    if (sar_hud_text.GetString()[0] != '\0') {
        DrawElement((char*)sar_hud_text.GetString());
    }
    if (sar_hud_position.GetBool()) {
        auto abs = Client::GetAbsOrigin();
        DrawElement("pos: %.3f %.3f %.3f", abs.x, abs.y, abs.z);
    }
    if (sar_hud_angles.GetBool()) {
        auto va = Engine::GetAngles();
        if (sar_hud_angles.GetInt() == 1) {
            DrawElement("ang: %.3f %.3f", va.x, va.y);
        } else {
            DrawElement("ang: %.3f %.3f %.3f", va.x, va.y, va.z);
        }
    }
    if (sar_hud_velocity.GetBool()) {
        auto vel = (sar_hud_velocity.GetInt() == 1)
            ? Client::GetLocalVelocity().Length()
            : Client::GetLocalVelocity().Length2D();
        DrawElement("vel: %.3f", vel);
    }
    // Session
    if (sar_hud_session.GetBool()) {
        auto tick = (Engine::isInSession) ? Engine::GetSessionTick() : 0;
        auto time = Engine::ToTime(tick);
        DrawElement("session: %i (%.3f)", tick, time);
    }
    if (sar_hud_last_session.GetBool()) {
        DrawElement("last session: %i (%.3f)", Session::LastSession, Engine::ToTime(Session::LastSession));
    }
    if (sar_hud_sum.GetBool()) {
        if (Summary::IsRunning && sar_sum_during_session.GetBool()) {
            auto tick = (Engine::isInSession) ? Engine::GetSessionTick() : 0;
            auto time = Engine::ToTime(tick);
            DrawElement("sum: %i (%.3f)", Summary::TotalTicks + tick, Engine::ToTime(Summary::TotalTicks) + time);
        } else {
            DrawElement("sum: %i (%.3f)", Summary::TotalTicks, Engine::ToTime(Summary::TotalTicks));
        }
    }
    // Timer
    if (sar_hud_timer.GetBool()) {
        auto tick = (!Timer::IsPaused) ? Timer::GetTick(*Engine::tickcount) : Timer::TotalTicks;
        auto time = Engine::ToTime(tick);
        DrawElement("timer: %i (%.3f)", tick, time);
    }
    if (sar_hud_avg.GetBool()) {
        DrawElement("avg: %i (%.3f)", Timer::Average::AverageTicks, Timer::Average::AverageTime);
    }
    if (sar_hud_cps.GetBool()) {
        DrawElement("last cp: %i (%.3f)", Timer::CheckPoints::LatestTick, Timer::CheckPoints::LatestTime);
    }
    // Demo
    if (sar_hud_demo.GetBool()) {
        if (!*Engine::m_bLoadgame && *Engine::DemoRecorder::m_bRecording && !Engine::DemoRecorder::CurrentDemo.empty()) {
            auto tick = Engine::DemoRecorder::GetTick();
            auto time = Engine::ToTime(tick);
            DrawElement("demo: %s %i (%.3f)", Engine::DemoRecorder::CurrentDemo.c_str(), tick, time);
        } else if (!*Engine::m_bLoadgame && Engine::DemoPlayer::IsPlaying()) {
            auto tick = Engine::DemoPlayer::GetTick();
            auto time = Engine::ToTime(tick);
            DrawElement("demo: %s %i (%.3f)", Engine::DemoPlayer::DemoName, tick, time);
        } else {
            DrawElement("demo: -");
        }
    }
    // Stats
    if (sar_hud_jumps.GetBool()) {
        DrawElement("jumps: %i", stats->jumps->total);
    }
    if (Game::IsPortalGame() && sar_hud_portals.GetBool()) {
        DrawElement("portals: %i", Server::GetPortals());
    }
    if (sar_hud_steps.GetBool()) {
        DrawElement("steps: %i", stats->steps->total);
    }
    if (sar_hud_jump.GetBool()) {
        DrawElement("jump: %.3f", stats->jumps->distance);
    }
    if (sar_hud_jump_peak.GetBool()) {
        DrawElement("jump peak: %.3f", stats->jumps->distance);
    }
    if (sar_hud_velocity_peak.GetBool()) {
        DrawElement("vel peak: %.3f", stats->velocity->peak);
    }
    // Routing
    if (sar_hud_trace.GetBool()) {
        auto xyz = Routing::Tracer::GetDifferences();
        auto result = (sar_hud_trace.GetInt() == 1)
            ? Routing::Tracer::GetResult(Routing::Tracer::ResultType::VEC3)
            : Routing::Tracer::GetResult(Routing::Tracer::ResultType::VEC2);
        DrawElement("trace: %.3f (%.3f/%.3f/%.3f)", result, std::get<0>(xyz), std::get<1>(xyz), std::get<2>(xyz));
    }
    if (sar_hud_frame.GetBool()) {
        DrawElement("frame: %i", Engine::currentFrame);
    }
    if (sar_hud_last_frame.GetBool()) {
        DrawElement("last frame: %i", Engine::lastFrame);
    }

    surface->FinishDrawing();

    // Draw other HUDs
    inputHud->Draw();
    speedrunHud->Draw();

    return Original::Paint(thisptr, mode);
}

void Init()
{
    inputHud = new InputHud();
    speedrunHud = new SpeedrunHud();

    enginevgui = Interface::Create(MODULE("engine"), "VEngineVGui0");
    if (enginevgui) {
        enginevgui->Hook(Detour::Paint, Original::Paint, Offsets::Paint);
    }

    if (Game::IsHalfLife2Engine()) {
        RespectClShowPos = false;
    }
}
void Shutdown()
{
    Interface::Delete(enginevgui);

    SAFE_DELETE(inputHud);
    SAFE_DELETE(speedrunHud);
}
}