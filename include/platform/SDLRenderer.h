/**
 * SDLRenderer.h
 * SDL2-based renderer for NiXX-32 arcade board emulation
 * 
 * This file defines the SDL2-based rendering implementation that handles
 * display output for the NiXX-32 arcade system emulation. It provides
 * hardware-accelerated rendering of the emulated graphics, supporting
 * both the original and enhanced hardware variants.
 */

 #pragma once

 #include <cstdint>
 #include <string>
 #include <vector>
 #include <memory>
 #include <functional>
 
 #include "GraphicsSystem.h"
 #include "Logger.h"
 
 // Forward declarations for SDL types to avoid including SDL headers
 struct SDL_Window;
 struct SDL_Renderer;
 struct SDL_Texture;
 
 namespace NiXX32 {
 
 // Forward declarations
 class System;
 
 /**
  * Renderer display modes
  */
 enum class DisplayMode {
	 WINDOWED,           // Windowed mode
	 FULLSCREEN,         // Exclusive fullscreen mode
	 FULLSCREEN_DESKTOP  // Borderless fullscreen mode
 };
 
 /**
  * Scaling modes for display
  */
 enum class ScalingMode {
	 NEAREST,            // Nearest-neighbor scaling (pixelated)
	 LINEAR,             // Linear filtering (smooth)
	 BEST                // Best quality scaling
 };
 
 /**
  * Aspect ratio handling modes
  */
 enum class AspectRatioMode {
	 STRETCH,            // Stretch to fill window
	 MAINTAIN,           // Maintain aspect ratio with black bars
	 PIXEL_PERFECT       // Integer scaling for pixel-perfect display
 };
 
 /**
  * SDL renderer configuration
  */
 struct SDLRendererConfig {
	 int windowWidth;               // Window width in pixels
	 int windowHeight;              // Window height in pixels
	 DisplayMode displayMode;       // Display mode
	 ScalingMode scalingMode;       // Scaling algorithm
	 AspectRatioMode aspectMode;    // Aspect ratio handling
	 bool vsync;                    // Whether to use vertical sync
	 bool scanlines;                // Whether to draw scanline effect
	 int scanlineIntensity;         // Scanline effect intensity (0-100)
	 bool crtEffect;                // CRT screen curvature effect
	 bool showFPS;                  // Whether to show FPS counter
	 std::string windowTitle;       // Window title
 };
 
 /**
  * Class that implements rendering using SDL2
  */
 class SDLRenderer {
 public:
	 /**
	  * Constructor
	  * @param system Reference to the parent system
	  * @param graphicsSystem Reference to the graphics system
	  * @param logger Reference to the system logger
	  */
	 SDLRenderer(System& system, GraphicsSystem& graphicsSystem, Logger& logger);
	 
	 /**
	  * Destructor
	  */
	 ~SDLRenderer();
	 
	 /**
	  * Initialize the SDL renderer
	  * @param config Renderer configuration
	  * @return True if initialization was successful
	  */
	 bool Initialize(const SDLRendererConfig& config);
	 
	 /**
	  * Reset the renderer state
	  */
	 void Reset();
	 
	 /**
	  * Render a frame
	  * @param frameBuffer Pointer to frame buffer data
	  * @param width Frame buffer width
	  * @param height Frame buffer height
	  * @param pitch Frame buffer pitch (bytes per row)
	  */
	 void RenderFrame(uint32_t* frameBuffer, int width, int height, int pitch);
	 
	 /**
	  * Present the rendered frame to the display
	  */
	 void Present();
	 
	 /**
	  * Set window title
	  * @param title New window title
	  */
	 void SetWindowTitle(const std::string& title);
	 
	 /**
	  * Set display mode
	  * @param mode New display mode
	  * @return True if successful
	  */
	 bool SetDisplayMode(DisplayMode mode);
	 
	 /**
	  * Set scaling mode
	  * @param mode New scaling mode
	  * @return True if successful
	  */
	 bool SetScalingMode(ScalingMode mode);
	 
	 /**
	  * Set aspect ratio mode
	  * @param mode New aspect ratio mode
	  * @return True if successful
	  */
	 bool SetAspectRatioMode(AspectRatioMode mode);
	 
	 /**
	  * Enable or disable vertical sync
	  * @param enabled True to enable, false to disable
	  * @return True if successful
	  */
	 bool SetVSync(bool enabled);
	 
	 /**
	  * Enable or disable scanline effect
	  * @param enabled True to enable, false to disable
	  * @param intensity Scanline intensity (0-100)
	  * @return True if successful
	  */
	 bool SetScanlines(bool enabled, int intensity = 50);
	 
	 /**
	  * Enable or disable CRT effect
	  * @param enabled True to enable, false to disable
	  * @return True if successful
	  */
	 bool SetCRTEffect(bool enabled);
	 
	 /**
	  * Enable or disable FPS display
	  * @param enabled True to enable, false to disable
	  */
	 void SetFPSDisplay(bool enabled);
	 
	 /**
	  * Resize the renderer
	  * @param width New width
	  * @param height New height
	  * @return True if successful
	  */
	 bool Resize(int width, int height);
	 
	 /**
	  * Take a screenshot
	  * @param filename File to save screenshot to (empty for automatic name)
	  * @return True if successful
	  */
	 bool TakeScreenshot(const std::string& filename = "");
	 
	 /**
	  * Register window event callback
	  * @param callback Function to call when window events occur
	  * @return Callback ID
	  */
	 int RegisterWindowCallback(std::function<void(int, int, bool)> callback);
	 
	 /**
	  * Remove window event callback
	  * @param callbackId Callback ID
	  * @return True if successful
	  */
	 bool RemoveWindowCallback(int callbackId);
	 
	 /**
	  * Get current window dimensions
	  * @param width Output parameter for width
	  * @param height Output parameter for height
	  */
	 void GetWindowSize(int& width, int& height) const;
	 
	 /**
	  * Check if window is fullscreen
	  * @return True if fullscreen
	  */
	 bool IsFullscreen() const;
	 
	 /**
	  * Get current FPS (frames per second)
	  * @return Current FPS
	  */
	 float GetFPS() const;
	 
	 /**
	  * Get the renderer configuration
	  * @return Current configuration
	  */
	 const SDLRendererConfig& GetConfig() const;
	 
	 /**
	  * Apply a post-processing shader effect
	  * @param shaderCode GLSL shader code
	  * @param uniformValues Uniform values for the shader
	  * @return True if successful
	  */
	 bool ApplyShader(const std::string& shaderCode, 
					const std::unordered_map<std::string, float>& uniformValues);
	 
	 /**
	  * Reset post-processing to default
	  * @return True if successful
	  */
	 bool ResetShader();
 
 private:
	 // Reference to parent system
	 System& m_system;
	 
	 // Reference to graphics system
	 GraphicsSystem& m_graphicsSystem;
	 
	 // Reference to logger
	 Logger& m_logger;
	 
	 // SDL objects
	 SDL_Window* m_window;
	 SDL_Renderer* m_renderer;
	 SDL_Texture* m_frameTexture;
	 SDL_Texture* m_scanlineTexture;
	 
	 // Current configuration
	 SDLRendererConfig m_config;
	 
	 // Source and destination rectangles for rendering
	 struct {
		 int srcX, srcY, srcW, srcH;
		 int dstX, dstY, dstW, dstH;
	 } m_renderRect;
	 
	 // FPS calculation
	 struct {
		 uint32_t frameCount;
		 uint32_t lastTime;
		 float fps;
		 bool display;
	 } m_fpsData;
	 
	 // Shader data for post-processing
	 struct {
		 bool enabled;
		 std::string code;
		 std::unordered_map<std::string, float> uniforms;
		 void* shaderProgram; // Opaque pointer to shader program
	 } m_shader;
	 
	 // Window callbacks
	 std::unordered_map<int, std::function<void(int, int, bool)>> m_windowCallbacks;
	 int m_nextCallbackId;
	 
	 // Scanline effect texture data
	 std::vector<uint8_t> m_scanlineTextureData;
	 
	 /**
	  * Initialize SDL subsystems
	  * @return True if successful
	  */
	 bool InitializeSDL();
	 
	 /**
	  * Create window and renderer
	  * @return True if successful
	  */
	 bool CreateWindowAndRenderer();
	 
	 /**
	  * Create frame texture
	  * @param width Texture width
	  * @param height Texture height
	  * @return True if successful
	  */
	 bool CreateFrameTexture(int width, int height);
	 
	 /**
	  * Create scanline texture
	  * @return True if successful
	  */
	 bool CreateScanlineTexture();
	 
	 /**
	  * Calculate render rectangles based on aspect ratio mode
	  * @param frameWidth Frame buffer width
	  * @param frameHeight Frame buffer height
	  */
	 void CalculateRenderRects(int frameWidth, int frameHeight);
	 
	 /**
	  * Update FPS counter
	  */
	 void UpdateFPS();
	 
	 /**
	  * Render FPS display
	  */
	 void RenderFPSDisplay();
	 
	 /**
	  * Apply scanline effect
	  */
	 void ApplyScanlines();
	 
	 /**
	  * Apply CRT effect
	  */
	 void ApplyCRTEffect();
	 
	 /**
	  * Handle window resize event
	  * @param width New width
	  * @param height New height
	  */
	 void HandleResize(int width, int height);
	 
	 /**
	  * Handle display mode change
	  */
	 void HandleDisplayModeChange();
	 
	 /**
	  * Notify window callbacks
	  * @param width Window width
	  * @param height Window height
	  * @param fullscreen Whether window is fullscreen
	  */
	 void NotifyWindowCallbacks(int width, int height, bool fullscreen);
	 
	 /**
	  * Compile and set up shader
	  * @param shaderCode GLSL shader code
	  * @return True if successful
	  */
	 bool CompileShader(const std::string& shaderCode);
	 
	 /**
	  * Apply shader uniforms
	  * @param uniformValues Uniform values
	  */
	 void ApplyShaderUniforms(const std::unordered_map<std::string, float>& uniformValues);
 };
 
 } // namespace NiXX32