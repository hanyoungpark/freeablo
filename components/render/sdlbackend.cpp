#include "render.h"

#include <iostream>
#include <complex>

#include <SDL.h>

#include "../cel/celfile.h"
#include "../cel/celframe.h"

#include "../level/level.h"


namespace Render
{
    #define WIDTH 1280
    #define HEIGHT 960
    #define BPP 4
    #define DEPTH 32

    SDL_Surface* screen;
    
    void init()
    {
        SDL_Init(SDL_INIT_VIDEO);
        atexit(SDL_Quit);
        screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE | SDL_DOUBLEBUF);
    }

    void draw()
    {
        SDL_Flip(screen); 
    }

    void drawAt(const Sprite& sprite, size_t x, size_t y)
    {
        SDL_Rect rcDest = { x, y, 0, 0 };
        SDL_BlitSurface ((SDL_Surface*)sprite , NULL, screen, &rcDest );
    }

    SDL_Surface* createTransparentSurface(size_t width, size_t height)
    {
         SDL_Surface* s; 
        
        // SDL y u do dis
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            s = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, DEPTH, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
        #else
            s = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, DEPTH, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        #endif

        SDL_FillRect(s, NULL, SDL_MapRGBA(s->format, 0, 0, 0, 0)); 

        return s;
    }

    void setpixel(SDL_Surface *s, int x, int y, Cel::Colour c)
    {
        y = y*s->pitch/BPP;

        Uint32 *pixmem32;
        Uint32 colour;  
     
        colour = SDL_MapRGB( s->format, c.r, c.g, c.b );
      
        pixmem32 = (Uint32*) s->pixels  + y + x;
        *pixmem32 = colour;
    }

    void drawFrame(SDL_Surface* s, int start_x, int start_y, const Cel::CelFrame& frame)
    {
        for(int x = 0; x < frame.mWidth; x++)
        {
            for(int y = 0; y < frame.mHeight; y++)
            {
                if(frame[x][y].visible)
                    setpixel(s, start_x+x, start_y+y, frame[x][y]);
            }
        }
    }

    SpriteGroup::SpriteGroup(const std::string& path)
    {
        Cel::CelFile cel(path);

        for(size_t i = 0; i < cel.numFrames(); i++)
        {
            SDL_Surface* s = createTransparentSurface(cel[i].mWidth, cel[i].mHeight);
            drawFrame(s, 0, 0, cel[i]);

            mSprites.push_back(s);
        }

        mAnimLength = cel.animLength();
    }

    SpriteGroup::~SpriteGroup()
    {
        for(size_t i = 0; i < mSprites.size(); i++)
            SDL_FreeSurface((SDL_Surface*)mSprites[i]);
    }
    
    void blit(SDL_Surface* from, SDL_Surface* to, int x, int y)
    {
        SDL_Rect rcDest = { x, y, 0, 0 };
        SDL_BlitSurface (from , NULL, to, &rcDest );
    }

    void drawMinTile(SDL_Surface* s, Cel::CelFile& f, int x, int y, int16_t l, int16_t r)
    {
        if(l != -1)
        {
            drawFrame(s, x, y, f[l]);
            
            #ifdef CEL_DEBUG
                /*TTF_Font* font = TTF_OpenFont("FreeMonoBold.ttf", 8);
                SDL_Color foregroundColor = { 0, 0, 0 }; 
                SDL_Color backgroundColor = { 255, 255, 255 };
                SDL_Surface* textSurface = TTF_RenderText_Shaded(font, SSTR(l).c_str(), foregroundColor, backgroundColor);
               
                blit(textSurface, s, x, y);

                SDL_FreeSurface(textSurface);
                TTF_CloseFont(font);*/
            #endif
        }
        if(r != -1)
        {
            drawFrame(s, x+32, y, f[r]);

            #ifdef CEL_DEBUG
                /*TTF_Font* font = TTF_OpenFont("FreeMonoBold.ttf", 8);
                SDL_Color foregroundColor = { 255, 0, 0 }; 
                SDL_Color backgroundColor = { 255, 255, 255 };
                SDL_Surface* textSurface = TTF_RenderText_Shaded(font, SSTR(r).c_str(), foregroundColor, backgroundColor);
               
                blit(textSurface, s, x+32, y);

                SDL_FreeSurface(textSurface);
                TTF_CloseFont(font);*/
           #endif
        }
    }

    void drawMinPillar(SDL_Surface* s, int x, int y, const Level::MinPillar& pillar, Cel::CelFile& tileset)
    {
        // compensate for maps using 5-row min files
        if(pillar.size() == 10)
            y += 3*32;

        // Each iteration draw one row of the min
        for(int i = 0; i < pillar.size(); i+=2)
        {
            int16_t l = (pillar[i]&0x0FFF)-1;
            int16_t r = (pillar[i+1]&0x0FFF)-1;
            
            drawMinTile(s, tileset, x, y, l, r);
        
            y += 32; // down 32 each row
        }
    }

    RenderLevel* setLevel(const Level::Level& level, const std::string& tilesetPath)
    {
        Cel::CelFile town(tilesetPath);
        
        RenderLevel* retval = new RenderLevel();

        retval->levelSprite = SDL_CreateRGBSurface(SDL_HWSURFACE, ((level.width()+level.height()))*32, ((level.width()+level.height()))*16 + 224, screen->format->BitsPerPixel,
                                              screen->format->Rmask,
                                              screen->format->Gmask,
                                              screen->format->Bmask,
                                              screen->format->Amask);

        for(size_t x = 0; x < level.width(); x++)
        {
            for(size_t y = 0; y < level.height(); y++)
            {
                drawMinPillar((SDL_Surface*)retval->levelSprite, (y*(-32)) + 32*x + level.height()*32-32, (y*16) + 16*x, level[x][y], town);
            }
        }

        SDL_SaveBMP((SDL_Surface*)retval->levelSprite, "test.bmp");//TODO: should probably get rid of this at some point, useful for now

        retval->levelWidth = level.width();
        retval->levelHeight = level.height();

        return retval;
    }
    
    void drawLevel(RenderLevel* level, int32_t x1, int32_t y1, int32_t x2, int32_t y2, size_t dist)
    {
        int16_t xPx1 = -((y1*(-32)) + 32*x1 + level->levelWidth*32) +WIDTH/2;
        int16_t yPx1 = -((y1*16) + (16*x1) +160) + HEIGHT/2;

        int16_t xPx2 = -((y2*(-32)) + 32*x2 + level->levelWidth*32) +WIDTH/2;
        int16_t yPx2 = -((y2*16) + (16*x2) +160) + HEIGHT/2;

        level->levelX = xPx1 + ((((float)(xPx2-xPx1))/100.0)*(float)dist);
        level->levelY = yPx1 + ((((float)(yPx2-yPx1))/100.0)*(float)dist);

        //TODO clean up the magic numbers here, and elsewhere in this file
        blit((SDL_Surface*)level->levelSprite, screen, level->levelX, level->levelY);
    }
    
    void drawAt(RenderLevel* level, const Sprite& sprite, int32_t x1, int32_t y1, int32_t x2, int32_t y2, size_t dist)
    {
        int32_t xPx1 = ((y1*(-32)) + 32*x1 + level->levelWidth*32) + level->levelX -((SDL_Surface*)sprite)->w/2;
        int32_t yPx1 = ((y1*16) + (16*x1) +160) + level->levelY;

        int32_t xPx2 = ((y2*(-32)) + 32*x2 + level->levelWidth*32) + level->levelX -((SDL_Surface*)sprite)->w/2;
        int32_t yPx2 = ((y2*16) + (16*x2) +160) + level->levelY;

        int32_t x = xPx1 + ((((float)(xPx2-xPx1))/100.0)*(float)dist);
        int32_t y = yPx1 + ((((float)(yPx2-yPx1))/100.0)*(float)dist);

        drawAt(sprite, x, y);
    }

    void clear()
    {
        SDL_FillRect(screen,NULL, SDL_MapRGB( screen->format, 0, 0, 255)); 
    }

    RenderLevel::~RenderLevel()
    {
        SDL_FreeSurface((SDL_Surface*)levelSprite);
    }
}
