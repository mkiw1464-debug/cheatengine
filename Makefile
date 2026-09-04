ARCHS   = arm64
TARGET  = iphone:clang:latest:14.0

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = ModEngine

ModEngine_FILES = \
    Sources/ModEngine.mm \
    Sources/MenuView.mm

# ← C++17 wajib untuk std::optional
ModEngine_CXXFLAGS  = -std=c++17 -fobjc-arc
ModEngine_CCFLAGS   = -fobjc-arc

ModEngine_FRAMEWORKS = UIKit CoreGraphics QuartzCore
ModEngine_LIBRARIES  = substrate

include $(THEOS_MAKE_PATH)/tweak.mk
