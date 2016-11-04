//
// Created by Sebastian on 08.10.2016.
//
#include "windowInternal.h"
#include "drawing.h"
#include "window.h"

Window SCREEN_WINDOW = NULL;

void initWindowStruct(WindowStruct* windowStruct, int x, int y, unsigned int width, unsigned int height,
                      Visual* visual, Colormap colormap, Bool inputOnly,
                      unsigned long backgroundColor, Pixmap backgroundPixmap) {
    windowStruct->parent = NULL;
    windowStruct->children = NULL;
    windowStruct->childSpace = 0;
    windowStruct->x = x;
    windowStruct->y = y;
    windowStruct->w = width;
    windowStruct->h = height;
    windowStruct->inputOnly = inputOnly;
    windowStruct->colormap = colormap;
    windowStruct->visual = visual;
    windowStruct->unmappedContent = NULL;
    windowStruct->sdlWindow = NULL;
    windowStruct->renderTarget = NULL;
    windowStruct->backgroundColor = backgroundColor;
    windowStruct->background = backgroundPixmap;
    windowStruct->colormapWindowsCount = -1;
    windowStruct->colormapWindows = NULL;
    windowStruct->propertyCount = 0;
    windowStruct->propertySize = 0;
    windowStruct->properties = NULL;
    windowStruct->windowName = NULL;
    windowStruct->icon = NULL;
    windowStruct->borderWidth = 0;
    windowStruct->depth = 0;
    windowStruct->mapState = UnMapped;
    windowStruct->eventMask = 0;
    windowStruct->overrideRedirect = False;
#ifdef DEBUG_WINDOWS
    windowStruct->debugId = ((unsigned long) rand() << 16) | rand();
#endif /* DEBUG_WINDOWS */
}

/* Screen window handles */

Bool initScreenWindow(Display* display) {
    if (SCREEN_WINDOW == NULL) {
        SCREEN_WINDOW = malloc(sizeof(Window));
        if (SCREEN_WINDOW == NULL) {
            fprintf(stderr, "Out of memory: Failed to allocate SCREEN_WINDOW in initScreenWindow!\n");
            return False;
        }
        SCREEN_WINDOW->type = WINDOW;
        WindowStruct* window = malloc(sizeof(WindowStruct));
        if (window == NULL) {
            free(SCREEN_WINDOW);
            SCREEN_WINDOW = NULL;
            fprintf(stderr, "Out of memory: Failed to allocate SCREEN_WINDOW in initScreenWindow!\n");
            return False;
        }
        initWindowStruct(window, 0, 0, display->screens[0].width, display->screens[0].height,
                         NULL, NULL, False, 0, NULL);
        SCREEN_WINDOW->dataPointer = window;
        window->mapState = Mapped;
    }
    return True;
}

void destroyScreenWindow(Display* display) {
    if (SCREEN_WINDOW != NULL) {
        GPU_FreeTarget(GET_WINDOW_STRUCT(SCREEN_WINDOW)->renderTarget);
        SDL_DestroyWindow(GET_WINDOW_STRUCT(SCREEN_WINDOW)->sdlWindow);
        free(SCREEN_WINDOW->dataPointer);
        free(SCREEN_WINDOW);
        SCREEN_WINDOW = NULL;
    }
}

WindowSdlIdMapper* mappingListStart = NULL;

WindowSdlIdMapper* getWindowSdlIdMapperStructFromId(Uint32 sdlWindowId) {
    WindowSdlIdMapper* mapper;
    for (mapper = mappingListStart; mapper != NULL; mapper = mapper->next) {
        if (mapper->sdlWindowId == sdlWindowId) { return mapper; }
    }
    return NULL;
}

