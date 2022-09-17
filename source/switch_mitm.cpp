/* -*- C++ -*-
 *
 *  switch_mitm - Intermediate functions for Switch interaction
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


#include <string>
#include "ONScripterLabel.h"
#include "ScriptParser.h"
#include "switch_mitm.h"

u32 prev_touchcount = 0;
bool ctrl_pressed;

void send_keyboard_event( SDL_Scancode scancode, SDL_Keycode keycode, bool press=true, bool release=true )
{
        SDL_Event event;
        SDL_KeyboardEvent keyboard_event;
        SDL_Keysym keysym;

        keysym.scancode = scancode;
        keysym.sym = keycode;
        // keysym.mod = KMOD_NONE;

        keyboard_event.type = SDL_KEYDOWN;
        // keyboard_event.windowID = window_id;
        // keyboard_event.state = SDL_PRESSED;
        // keyboard_event.repeat = 0;
        keyboard_event.keysym = keysym;

        event.key = keyboard_event;
        if (press)
        {
            event.type = SDL_KEYDOWN;

            SDL_PushEvent( &event );
        }
        if (release)
        {
            keyboard_event.type = SDL_KEYUP;
            // keyboard_event.state = SDL_RELEASED;
            event.type = SDL_KEYUP;

            SDL_PushEvent( &event );
        }

}

void switch_init()
{
    padConfigureInput( 1, HidNpadStyleSet_NpadStandard );

    padInitializeDefault( &pad );

    hidInitializeTouchScreen();

    Result rc = romfsInit();
    if ( R_FAILED( rc ) )
        fprintf( stderr, "romfsInit: %08X\n", rc );
}

void switch_deinit()
{
    romfsExit();
}

int switch_getinput( void* data )
{   
    ThreadData *vars = (ThreadData *)data;

    while (true) {
        if ( data && vars->break_flag )      // stop if break flag is set
            break;

        // time_t unix_time = time( NULL );
        // struct tm* time_struct = localtime( (const time_t *)&unix_time ) ;
        // int hh = time_struct->tm_hour;
        // int mm = time_struct->tm_min;
        // int ss = time_struct->tm_sec;
        // int dd = time_struct->tm_mday;
        // int MM = time_struct->tm_mon + 1;
        // int YY = time_struct->tm_year + 1900;

        // printf( "[%02i/%02i/%i %02i:%02i:%02i] Getting inputs...\n", dd, MM, YY, hh, mm, ss );
        padUpdate( &pad );

        u64 kDown = padGetButtonsDown( &pad );
        u64 kHold = padGetButtons( &pad );

        if ( kDown & HidNpadButton_Plus )
        {
            SDL_Event event;
            event.type = SDL_QUIT;

            SDL_PushEvent( &event );

            break;
        }

        if ( kDown & HidNpadButton_AnyUp && !( kDown & HidNpadButton_Up ) ) {
            send_keyboard_event( SDL_SCANCODE_H, SDLK_h );
            // send_keyboard_event( SDL_SCANCODE_UP, SDLK_UP );         // this breaks in-game choices menu
        }

        if ( kDown & HidNpadButton_AnyDown && !( kDown & HidNpadButton_Down ) ) {
            send_keyboard_event( SDL_SCANCODE_L, SDLK_l );
            // send_keyboard_event( SDL_SCANCODE_DOWN, SDLK_DOWN );     // this breaks in-game choices menu
        }

        if ( kDown & HidNpadButton_AnyLeft )
            send_keyboard_event( SDL_SCANCODE_H, SDLK_h );

        if ( kDown & HidNpadButton_AnyRight )
            send_keyboard_event( SDL_SCANCODE_L, SDLK_l );

        if ( kDown & HidNpadButton_Up || kDown & HidNpadButton_L )
            send_keyboard_event( SDL_SCANCODE_UP, SDLK_UP );

        if ( kDown & HidNpadButton_Down || kDown & HidNpadButton_R )
            send_keyboard_event( SDL_SCANCODE_DOWN, SDLK_DOWN );
        
        if ( kDown & HidNpadButton_A )
            send_keyboard_event( SDL_SCANCODE_RETURN, SDLK_RETURN );
        
        if ( kDown & HidNpadButton_B )
            if ( *vars->current_mode & 1 ||
                 *vars->current_mode & 256 )                                  // bind RETURN to B if in text mode
                send_keyboard_event( SDL_SCANCODE_RETURN, SDLK_RETURN );
            else
                send_keyboard_event(SDL_SCANCODE_ESCAPE, SDLK_ESCAPE );

        if ( kDown & HidNpadButton_X )
            send_keyboard_event( SDL_SCANCODE_ESCAPE, SDLK_ESCAPE );

        if ( kDown & HidNpadButton_Y )
            send_keyboard_event( SDL_SCANCODE_O, SDLK_o );

        if ( kDown & HidNpadButton_StickL )
            send_keyboard_event( SDL_SCANCODE_0, SDLK_0 );


        //          ============= Skip mode ===============
        if ( !ctrl_pressed &&
            kDown & HidNpadButton_ZL || kDown & HidNpadButton_ZR )
        {
            ctrl_pressed = true;
            send_keyboard_event( SDL_SCANCODE_LCTRL, SDLK_LCTRL, true, false );
        }

        if ( ctrl_pressed &&
            !( kHold & HidNpadButton_ZL ) && !( kHold & HidNpadButton_ZR ) )
        {
            ctrl_pressed = false;
            send_keyboard_event( SDL_SCANCODE_LCTRL, SDLK_LCTRL, false, true );
        }

    
        // HidTouchScreenState state = {0};
        // if ( hidGetTouchScreenStates( &state, 1 ) &&
        //      state.count != prev_touchcount )
        // {
        //     prev_touchcount = state.count;

        //     if ( state.count == 1 )
        //     {
        //         HidTouchState touch = state.touches[0];

        //         printf( "Touch registered at x:%4u y:%3u ", touch.x, touch.y );
        //         touch.x = (screen_width_wide * touch.x) / 1264;
        //         touch.y = (screen_height * touch.y) / 704;
        //         printf( "(adjusted: x=%3u y=%3u)\n", touch.x, touch.y );

        //         if ( touch.x >= bar_length[0] &&
        //              touch.x < screen_width_wide - bar_length[1] ) {
        //             printf( "Registered!\n" );
        //             SDL_Event event;
        //             SDL_MouseButtonEvent mouse_event;
        //             SDL_MouseMotionEvent motion_event;

        //             motion_event.state = 0;
        //             motion_event.x = touch.x - 107;
        //             motion_event.y = touch.y;

        //             event.type = SDL_MOUSEMOTION;
        //             event.motion = motion_event;

        //             SDL_PushEvent(&event);

        //             mouse_event.button = SDL_BUTTON_LEFT;
        //             mouse_event.state = SDL_PRESSED;
        //             mouse_event.clicks = 1;
        //             mouse_event.which = 0;
        //             mouse_event.x = touch.x - 107;
        //             mouse_event.y = touch.y;

        //             event.type = SDL_MOUSEBUTTONDOWN;
        //             event.button = mouse_event;

        //             SDL_PushEvent(&event);

        //             mouse_event.state = SDL_RELEASED;

        //             event.type = SDL_MOUSEBUTTONUP;
        //             event.button = mouse_event;

        //             SDL_PushEvent(&event);
        //         }
        //     }
        // }


        SDL_Delay( 10 );
    }

}

FILE *romfsOpen( const char *path, const char *mode )
{
    if ( *mode != 'r' ) return NULL;

    std::string path_str = path;

    // clean filename
    size_t pos = path_str.find( "\\\\" );
    if ( pos != std::string::npos )
        path_str.replace( pos, 2, "\\" );
    while (1)
    {
        pos = path_str.find("\\");
        if ( pos == std::string::npos ) break;
        path_str.replace( pos, 1, "/" );
    }

    path_str.insert( 0, "romfs:/" );

    FILE* fp = fopen( path_str.c_str(), mode );
    
    return fp;
}