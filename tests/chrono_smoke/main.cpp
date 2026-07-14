#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>

#include <chrono/physics/ChBody.h>
#include <chrono/physics/ChSystemNSC.h>
#include <chrono_vehicle/ChWorldFrame.h>

int main()
{
    using chrono::ChBody;
    using chrono::ChSystemNSC;
    using chrono::ChVector3d;
    using chrono::vehicle::ChWorldFrame;

    ChWorldFrame::SetYUP();
    const ChVector3d up = ChWorldFrame::Vertical();
    if (std::abs(up.x()) > 1e-12 || std::abs(up.y() - 1.0) > 1e-12 || std::abs(up.z()) > 1e-12)
    {
        std::cerr << "Chrono Vehicle did not establish the expected Y-up world frame.\n";
        return 1;
    }

    ChSystemNSC system;
    system.SetGravitationalAcceleration(-9.81 * up);

    auto body = std::make_shared<ChBody>();
    body->SetMass(1.0);
    body->SetInertiaXX(ChVector3d(1.0, 1.0, 1.0));
    body->SetPos(ChVector3d(0.0, 2.0, 0.0));
    system.AddBody(body);

    constexpr double step = 0.005;
    constexpr int stepCount = 120;
    for (int i = 0; i < stepCount; ++i)
    {
        system.DoStepDynamics(step);
    }

    const double expectedTime = step * stepCount;
    const double finalHeight = ChWorldFrame::Height(body->GetPos());
    if (!std::isfinite(finalHeight) || std::abs(system.GetChTime() - expectedTime) > 1e-10 || finalHeight >= 1.0)
    {
        std::cerr << "Chrono integration check failed: time=" << system.GetChTime()
                  << ", height=" << finalHeight << "\n";
        return 2;
    }

    std::cout << "Chrono Core/Vehicle smoke passed: time=" << system.GetChTime()
              << ", height=" << finalHeight << "\n";
    return 0;
}