void registerWindowMapping(Window window, Uint32 sdlWindowId) {
    WindowSdlIdMapper* mapper = getWindowSdlIdMapperStructFromId(sdlWindowId);
    if (mapper == NULL) {
        mapper = malloc(sizeof(WindowSdlIdMapper));
        if (mapper == NULL) {
            fprintf(stderr, "Failed to allocate mapping object to map xWindow to SDL window ID!\n");
            return;
        }
        mapper->next = mappingListStart;
        mappingListStart = mapper;
        mapper->sdlWindowId = sdlWindowId;
    }
    mapper->window = window;
}

Window getWindowFromId(Uint32 sdlWindowId) {
    WindowSdlIdMapper* mapper = getWindowSdlIdMapperStructFromId(sdlWindowId);
    return mapper == NULL ? None : mapper->window;
}

Window getContainingWindow(Window window, int x, int y) {
    int i, child_x, child_y, child_w, child_h;
    Window* childern = GET_CHILDREN(window);
    for (i = GET_WINDOW_STRUCT(window)->childSpace - 1; i >= 0 ; i--) {
        if (childern[i] == NULL) continue;
        GET_WINDOW_POS(childern[i], child_x, child_y);
        GET_WINDOW_DIMS(childern[i], child_w, child_h);
        if (x >= child_x && x <= child_x + child_w && y >= child_y && y <= child_y + child_h) {
            return getContainingWindow(childern[i], x - child_x, y - child_y);
        }
    }
    return window;
}

void removeChildFromParent(Window child) {
    if (child == SCREEN_WINDOW) { return; }
    Window parent = GET_PARENT(child);
    if (parent != NULL) {
        unsigned int parentChildSpace = GET_WINDOW_STRUCT(parent)->childSpace;
        Window* childPointer = GET_CHILDREN(parent);
        int i;
        for (i = 0; i < parentChildSpace; i++) {
            fflush(stderr);
            if (childPointer[i] == child) {
                childPointer[i] = NULL;
            }
        }
    }
}

void destroyWindow(Display* display, Window window, Bool freeParentData) {
    if (window == NULL || window == SCREEN_WINDOW) { return; }
    int i;
    if (freeParentData) {
        removeChildFromParent(window);
    }
    XFreeColormap(display, GET_COLORMAP(window));
    WindowStruct* windowStruct = GET_WINDOW_STRUCT(window);
    if (windowStruct->childSpace > 0) {
        Window* childPointer = GET_CHILDREN(window);
        for (i = 0; i < windowStruct->childSpace; i++) {
            if (childPointer[i] != NULL) {
                destroyWindow(display, childPointer[i], False);
            }
        }
        free(childPointer);
    }
    if (windowStruct->propertySize > 0) {
        WindowProperty* propertyPointer = windowStruct->properties;
        free(propertyPointer);
    }
    if (windowStruct->background != NULL && windowStruct->background != None) {
        XFreePixmap(display, windowStruct->background);
    }
    if (windowStruct->windowName != NULL) {
        free(windowStruct->windowName);
    }
    if (windowStruct->icon != NULL) {
        SDL_FreeSurface(windowStruct->icon);
    }
    if (windowStruct->sdlWindow != NULL) {
        SDL_DestroyWindow(windowStruct->sdlWindow);
    }
    if (windowStruct->renderTarget != NULL) {
        GPU_FreeTarget(windowStruct->renderTarget);
    }
    if (windowStruct->unmappedContent != NULL) {
        GPU_FreeImage(windowStruct->unmappedContent);
    }
    free(windowStruct);
    free(window);
    // TODO: DestroyNotify Event
}


void print_bytes(const void *object, size_t size)
{
    size_t i;

    printf("[ ");
    for(i = 0; i < size; i++)
    {
        printf("%02x ", ((const unsigned char *) object)[i] & 0xff);
    }
    printf("]\n");
    fflush(stdout);
}

#include <unistd.h>

