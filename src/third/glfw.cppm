module;

// Vulkan related functions are located at the bottom of the file.
// There are only three functions and two variables.
#include <vulkan/vulkan.h>
// Exported all content from <GLFW/glfw3.h>
#include <GLFW/glfw3.h>

export module glfw;

export namespace glfw {
    constexpr int VERSION_MAJOR = GLFW_VERSION_MAJOR;
    constexpr int VERSION_MINOR = GLFW_VERSION_MINOR;
    constexpr int VERSION_REVISION = GLFW_VERSION_REVISION;
    constexpr int TRUE = GLFW_TRUE;
    constexpr int FALSE = GLFW_FALSE;
    constexpr int RELEASE = GLFW_RELEASE;
    constexpr int PRESS = GLFW_PRESS;
    constexpr int REPEAT = GLFW_REPEAT;
    constexpr int HAT_CENTERED =  GLFW_HAT_CENTERED;
    constexpr int HAT_UP = GLFW_HAT_UP;
    constexpr int HAT_RIGHT = GLFW_HAT_RIGHT;
    constexpr int HAT_DOWN = GLFW_HAT_DOWN;
    constexpr int HAT_LEFT = GLFW_HAT_LEFT;
    constexpr int HAT_RIGHT_UP = GLFW_HAT_RIGHT_UP;
    constexpr int HAT_RIGHT_DOWN = GLFW_HAT_RIGHT_DOWN;
    constexpr int HAT_LEFT_UP = GLFW_HAT_LEFT_UP;
    constexpr int HAT_LEFT_DOWN = GLFW_HAT_LEFT_DOWN;
    constexpr int KEY_UNKNOWN = GLFW_KEY_UNKNOWN;

    constexpr int KEY_SPACE = GLFW_KEY_SPACE;
    constexpr int KEY_APOSTROPHE = GLFW_KEY_APOSTROPHE;
    constexpr int KEY_COMMA = GLFW_KEY_COMMA;
    constexpr int KEY_MINUS = GLFW_KEY_MINUS;
    constexpr int KEY_PERIOD = GLFW_KEY_PERIOD;
    constexpr int KEY_SLASH = GLFW_KEY_SLASH;
    constexpr int KEY_0 = GLFW_KEY_0;
    constexpr int KEY_1 = GLFW_KEY_1;
    constexpr int KEY_2 = GLFW_KEY_2;
    constexpr int KEY_3 = GLFW_KEY_3;
    constexpr int KEY_4 = GLFW_KEY_4;
    constexpr int KEY_5 = GLFW_KEY_5;
    constexpr int KEY_6 = GLFW_KEY_6;
    constexpr int KEY_7 = GLFW_KEY_7;
    constexpr int KEY_8 = GLFW_KEY_8;
    constexpr int KEY_9 = GLFW_KEY_9;
    constexpr int KEY_SEMICOLON = GLFW_KEY_SEMICOLON;
    constexpr int KEY_EQUAL = GLFW_KEY_EQUAL;
    constexpr int KEY_A = GLFW_KEY_A;
    constexpr int KEY_B = GLFW_KEY_B;
    constexpr int KEY_C = GLFW_KEY_C;
    constexpr int KEY_D = GLFW_KEY_D;
    constexpr int KEY_E = GLFW_KEY_E;
    constexpr int KEY_F = GLFW_KEY_F;
    constexpr int KEY_G = GLFW_KEY_G;
    constexpr int KEY_H = GLFW_KEY_H;
    constexpr int KEY_I = GLFW_KEY_I;
    constexpr int KEY_J = GLFW_KEY_J;
    constexpr int KEY_K = GLFW_KEY_K;
    constexpr int KEY_L = GLFW_KEY_L;
    constexpr int KEY_M = GLFW_KEY_M;
    constexpr int KEY_N = GLFW_KEY_N;
    constexpr int KEY_O = GLFW_KEY_O;
    constexpr int KEY_P = GLFW_KEY_P;
    constexpr int KEY_Q = GLFW_KEY_Q;
    constexpr int KEY_R = GLFW_KEY_R;
    constexpr int KEY_S = GLFW_KEY_S;
    constexpr int KEY_T = GLFW_KEY_T;
    constexpr int KEY_U = GLFW_KEY_U;
    constexpr int KEY_V = GLFW_KEY_V;
    constexpr int KEY_W = GLFW_KEY_W;
    constexpr int KEY_X = GLFW_KEY_X;
    constexpr int KEY_Y = GLFW_KEY_Y;
    constexpr int KEY_Z = GLFW_KEY_Z;
    constexpr int KEY_LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET;
    constexpr int KEY_BACKSLASH = GLFW_KEY_BACKSLASH;
    constexpr int KEY_RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET;
    constexpr int KEY_GRAVE_ACCENT = GLFW_KEY_GRAVE_ACCENT;

    constexpr int KEY_WORLD_1 = GLFW_KEY_WORLD_1;
    constexpr int KEY_WORLD_2 = GLFW_KEY_WORLD_2;

