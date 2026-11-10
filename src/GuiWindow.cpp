#include "GuiWindow.hpp"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/Fonts.hpp"
//#include <implot.h>
#include <iostream>
#include <limits>
//#include <mutex>


// global variables
//static std::mutex g_mutex;
static int g_count;
static GLFWcursor* g_MouseCursors[ImGuiMouseCursor_COUNT] = {};


// static functions

static void errorCallback(int error, const char* description) {
    //fprintf(stderr, "Error: %s\n", description);
    std::cerr << "Error: " << description << std::endl;
}

static const char *getClipboardText(void *user_data) {
    return glfwGetClipboardString((GLFWwindow *)user_data);
}

static void setClipboardText(void *user_data, const char* text) {
    glfwSetClipboardString((GLFWwindow *)user_data, text);
}

// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_glfw.cpp
ImGuiKey glfw2ImGuiKey(int keycode, int modifiers) {
    bool numLock = (modifiers & GLFW_MOD_NUM_LOCK) != 0;
    switch (keycode) {
    case GLFW_KEY_TAB: return ImGuiKey_Tab;
    case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
    case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
    case GLFW_KEY_UP: return ImGuiKey_UpArrow;
    case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
    case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
    case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
    case GLFW_KEY_HOME: return ImGuiKey_Home;
    case GLFW_KEY_END: return ImGuiKey_End;
    case GLFW_KEY_INSERT: return ImGuiKey_Insert;
    case GLFW_KEY_DELETE: return ImGuiKey_Delete;
    case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
    case GLFW_KEY_SPACE: return ImGuiKey_Space;
    case GLFW_KEY_ENTER: return ImGuiKey_Enter;
    case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
    case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
    case GLFW_KEY_COMMA: return ImGuiKey_Comma;
    case GLFW_KEY_MINUS: return ImGuiKey_Minus;
    case GLFW_KEY_PERIOD: return ImGuiKey_Period;
    case GLFW_KEY_SLASH: return ImGuiKey_Slash;
    case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
    case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
    case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
    case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
    case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
    case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
    case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
    case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
    case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
    case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
    case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
    case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
    case GLFW_KEY_KP_1: return numLock ? ImGuiKey_Keypad1 : ImGuiKey_End;
    case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
    case GLFW_KEY_KP_3: return numLock ? ImGuiKey_Keypad3 : ImGuiKey_PageDown;
    case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
    case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
    case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
    case GLFW_KEY_KP_7: return numLock ? ImGuiKey_Keypad7 : ImGuiKey_Home;
    case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
    case GLFW_KEY_KP_9: return numLock ? ImGuiKey_Keypad9 : ImGuiKey_PageUp;
    case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
    case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
    case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
    case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
    case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
    case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
    case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
    case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
    case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
    case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
    case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
    case GLFW_KEY_MENU: return ImGuiKey_Menu;
    case GLFW_KEY_0: return ImGuiKey_0;
    case GLFW_KEY_1: return ImGuiKey_1;
    case GLFW_KEY_2: return ImGuiKey_2;
    case GLFW_KEY_3: return ImGuiKey_3;
    case GLFW_KEY_4: return ImGuiKey_4;
    case GLFW_KEY_5: return ImGuiKey_5;
    case GLFW_KEY_6: return ImGuiKey_6;
    case GLFW_KEY_7: return ImGuiKey_7;
    case GLFW_KEY_8: return ImGuiKey_8;
    case GLFW_KEY_9: return ImGuiKey_9;
    case GLFW_KEY_A: return ImGuiKey_A;
    case GLFW_KEY_B: return ImGuiKey_B;
    case GLFW_KEY_C: return ImGuiKey_C;
    case GLFW_KEY_D: return ImGuiKey_D;
    case GLFW_KEY_E: return ImGuiKey_E;
    case GLFW_KEY_F: return ImGuiKey_F;
    case GLFW_KEY_G: return ImGuiKey_G;
    case GLFW_KEY_H: return ImGuiKey_H;
    case GLFW_KEY_I: return ImGuiKey_I;
    case GLFW_KEY_J: return ImGuiKey_J;
    case GLFW_KEY_K: return ImGuiKey_K;
    case GLFW_KEY_L: return ImGuiKey_L;
    case GLFW_KEY_M: return ImGuiKey_M;
    case GLFW_KEY_N: return ImGuiKey_N;
    case GLFW_KEY_O: return ImGuiKey_O;
    case GLFW_KEY_P: return ImGuiKey_P;
    case GLFW_KEY_Q: return ImGuiKey_Q;
    case GLFW_KEY_R: return ImGuiKey_R;
    case GLFW_KEY_S: return ImGuiKey_S;
    case GLFW_KEY_T: return ImGuiKey_T;
    case GLFW_KEY_U: return ImGuiKey_U;
    case GLFW_KEY_V: return ImGuiKey_V;
    case GLFW_KEY_W: return ImGuiKey_W;
    case GLFW_KEY_X: return ImGuiKey_X;
    case GLFW_KEY_Y: return ImGuiKey_Y;
    case GLFW_KEY_Z: return ImGuiKey_Z;
    case GLFW_KEY_F1: return ImGuiKey_F1;
    case GLFW_KEY_F2: return ImGuiKey_F2;
    case GLFW_KEY_F3: return ImGuiKey_F3;
    case GLFW_KEY_F4: return ImGuiKey_F4;
    case GLFW_KEY_F5: return ImGuiKey_F5;
    case GLFW_KEY_F6: return ImGuiKey_F6;
    case GLFW_KEY_F7: return ImGuiKey_F7;
    case GLFW_KEY_F8: return ImGuiKey_F8;
    case GLFW_KEY_F9: return ImGuiKey_F9;
    case GLFW_KEY_F10: return ImGuiKey_F10;
    case GLFW_KEY_F11: return ImGuiKey_F11;
    case GLFW_KEY_F12: return ImGuiKey_F12;
    case GLFW_KEY_F13: return ImGuiKey_F13;
    case GLFW_KEY_F14: return ImGuiKey_F14;
    case GLFW_KEY_F15: return ImGuiKey_F15;
    case GLFW_KEY_F16: return ImGuiKey_F16;
    case GLFW_KEY_F17: return ImGuiKey_F17;
    case GLFW_KEY_F18: return ImGuiKey_F18;
    case GLFW_KEY_F19: return ImGuiKey_F19;
    case GLFW_KEY_F20: return ImGuiKey_F20;
    case GLFW_KEY_F21: return ImGuiKey_F21;
    case GLFW_KEY_F22: return ImGuiKey_F22;
    case GLFW_KEY_F23: return ImGuiKey_F23;
    case GLFW_KEY_F24: return ImGuiKey_F24;
    default: return ImGuiKey_None;
    }
}

