/* -*- C++ -*-
 *
 *  switch_nofile_splash.cpp - "Files not found" error screen for ONScripter-NX
 *
 *  Copyright (c) 2022 clamintus.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "switch_mitm.h"
#include "version.h"
#include <unistd.h>

const int NUM_LABELS = 5;
const char* _SPLASH_TITLE_  = "Error while initializing ONScripter:\n ";
const char* _SPLASH_DESC_   = "No assets file found.\nPlease put your .nsa and .sar files in the app directory.\n\n\n\n\n\n\n\n\n ";
const char* _SPLASH_FOOTER_ = "Current directory: %s\nPress + to exit.";
const char* _CORNER_TEXT_1_  = "ONScripter-NX v%d.%d";
const char* _CORNER_TEXT_2_  = "by clamintus";

int switch_check_files()
{
    FILE* romfs_fp1 = fopen( "romfs:/arc.nsa", "rb" );
    FILE* romfs_fp2 = fopen( "romfs:/arc.sar", "rb" );
    FILE* fp1 = fopen( "arc.nsa", "rb" );
    FILE* fp2 = fopen( "arc.sar", "rb" );

    if (romfs_fp1) fclose(fp1);
    if (romfs_fp2) fclose(fp2);
    if (fp1) fclose(fp1);
    if (fp2) fclose(fp2);

    if ( !( romfs_fp1 || romfs_fp2 || fp1 || fp2 ) )
        return -1;
    
    return 0;
}

void switch_nofile_splash() {
    fprintf( stdout, "Cannot load arc.sar or arc.nsa! Stopping...\n" );
    SDL_Window *window;
    SDL_Surface *screen, *text[ NUM_LABELS ];
    TTF_Font *font;

    SDL_Init( SDL_INIT_VIDEO );
    if ( TTF_Init() < 0 ) SDL_Quit();

    window = SDL_CreateWindow( "Error",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               853,
                               480,
                               SDL_WINDOW_SHOWN );
    screen = SDL_GetWindowSurface( window );

    char* current_dir = getcwd( NULL, 0 );
    char* footer = new char[ strlen(_SPLASH_FOOTER_) + strlen(current_dir) - 1 ];
    sprintf( footer, _SPLASH_FOOTER_, current_dir );
    char* version_label = new char[ 256 ];
    sprintf( version_label, _CORNER_TEXT_1_, NX_VERSION / 100, NX_VERSION / 10 );
    if ( NX_VERSION % 10 != 0 )
    {
        sprintf( version_label + strlen( version_label ), ".%d", NX_VERSION % 10 );
    }

    FILE *fp = romfsOpen( "default.ttf", "rb" );
    SDL_Color white = { 0xFF, 0xFF, 0xFF };
    SDL_Color label = { 0xAA, 0xAA, 0xAA };
    SDL_Color label_dark = { 0x55, 0x55, 0x55 };
    SDL_RWops *font_rw = SDL_RWFromFP( fp, SDL_TRUE );
    font = TTF_OpenFontRW( font_rw, 1, 17 );

    text[0] = TTF_RenderText_Blended_Wrapped( font, _SPLASH_TITLE_, label, 853-107 );
    text[1] = TTF_RenderText_Blended_Wrapped( font, _SPLASH_DESC_, white, 853-107 );
    text[2] = TTF_RenderText_Blended_Wrapped( font, footer, label, 853-107 );
    text[3] = TTF_RenderText_Blended_Wrapped( font, version_label, label_dark, 853-107 );
    text[4] = TTF_RenderText_Blended_Wrapped( font, _CORNER_TEXT_2_, label_dark, 853-107 );

    SDL_FillRect( screen, &screen->clip_rect, SDL_MapRGBA( screen->format, 0, 0, 0, 0xFF ) );

    SDL_Rect rect;
    int ypos = 0;
    for ( int i=0; (i < 3) && (ypos < 480); i++ )
    {
        rect.x = 107;
        rect.y = 80 + ypos;
        rect.w = text[i]->w;
        rect.h = text[i]->h;
        if ( SDL_BlitSurface( text[i], NULL, screen, &rect ) != 0 )
            SDL_Quit();
        
        ypos += rect.h;
    }
    rect.x = 0;
    rect.y = 480 - text[3]->h;
    rect.w = text[3]->w;
    rect.h = text[3]->h;
    if ( SDL_BlitSurface( text[3], NULL, screen, &rect ) != 0 )
        SDL_Quit();
    rect.x = 853 - text[4]->w;
    rect.y = 480 - text[4]->h;
    rect.w = text[4]->w;
    rect.h = text[4]->h;
    if ( SDL_BlitSurface( text[4], NULL, screen, &rect ) != 0 )
        SDL_Quit();

    SDL_UpdateWindowSurface( window );

    while ( appletMainLoop() )
    {
        padUpdate( &pad );
        u64 keys = padGetButtonsDown( &pad );
        if ( keys & HidNpadButton_Plus )
            break;

        SDL_Delay(16);
    }

    for ( int i=0; i < NUM_LABELS; i++ ) SDL_FreeSurface( text[i] );
    TTF_CloseFont( font );
    SDL_Quit();
    exit(1);
}