    constexpr int KEY_ESCAPE = GLFW_KEY_ESCAPE;
    constexpr int KEY_ENTER = GLFW_KEY_ENTER;
    constexpr int KEY_TAB = GLFW_KEY_TAB;
    constexpr int KEY_BACKSPACE = GLFW_KEY_BACKSPACE;
    constexpr int KEY_INSERT = GLFW_KEY_INSERT;
    constexpr int KEY_DELETE = GLFW_KEY_DELETE;
    constexpr int KEY_RIGHT = GLFW_KEY_RIGHT;
    constexpr int KEY_LEFT = GLFW_KEY_LEFT;
    constexpr int KEY_DOWN = GLFW_KEY_DOWN;
    constexpr int KEY_UP = GLFW_KEY_UP;
    constexpr int KEY_PAGE_UP = GLFW_KEY_PAGE_UP;
    constexpr int KEY_PAGE_DOWN = GLFW_KEY_PAGE_DOWN;
    constexpr int KEY_HOME = GLFW_KEY_HOME;
    constexpr int KEY_END = GLFW_KEY_END;
    constexpr int KEY_CAPS_LOCK = GLFW_KEY_CAPS_LOCK;
    constexpr int KEY_SCROLL_LOCK = GLFW_KEY_SCROLL_LOCK;
    constexpr int KEY_NUM_LOCK = GLFW_KEY_NUM_LOCK;
    constexpr int KEY_PRINT_SCREEN = GLFW_KEY_PRINT_SCREEN;
    constexpr int KEY_PAUSE = GLFW_KEY_PAUSE;
    constexpr int KEY_F1 = GLFW_KEY_F1;
    constexpr int KEY_F2 = GLFW_KEY_F2;
    constexpr int KEY_F3 = GLFW_KEY_F3;
    constexpr int KEY_F4 = GLFW_KEY_F4;
    constexpr int KEY_F5 = GLFW_KEY_F5;
    constexpr int KEY_F6 = GLFW_KEY_F6;
    constexpr int KEY_F7 = GLFW_KEY_F7;
    constexpr int KEY_F8 = GLFW_KEY_F8;
    constexpr int KEY_F9 = GLFW_KEY_F9;
    constexpr int KEY_F10 = GLFW_KEY_F10;
    constexpr int KEY_F11 = GLFW_KEY_F11;
    constexpr int KEY_F12 = GLFW_KEY_F12;
    constexpr int KEY_F13 = GLFW_KEY_F13;
    constexpr int KEY_F14 = GLFW_KEY_F14;
    constexpr int KEY_F15 = GLFW_KEY_F15;
    constexpr int KEY_F16 = GLFW_KEY_F16;
    constexpr int KEY_F17 = GLFW_KEY_F17;
    constexpr int KEY_F18 = GLFW_KEY_F18;
    constexpr int KEY_F19 = GLFW_KEY_F19;
    constexpr int KEY_F20 = GLFW_KEY_F20;
    constexpr int KEY_F21 = GLFW_KEY_F21;
    constexpr int KEY_F22 = GLFW_KEY_F22;
    constexpr int KEY_F23 = GLFW_KEY_F23;
    constexpr int KEY_F24 = GLFW_KEY_F24;
    constexpr int KEY_F25 = GLFW_KEY_F25;
    constexpr int KEY_KP_0 = GLFW_KEY_KP_0;
    constexpr int KEY_KP_1 = GLFW_KEY_KP_1;
    constexpr int KEY_KP_2 = GLFW_KEY_KP_2;
    constexpr int KEY_KP_3 = GLFW_KEY_KP_3;
    constexpr int KEY_KP_4 = GLFW_KEY_KP_4;
    constexpr int KEY_KP_5 = GLFW_KEY_KP_5;
    constexpr int KEY_KP_6 = GLFW_KEY_KP_6;
    constexpr int KEY_KP_7 = GLFW_KEY_KP_7;
    constexpr int KEY_KP_8 = GLFW_KEY_KP_8;
    constexpr int KEY_KP_9 = GLFW_KEY_KP_9;
    constexpr int KEY_KP_DECIMAL = GLFW_KEY_KP_DECIMAL;
    constexpr int KEY_KP_DIVIDE = GLFW_KEY_KP_DIVIDE;
    constexpr int KEY_KP_MULTIPLY = GLFW_KEY_KP_MULTIPLY;
    constexpr int KEY_KP_SUBTRACT = GLFW_KEY_KP_SUBTRACT;
    constexpr int KEY_KP_ADD = GLFW_KEY_KP_ADD;
    constexpr int KEY_KP_ENTER = GLFW_KEY_KP_ENTER;
    constexpr int KEY_KP_EQUAL = GLFW_KEY_KP_EQUAL;
    constexpr int KEY_LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT;
    constexpr int KEY_LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL;
    constexpr int KEY_LEFT_ALT = GLFW_KEY_LEFT_ALT;
    constexpr int KEY_LEFT_SUPER = GLFW_KEY_LEFT_SUPER;
    constexpr int KEY_RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT;
    constexpr int KEY_RIGHT_CONTROL = GLFW_KEY_RIGHT_CONTROL;
    constexpr int KEY_RIGHT_ALT = GLFW_KEY_RIGHT_ALT;
    constexpr int KEY_RIGHT_SUPER = GLFW_KEY_RIGHT_SUPER;
    constexpr int KEY_MENU = GLFW_KEY_MENU;
    constexpr int KEY_LAST = GLFW_KEY_LAST;

    constexpr int MOD_SHIFT = GLFW_MOD_SHIFT;
    constexpr int MOD_CONTROL = GLFW_MOD_CONTROL;
    constexpr int MOD_ALT = GLFW_MOD_ALT;
    constexpr int MOD_SUPER = GLFW_MOD_SUPER;
    constexpr int MOD_CAPS_LOCK = GLFW_MOD_CAPS_LOCK;
    constexpr int MOD_NUM_LOCK = GLFW_MOD_NUM_LOCK;

    constexpr int MOUSE_BUTTON_1 = GLFW_MOUSE_BUTTON_1;
    constexpr int MOUSE_BUTTON_2 = GLFW_MOUSE_BUTTON_2;
    constexpr int MOUSE_BUTTON_3 = GLFW_MOUSE_BUTTON_3;
    constexpr int MOUSE_BUTTON_4 = GLFW_MOUSE_BUTTON_4;
    constexpr int MOUSE_BUTTON_5 = GLFW_MOUSE_BUTTON_5;
    constexpr int MOUSE_BUTTON_6 = GLFW_MOUSE_BUTTON_6;
    constexpr int MOUSE_BUTTON_7 = GLFW_MOUSE_BUTTON_7;
    constexpr int MOUSE_BUTTON_8 = GLFW_MOUSE_BUTTON_8;
    constexpr int MOUSE_BUTTON_LAST = GLFW_MOUSE_BUTTON_LAST;
    constexpr int MOUSE_BUTTON_LEFT = GLFW_MOUSE_BUTTON_LEFT;
    constexpr int MOUSE_BUTTON_RIGHT = GLFW_MOUSE_BUTTON_RIGHT;
    constexpr int MOUSE_BUTTON_MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE;