// gets called by glfw
void GuiWindow::keyCallback(GLFWwindow* w, int keycode, int scancode, int action, int modifiers) {
    GuiWindow *window = (GuiWindow *)glfwGetWindowUserPointer(w);
    ImGuiIO &io = ImGui::GetIO();

    // ImGui_ImplGlfw_KeyCallback in https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_glfw.cpp
    ImGuiKey key = glfw2ImGuiKey(keycode, modifiers);

    // call user code
    if (!window->onKey(key, scancode, action, modifiers, io.WantCaptureKeyboard)) {
        // handle ImGui
        io.AddKeyEvent(key, (action != GLFW_RELEASE)); // down on GLFW_PRESS or GLFW_REPEAT

        // ImGui_ImplGlfw_UpdateKeyModifiers in https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_glfw.cpp
        io.AddKeyEvent(ImGuiMod_Shift, (modifiers & GLFW_MOD_SHIFT) != 0);
        io.AddKeyEvent(ImGuiMod_Ctrl, (modifiers & GLFW_MOD_CONTROL) != 0);
        io.AddKeyEvent(ImGuiMod_Alt, (modifiers & GLFW_MOD_ALT) != 0);
        io.AddKeyEvent(ImGuiMod_Super, (modifiers & GLFW_MOD_SUPER) != 0);
    }
}

// gets called by glfw
void GuiWindow::charCallback(GLFWwindow* w, unsigned int c) {
    GuiWindow *window = (GuiWindow *)glfwGetWindowUserPointer(w);

    // call user code
    if (!window->onChar(c)) {
        // handle ImGui
        ImGuiIO &io = ImGui::GetIO();
        io.AddInputCharacter(c);
    }
}

