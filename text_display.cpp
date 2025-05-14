#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>

struct Config {
    std::string text = "abc";
    std::string position = "top-right"; // top-right, top-left, bottom-right, bottom-left, center
    std::string font = "fixed";
    int fontSize = 12; // Not directly used with X11 default fonts, but helpful for documentation
    int marginX = 10;
    int marginY = 20;
    unsigned long color = 0xFF0000; // Red in RGB hex
    int displayTime = 30; // seconds
    bool transparent = true;
};

void printUsage() {
    std::cout << "Usage: ./text_display [options]\n"
              << "Options:\n"
              << "  --text TEXT            Text to display (default: \"abc\")\n"
              << "  --position POSITION    Position on screen (top-right, top-left, bottom-right, bottom-left, center) (default: top-right)\n"
              << "  --font FONT            X11 font name (default: fixed)\n"
              << "  --marginx PIXELS       X margin in pixels (default: 10)\n"
              << "  --marginy PIXELS       Y margin in pixels (default: 20)\n"
              << "  --color RRGGBB         Text color in hex RGB (default: FF0000 for red)\n"
              << "  --time SECONDS         Display time in seconds (default: 30)\n"
              << "  --transparent BOOL     Use transparent background (true/false) (default: true)\n"
              << "  --help                 Show this help message\n";
}

Config parseArgs(int argc, char *argv[]) {
    Config config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage();
            exit(0);
        } else if (arg == "--text" && i+1 < argc) {
            config.text = argv[++i];
        } else if (arg == "--position" && i+1 < argc) {
            config.position = argv[++i];
        } else if (arg == "--font" && i+1 < argc) {
            config.font = argv[++i];
        } else if (arg == "--marginx" && i+1 < argc) {
            config.marginX = std::stoi(argv[++i]);
        } else if (arg == "--marginy" && i+1 < argc) {
            config.marginY = std::stoi(argv[++i]);
        } else if (arg == "--color" && i+1 < argc) {
            std::string colorStr = argv[++i];
            config.color = std::stoul(colorStr, nullptr, 16);
        } else if (arg == "--time" && i+1 < argc) {
            config.displayTime = std::stoi(argv[++i]);
        } else if (arg == "--transparent" && i+1 < argc) {
            std::string val = argv[++i];
            config.transparent = (val == "true" || val == "1" || val == "yes");
        }
    }
    
    return config;
}

int main(int argc, char *argv[]) {
    // Parse command-line arguments
    Config config = parseArgs(argc, argv);
    
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "Cannot open display" << std::endl;
        return 1;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    // Get screen dimensions
    int screen_width = DisplayWidth(display, screen);
    int screen_height = DisplayHeight(display, screen);

    // Create the window with initial size
    XSetWindowAttributes attributes;
    attributes.override_redirect = True;  // Skip window manager
    
    // Handle transparency
    unsigned long valuemask = CWOverrideRedirect;
    if (config.transparent) {
        // For a transparent background, we'll use a black background and 
        // then use a shape mask to make it transparent
        attributes.background_pixel = BlackPixel(display, screen);
        valuemask |= CWBackPixel;
    } else {
        attributes.background_pixel = BlackPixel(display, screen);
        valuemask |= CWBackPixel;
    }

    // Create initial window - we'll resize it based on text size later
    Window window = XCreateWindow(
        display, root,
        0, 0,                             // position (will be adjusted)
        100, 50,                          // width, height (will be adjusted)
        0,                                // border width
        CopyFromParent,                   // depth
        InputOutput,                      // class
        CopyFromParent,                   // visual
        valuemask,                        // value mask
        &attributes                       // attributes
    );

    // Set window to stay on top
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wm_state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);

    XChangeProperty(
        display, window,
        wm_state, XA_ATOM, 32,
        PropModeReplace, (unsigned char*)&wm_state_above, 1
    );

    // Set up graphics context for drawing text
    GC gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, config.color);  // Set text color

    // Try to load the font
    std::string fontWithSize;
    if (config.font.find("-") == std::string::npos) {
        // If it's a simple font name, not an XLFD, try to add size info
        fontWithSize = "-*-" + config.font + "-*-*-*-*-" + 
                      std::to_string(config.fontSize) + "-*-*-*-*-*-*-*";
    } else {
        fontWithSize = config.font;  // Already in XLFD format
    }

    XFontStruct *font = XLoadQueryFont(display, fontWithSize.c_str());
    if (!font) {
        // Fall back to default fixed font if specified font fails
        std::cerr << "Could not load font: " << fontWithSize << ", trying default..." << std::endl;
        font = XLoadQueryFont(display, "fixed");
        if (!font) {
            std::cerr << "Could not load any font" << std::endl;
            XFreeGC(display, gc);
            XDestroyWindow(display, window);
            XCloseDisplay(display);
            return 1;
        }
    }
    XSetFont(display, gc, font->fid);

    // Calculate text dimensions
    int text_width = XTextWidth(font, config.text.c_str(), config.text.length());
    int text_height = font->ascent + font->descent;
    
    // Add margins to get window size
    int window_width = text_width + 2 * config.marginX;
    int window_height = text_height + 2 * config.marginY;

    // Calculate window position based on desired position
    int window_x = 0;
    int window_y = 0;

    if (config.position == "top-right") {
        window_x = screen_width - window_width - config.marginX;
        window_y = config.marginY;
    } else if (config.position == "top-left") {
        window_x = config.marginX;
        window_y = config.marginY;
    } else if (config.position == "bottom-right") {
        window_x = screen_width - window_width - config.marginX;
        window_y = screen_height - window_height - config.marginY;
    } else if (config.position == "bottom-left") {
        window_x = config.marginX;
        window_y = screen_height - window_height - config.marginY;
    } else if (config.position == "center") {
        window_x = (screen_width - window_width) / 2;
        window_y = (screen_height - window_height) / 2;
    }

    // Move and resize the window to the desired position and size
    XMoveResizeWindow(display, window, window_x, window_y, window_width, window_height);

    // Calculate text position within the window
    int text_x = config.marginX;
    int text_y = config.marginY + font->ascent; // Add ascent to position text correctly

    // Map (show) the window
    XMapWindow(display, window);

    // Draw text
    XDrawString(display, window, gc, text_x, text_y, 
                config.text.c_str(), config.text.length());
    XFlush(display);

    // If transparent background is requested
    if (config.transparent) {
        // Create a 1-bit deep pixmap for the shape mask
        Pixmap shape_mask = XCreatePixmap(display, window, window_width, window_height, 1);
        GC shape_gc = XCreateGC(display, shape_mask, 0, NULL);
        
        // Clear the mask
        XSetForeground(display, shape_gc, 0);
        XFillRectangle(display, shape_mask, shape_gc, 0, 0, window_width, window_height);
        
        // Draw the text area as the only visible part
        XSetForeground(display, shape_gc, 1);
        XSetFont(display, shape_gc, font->fid);
        XDrawString(display, shape_mask, shape_gc, text_x, text_y, 
                   config.text.c_str(), config.text.length());
        
        // Apply the shape mask to the window
        XShapeCombineMask(display, window, ShapeBounding, 0, 0, shape_mask, ShapeSet);
        
        // Free resources
        XFreeGC(display, shape_gc);
        XFreePixmap(display, shape_mask);
    }

    // Wait for specified time
    std::cout << "Displaying \"" << config.text << "\" for " 
              << config.displayTime << " seconds..." << std::endl;
    sleep(config.displayTime);

    // Clean up
    XFreeFont(display, font);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}
