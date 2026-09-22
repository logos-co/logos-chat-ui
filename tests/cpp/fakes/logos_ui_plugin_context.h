#pragma once

// Test double for the SDK's LogosUiPluginContext: the surface a backend uses,
// with the test handing over the modules the generated plugin glue would.
struct LogosModules;

class LogosUiPluginContext {
public:
    virtual ~LogosUiPluginContext() = default;

    LogosModules& modules() const { return *static_cast<LogosModules*>(m_logosModulesPtr); }
    bool isContextReady() const { return m_logosModulesPtr != nullptr; }

    void _logosCoreSetLogosModulesPtr_(void* ptr)
    {
        m_logosModulesPtr = ptr;
        onContextReady();
    }

protected:
    virtual void onContextReady() {}

private:
    void* m_logosModulesPtr = nullptr;
};