Bool increaseChildList(Window window) {
    int i;
    int oldChildSpace = GET_WINDOW_STRUCT(window)->childSpace;
    Window* children = GET_CHILDREN(window);
    int newChildSpace = oldChildSpace == 0 ? 8 : oldChildSpace * 2;
    Window* newSpace = malloc(sizeof(Window) * newChildSpace);
    if (newSpace == NULL) {
        return False;
    }
    memcpy(newSpace, children, sizeof(Window) * oldChildSpace);
    memset(newSpace + oldChildSpace, (int) NULL, sizeof(Window) * (newChildSpace - oldChildSpace));
    fprintf(stderr, "oldChildSpace: %d\n", oldChildSpace);
    for (i = 0; i < oldChildSpace; i++) {
        print_bytes(children + i, sizeof(Window));
    }
    free(children);
    GET_WINDOW_STRUCT(window)->childSpace = newChildSpace;
    GET_WINDOW_STRUCT(window)->children = newSpace;
    return True;
}

// TODO: Improve
Bool addChildToWindow(Window parent, Window child) {
    int parentSpace = GET_WINDOW_STRUCT(parent)->childSpace;
    Window* parentChildren = GET_CHILDREN(parent);
    int startOffset = 0;
    if (parentSpace == 0 || *(parentChildren + parentSpace - 1) != NULL) {
        startOffset = parentSpace;
        if (!increaseChildList(parent)) {
            return False;
        }
        parentSpace = GET_WINDOW_STRUCT(parent)->childSpace;
    }
    parentChildren = GET_CHILDREN(parent);
    parentChildren += startOffset;
    while (*parentChildren != NULL) { parentChildren++; }
    *parentChildren = child;
    if (*(GET_CHILDREN(parent) + parentSpace - 1) != NULL) { // There must be at least one NULL entity at the end
        if (!increaseChildList(parent)) {
            *parentChildren = NULL;
            return False;
        }
    }
    GET_WINDOW_STRUCT(child)->parent = parent;
    return True;
}

Bool isParent(Window window1, Window window2) {
    Window parent = GET_PARENT(window2);
    while (parent != NULL) {
        if (parent == window1) {
            return True;
        }
        parent = GET_PARENT(parent);
    }
    return False;
}

WindowProperty* increasePropertySize(WindowProperty* properties, unsigned int currSize,
                                     unsigned int* newSize) {
    if (currSize == 0) {
        *newSize = 8;
    } else {
        *newSize = currSize * 2;
    }
    WindowProperty* newProperties = malloc(sizeof(WindowProperty) * *newSize);
    if (newProperties == NULL) { return NULL; }
    if (currSize != 0) {
        memcpy(newProperties, properties, sizeof(WindowProperty) * currSize);
    }
    // TODO: Free old properties?
    return newProperties;
}

WindowProperty* findProperty(WindowProperty* properties, unsigned int numProperties, Atom property) {
    int i;
    for (i = 0; i < numProperties; i++) {
        if (properties->property == property) {
            return properties;
        }
        properties++;
    }
    return NULL;
}

Bool resizeWindowSurface(Window window) {
    WindowStruct* windowStruct = GET_WINDOW_STRUCT(window);
    GPU_Image* oldContent = windowStruct->unmappedContent;
    if (oldContent != NULL) {
        GPU_Image* newContent = GPU_CreateImage((Uint16) windowStruct->w, (Uint16) windowStruct->h, oldContent->format);
        if (newContent == NULL) {
            fprintf(stderr, "Failed to resize the window surface: Failed to create new window surface!\n");
            return FALSE;
        }
        GPU_Target* newTarget = GPU_LoadTarget(newContent);
        if (newTarget == NULL) {
            GPU_FreeImage(newContent);
            fprintf(stderr, "Failed to resize the window surface: Failed to create render target from new window surface!\n");
            return FALSE;
        }
/**/        if (windowStruct->renderTarget != NULL) GPU_Flip(windowStruct->renderTarget);
        fprintf(stderr, "Resizing surface of window %p\n", window);
        fprintf(stderr, "BLITTING in %s\n", __func__);
        GPU_Blit(oldContent, NULL, newTarget, oldContent->w / 2, oldContent->h / 2);
        if (windowStruct->renderTarget != NULL) GPU_FreeTarget(windowStruct->renderTarget);
        GPU_FreeImage(oldContent);
        windowStruct->unmappedContent = newContent;
        windowStruct->renderTarget = newTarget;
    }
    return TRUE;
}