    constexpr int JOYSTICK_1 = GLFW_JOYSTICK_1;
    constexpr int JOYSTICK_2 = GLFW_JOYSTICK_2;
    constexpr int JOYSTICK_3 = GLFW_JOYSTICK_3;
    constexpr int JOYSTICK_4 = GLFW_JOYSTICK_4;
    constexpr int JOYSTICK_5 = GLFW_JOYSTICK_5;
    constexpr int JOYSTICK_6 = GLFW_JOYSTICK_6;
    constexpr int JOYSTICK_7 = GLFW_JOYSTICK_7;
    constexpr int JOYSTICK_8 = GLFW_JOYSTICK_8;
    constexpr int JOYSTICK_9 = GLFW_JOYSTICK_9;
    constexpr int JOYSTICK_10 = GLFW_JOYSTICK_10;
    constexpr int JOYSTICK_11 = GLFW_JOYSTICK_11;
    constexpr int JOYSTICK_12 = GLFW_JOYSTICK_12;
    constexpr int JOYSTICK_13 = GLFW_JOYSTICK_13;
    constexpr int JOYSTICK_14 = GLFW_JOYSTICK_14;
    constexpr int JOYSTICK_15 = GLFW_JOYSTICK_15;
    constexpr int JOYSTICK_16 = GLFW_JOYSTICK_16;
    constexpr int JOYSTICK_LAST = GLFW_JOYSTICK_LAST;

    constexpr int GAMEPAD_BUTTON_A = GLFW_GAMEPAD_BUTTON_A;
    constexpr int GAMEPAD_BUTTON_B = GLFW_GAMEPAD_BUTTON_B;
    constexpr int GAMEPAD_BUTTON_X = GLFW_GAMEPAD_BUTTON_X;
    constexpr int GAMEPAD_BUTTON_Y = GLFW_GAMEPAD_BUTTON_Y;
    constexpr int GAMEPAD_BUTTON_LEFT_BUMPER = GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
    constexpr int GAMEPAD_BUTTON_RIGHT_BUMPER = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
    constexpr int GAMEPAD_BUTTON_BACK = GLFW_GAMEPAD_BUTTON_BACK;
    constexpr int GAMEPAD_BUTTON_START = GLFW_GAMEPAD_BUTTON_START;
    constexpr int GAMEPAD_BUTTON_GUIDE = GLFW_GAMEPAD_BUTTON_GUIDE;
    constexpr int GAMEPAD_BUTTON_LEFT_THUMB = GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
    constexpr int GAMEPAD_BUTTON_RIGHT_THUMB = GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
    constexpr int GAMEPAD_BUTTON_DPAD_UP = GLFW_GAMEPAD_BUTTON_DPAD_UP;
    constexpr int GAMEPAD_BUTTON_DPAD_RIGHT = GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
    constexpr int GAMEPAD_BUTTON_DPAD_DOWN = GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
    constexpr int GAMEPAD_BUTTON_DPAD_LEFT = GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
    constexpr int GAMEPAD_BUTTON_LAST = GLFW_GAMEPAD_BUTTON_LAST;
    constexpr int GAMEPAD_BUTTON_CROSS = GLFW_GAMEPAD_BUTTON_CROSS;
    constexpr int GAMEPAD_BUTTON_CIRCLE = GLFW_GAMEPAD_BUTTON_CIRCLE;
    constexpr int GAMEPAD_BUTTON_SQUARE = GLFW_GAMEPAD_BUTTON_SQUARE;
    constexpr int GAMEPAD_BUTTON_TRIANGLE = GLFW_GAMEPAD_BUTTON_TRIANGLE;

    constexpr int GAMEPAD_AXIS_LEFT_X = GLFW_GAMEPAD_AXIS_LEFT_X;
    constexpr int GAMEPAD_AXIS_LEFT_Y = GLFW_GAMEPAD_AXIS_LEFT_Y;
    constexpr int GAMEPAD_AXIS_RIGHT_X = GLFW_GAMEPAD_AXIS_RIGHT_X;
    constexpr int GAMEPAD_AXIS_RIGHT_Y = GLFW_GAMEPAD_AXIS_RIGHT_Y;
    constexpr int GAMEPAD_AXIS_LEFT_TRIGGER = GLFW_GAMEPAD_AXIS_LEFT_TRIGGER;
    constexpr int GAMEPAD_AXIS_RIGHT_TRIGGER = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;
    constexpr int GAMEPAD_AXIS_LAST = GLFW_GAMEPAD_AXIS_LAST;

