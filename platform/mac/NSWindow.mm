#include "NSWindow.h"
#import <Cocoa/Cocoa.h>

void Platform::Mac::NSWindow::setAllowsAutomaticWindowTabbing(bool flag) {
    [::NSWindow setAllowsAutomaticWindowTabbing:flag];
}
