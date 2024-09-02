// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <sstream>
#include <functional>
#include "Transform.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include "RenderSystem.h"
#include "UISystem.h"
#include "Hierarchy.h"
#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h>
#endif // WINDOWS

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif
#include <iostream>

static bool WindowReSize = false;

enum LogLevel {
    OFF,
    ERR,
    WARN,
    INFO,
    DEBUG,
    ALL,
};

enum FileType {
    File,
    Directory
};

struct FileCell {
    FileType type;
    std::string fileName;
};

void splitStr(const std::string& str,char delimiter,std::vector<std::string>& tokens) {
    std::istringstream stream(str);
    std::string token;
    while(std::getline(stream,token,delimiter)){
        tokens.push_back(token);
    }
}

void defaultLayout(ImGuiID& dockspace_id ,float w,float h,float posX=0,float posY=0){
    dockspace_id = ImGui::DockBuilderAddNode(NULL, ImGuiWindowFlags_NoCollapse);
    ImGui::DockBuilderSetNodeSize(dockspace_id, { w,h});
    ImGui::DockBuilderSetNodePos(dockspace_id, { posX,posY});
    ImGuiID dock_main_id = dockspace_id;
    ImGuiID dock_id_down = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);
    ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
    ImGuiID dock_id_center= ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.70f, nullptr, &dock_main_id);
    ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.30f, nullptr, &dock_main_id);
    ImGui::DockBuilderDockWindow("Hierarchy",dock_id_left);
    ImGui::DockBuilderDockWindow("View", dock_id_center);
    ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
    ImGui::DockBuilderDockWindow("Console", dock_id_down);
    ImGui::DockBuilderDockWindow("Logcat", dock_id_down);
    ImGui::DockBuilderDockWindow("Project", dock_id_down);
    ImGui::DockBuilderFinish(dockspace_id);
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void onWindowSizeChange(GLFWwindow* window,int w,int h) {
    WindowReSize = true;
}

struct Log{
    LogLevel level;
    std::string msg;
};

ImVec4 GetLogLevelColor(LogLevel level) {
    if (level == ERROR){
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // 红色
    } else if (level == WARN) {
        return ImVec4(1.0f, 0.647f, 0.0f, 1.0f); // 橙色
    } else if (level == INFO) {
        return ImVec4(0.0f, 1.0f, 1.0f, 1.0f); // 青色
    } else if (level == DEBUG) {
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 绿色
    } else if (level == ALL) {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
    } else {
        return ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // 默认黑色
    }
}


std::vector<Log> logBuffer;
const size_t maxLogLines = 100;

void AddLog(LogLevel level,const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    logBuffer.push_back({level,buf});
    if (logBuffer.size() > maxLogLines)
        logBuffer.erase(logBuffer.begin());
}

std::string selectFlodle() {
#ifdef _WIN32
    BROWSEINFO bi = {0};
    bi.ulFlags = BIF_USENEWUI | BIF_NEWDIALOGSTYLE | BIF_BROWSEINCLUDEURLS;
    bi.hwndOwner = NULL;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL) {
        char path[1024] = { 0 };
        if (SHGetPathFromIDList(pidl, path)) {
            return std::string(path);
        }
        CoTaskMemFree(pidl);
    }
#endif // _WIN32
    return "";
}

void getDirectorys(const std::string& path,std::vector<FileCell>& fileList) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((path + "\\*").c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    fileList.clear();
    do {
        std::string str(findFileData.cFileName);
        if (str != "." ){
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                fileList.push_back({ Directory,findFileData.cFileName });
            }
            else {
                fileList.push_back({ File,findFileData.cFileName });
            }
        }
    } while (FindNextFile(hFind, &findFileData) != NULL);
}

unsigned int loadTexture(const char* path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (data == NULL) {
        std::cerr << "Error: Load Texture:" << path << std::endl;
        return -1;
    }
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return textureId;
}

bool clickInWindow() {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        // 条件过长不处理等于
        if (mousePos.x < (windowPos.x + windowSize.x)&&
            mousePos.x>windowPos.x&&
            mousePos.y>windowPos.y&&
            mousePos.y<(windowPos.y+windowSize.y)) {
            return true;
        }
    }
    return false;
}


FakeEngine::RenderSystem renderSystem;
FakeEngine::UISystem uiSystem;