    constexpr int NO_ERROR = GLFW_NO_ERROR;
    constexpr int NOT_INITIALIZED = GLFW_NOT_INITIALIZED;
    constexpr int NO_CURRENT_CONTEXT = GLFW_NO_CURRENT_CONTEXT;
    constexpr int INVALID_ENUM = GLFW_INVALID_ENUM;
    constexpr int INVALID_VALUE = GLFW_INVALID_VALUE;
    constexpr int OUT_OF_MEMORY = GLFW_OUT_OF_MEMORY;
    constexpr int API_UNAVAILABLE = GLFW_API_UNAVAILABLE;
    constexpr int VERSION_UNAVAILABLE = GLFW_VERSION_UNAVAILABLE;
    constexpr int PLATFORM_ERROR = GLFW_PLATFORM_ERROR;
    constexpr int FORMAT_UNAVAILABLE = GLFW_FORMAT_UNAVAILABLE;
    constexpr int NO_WINDOW_CONTEXT = GLFW_NO_WINDOW_CONTEXT;
    constexpr int CURSOR_UNAVAILABLE = GLFW_CURSOR_UNAVAILABLE;
    constexpr int FEATURE_UNAVAILABLE = GLFW_FEATURE_UNAVAILABLE;
    constexpr int FEATURE_UNIMPLEMENTED = GLFW_FEATURE_UNIMPLEMENTED;
    constexpr int PLATFORM_UNAVAILABLE = GLFW_PLATFORM_UNAVAILABLE;
    constexpr int FOCUSED = GLFW_FOCUSED;
    constexpr int ICONIFIED = GLFW_ICONIFIED;
    constexpr int RESIZABLE = GLFW_RESIZABLE;
    constexpr int VISIBLE = GLFW_VISIBLE;
    constexpr int DECORATED = GLFW_DECORATED;
    constexpr int AUTO_ICONIFY = GLFW_AUTO_ICONIFY;
    constexpr int FLOATING = GLFW_FLOATING;
    constexpr int MAXIMIZED = GLFW_MAXIMIZED;
    constexpr int CENTER_CURSOR = GLFW_CENTER_CURSOR;
    constexpr int TRANSPARENT_FRAMEBUFFER = GLFW_TRANSPARENT_FRAMEBUFFER;
    constexpr int HOVERED = GLFW_HOVERED;
    constexpr int FOCUS_ON_SHOW = GLFW_FOCUS_ON_SHOW;
    constexpr int MOUSE_PASSTHROUGH = GLFW_MOUSE_PASSTHROUGH;
    constexpr int POSITION_X = GLFW_POSITION_X;
    constexpr int POSITION_Y = GLFW_POSITION_Y;
    constexpr int RED_BITS = GLFW_RED_BITS;
    constexpr int GREEN_BITS = GLFW_GREEN_BITS;
    constexpr int BLUE_BITS = GLFW_BLUE_BITS;
    constexpr int ALPHA_BITS = GLFW_ALPHA_BITS;
    constexpr int DEPTH_BITS = GLFW_DEPTH_BITS;
    constexpr int STENCIL_BITS = GLFW_STENCIL_BITS;
    constexpr int ACCUM_RED_BITS = GLFW_ACCUM_RED_BITS;
    constexpr int ACCUM_GREEN_BITS = GLFW_ACCUM_GREEN_BITS;
    constexpr int ACCUM_BLUE_BITS = GLFW_ACCUM_BLUE_BITS;
    constexpr int ACCUM_ALPHA_BITS = GLFW_ACCUM_ALPHA_BITS;
    constexpr int AUX_BUFFERS = GLFW_AUX_BUFFERS;
    constexpr int STEREO = GLFW_STEREO;
    constexpr int SAMPLES = GLFW_SAMPLES;
    constexpr int SRGB_CAPABLE = GLFW_SRGB_CAPABLE;
    constexpr int REFRESH_RATE = GLFW_REFRESH_RATE;
    constexpr int DOUBLEBUFFER = GLFW_DOUBLEBUFFER;
    constexpr int CLIENT_API = GLFW_CLIENT_API;
    constexpr int CONTEXT_VERSION_MAJOR = GLFW_CONTEXT_VERSION_MAJOR;
    constexpr int CONTEXT_VERSION_MINOR = GLFW_CONTEXT_VERSION_MINOR;
    constexpr int CONTEXT_REVISION = GLFW_CONTEXT_REVISION;
    constexpr int CONTEXT_ROBUSTNESS = GLFW_CONTEXT_ROBUSTNESS;
    constexpr int OPENGL_FORWARD_COMPAT = GLFW_OPENGL_FORWARD_COMPAT;
    constexpr int CONTEXT_DEBUG = GLFW_CONTEXT_DEBUG;
    constexpr int OPENGL_DEBUG_CONTEXT = GLFW_OPENGL_DEBUG_CONTEXT;
    constexpr int OPENGL_PROFILE = GLFW_OPENGL_PROFILE;
    constexpr int CONTEXT_RELEASE_BEHAVIOR = GLFW_CONTEXT_RELEASE_BEHAVIOR;
    constexpr int CONTEXT_NO_ERROR = GLFW_CONTEXT_NO_ERROR;
    constexpr int CONTEXT_CREATION_API = GLFW_CONTEXT_CREATION_API;
    constexpr int SCALE_TO_MONITOR = GLFW_SCALE_TO_MONITOR;
    constexpr int SCALE_FRAMEBUFFER = GLFW_SCALE_FRAMEBUFFER;
    constexpr int COCOA_RETINA_FRAMEBUFFER = GLFW_COCOA_RETINA_FRAMEBUFFER;
    constexpr int COCOA_FRAME_NAME = GLFW_COCOA_FRAME_NAME;
    constexpr int COCOA_GRAPHICS_SWITCHING = GLFW_COCOA_GRAPHICS_SWITCHING;
    constexpr int X11_CLASS_NAME = GLFW_X11_CLASS_NAME;
    constexpr int X11_INSTANCE_NAME = GLFW_X11_INSTANCE_NAME;
    constexpr int WIN32_KEYBOARD_MENU = GLFW_WIN32_KEYBOARD_MENU;
    constexpr int WIN32_SHOWDEFAULT = GLFW_WIN32_SHOWDEFAULT;
    constexpr int WAYLAND_APP_ID = GLFW_WAYLAND_APP_ID;


    constexpr int NO_API = GLFW_NO_API;
    constexpr int OPENGL_API = GLFW_OPENGL_API;
    constexpr int OPENGL_ES_API = GLFW_OPENGL_ES_API;