// gets called by glfw
void GuiWindow::mouseCallback(GLFWwindow* w, int button, int action, int mods) {
    GuiWindow *window = (GuiWindow *)glfwGetWindowUserPointer(w);

    // call user code
    if (!window->onMouse(button, action, mods)) {
        // handle ImGui
        if (action == GLFW_PRESS && button >= 0 && button < std::size(window->mouseJustPressed))
            window->mouseJustPressed[button] = true;
    }
}

// gets called by glfw
void GuiWindow::scrollCallback(GLFWwindow* w, double xoffset, double yoffset) {
    GuiWindow *window = (GuiWindow *)glfwGetWindowUserPointer(w);

    // call user code
    if (!window->onScroll(float(xoffset), float(yoffset))) {
        // handle ImGui
        ImGuiIO &io = ImGui::GetIO();
        io.MouseWheelH += float(xoffset);
        io.MouseWheel += float(yoffset);
    }
}

// GuiWindow

GuiWindow::GuiWindow(int width, int height, char const *title, bool visible)
    : mouseJustPressed{}
{
    //std::lock_guard<std::mutex> lock(g_mutex);

    // global init for GLFW
    if (g_count == 0) {
        if (!glfwInit())
            exit(EXIT_FAILURE);

        // Create mouse cursors
        // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
        // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
        // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
        g_MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
        g_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
        g_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif

        glfwSetErrorCallback(errorCallback);
    }

    // determine content scale (scale between window and framebuffer coordinates)
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    float xScale, yScale;
    glfwGetMonitorContentScale(monitor, &xScale, &yScale);
    this->scale = xScale;

    // scale window size on linux, is done automatically on mac
#ifdef __linux__
    width = int(width * xScale);
    height = int(height * yScale);
#endif

    // create GLFW window and OpenGL 3.3 Core context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_VISIBLE, visible);
    this->window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!this->window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetWindowUserPointer(this->window, this);

    // set callbacks
    glfwSetKeyCallback(this->window, keyCallback);
    glfwSetCharCallback(this->window, charCallback);
    glfwSetMouseButtonCallback(this->window, mouseCallback);
    glfwSetScrollCallback(this->window, scrollCallback);

    // make OpenGL context current
    glfwMakeContextCurrent(this->window);

    // load OpenGL functions
    if (g_count == 0)
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // v-sync
    glfwSwapInterval(1);
/*
    // determine scale between window and framebuffer coordinates which is > 1 on high-dpi displays
    // get window size in screen coordinates, scaled on high-dpi (retina) displays
    int wWidth, wHeight;
    glfwGetWindowSize(this->window, &wWidth, &wHeight);

    // get frame buffer size in pixels, real resolution on high-dpi displays
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(this->window, &fbWidth, &fbHeight);

    this->scale = xScale;//float(fbWidth) / wWidth * (linuxhdpi ? 2 : 1);
*/
    // global init for ImGui, requires OpenGL context
    if (g_count == 0) {
        // setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        // setup font, has to be done before ImGui_ImplOpenGL3_Init()
        const float fontSize = 14 * this->scale;
        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(FONT_ROBOTO_REGULAR, sizeof(FONT_ROBOTO_REGULAR), fontSize, &fontConfig);
        //io.Fonts->AddFontFromFileTTF("Roboto-Regular.ttf", 26);

        // setup OpenGL backend, needs ImGui context
        ImGui_ImplOpenGL3_Init();

        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // setup ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // setup platform/renderer backends
        //ImGui_ImplGlfw_InitForOpenGL(this->window, true);
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendPlatformName = "GLFW+OpenGL3.3-Core";

        io.SetClipboardTextFn = setClipboardText;
        io.GetClipboardTextFn = getClipboardText;
        io.ClipboardUserData = this->window;

        //ImPlot::CreateContext();
    }

    this->time = glfwGetTime();

    ++g_count;
}

