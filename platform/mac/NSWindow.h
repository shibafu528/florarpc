#ifndef FLORARPC_NS_WINDOW_H
#define FLORARPC_NS_WINDOW_H

namespace Platform {
    namespace Mac {
        class NSWindow {
        public:
            static void setAllowsAutomaticWindowTabbing(bool flag);
        };
    }  // namespace Mac
}  // namespace Platform

#endif  // FLORARPC_NS_WINDOW_H