    constexpr int NO_ROBUSTNESS = GLFW_NO_ROBUSTNESS;
    constexpr int NO_RESET_NOTIFICATION = GLFW_NO_RESET_NOTIFICATION;
    constexpr int LOSE_CONTEXT_ON_RESET = GLFW_LOSE_CONTEXT_ON_RESET;

    constexpr int OPENGL_ANY_PROFILE = GLFW_OPENGL_ANY_PROFILE;
    constexpr int OPENGL_CORE_PROFILE = GLFW_OPENGL_CORE_PROFILE;
    constexpr int OPENGL_COMPAT_PROFILE = GLFW_OPENGL_COMPAT_PROFILE;

    constexpr int CURSOR = GLFW_CURSOR;
    constexpr int STICKY_KEYS = GLFW_STICKY_KEYS;
    constexpr int STICKY_MOUSE_BUTTONS = GLFW_STICKY_MOUSE_BUTTONS;
    constexpr int LOCK_KEY_MODS = GLFW_LOCK_KEY_MODS;
    constexpr int RAW_MOUSE_MOTION = GLFW_RAW_MOUSE_MOTION;

    constexpr int CURSOR_NORMAL = GLFW_CURSOR_NORMAL;
    constexpr int CURSOR_HIDDEN = GLFW_CURSOR_HIDDEN;
    constexpr int CURSOR_DISABLED = GLFW_CURSOR_DISABLED;
    constexpr int CURSOR_CAPTURED = GLFW_CURSOR_CAPTURED;

    constexpr int ANY_RELEASE_BEHAVIOR = GLFW_ANY_RELEASE_BEHAVIOR;
    constexpr int RELEASE_BEHAVIOR_FLUSH = GLFW_RELEASE_BEHAVIOR_FLUSH;
    constexpr int RELEASE_BEHAVIOR_NONE = GLFW_RELEASE_BEHAVIOR_NONE;

    constexpr int NATIVE_CONTEXT_API = GLFW_NATIVE_CONTEXT_API;
    constexpr int EGL_CONTEXT_API = GLFW_EGL_CONTEXT_API;
    constexpr int OSMESA_CONTEXT_API = GLFW_OSMESA_CONTEXT_API;
    constexpr int ANGLE_PLATFORM_TYPE_NONE = GLFW_ANGLE_PLATFORM_TYPE_NONE;
    constexpr int ANGLE_PLATFORM_TYPE_OPENGL = GLFW_ANGLE_PLATFORM_TYPE_OPENGL;
    constexpr int ANGLE_PLATFORM_TYPE_OPENGLES = GLFW_ANGLE_PLATFORM_TYPE_OPENGLES;
    constexpr int ANGLE_PLATFORM_TYPE_D3D9 = GLFW_ANGLE_PLATFORM_TYPE_D3D9;
    constexpr int ANGLE_PLATFORM_TYPE_D3D11 = GLFW_ANGLE_PLATFORM_TYPE_D3D11;
    constexpr int ANGLE_PLATFORM_TYPE_VULKAN = GLFW_ANGLE_PLATFORM_TYPE_VULKAN;
    constexpr int ANGLE_PLATFORM_TYPE_METAL = GLFW_ANGLE_PLATFORM_TYPE_METAL;
    constexpr int WAYLAND_PREFER_LIBDECOR = GLFW_WAYLAND_PREFER_LIBDECOR;
    constexpr int WAYLAND_DISABLE_LIBDECOR = GLFW_WAYLAND_DISABLE_LIBDECOR;

    constexpr int ANY_POSITION = GLFW_ANY_POSITION;
    constexpr int ARROW_CURSOR = GLFW_ARROW_CURSOR;
    constexpr int IBEAM_CURSOR = GLFW_IBEAM_CURSOR;
    constexpr int CROSSHAIR_CURSOR = GLFW_CROSSHAIR_CURSOR;
    constexpr int POINTING_HAND_CURSOR = GLFW_POINTING_HAND_CURSOR;
    constexpr int RESIZE_EW_CURSOR = GLFW_RESIZE_EW_CURSOR;
    constexpr int RESIZE_NS_CURSOR = GLFW_RESIZE_NS_CURSOR;
    constexpr int RESIZE_NWSE_CURSOR = GLFW_RESIZE_NWSE_CURSOR;
    constexpr int RESIZE_NESW_CURSOR = GLFW_RESIZE_NESW_CURSOR;
    constexpr int RESIZE_ALL_CURSOR = GLFW_RESIZE_ALL_CURSOR;
    constexpr int NOT_ALLOWED_CURSOR = GLFW_NOT_ALLOWED_CURSOR;
    constexpr int HRESIZE_CURSOR = GLFW_HRESIZE_CURSOR;
    constexpr int VRESIZE_CURSOR = GLFW_VRESIZE_CURSOR;
    constexpr int HAND_CURSOR = GLFW_HAND_CURSOR;

    constexpr int CONNECTED = GLFW_CONNECTED;
    constexpr int DISCONNECTED = GLFW_DISCONNECTED;

    constexpr int JOYSTICK_HAT_BUTTONS = GLFW_JOYSTICK_HAT_BUTTONS;
    constexpr int ANGLE_PLATFORM_TYPE = GLFW_ANGLE_PLATFORM_TYPE;
    constexpr int PLATFORM = GLFW_PLATFORM;
    constexpr int COCOA_CHDIR_RESOURCES = GLFW_COCOA_CHDIR_RESOURCES;
    constexpr int COCOA_MENUBAR = GLFW_COCOA_MENUBAR;
    constexpr int X11_XCB_VULKAN_SURFACE = GLFW_X11_XCB_VULKAN_SURFACE;
    constexpr int WAYLAND_LIBDECOR = GLFW_WAYLAND_LIBDECOR;

