#include "core/physics.hpp"

void Physics::Init()
{
    mWorld = mCommon.createPhysicsWorld();

    // Create the default logger
    Params.InfoLogs    = false;
    Params.WarningLogs = false;
    Params.ErrorLogs   = true;
    Params.Logger      = mCommon.createDefaultLogger();
#if WATO_DEBUG
    mWorld->setIsDebugRenderingEnabled(true);
#endif
    InitLogger();
}

void Physics::InitLogger()
{
    uint logLevel = 0;
    if (Params.InfoLogs) {
        logLevel |= static_cast<uint>(rp3d::Logger::Level::Information);
    }
    if (Params.WarningLogs) {
        logLevel |= static_cast<uint>(rp3d::Logger::Level::Warning);
    }
    if (Params.ErrorLogs) {
        logLevel |= static_cast<uint>(rp3d::Logger::Level::Error);
    }

    // Output the logs into an HTML file
    Params.Logger->addFileDestination("rp3d_log.html", logLevel, rp3d::DefaultLogger::Format::HTML);

    // Output the logs into the standard output
    Params.Logger->addStreamDestination(std::cout, logLevel, rp3d::DefaultLogger::Format::Text);
    mCommon.setLogger(Params.Logger);
}