void test(FakeEngine::EntityID id) {
    //std::vector<FakeEngine::EntityID> idList = uiSystem.HierarchyUpdate();
    //for (FakeEngine::EntityID id : idList) {
    FakeEngine::Hierarchy* hierarchy = FakeEngine::ComponentManager::GetInstance().GetComponent<FakeEngine::Hierarchy>(id);
    if (hierarchy->parent == NULL) {
        ImGui::TreeNode(hierarchy->name.c_str());
    }
    else {
        test(hierarchy->parent);
    }
    //}
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "FakeEngine", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    glfwSetFramebufferSizeCallback(window,onWindowSizeChange);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    float menubarHeight = 0;
    bool showLogcat = false;
    bool wireframe = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // init glad
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);

    ImGuiID dockspace_id = NULL;
    std::vector<FileCell> currFileList = {};
    std::string currPath="";
    std::string selectPath = "";
    std::string currProjectPath = "";
    ImVec2 fileCellSize = {80,80};
    GLuint fileTextureId= loadTexture("file.png");
    GLuint flodersTextureId= loadTexture("folders.png");

    // Editor Camera LookAt Matrx
    glm::vec3 cameraPos{ 0.0f,0.0f,3.0f };
    glm::vec3 defaultTargetPos = {0.0f,0.0f,-1.0f};
    glm::vec3 defaultUpVector = { 0.0f,1.0f,0.0f };
    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        int display_w, display_h;
        ImVec2 viewPos, viewSize;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        //// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;
            static bool lightModel = false;
            bool flag = true;

            if (ImGui::BeginMainMenuBar()) {
                menubarHeight = ImGui::GetWindowSize().y;

                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open")) {
                        std::string path = selectFlodle();
                        if (!path.empty()) {
                            selectPath = path;
                            currProjectPath = path;
                        }
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Tools")) {
                    if (ImGui::MenuItem("Logcat")) {
                        showLogcat = true;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help")) {
                    if (ImGui::MenuItem("ImGuiDemoWindow")) {
                        show_demo_window = true;
                    }
                    if (ImGui::BeginMenu("Theme")) {
                        if (ImGui::MenuItem("Light")) {
                            ImGui::StyleColorsLight();
                        }
                        if (ImGui::MenuItem("Dark")) {
                            ImGui::StyleColorsDark();
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            if (dockspace_id == NULL || WindowReSize) {
                defaultLayout(dockspace_id, (float)display_w, (float)display_h - menubarHeight, 0, menubarHeight);
                WindowReSize = false;
            }

            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

            ImGui::DockSpaceOverViewport(NULL, ImGui::GetMainViewport());

            ImGui::Begin("Project", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);


            if (currPath != selectPath) {
                getDirectorys(selectPath, currFileList);
                currPath = selectPath;
            }
            for (int i = 0; i < currFileList.size(); i++) {
                ImGui::PushID(i);
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.412f, 0.412f, 0.412f, 1.0f));
                if (ImGui::ImageButton((ImTextureID)(currFileList[i].type == Directory ? flodersTextureId : fileTextureId), fileCellSize)) {
                    if (currFileList[i].type == Directory) {
                        selectPath = currPath + "\\" + currFileList[i].fileName;
                    }
                    else {
                        std::string path("code \"" + currProjectPath + "\" \"" + currPath + "\\" + currFileList[i].fileName + "\"");
                        system(path.c_str());
                    }

                }
                ImGui::PopStyleColor(3);
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + fileCellSize.x);
                ImGui::Text(currFileList[i].fileName.c_str());
                ImGui::EndGroup();
                int cellSum = ImGui::GetWindowSize().x / fileCellSize.x;
                cellSum = cellSum == 0 ? 1 : cellSum;
                if (((i + 1) % cellSum) != 0)
                    ImGui::SameLine();
                ImGui::PopID();
            }
            ImGui::End();

            ImGui::Begin("Console", NULL, ImGuiWindowFlags_NoCollapse);
            ImGui::BeginChild("Log", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& log : logBuffer) {
                ImGui::TextColored(GetLogLevelColor(log.level), log.msg.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
            ImGui::End();

            if (showLogcat) {
                ImGui::Begin("Logcat", &showLogcat, ImGuiWindowFlags_NoCollapse);
                ImGui::BeginChild("Log", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::EndChild();
                ImGui::End();
            }

            ImGui::Begin("View", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Model")) {
                    if (ImGui::MenuItem("Wireframe"))
                        wireframe = !wireframe;
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImVec2 viewFullSize = ImGui::GetWindowSize();
            viewSize = ImGui::GetContentRegionAvail();
            viewPos = ImGui::GetWindowPos();
            float side = (viewFullSize.x - viewSize.x) / 2;
            viewPos.x = viewPos.x + side;
            viewPos.y = display_h - viewPos.y - viewSize.y - (viewFullSize.y - viewSize.y) + side;
            ImGui::End();

            // Hierarchy Window
            ImGui::Begin("Hierarchy", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            //renderHierarchy(rootNode);

            // Right Key Click
            if (clickInWindow()) {
                ImGui::OpenPopup("#HierarchyPopup");
            }
            // Popup
            if (ImGui::BeginPopup("#HierarchyPopup")) {
                if (ImGui::MenuItem("Create Entity")) {
                    FakeEngine::EntityID id = FakeEngine::EntityManager::GetInstance().CreateEntity();
                    FakeEngine::ComponentManager::GetInstance().AddComponent<FakeEngine::Transform>(id, std::make_unique<FakeEngine::Transform>());
                    FakeEngine::ComponentManager::GetInstance().AddComponent<FakeEngine::Mesh>(id, std::make_unique<FakeEngine::Mesh>());
                }
                ImGui::EndPopup();
            }
            ImGui::End();

            ImGui::Begin("Inspector", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::End();
        }

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Rendering
        ImGui::Render();

        glViewport(viewPos.x, viewPos.y, viewSize.x, viewSize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        if (io.KeysDown[ImGuiKey_W]) {
            cameraPos += 0.05f * defaultTargetPos;
        }
        if (io.KeysDown[ImGuiKey_A]) {cameraPos -= 0.05f * glm::normalize(glm::cross(defaultTargetPos,defaultUpVector));}
        if (io.KeysDown[ImGuiKey_S]) {cameraPos -= 0.05f * defaultTargetPos;}
        if (io.KeysDown[ImGuiKey_D]) {cameraPos += 0.05f * glm::normalize(glm::cross(defaultTargetPos,defaultUpVector));}
        glm::mat4 lookAt= glm::lookAt(cameraPos, defaultTargetPos, defaultUpVector);

        renderSystem.Update(lookAt);

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