GuiWindow::~GuiWindow() {
    //std::lock_guard<std::mutex> lock(g_mutex);
    --g_count;

    // global terminate for ImGui, requires OpenGL contex
    if (g_count == 0) {
        //ImPlot::DestroyContext();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }

    glfwDestroyWindow(window);

    // global terminate for GLFW
    if (g_count == 0) {
        glfwTerminate();
    }
}

Size<int> GuiWindow::getSize() {
    int width, height;
    glfwGetWindowSize(this->window, &width, &height);
    width = int(width / this->scale);
    height = int(height / this->scale);
    return {width, height};
}

void GuiWindow::draw() {
    // make OpenGL context current
    glfwMakeContextCurrent(this->window);

    // get window size in screen coordinates, scaled on high-dpi (retina) displays
    int width, height;
    glfwGetWindowSize(this->window, &width, &height);

    // get frame buffer size in pixels, real resolution on high-dpi displays
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(this->window, &fbWidth, &fbHeight);

    // set viewport
    glViewport(0, 0, fbWidth, fbHeight);

    ImGui_ImplOpenGL3_NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    //IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2(float(width), float(height));
    if (width > 0 && height > 0) {
        io.DisplayFramebufferScale = ImVec2(float(fbWidth) / width, float(fbHeight) / height);

        // counteract upscaling of font on high-dpi displays
        io.FontGlobalScale = float(width) / fbWidth;
    }

    // Setup time step
    double currentTime = glfwGetTime();
    io.DeltaTime = float(currentTime - this->time);
    this->time = currentTime;

    // Update buttons
    int buttons = 0;
    for (int i = 0; i < std::size(io.MouseDown); i++) {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        bool down = this->mouseJustPressed[i] || glfwGetMouseButton(this->window, i) != 0;
        io.MouseDown[i] = down;
        this->mouseJustPressed[i] = false;
        if (down)
            buttons |= 1 << i;
    }

    // get focused state
    const bool focused = glfwGetWindowAttrib(this->window, GLFW_FOCUSED) != 0;

    // update mouse position
    if (focused) {
        if (io.WantSetMousePos) {
            // set mouse position
            glfwSetCursorPos(this->window, (double)io.MousePos.x, (double)io.MousePos.y);
        } else {
            // get mouse position
            double mouse_x, mouse_y;
            glfwGetCursorPos(this->window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    } else {
        // not focused: set mouse position to nan
        io.MousePos = ImVec2(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
    }

    // update mouse cursor
    if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) && glfwGetInputMode(this->window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
        ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        } else {
            // Show OS mouse cursor
            // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
            glfwSetCursor(this->window, g_MouseCursors[imgui_cursor] ? g_MouseCursors[imgui_cursor]
                                                                     : g_MouseCursors[ImGuiMouseCursor_Arrow]);
            glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    // draw
    ImGui::NewFrame();

    State state;
    state.windowSize.width = width;
    state.windowSize.height = height;
    state.framebufferSize.width = float(fbWidth);
    state.framebufferSize.height = float(fbHeight);
    state.modifiers = 0;
    if (io.KeyCtrl)
        state.modifiers |= GLFW_MOD_CONTROL;
    if (io.KeyShift)
        state.modifiers |= GLFW_MOD_SHIFT;
    if (io.KeyAlt)
        state.modifiers |= GLFW_MOD_ALT;
    if (io.KeySuper)
        state.modifiers |= GLFW_MOD_SUPER;
    state.mouseButtons = buttons;
    state.mouseX = io.MousePos.x;
    state.mouseY = io.MousePos.y;
    state.mouseOverGui = io.WantCaptureMouse;
    state.scrollX = io.MouseWheelH;
    state.scrollY = io.MouseWheel;
    state.timeStep = io.DeltaTime;

    onDraw(state);

    // swap render buffer to screen
    glfwSwapBuffers(window);
}

bool GuiWindow::onKey(ImGuiKey key, int scancode, int action, int mods, bool neededByGui) {
    return false;
}

bool GuiWindow::onChar(unsigned int c) {
    return false;
}

bool GuiWindow::onMouse(int button, int action, int mods) {
    return false;
}

bool GuiWindow::onScroll(float dx, float dy) {
    return false;
}

void GuiWindow::onDraw(State const &state) {
}

void GuiWindow::drawGui() {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), [](char const *){return true;});
}

void GuiWindow::drawGui(std::function<bool (char const *)> filter) {
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData(), filter);
}