    constexpr int ANY_PLATFORM = GLFW_ANY_PLATFORM;
    constexpr int PLATFORM_WIN32 = GLFW_PLATFORM_WIN32;
    constexpr int PLATFORM_COCOA = GLFW_PLATFORM_COCOA;
    constexpr int PLATFORM_WAYLAND = GLFW_PLATFORM_WAYLAND;
    constexpr int PLATFORM_X11 = GLFW_PLATFORM_X11;
    constexpr int PLATFORM_NULL = GLFW_PLATFORM_NULL;

    constexpr int DONT_CARE = GLFW_DONT_CARE;

    // Function pointers start with Fun
    using FunGlProc  = GLFWglproc;
    using FunVkProc = GLFWvkproc;
    using Monitor = GLFWmonitor;
    using Window = GLFWwindow;
    using Cursor = GLFWcursor;
    using FunAllocate = GLFWallocatefun;
    using FunReallocate = GLFWreallocatefun;
    using FunDeallocate = GLFWdeallocatefun;
    using FunError = GLFWerrorfun;
    using FunWindowPos = GLFWwindowposfun;
    using FunWindowSize = GLFWwindowsizefun;
    using FunWindowClose = GLFWwindowclosefun;
    using FunWindowRefresh = GLFWwindowrefreshfun;
    using FunWindowFocus = GLFWwindowfocusfun;
    using FunWindowIconify = GLFWwindowiconifyfun;
    using FunWindowMaximize = GLFWwindowmaximizefun;
    using FunFramebufferSize = GLFWframebuffersizefun;
    using FunWindowContentScale = GLFWwindowcontentscalefun;
    using FunMouseButton = GLFWmousebuttonfun;
    using FunCursorPos = GLFWcursorposfun;
    using FunCursorEnter = GLFWcursorenterfun;
    using FunScroll = GLFWscrollfun;
    using FunKey = GLFWkeyfun;
    using FunChar = GLFWcharfun;
    using FunCharMods = GLFWcharmodsfun;
    using FunDrop = GLFWdropfun;
    using FunMonitor = GLFWmonitorfun;
    using FunJoystick = GLFWjoystickfun;
    using VidMode = GLFWvidmode;
    using GammaRamp = GLFWgammaramp;
    using Image = GLFWimage;
    using GamepadState = GLFWgamepadstate;
    using Allocator = GLFWallocator;


    inline int init() {
        return glfwInit();
    }

    inline void terminate() {
        glfwTerminate();
    }

    inline void init_hint(const int hint, const int value) {
        glfwInitHint(hint, value);
    }

    inline void init_allocator(const GLFWallocator* allocator) {
        glfwInitAllocator(allocator);
    }
    inline void init_vulkan_loader(PFN_vkGetInstanceProcAddr loader) {
        glfwInitVulkanLoader(loader);
    }

    inline void get_version(int *major, int *minor, int *revision) {
        glfwGetVersion(major, minor, revision);
    }

    inline const char* get_version_string() {
        return glfwGetVersionString();
    }

    inline int get_error(const char** description) {
        return glfwGetError(description);
    }

    inline void set_error_callback(const FunError callback) {
        glfwSetErrorCallback(callback);
    }

    inline int get_platform() {
        return glfwGetPlatform();
    }

    inline int get_platform_supported(const int platform) {
        return glfwPlatformSupported(platform);
    }

    inline Monitor** get_monitors(int* count) {
        return glfwGetMonitors(count);
    }

    inline Monitor* get_primary_monitor() {
        return glfwGetPrimaryMonitor();
    }

    inline void get_monitor_pos(Monitor *monitor, int *x, int *y) {
        glfwGetMonitorPos(monitor, x, y);
    }

    inline void get_monitor_workarea(
        Monitor *monitor,
        int *x, int *y,
        int *width, int *height
    ) {
        glfwGetMonitorWorkarea(monitor, x, y, width, height);
    }

    inline void get_monitor_physical_size( Monitor *monitor, int *widthMM, int *heightMM ) {
        glfwGetMonitorPhysicalSize(monitor, widthMM, heightMM);
    }

    inline void get_monitor_content_scale( Monitor *monitor, float *xscale, float *yscale ) {
        glfwGetMonitorContentScale(monitor, xscale, yscale);
    }

    inline const char* get_monitor_name(Monitor *monitor) {
        return glfwGetMonitorName(monitor);
    }

    inline void set_monitor_user_pointer(Monitor *monitor, void *pointer) {
        glfwSetMonitorUserPointer(monitor, pointer);
    }

    inline void* get_monitor_user_pointer(Monitor *monitor) {
        return glfwGetMonitorUserPointer(monitor);
    }

    inline void set_monitor_callback(const FunMonitor callback) {
        glfwSetMonitorCallback(callback);
    }

    inline const VidMode* get_video_modes(Monitor *monitor, int *count) {
        return glfwGetVideoModes(monitor, count);
    }

    inline const VidMode* get_video_mode(Monitor *monitor) {
        return glfwGetVideoMode(monitor);
    }

    inline void set_gamma(Monitor *monitor,const float gamma) {
        glfwSetGamma(monitor, gamma);
    }

    inline const GammaRamp* get_gamma_ramp(Monitor *monitor) {
        return glfwGetGammaRamp(monitor);
    }

    inline void set_gamma_ramp(Monitor *monitor, const GammaRamp *ramp) {
        glfwSetGammaRamp(monitor, ramp);
    }

    inline void default_window_hints() {
        glfwDefaultWindowHints();
    }

    inline void window_hint(const int hint, const int value) {
        glfwWindowHint(hint, value);
    }

    inline void window_hint_string(const int hint, const char *value) {
        glfwWindowHintString(hint, value);
    }

    inline Window* create_window(
        const int width, const int height,
        const char* title,
        Monitor *monitor,
        Window *share
    ) {
        return glfwCreateWindow(width, height, title, monitor, share);;
    }

    inline void destroy_window(Window *window) {
        glfwDestroyWindow(window);
    }

    inline bool window_should_close(Window *window) {
        return glfwWindowShouldClose(window);
    }

