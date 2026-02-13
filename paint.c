#define _POSIX_C_SOURCE 200809L

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#else
#include <unistd.h>
#endif

Uint32 black, white, red, green, blue, light_coral, light_blue, yellow, cyan;
Uint32 color_palette[9];

typedef struct {
    float center_x, center_y;
    float radius;
} Circle;

Circle circle = {.radius = 15};

static inline void init_colors(SDL_Surface *surface)
{
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(surface->format);

    black = SDL_MapRGBA(fmt, NULL, 0, 0, 0, 255);
    white = SDL_MapRGBA(fmt, NULL, 255, 255, 255, 255);
    red = SDL_MapRGBA(fmt, NULL, 255, 0, 0, 255);
    green = SDL_MapRGBA(fmt, NULL, 0, 255, 0, 255);
    blue = SDL_MapRGBA(fmt, NULL, 0, 0, 255, 255);
    light_coral = SDL_MapRGBA(fmt, NULL, 240, 128, 128, 255);
    light_blue = SDL_MapRGBA(fmt, NULL, 173, 216, 230, 255);
    yellow = SDL_MapRGBA(fmt, NULL, 255, 255, 0, 255);
    cyan = SDL_MapRGBA(fmt, NULL, 0, 255, 255, 255);

    color_palette[0] = black;
    color_palette[1] = white;
    color_palette[2] = red;
    color_palette[3] = green;
    color_palette[4] = blue;
    color_palette[5] = light_coral;
    color_palette[6] = light_blue;
    color_palette[7] = yellow;
    color_palette[8] = cyan;
}

bool is_inside_circle(Circle *circle, int x, int y)
{
    float dx = x - circle->center_x;
    float dy = y - circle->center_y;
    return (dx * dx + dy * dy <= circle->radius * circle->radius);
}

void draw_circle(SDL_Surface *surface, Circle *circle, Uint32 color)
{
    SDL_Rect rect;

    for (int x = circle->center_x - circle->radius;
         x <= circle->center_x + circle->radius; x++)
    {
        for (int y = circle->center_y - circle->radius;
             y <= circle->center_y + circle->radius; y++)
        {
            if (is_inside_circle(circle, x, y))
            {
                rect = (SDL_Rect){x, y, 1, 1};
                SDL_FillSurfaceRect(surface, &rect, color);
            }
        }
    }
}

void draw_palette(SDL_Surface *surface, Uint32 palette[], int size)
{
    SDL_Rect rect = {.y = 0, .w = 20, .h = 20};

    for (int i = 0; i < size; i++)
    {
        rect.x = i * 20;
        SDL_FillSurfaceRect(surface, &rect, palette[i]);
    }
}


char* open_save_dialog_ppm()
{
#ifdef _WIN32
    static char filename[MAX_PATH] = "untitled.ppm";

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "PPM Files (*.ppm)\0*.ppm\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "ppm";

    if (GetSaveFileName(&ofn))
        return strdup(filename);

    return NULL;

#else
    FILE *fp = popen(
        "zenity --file-selection "
        "--save "
        "--confirm-overwrite "
        "--filename=\"untitled.ppm\" "
        "--file-filter=\"PPM files | *.ppm\"",
        "r"
    );

    if (!fp){
      return NULL;
    } 

    char path[1024];

    if (!fgets(path, sizeof(path), fp)) {
        pclose(fp);
        return NULL;
    }

    pclose(fp);

    path[strcspn(path, "\n")] = 0;
    return strdup(path);
#endif
}

char* mandating_ppm_extension(const char* filename)
{
    if (!filename)
    {
      return NULL;
    } 

    const char *dot = strrchr(filename, '.');

    if (dot && strcmp(dot, ".ppm") == 0){
        return strdup(filename);
    }

    char *newname = malloc(strlen(filename) + 5);
    sprintf(newname, "%s.ppm", filename);
    return newname;
}


void save_as_ppm(SDL_Surface *surface, const char *filepath)
{
    if (!surface || !filepath){
      return;
    } 

    FILE *fp = fopen(filepath, "wb");
    if (!fp){
      return;
    } 

    fprintf(fp, "P6\n%d %d\n255\n", surface->w, surface->h);

    SDL_LockSurface(surface);

    Uint8 *pixels = surface->pixels;
    int pitch = surface->pitch;

    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(surface->format);

    for (int y = 0; y < surface->h; y++)
    {
        Uint8 *row = pixels + y * pitch;

        for (int x = 0; x < surface->w; x++)
        {
            Uint32 pixel;
            memcpy(&pixel, row + x * SDL_BYTESPERPIXEL(surface->format), SDL_BYTESPERPIXEL(surface->format));

            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, fmt, NULL, &r, &g, &b, &a);

            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }

    SDL_UnlockSurface(surface);
    fclose(fp);

    printf("Saved to %s\n", filepath);
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("Minimal Paint", 900, 600, 0);

    SDL_Surface *surface = SDL_GetWindowSurface(window);

    init_colors(surface);

    bool running = true;
    bool draw = false;
    int which_color = 1;
    int num_colors = 9;

    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                running = false;

            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.repeat == false)
            {
                if (event.key.key == SDLK_S &&
                    (event.key.mod & SDL_KMOD_CTRL))
                {
                    char *raw = open_save_dialog_ppm();
                    char *path = mandating_ppm_extension(raw);

                    if (path) {
                        save_as_ppm(surface, path);
                        free(path);
                    }
                    free(raw);
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (event.button.y < 20)
                {
                    which_color = event.button.x / 20;
                    if (which_color >= num_colors)
                        which_color = num_colors - 1;
                }
                else
                {
                    circle.center_x = event.button.x;
                    circle.center_y = event.button.y;
                    draw = true;
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
                draw = false;

            if (event.type == SDL_EVENT_MOUSE_MOTION && draw)
            {
                circle.center_x = event.motion.x;
                circle.center_y = event.motion.y;
            }

            if (event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                circle.radius += event.wheel.y;
                if (circle.radius < 1)
                    circle.radius = 1;
                printf("Cursor size: %.1f\n",circle.radius);
            }
        }

        if (draw){
            draw_circle(surface, &circle,color_palette[which_color]);
        }

        draw_palette(surface, color_palette, num_colors);

        SDL_UpdateWindowSurface(window);
        SDL_Delay(10);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}