Window getParentWithEventBit(Window window, long eventBit) {
    for (window = GET_PARENT(window); window != SCREEN_WINDOW; window = GET_PARENT(window)) {
        if (GET_WINDOW_STRUCT(window)->eventMask & eventBit) {
            return window;
        }
    }
    return None;
}

Bool mergeWindowDrawables(Window parent, Window child) {
    WindowStruct* childWindowStruct = GET_WINDOW_STRUCT(child);
    if (childWindowStruct->unmappedContent == NULL) { return True; }
    fprintf(stderr, "getWindowRenderTarget of window %p in %s.\n", parent, __func__);
    GPU_Target* parentTarget = getWindowRenderTarget(parent);
    if (parentTarget == NULL) return FALSE;
    if (childWindowStruct->renderTarget != NULL) {
/**/        GPU_Flip(childWindowStruct->renderTarget);
    }
    fprintf(stderr, "BLITTING in %s\b", __func__);
    GPU_Blit(childWindowStruct->unmappedContent, NULL, parentTarget,
             childWindowStruct->x + childWindowStruct->w / 2, childWindowStruct->y + childWindowStruct->h / 2);
    if (childWindowStruct->renderTarget != NULL) {
        GPU_FreeTarget(childWindowStruct->renderTarget);
        childWindowStruct->renderTarget = NULL;
    }
    GPU_FreeImage(childWindowStruct->unmappedContent);
    childWindowStruct->unmappedContent = NULL;
    return True;
}

Window enqueueMapEvent(Display* display, Window window, Window parentWithSubstructureRedirect, Bool searchForSRParent) {
    if (searchForSRParent) {
        parentWithSubstructureRedirect = getParentWithEventBit(window, SubstructureRedirectMask);
    }
    XEvent event;
    if (parentWithSubstructureRedirect == None) {
        event.type = MapNotify;
        event.xmap.type = MapNotify;
        event.xmap.serial = 0;
        event.xmap.send_event = False;
        event.xmap.display = display;
        event.xmap.window = window;
        event.xmap.event = window;
        event.xmap.override_redirect = False;
        event.xany.serial = 0;
        event.xany.display = display;
        event.xany.send_event = False;
        event.xany.type = MapNotify;
        event.xany.window = window;
    } else {
        event.type = MapRequest;
        event.xmaprequest.type = MapRequest;
        event.xmaprequest.serial = 0;
        event.xmaprequest.send_event = False;
        event.xmaprequest.display = display;
        event.xmaprequest.parent = parentWithSubstructureRedirect;
        event.xmaprequest.window = window;
        event.xany.serial = 0;
        event.xany.display = display;
        event.xany.send_event = False;
        event.xany.type = MapRequest;
        event.xany.window = window;
    }
    enqueueEvent(display, &event);
}

void mapRequestedChildren(Display* display, Window window, Window subStructureRedirectParent) {
    Window* children = GET_CHILDREN(window);
    size_t i;
    for (i = 0; i < GET_WINDOW_STRUCT(window)->childSpace; i++) {
        if (children[i] != NULL && GET_WINDOW_STRUCT(children[i])->mapState == MapRequested) {
            if (!mergeWindowDrawables(window, children[i])) {
                fprintf(stderr, "Failed to merge the window drawables in %s\n", __func__);
                return;
            }
            enqueueMapEvent(display, children[i], subStructureRedirectParent, False);
            GET_WINDOW_STRUCT(children[i])->mapState = Mapped;
            mapRequestedChildren(display, children[i], subStructureRedirectParent);
        }
    }
}