    inline void set_window_should_close(Window *window, const int value) {
        return glfwSetWindowShouldClose(window, value);
    }

    inline const char* get_window_title(Window *window) {
        return glfwGetWindowTitle(window);
    }

    inline void set_window_title(Window *window, const char *title) {
        glfwSetWindowTitle(window, title);
    }

    inline void set_window_icon(Window *window, const int count, const Image *images) {
        glfwSetWindowIcon(window, count, images);
    }

    inline void get_window_pos(Window *window, int *x, int *y) {
        glfwGetWindowPos(window, x, y);
    }

    inline void set_window_pos(Window *window, const int x, const int y) {
        glfwSetWindowPos(window, x, y);
    }

    inline void get_window_size(Window *window, int *width, int *height) {
        glfwGetWindowSize(window, width, height);
    }

    inline void set_window_size_limits(
        Window *window,
        const int minWidth, const int minHeight,
        const int maxWidth, const int maxHeight
    ) {
        glfwSetWindowSizeLimits(window, minWidth, minHeight, maxWidth, maxHeight);
    }

    inline void set_window_aspect_ratio(Window *window, const int numer, const int denom) {
        glfwSetWindowAspectRatio(window, numer, denom);
    }

    inline void set_window_size(Window *window, const int width, const int height) {
        glfwSetWindowSize(window, width, height);
    }

    inline void get_framebuffer_size(Window *window, int *width, int *height) {
        glfwGetFramebufferSize(window, width, height);
    }

    inline void get_window_frame_size(Window *window, int *left, int *top, int *right, int *bottom) {
        glfwGetWindowFrameSize(window, left, top, right, bottom);
    }

    inline void get_window_content_scale(Window *window, float *xscale, float *yscale) {
        glfwGetWindowContentScale(window, xscale, yscale);
    }

    inline float get_window_opacity(Window *window) {
        return glfwGetWindowOpacity(window);
    }

    inline void set_window_opacity(Window *window, const float opacity) {
        glfwSetWindowOpacity(window, opacity);
    }

    inline void inconify_window(Window *window) {
        glfwIconifyWindow(window);
    }

    inline void restore_window(Window *window) {
        glfwRestoreWindow(window);
    }

    inline void maximize_window(Window *window) {
        glfwMaximizeWindow(window);
    }

    inline void show_window(Window *window) {
        glfwShowWindow(window);
    }

    inline void hide_window(Window *window) {
        glfwHideWindow(window);
    }

    inline void focus_window(Window *window) {
        glfwFocusWindow(window);
    }

    inline void request_window_attention(Window *window) {
        glfwRequestWindowAttention(window);
    }

    inline Monitor* get_window_monitor(Window *window) {
        return glfwGetWindowMonitor(window);
    }

    inline void set_window_monitor(
        Window *window,
        Monitor *monitor,
        const int xpos, const int ypos,
        const int width, const int height,
        const int refreshRate
    ) {
        glfwSetWindowMonitor(window, monitor, xpos, ypos, width, height, refreshRate);
    }

    inline int get_window_attrib(Window *window, const int attrib) {
        return glfwGetWindowAttrib(window, attrib);
    }

    inline void set_window_attrib(Window *window, const int attrib, const int value) {
        glfwSetWindowAttrib(window, attrib, value);
    }

    inline void set_window_user_pointer(Window *window, void *pointer) {
        glfwSetWindowUserPointer(window, pointer);
    }

    inline void* get_window_user_pointer(Window *window) {
        return glfwGetWindowUserPointer(window);
    }

    inline FunWindowPos set_window_pos_callback(Window *window, const FunWindowPos callback) {
        return glfwSetWindowPosCallback(window, callback);
    }

    inline FunWindowSize set_window_size_callback(Window *window, const FunWindowSize callback) {
        return glfwSetWindowSizeCallback(window, callback);
    }

    inline FunWindowClose set_window_close_callback(Window *window, const FunWindowClose callback) {
        return glfwSetWindowCloseCallback(window, callback);
    }

    inline FunWindowRefresh set_window_refresh_callback(Window *window, const FunWindowRefresh callback) {
        return glfwSetWindowRefreshCallback(window, callback);
    }

    inline FunWindowFocus set_window_focus_callback(Window *window, const FunWindowFocus callback) {
        return glfwSetWindowFocusCallback(window, callback);
    }

    inline FunWindowIconify set_window_iconify_callback(Window *window, const FunWindowIconify callback) {
        return glfwSetWindowIconifyCallback(window, callback);
    }

    inline FunWindowMaximize set_window_maximize_callback(Window *window, const FunWindowMaximize callback) {
        return glfwSetWindowMaximizeCallback(window, callback);
    }

    inline GLFWframebuffersizefun set_framebuffer_size_callback(Window *window,const FunFramebufferSize callback) {
        return glfwSetFramebufferSizeCallback(window,callback);
    }

    inline FunWindowContentScale set_window_content_scale_callback(Window *window, const FunWindowContentScale callback) {
        return glfwSetWindowContentScaleCallback(window, callback);
    }

    inline void poll_events() {
        glfwPollEvents();
    }

    inline void wait_events() {
        glfwWaitEvents();
    }

    inline void wait_events_timeout(const double timeout) {
        glfwWaitEventsTimeout(timeout);
    }

    inline void post_empty_event() {
        glfwPostEmptyEvent();
    }

    inline int get_input_mode(Window *window, const int mode) {
        return glfwGetInputMode(window, mode);
    }

    inline void set_input_mode(Window *window, const int mode, const int value) {
        glfwSetInputMode(window, mode, value);
    }

    inline int raw_mouse_motion_supported() {
        return glfwRawMouseMotionSupported();
    }

    inline const char* get_key_name(const int key, const int scancode) {
        return glfwGetKeyName(key, scancode);
    }

    inline int get_key_scancode(const int key) {
        return glfwGetKeyScancode(key);
    }

    inline int get_key(Window *window, const int key) {
        return glfwGetKey(window, key);
    }

    inline int get_mouse_button(Window *window, const int button) {
        return glfwGetMouseButton(window, button);
    }

    inline void get_cursor_pos(Window *window, double *xpos, double *ypos) {
        glfwGetCursorPos(window, xpos, ypos);
    }

    inline void set_cursor_pos(Window *window, const double xpos, const double ypos) {
        glfwSetCursorPos(window, xpos, ypos);
    }

    inline Cursor* create_cursor(const Image *image, const int xhot, const int yhot) {
        return glfwCreateCursor(image, xhot, yhot);
    }

    inline Cursor* create_standard_cursor(const int shape) {
        return glfwCreateStandardCursor(shape);
    }

    inline void destroy_cursor(Cursor *cursor) {
        glfwDestroyCursor(cursor);
    }

    inline void set_cursor(Window *window, Cursor *cursor) {
        glfwSetCursor(window, cursor);
    }

    inline FunKey set_key_callback(Window *window, const FunKey callback) {
        return glfwSetKeyCallback(window, callback);
    }

    inline FunChar set_char_callback(Window *window, const FunChar callback) {
        return glfwSetCharCallback(window, callback);
    }

    inline FunCharMods set_char_mods_callback(Window *window, const FunCharMods callback) {
        return glfwSetCharModsCallback(window, callback);
    }

    inline FunMouseButton set_mouse_button_callback(Window *window, const FunMouseButton callback) {
        return glfwSetMouseButtonCallback(window, callback);
    }

    inline FunCursorPos set_cursor_pos_callback(Window *window, const FunCursorPos callback) {
        return glfwSetCursorPosCallback(window, callback);
    }

    inline FunCursorEnter set_cursor_enter_callback(Window *window, const FunCursorEnter callback) {
        return glfwSetCursorEnterCallback(window, callback);
    }

    inline FunScroll set_scroll_callback(Window *window, const FunScroll callback) {
        return glfwSetScrollCallback(window, callback);
    }

    inline FunDrop set_drop_callback(Window *window, const FunDrop callback) {
        return glfwSetDropCallback(window, callback);
    }

    inline int joystick_present(const int jid) {
        return glfwJoystickPresent(jid);
    }

    inline const float* get_joystick_axes(const int jid, int *count) {
        return glfwGetJoystickAxes(jid, count);
    }

    inline const unsigned char* get_joystick_buttons(const int jid, int *count) {
        return glfwGetJoystickButtons(jid, count);
    }

    inline const unsigned char* get_joystick_hats(const int jid, int *count) {
        return glfwGetJoystickHats(jid, count);
    }

    inline const char* get_joystick_name(const int jid) {
        return glfwGetJoystickName(jid);
    }

    inline const char* get_joystick_guid(const int jid) {
        return glfwGetJoystickGUID(jid);
    }

    inline void set_joystick_user_pointer(const int jid, void *pointer) {
        glfwSetJoystickUserPointer(jid, pointer);
    }

    inline void* get_joystick_user_pointer(const int jid) {
        return glfwGetJoystickUserPointer(jid);
    }

    inline int joystick_is_gamepad(const int jid) {
        return glfwJoystickIsGamepad(jid);
    }

    inline FunJoystick set_joystick_callback(const FunJoystick callback) {
        return glfwSetJoystickCallback(callback);
    }

    inline int update_gamepad_mappings(const char *string) {
        return glfwUpdateGamepadMappings(string);
    }

    inline const char* get_gamepad_name(const int jid) {
        return glfwGetGamepadName(jid);
    }

    inline int get_gamepad_state(const int jid, GamepadState *state) {
        return glfwGetGamepadState(jid, state);
    }

    inline void set_clipboard_string(Window *window, const char *string) {
        glfwSetClipboardString(window, string);
    }

    inline const char* get_clipboard_string(Window *window) {
        return glfwGetClipboardString(window);
    }

    inline double get_time() {
        return glfwGetTime();
    }

    inline void set_time(const double time) {
        glfwSetTime(time);
    }

    inline uint64_t get_timer_value() {
        return glfwGetTimerValue();
    }

    inline uint64_t get_timer_frequency() {
        return glfwGetTimerFrequency();
    }

    inline void make_context_current(Window *window) {
        glfwMakeContextCurrent(window);
    }

    inline Window* get_current_context() {
        return glfwGetCurrentContext();
    }

    inline void swap_buffers(Window *window) {
        glfwSwapBuffers(window);
    }

    inline void swap_interval(const int interval) {
        glfwSwapInterval(interval);
    }

    inline int extension_supported(const char *extension) {
        return glfwExtensionSupported(extension);
    }

    inline FunGlProc get_proc_address(const char *procname) {
        return glfwGetProcAddress(procname);
    }

    inline int vulkan_supported() {
        return glfwVulkanSupported();
    }

    inline const char** get_required_instance_extensions(uint32_t *count) {
        return glfwGetRequiredInstanceExtensions(count);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    /// Vulkan related content, dependent on Vulkan header files
    inline FunVkProc get_instance_proc_address( VkInstance instance, const char *procname ) {
        return glfwGetInstanceProcAddress(instance, procname);
    }

    inline int get_physical_device_presentation_support(
        VkInstance instance,
        VkPhysicalDevice physicalDevice,
        const uint32_t queueFamilyIndex
    ) {
        return glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, queueFamilyIndex);
    }

    inline VkResult create_window_surface(
        VkInstance instance,
        Window *window,
        const VkAllocationCallbacks *allocator,
        ::VkSurfaceKHR *surface
    ) {
        return glfwCreateWindowSurface(instance, window, allocator, surface);
    }

    using VkSurfaceKHR = ::VkSurfaceKHR;
    /////////////////////////////////////////////////////////////////////////////////////////////
}

