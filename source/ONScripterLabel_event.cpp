/* -*- C++ -*-
 *
 *  ONScripterLabel_event.cpp - Event handler of ONScripter
 *
 *  Copyright (c) 2001-2006 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
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

#include "ONScripterLabel.h"
#if defined(LINUX)
#include <sys/types.h>
#include <sys/wait.h>
#endif
#if defined(__SWITCH__)
#include "switch_mitm.h"
#endif

#define ONS_TIMER_EVENT   (SDL_USEREVENT)
#define ONS_SOUND_EVENT   (SDL_USEREVENT+1)
#define ONS_CDAUDIO_EVENT (SDL_USEREVENT+2)
#define ONS_MIDI_EVENT    (SDL_USEREVENT+3)
#define ONS_WAVE_EVENT    (SDL_USEREVENT+4)
#define ONS_MUSIC_EVENT   (SDL_USEREVENT+5)

// This sets up the fadeout event flag for use in mp3 fadeout.  Recommend for integration.  [Seung Park, 20060621]
#if defined(INSANI)
#define ONS_FADE_EVENT    (SDL_USEREVENT+6)
#endif

#ifdef SDL2
#define FPS_TO_FRAMETIME(frames) 1000 / frames
#endif 

#define EDIT_MODE_PREFIX "[EDIT MODE]  "
#define EDIT_SELECT_STRING "MP3 vol (m)  SE vol (s)  Voice vol (v)  Numeric variable (n)"

static SDL_TimerID timer_id = NULL;
SDL_TimerID timer_cdaudio_id = NULL;

// This block does two things: it sets up the timer id for mp3 fadeout, and it also sets up a timer id for midi looping --
// the reason we have a separate midi loop timer id here is that on Mac OS X, looping midis via SDL will cause SDL itself
// to hard crash after the first play.  So, we work around that by manually causing the midis to loop.  This OS X midi
// workaround is the work of Ben Carter.  Recommend for integration.  [Seung Park, 20060621]
#if defined(INSANI)
SDL_TimerID timer_mp3fadeout_id = NULL;
#if defined(MACOSX)
SDL_TimerID timer_midi_id = NULL;
#endif
#endif
bool ext_music_play_once_flag = false;

extern long decodeOggVorbis(OVInfo *ovi, unsigned char *buf_dst, long len, bool do_rate_conversion);

/* **************************************** *
 * Callback functions
 * **************************************** */
extern "C" void mp3callback( void *userdata, Uint8 *stream, int len )
{
    if ( SMPEG_playAudio( (SMPEG*)userdata, stream, len ) == 0 ){
        SDL_Event event;
        event.type = ONS_SOUND_EVENT;
        SDL_PushEvent(&event);
    }
}

extern "C" void oggcallback( void *userdata, Uint8 *stream, int len )
{
    if (decodeOggVorbis((OVInfo*)userdata, stream, len, true) == 0){
        SDL_Event event;
        event.type = ONS_SOUND_EVENT;
        SDL_PushEvent(&event);
    }
}

extern "C" Uint32 SDLCALL timerCallback( Uint32 interval, void *param )
{
    SDL_RemoveTimer( timer_id );
    timer_id = NULL;

	SDL_Event event;
	event.type = ONS_TIMER_EVENT;
	SDL_PushEvent( &event );

    return interval;
}

extern "C" Uint32 cdaudioCallback( Uint32 interval, void *param )
{
    SDL_RemoveTimer( timer_cdaudio_id );
    timer_cdaudio_id = NULL;

    SDL_Event event;
    event.type = ONS_CDAUDIO_EVENT;
    SDL_PushEvent( &event );

    return interval;
}

// Pushes the mp3 fadeout event onto the stack.  Part of our mp3 fadeout enabling patch.  Recommend for integration.  [Seung Park, 20060621]
#if defined(INSANI)
extern "C" Uint32 SDLCALL mp3fadeoutCallback( Uint32 interval, void *param )
{
    SDL_Event event;
    event.type = ONS_FADE_EVENT;
    SDL_PushEvent( &event );

    return interval;
}
#endif

#ifdef SDL2
#define SDLKey SDL_Keycode
#endif

SDLKey transKey(SDLKey key)
{
#if defined(IPODLINUX)
 	switch(key){
      case SDLK_m:      key = SDLK_UP;      break; /* Menu                   */
      case SDLK_d:      key = SDLK_DOWN;    break; /* Play/Pause             */
      case SDLK_f:      key = SDLK_RIGHT;   break; /* Fast forward           */
      case SDLK_w:      key = SDLK_LEFT;    break; /* Rewind                 */
      case SDLK_RETURN: key = SDLK_RETURN;  break; /* Action                 */
      case SDLK_h:      key = SDLK_ESCAPE;  break; /* Hold                   */
      case SDLK_r:      key = SDLK_UNKNOWN; break; /* Wheel clockwise        */
      case SDLK_l:      key = SDLK_UNKNOWN; break; /* Wheel counterclockwise */
      default: break;
    }
#endif
    return key;
}

SDLKey transJoystickButton(Uint8 button)
{
#if defined(PSP)
    SDLKey button_map[] = { SDLK_ESCAPE, /* TRIANGLE */
                            SDLK_RETURN, /* CIRCLE   */
                            SDLK_SPACE,  /* CROSS    */
                            SDLK_RCTRL,  /* SQUARE   */
                            SDLK_o,      /* LTRIGGER */
                            SDLK_s,      /* RTRIGGER */
                            SDLK_DOWN,   /* DOWN     */
                            SDLK_LEFT,   /* LEFT     */
                            SDLK_UP,     /* UP       */
                            SDLK_RIGHT,  /* RIGHT    */
                            SDLK_0,      /* SELECT   */
                            SDLK_a,      /* START    */
                            SDLK_UNKNOWN,/* HOME     */ /* kernel mode only */
                            SDLK_UNKNOWN,/* HOLD     */};
    return button_map[button];
#endif
    return SDLK_UNKNOWN;
}

SDL_KeyboardEvent transJoystickAxis(SDL_JoyAxisEvent &jaxis)
{
    static int old_axis=-1;

    SDL_KeyboardEvent event;

    SDLKey axis_map[] = {SDLK_LEFT,  /* AL-LEFT  */
                         SDLK_RIGHT, /* AL-RIGHT */
                         SDLK_UP,    /* AL-UP    */
                         SDLK_DOWN   /* AL-DOWN  */};

    int axis = ((3200 > jaxis.value) && (jaxis.value > -3200) ? -1 :
                (jaxis.axis * 2 + (jaxis.value>0 ? 1 : 0) ));

    if (axis != old_axis){
        if (axis == -1){
            event.type = SDL_KEYUP;
            event.keysym.sym = axis_map[old_axis];
        }
        else{
            event.type = SDL_KEYDOWN;
            event.keysym.sym = axis_map[axis];
        }
        old_axis = axis;
    }
    else{
        event.keysym.sym = SDLK_UNKNOWN;
    }

    return event;
}

void ONScripterLabel::flushEventSub( SDL_Event &event )
{
    if ( event.type == ONS_SOUND_EVENT ){
        if ( music_play_loop_flag ||
             (cd_play_loop_flag && !cdaudio_flag ) ){
            stopBGM( true );
            if (music_file_name)
            playSound(music_file_name, SOUND_OGG_STREAMING|SOUND_MP3, true);
            else
                playCDAudio();
        }
        else{
            stopBGM( false );
        }
    }

// The event handler for the mp3 fadeout event itself.  Simply sets the volume of the mp3 being played lower and lower until it's 0,
// and until the requisite mp3 fadeout time has passed.  Recommend for integration.  [Seung Park, 20060621]
#if defined(INSANI)
    else if ( event.type == ONS_FADE_EVENT ){
		if (skip_flag || draw_one_page_flag || ctrl_pressed_status || skip_to_wait ) {
			mp3fadeout_duration = 0;
			if ( mp3_sample ) SMPEG_setvolume( mp3_sample, 0 );
		}
        Uint32 tmp = SDL_GetTicks() - mp3fadeout_start;
        if ( tmp < mp3fadeout_duration ) {
			tmp = mp3fadeout_duration - tmp;
            tmp *= music_volume;
			tmp /= mp3fadeout_duration;

            if ( mp3_sample ) SMPEG_setvolume( mp3_sample, tmp );
        } else {
            SDL_RemoveTimer( timer_mp3fadeout_id );
            timer_mp3fadeout_id = NULL;

            event_mode &= ~WAIT_TIMER_MODE;
            stopBGM( false );
            advancePhase();
        }
	}
#endif

    else if ( event.type == ONS_CDAUDIO_EVENT ){
        if ( cd_play_loop_flag ){
            stopBGM( true );
            playCDAudio();
        }
        else{
            stopBGM( false );
        }
    }
    else if ( event.type == ONS_MIDI_EVENT ){
#if defined(MACOSX) && defined(INSANI)
		if (!Mix_PlayingMusic())
		{
			ext_music_play_once_flag = !midi_play_loop_flag;
			Mix_FreeMusic( midi_info );
			playMIDI(midi_play_loop_flag);
		}
#else
		ext_music_play_once_flag = !midi_play_loop_flag;
		Mix_FreeMusic( midi_info );
		playMIDI(midi_play_loop_flag);
#endif
    }
    else if ( event.type == ONS_MUSIC_EVENT ){
        ext_music_play_once_flag = !music_play_loop_flag;
        Mix_FreeMusic(music_info);
        playExternalMusic(music_play_loop_flag);
    }
    else if ( event.type == ONS_WAVE_EVENT ){ // for processing btntim2 and automode correctly
        if ( wave_sample[event.user.code] ){
            Mix_FreeChunk( wave_sample[event.user.code] );
            wave_sample[event.user.code] = NULL;
            if (event.user.code == MIX_LOOPBGM_CHANNEL0 &&
                loop_bgm_name[1] &&
                wave_sample[MIX_LOOPBGM_CHANNEL1])
                Mix_PlayChannel(MIX_LOOPBGM_CHANNEL1,
                                wave_sample[MIX_LOOPBGM_CHANNEL1], -1);
        }
    }
}

void ONScripterLabel::flushEvent()
{
    SDL_Event event;
    while( SDL_PollEvent( &event ) )
        flushEventSub( event );
}

void ONScripterLabel::startTimer( int count )
{
    int duration = proceedAnimation();

    if ( duration > 0 && duration < count ){
        resetRemainingTime( duration );
        advancePhase( duration );
        remaining_time = count;
    }
    else{
        advancePhase( count );
        remaining_time = 0;
    }
    event_mode |= WAIT_TIMER_MODE;
}

void ONScripterLabel::advancePhase( int count )
{
    if ( timer_id != NULL ){
        SDL_RemoveTimer( timer_id );
    }

    if (count > 0){
        timer_id = SDL_AddTimer( count, timerCallback, NULL );
        if (timer_id != NULL) return;
    }

        SDL_Event event;
        event.type = ONS_TIMER_EVENT;
        SDL_PushEvent( &event );
}

void midiCallback( int sig )
{
#if defined(LINUX)
    int status;
    wait( &status );
#endif
    if ( !ext_music_play_once_flag ){
    SDL_Event event;
    event.type = ONS_MIDI_EVENT;
    SDL_PushEvent(&event);
    }
}

// Pushes the midi loop event onto the stack.  Part of a workaround for ONScripter
// crashing in Mac OS X after a midi is looped for the first time.  Recommend for
// integration.  This is the work of Ben Carter.  [Seung Park, 20060621]
#if defined(MACOSX) && defined(INSANI)
extern "C" Uint32 midiSDLCallback( Uint32 interval, void *param )
{
	SDL_Event event;
	event.type = ONS_MIDI_EVENT;
	SDL_PushEvent( &event );
	return interval;
}
#endif

extern "C" void waveCallback( int channel )
{
    SDL_Event event;
    event.type = ONS_WAVE_EVENT;
    event.user.code = channel;
    SDL_PushEvent(&event);
}

void musicCallback( int sig )
{
#if defined(LINUX)
    int status;
    wait( &status );
#endif
    if ( !ext_music_play_once_flag ){
        SDL_Event event;
        event.type = ONS_MUSIC_EVENT;
        SDL_PushEvent(&event);
    }
}

void ONScripterLabel::trapHandler()
{
    trap_mode = TRAP_NONE;
    setCurrentLabel( trap_dist );
    readToken();
    stopAnimation( clickstr_state );
    event_mode = IDLE_EVENT_MODE;
    advancePhase();
}

/* **************************************** *
 * Event handlers
 * **************************************** */
void ONScripterLabel::mouseMoveEvent( SDL_MouseMotionEvent *event )
{
    current_button_state.x = event->x;
    current_button_state.y = event->y;

    if ( event_mode & WAIT_BUTTON_MODE )
        mouseOverCheck( current_button_state.x, current_button_state.y );
}

#ifdef __SWITCH__

void ONScripterLabel::fingerPressEvent( SDL_TouchFingerEvent *event )
{
    if ( variable_edit_mode ) return;

    if ( automode_flag ){
        remaining_time = -1;
        automode_flag = false;
        return;
    }
    if ( trap_mode & TRAP_RIGHT_CLICK ){
        trapHandler();
        return;
    }
    if ( trap_mode & TRAP_LEFT_CLICK ){
        trapHandler();
        return;
    }

    int new_x = (float)screen_width_wide * event->x;
    int new_y = (float)screen_height * event->y;
    if ( new_x < bar_length[0] || new_x > screen_width_wide - bar_length[1] )
        return;

    current_button_state.x = new_x;
    current_button_state.y = new_y;
    current_button_state.down_flag = false;
    skip_flag = false;

    // if ( (rmode_flag && event_mode & WAIT_TEXT_MODE ||
    //       event_mode & WAIT_BUTTON_MODE) ){
    //     current_button_state.button = -1;
    //     volatile_button_state.button = -1;
    //     if (event_mode & WAIT_TEXT_MODE){
    //         if (!root_rmenu_link.next)
    //             system_menu_mode = SYSTEM_WINDOWERASE;
    //     }
    // }
    if ( event->type == SDL_FINGERUP || btndown_flag ){
        current_button_state.button = current_over_button;
        volatile_button_state.button = current_over_button;
#if defined(INSANI)
		//fprintf(stderr, "event_mode = %d\n", event_mode);
		if ( event_mode & WAIT_SLEEP_MODE) skip_to_wait=1;
#endif
        if ( event->type == SDL_FINGERUP )
            current_button_state.down_flag = true;
    }
    else return;

    if (event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE)){
        playClickVoice();
        stopAnimation( clickstr_state );
        advancePhase();
    }
}

void ONScripterLabel::fingerMoveEvent( SDL_TouchFingerEvent *event )
{
    int new_x = (float)screen_width_wide * event->x;
    int new_y = (float)screen_height * event->y;
    if ( new_x < bar_length[0] || new_x > screen_width_wide - bar_length[1] )
        return;

    current_button_state.x = new_x - bar_length[0];
    current_button_state.y = new_y;

    if ( event_mode & WAIT_BUTTON_MODE )
        mouseOverCheck( current_button_state.x, current_button_state.y );
}
#endif

void ONScripterLabel::mousePressEvent( SDL_MouseButtonEvent *event )
{
    if ( variable_edit_mode ) return;

    if ( automode_flag ){
        remaining_time = -1;
        automode_flag = false;
        return;
    }
    if ( event->button == SDL_BUTTON_RIGHT &&
         trap_mode & TRAP_RIGHT_CLICK ){
        trapHandler();
        return;
    }
    else if ( event->button == SDL_BUTTON_LEFT &&
              trap_mode & TRAP_LEFT_CLICK ){
        trapHandler();
        return;
    }

    current_button_state.x = event->x;
    current_button_state.y = event->y;
    current_button_state.down_flag = false;
    skip_flag = false;

    if ( event->button == SDL_BUTTON_RIGHT &&
         event->type == SDL_MOUSEBUTTONUP &&
         (rmode_flag && event_mode & WAIT_TEXT_MODE ||
          event_mode & WAIT_BUTTON_MODE) ){
        current_button_state.button = -1;
        volatile_button_state.button = -1;
        if (event_mode & WAIT_TEXT_MODE){
            if (root_rmenu_link.next)
                system_menu_mode = SYSTEM_MENU;
            else
                system_menu_mode = SYSTEM_WINDOWERASE;
        }
    }
    else if ( event->button == SDL_BUTTON_LEFT &&
              ( event->type == SDL_MOUSEBUTTONUP || btndown_flag ) ){
        current_button_state.button = current_over_button;
        volatile_button_state.button = current_over_button;
#if defined(INSANI)
		//fprintf(stderr, "event_mode = %d\n", event_mode);
		if ( event_mode & WAIT_SLEEP_MODE) skip_to_wait=1;
#endif
        if ( event->type == SDL_MOUSEBUTTONDOWN )
            current_button_state.down_flag = true;
    }
#if !defined(SDL2) && SDL_VERSION_ATLEAST(1, 2, 5)
    else if (event->button == SDL_BUTTON_WHEELUP &&
             (event_mode & WAIT_TEXT_MODE ||
              usewheel_flag && event_mode & WAIT_BUTTON_MODE ||
              system_menu_mode == SYSTEM_LOOKBACK)){
        current_button_state.button = -2;
        volatile_button_state.button = -2;
        if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
    }
    else if ( event->button == SDL_BUTTON_WHEELDOWN &&
              (enable_wheeldown_advance_flag && event_mode & WAIT_TEXT_MODE ||
               usewheel_flag && event_mode & WAIT_BUTTON_MODE||
               system_menu_mode == SYSTEM_LOOKBACK ) ){
        if (event_mode & WAIT_TEXT_MODE){
            current_button_state.button = 0;
            volatile_button_state.button = 0;
        }
        else{
            current_button_state.button = -3;
            volatile_button_state.button = -3;
        }
    }
#endif
    else return;

    if (event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE)){
        playClickVoice();
        stopAnimation( clickstr_state );
        advancePhase();
    }
}

#ifdef SDL2
void ONScripterLabel::mouseWheelEvent( SDL_MouseWheelEvent *event )
{
    if ( variable_edit_mode ) return;

    if ( automode_flag ){
        remaining_time = -1;
        automode_flag = false;
        return;
    }

    current_button_state.x = event->x;
    current_button_state.y = event->y;
    current_button_state.down_flag = false;
    skip_flag = false;

    if ( event->y > 0 &&
         (event_mode & WAIT_TEXT_MODE ||
          usewheel_flag && event_mode & WAIT_BUTTON_MODE ||
          system_menu_mode == SYSTEM_LOOKBACK)){
        current_button_state.button = -2;
        volatile_button_state.button = -2;
        if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
    }
    else if ( event->y < 0 &&
              (enable_wheeldown_advance_flag && event_mode & WAIT_TEXT_MODE ||
               usewheel_flag && event_mode & WAIT_BUTTON_MODE||
               system_menu_mode == SYSTEM_LOOKBACK ) ){
        if (event_mode & WAIT_TEXT_MODE){
            current_button_state.button = 0;
            volatile_button_state.button = 0;
        }
        else{
            current_button_state.button = -3;
            volatile_button_state.button = -3;
        }
    }
    else return;

    if (event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE)){
        playClickVoice();
        stopAnimation( clickstr_state );
        advancePhase();
    }
}
#endif

void ONScripterLabel::variableEditMode( SDL_KeyboardEvent *event )
{
    int  i;
    char *var_name, var_index[12];

    switch ( event->keysym.sym ) {
      case SDLK_m:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_MP3_VOLUME_MODE;
        variable_edit_num = music_volume;
        break;

      case SDLK_s:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_SE_VOLUME_MODE;
        variable_edit_num = se_volume;
        break;

      case SDLK_v:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_VOICE_VOLUME_MODE;
        variable_edit_num = voice_volume;
        break;

      case SDLK_n:
        if ( variable_edit_mode != EDIT_SELECT_MODE ) return;
        variable_edit_mode = EDIT_VARIABLE_INDEX_MODE;
        variable_edit_num = 0;
        break;

    #ifdef SDL2
      #define SDLK_KP9 SDLK_KP_9
      #define SDLK_KP8 SDLK_KP_8
      #define SDLK_KP7 SDLK_KP_7
      #define SDLK_KP6 SDLK_KP_6
      #define SDLK_KP5 SDLK_KP_5
      #define SDLK_KP4 SDLK_KP_4
      #define SDLK_KP3 SDLK_KP_3
      #define SDLK_KP2 SDLK_KP_2
      #define SDLK_KP1 SDLK_KP_1
      #define SDLK_KP0 SDLK_KP_0
    #endif

      case SDLK_9: case SDLK_KP9: variable_edit_num = variable_edit_num * 10 + 9; break;
      case SDLK_8: case SDLK_KP8: variable_edit_num = variable_edit_num * 10 + 8; break;
      case SDLK_7: case SDLK_KP7: variable_edit_num = variable_edit_num * 10 + 7; break;
      case SDLK_6: case SDLK_KP6: variable_edit_num = variable_edit_num * 10 + 6; break;
      case SDLK_5: case SDLK_KP5: variable_edit_num = variable_edit_num * 10 + 5; break;
      case SDLK_4: case SDLK_KP4: variable_edit_num = variable_edit_num * 10 + 4; break;
      case SDLK_3: case SDLK_KP3: variable_edit_num = variable_edit_num * 10 + 3; break;
      case SDLK_2: case SDLK_KP2: variable_edit_num = variable_edit_num * 10 + 2; break;
      case SDLK_1: case SDLK_KP1: variable_edit_num = variable_edit_num * 10 + 1; break;
      case SDLK_0: case SDLK_KP0: variable_edit_num = variable_edit_num * 10 + 0; break;

      case SDLK_MINUS: case SDLK_KP_MINUS:
        if ( variable_edit_mode == EDIT_VARIABLE_NUM_MODE && variable_edit_num == 0 ) variable_edit_sign = -1;
        break;

      case SDLK_BACKSPACE:
        if ( variable_edit_num ) variable_edit_num /= 10;
        else if ( variable_edit_sign == -1 ) variable_edit_sign = 1;
        break;

      case SDLK_RETURN: case SDLK_KP_ENTER:
        switch( variable_edit_mode ){

          case EDIT_VARIABLE_INDEX_MODE:
            variable_edit_index = variable_edit_num;
            variable_edit_num = script_h.variable_data[ variable_edit_index ].num;
            if ( variable_edit_num < 0 ){
                variable_edit_num = -variable_edit_num;
                variable_edit_sign = -1;
            }
            else{
                variable_edit_sign = 1;
            }
            break;

          case EDIT_VARIABLE_NUM_MODE:
            script_h.setNumVariable( variable_edit_index, variable_edit_sign * variable_edit_num );
            break;

          case EDIT_MP3_VOLUME_MODE:
            music_volume = variable_edit_num;
            if ( mp3_sample ) SMPEG_setvolume( mp3_sample, music_volume );
            break;

          case EDIT_SE_VOLUME_MODE:
            se_volume = variable_edit_num;
            for ( i=1 ; i<ONS_MIX_CHANNELS ; i++ )
                if ( wave_sample[i] ) Mix_Volume( i, se_volume * 128 / 100 );
            if ( wave_sample[MIX_LOOPBGM_CHANNEL0] ) Mix_Volume( MIX_LOOPBGM_CHANNEL0, se_volume * 128 / 100 );
            if ( wave_sample[MIX_LOOPBGM_CHANNEL1] ) Mix_Volume( MIX_LOOPBGM_CHANNEL1, se_volume * 128 / 100 );
            break;

          case EDIT_VOICE_VOLUME_MODE:
            voice_volume = variable_edit_num;
            if ( wave_sample[0] ) Mix_Volume( 0, se_volume * 128 / 100 );

          default:
            break;
        }
        if ( variable_edit_mode == EDIT_VARIABLE_INDEX_MODE )
            variable_edit_mode = EDIT_VARIABLE_NUM_MODE;
        else
            variable_edit_mode = EDIT_SELECT_MODE;
        break;

      case SDLK_ESCAPE:
        if ( variable_edit_mode == EDIT_SELECT_MODE ){
            variable_edit_mode = NOT_EDIT_MODE;
        #ifdef SDL2
            SDL_SetWindowTitle( window, DEFAULT_WM_TITLE );
            SDL_Delay( 100 );
            SDL_SetWindowTitle( window, wm_title_string );
        #else
            SDL_WM_SetCaption( DEFAULT_WM_TITLE, DEFAULT_WM_ICON );
            SDL_Delay( 100 );
            SDL_WM_SetCaption( wm_title_string, wm_icon_string );
        #endif
            return;
        }
        variable_edit_mode = EDIT_SELECT_MODE;

      default:
        break;
    }

    if ( variable_edit_mode == EDIT_SELECT_MODE ){
        sprintf( wm_edit_string, "%s%s", EDIT_MODE_PREFIX, EDIT_SELECT_STRING );
    }
    else if ( variable_edit_mode == EDIT_VARIABLE_INDEX_MODE ) {
        sprintf( wm_edit_string, "%s%s%d", EDIT_MODE_PREFIX, "Variable Index?  %", variable_edit_sign * variable_edit_num );
    }
    else if ( variable_edit_mode >= EDIT_VARIABLE_NUM_MODE ){
        int p=0;

        switch( variable_edit_mode ){

          case EDIT_VARIABLE_NUM_MODE:
            sprintf( var_index, "%%%d", variable_edit_index );
            var_name = var_index; p = script_h.variable_data[ variable_edit_index ].num; break;

          case EDIT_MP3_VOLUME_MODE:
            var_name = "MP3 Volume"; p = music_volume; break;

          case EDIT_VOICE_VOLUME_MODE:
            var_name = "Voice Volume"; p = voice_volume; break;

          case EDIT_SE_VOLUME_MODE:
            var_name = "Sound effect Volume"; p = se_volume; break;

          default:
            var_name = "";
        }
        sprintf( wm_edit_string, "%sCurrent %s=%d  New value? %s%d",
                 EDIT_MODE_PREFIX, var_name, p, (variable_edit_sign==1)?"":"-", variable_edit_num );
    }

#ifdef SDL2
    SDL_SetWindowTitle( window, wm_edit_string );
#else
    SDL_WM_SetCaption( wm_edit_string, wm_icon_string );
#endif
}

void ONScripterLabel::shiftCursorOnButton( int diff )
{
    int i;
    shortcut_mouse_line += diff;
    if ( shortcut_mouse_line < 0 ) shortcut_mouse_line = 0;

    ButtonLink *button = root_button_link.next;

    for ( i=0 ; i<shortcut_mouse_line && button ; i++ )
        button = button->next;

    if ( !button ){
        if ( diff == -1 )
            shortcut_mouse_line = 0;
        else
            shortcut_mouse_line = i-1;

        button = root_button_link.next;
        for ( i=0 ; i<shortcut_mouse_line ; i++ )
            button  = button->next;
    }
    if ( button ){
    #ifdef SDL2
        SDL_WarpMouseInWindow( window,
                               button->select_rect.x + button->select_rect.w / 2,
                               button->select_rect.y + button->select_rect.h / 2 );
    #else
        SDL_WarpMouse( button->select_rect.x + button->select_rect.w / 2,
                       button->select_rect.y + button->select_rect.h / 2 );
    #endif
    }
}

void ONScripterLabel::keyDownEvent( SDL_KeyboardEvent *event )
{
    switch ( event->keysym.sym ) {
      case SDLK_RCTRL:
        ctrl_pressed_status  |= 0x01;
        goto ctrl_pressed;
      case SDLK_LCTRL:
        ctrl_pressed_status  |= 0x02;
        goto ctrl_pressed;
      case SDLK_RSHIFT:
        shift_pressed_status |= 0x01;
        break;
      case SDLK_LSHIFT:
        shift_pressed_status |= 0x02;
        break;
      default:
        break;
    }

    return;

  ctrl_pressed:
    current_button_state.button  = 0;
    volatile_button_state.button = 0;
    playClickVoice();
    stopAnimation( clickstr_state );
    advancePhase();
    return;
}

void ONScripterLabel::keyUpEvent( SDL_KeyboardEvent *event )
{
    switch ( event->keysym.sym ) {
      case SDLK_RCTRL:
        ctrl_pressed_status  &= ~0x01;
        break;
      case SDLK_LCTRL:
        ctrl_pressed_status  &= ~0x02;
        break;
      case SDLK_RSHIFT:
        shift_pressed_status &= ~0x01;
        break;
      case SDLK_LSHIFT:
        shift_pressed_status &= ~0x02;
        break;
      default:
        break;
    }
}

void ONScripterLabel::keyPressEvent( SDL_KeyboardEvent *event )
{
    current_button_state.button = 0;
    if ( automode_flag ){
        remaining_time = -1;
        automode_flag = false;
        return;
    }

    if ( event->type == SDL_KEYUP ){
        if ( variable_edit_mode ){
            variableEditMode( event );
            return;
        }

        if ( edit_flag && event->keysym.sym == SDLK_z ){
            variable_edit_mode = EDIT_SELECT_MODE;
            variable_edit_sign = 1;
            variable_edit_num = 0;
            sprintf( wm_edit_string, "%s%s", EDIT_MODE_PREFIX, EDIT_SELECT_STRING );
        #ifdef SDL2
            SDL_SetWindowTitle( window, wm_edit_string );
        #else
            SDL_WM_SetCaption( wm_edit_string, wm_icon_string );
        #endif
        }
    }

    if (event->type == SDL_KEYUP &&
        (event->keysym.sym == SDLK_RETURN ||
         event->keysym.sym == SDLK_KP_ENTER ||
         event->keysym.sym == SDLK_SPACE ||
         event->keysym.sym == SDLK_s))
            skip_flag = false;

    if ( shift_pressed_status && event->keysym.sym == SDLK_q && current_mode == NORMAL_MODE ){
        endCommand();
    }

    if ( (trap_mode & TRAP_LEFT_CLICK) &&
         (event->keysym.sym == SDLK_RETURN ||
          event->keysym.sym == SDLK_KP_ENTER ||
          event->keysym.sym == SDLK_SPACE ) ){
        trapHandler();
        return;
    }
    else if ( (trap_mode & TRAP_RIGHT_CLICK) &&
              (event->keysym.sym == SDLK_ESCAPE) ){
        trapHandler();
        return;
    }

    if ( event_mode & WAIT_BUTTON_MODE &&
         ( event->type == SDL_KEYUP ||
           ( btndown_flag && event->keysym.sym == SDLK_RETURN ||
             btndown_flag && event->keysym.sym == SDLK_KP_ENTER) ) ){
        if (!getcursor_flag && event->keysym.sym == SDLK_LEFT ||
            event->keysym.sym == SDLK_h){

            shiftCursorOnButton( 1 );
            return;
        }
        else if (!getcursor_flag && event->keysym.sym == SDLK_RIGHT ||
                 event->keysym.sym == SDLK_l){

            shiftCursorOnButton( -1 );
            return;
        }
        else if ( ( !getenter_flag  && event->keysym.sym == SDLK_RETURN ) ||
                  ( !getenter_flag  && event->keysym.sym == SDLK_KP_ENTER ) ||
                  ( (spclclk_flag || !useescspc_flag) && event->keysym.sym == SDLK_SPACE  ) ){
            if ( event->keysym.sym == SDLK_RETURN ||
                 event->keysym.sym == SDLK_KP_ENTER ||
                 spclclk_flag && event->keysym.sym == SDLK_SPACE ){
                current_button_state.button = current_over_button;
                volatile_button_state.button = current_over_button;
                if ( event->type == SDL_KEYDOWN )
                    current_button_state.down_flag = true;
                else
                    current_button_state.down_flag = false;
            }
            else{
                current_button_state.button = 0;
                volatile_button_state.button = 0;
            }
            playClickVoice();
            stopAnimation( clickstr_state );
            advancePhase();
            return;
        }
    }

    if ( event->type == SDL_KEYDOWN ) return;

    if ( ( event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) ) &&
         ( autoclick_time == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        if ( !useescspc_flag && event->keysym.sym == SDLK_ESCAPE){
            current_button_state.button  = -1;
            if (rmode_flag && event_mode & WAIT_TEXT_MODE){
                if (root_rmenu_link.next)
                    system_menu_mode = SYSTEM_MENU;
                else
                    system_menu_mode = SYSTEM_WINDOWERASE;
            }
        }
        else if ( useescspc_flag && event->keysym.sym == SDLK_ESCAPE ){
            current_button_state.button  = -10;
        }
        else if ( !spclclk_flag && useescspc_flag && event->keysym.sym == SDLK_SPACE ){
            current_button_state.button  = -11;
        }
        else if ((event_mode & WAIT_TEXT_MODE ||
                  usewheel_flag && event_mode & WAIT_BUTTON_MODE||
                  system_menu_mode == SYSTEM_LOOKBACK) &&
                 (!getcursor_flag && event->keysym.sym == SDLK_UP ||
                  event->keysym.sym == SDLK_k)){
            current_button_state.button = -2;
            volatile_button_state.button = -2;
            if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
        }
        else if ((enable_wheeldown_advance_flag && event_mode & WAIT_TEXT_MODE ||
                  usewheel_flag && event_mode & WAIT_BUTTON_MODE||
                  system_menu_mode == SYSTEM_LOOKBACK) &&
                 (!getcursor_flag && event->keysym.sym == SDLK_DOWN ||
                  event->keysym.sym == SDLK_j)){
            if (event_mode & WAIT_TEXT_MODE){
                current_button_state.button = 0;
                volatile_button_state.button = 0;
            }
            else{
                current_button_state.button = -3;
                volatile_button_state.button = -3;
            }
        }
        else if ( getpageup_flag && event->keysym.sym == SDLK_PAGEUP ){
            current_button_state.button  = -12;
        }
        else if ( getpagedown_flag && event->keysym.sym == SDLK_PAGEDOWN ){
            current_button_state.button  = -13;
        }
        else if ( getenter_flag && event->keysym.sym == SDLK_RETURN ||
                  getenter_flag && event->keysym.sym == SDLK_KP_ENTER ){
            current_button_state.button  = -19;
        }
        else if ( gettab_flag && event->keysym.sym == SDLK_TAB ){
            current_button_state.button  = -20;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_UP ){
            current_button_state.button  = -40;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_RIGHT ){
            current_button_state.button  = -41;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_DOWN ){
            current_button_state.button  = -42;
        }
        else if ( getcursor_flag && event->keysym.sym == SDLK_LEFT ){
            current_button_state.button  = -43;
        }
        else if ( getinsert_flag && event->keysym.sym == SDLK_INSERT ){
            current_button_state.button  = -50;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_z ){
            current_button_state.button  = -51;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_x ){
            current_button_state.button  = -52;
        }
        else if ( getzxc_flag && event->keysym.sym == SDLK_c ){
            current_button_state.button  = -53;
        }
        else if ( getfunction_flag ){
            if      ( event->keysym.sym == SDLK_F1 )
                current_button_state.button = -21;
            else if ( event->keysym.sym == SDLK_F2 )
                current_button_state.button = -22;
            else if ( event->keysym.sym == SDLK_F3 )
                current_button_state.button = -23;
            else if ( event->keysym.sym == SDLK_F4 )
                current_button_state.button = -24;
            else if ( event->keysym.sym == SDLK_F5 )
                current_button_state.button = -25;
            else if ( event->keysym.sym == SDLK_F6 )
                current_button_state.button = -26;
            else if ( event->keysym.sym == SDLK_F7 )
                current_button_state.button = -27;
            else if ( event->keysym.sym == SDLK_F8 )
                current_button_state.button = -28;
            else if ( event->keysym.sym == SDLK_F9 )
                current_button_state.button = -29;
            else if ( event->keysym.sym == SDLK_F10 )
                current_button_state.button = -30;
            else if ( event->keysym.sym == SDLK_F11 )
                current_button_state.button = -31;
            else if ( event->keysym.sym == SDLK_F12 )
                current_button_state.button = -32;
        }
        if ( current_button_state.button != 0 ){
            volatile_button_state.button = current_button_state.button;
            stopAnimation( clickstr_state );
            advancePhase();
            return;
        }
    }

    if ( event_mode & WAIT_INPUT_MODE && !key_pressed_flag &&
         ( autoclick_time == 0 || (event_mode & WAIT_BUTTON_MODE)) ){
        if (event->keysym.sym == SDLK_RETURN ||
            event->keysym.sym == SDLK_KP_ENTER ||
            event->keysym.sym == SDLK_SPACE ){
            key_pressed_flag = true;
            playClickVoice();
            stopAnimation( clickstr_state );
            advancePhase();
        }
    }

    if ( event_mode & (WAIT_INPUT_MODE | WAIT_TEXTBTN_MODE) &&
         !key_pressed_flag ){
        if (event->keysym.sym == SDLK_s && !automode_flag ){
            skip_flag = true;
            printf("toggle skip to true\n");
            key_pressed_flag = true;
            stopAnimation( clickstr_state );
            advancePhase();
        }
        else if (event->keysym.sym == SDLK_o){
            draw_one_page_flag = !draw_one_page_flag;
            printf("toggle draw one page flag to %s\n", (draw_one_page_flag?"true":"false") );
            if ( draw_one_page_flag ){
                stopAnimation( clickstr_state );
                advancePhase();
            }
        }
        else if ( event->keysym.sym == SDLK_a && mode_ext_flag && !automode_flag ){
            automode_flag = true;
            skip_flag = false;
            printf("change to automode\n");
            key_pressed_flag = true;
            stopAnimation( clickstr_state );
            advancePhase();
        }
        else if ( event->keysym.sym == SDLK_0 ){
            if (++text_speed_no > 2) text_speed_no = 0;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_1 ){
            text_speed_no = 0;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_2 ){
            text_speed_no = 1;
            sentence_font.wait_time = -1;
        }
        else if ( event->keysym.sym == SDLK_3 ){
            text_speed_no = 2;
            sentence_font.wait_time = -1;
        }
    }

    if ( event_mode & ( WAIT_INPUT_MODE | WAIT_BUTTON_MODE ) ){
        if ( event->keysym.sym == SDLK_f ){
            if ( fullscreen_mode ) menu_windowCommand();
            else                   menu_fullCommand();
        }
    }

#if defined(INSANI)
		if ( event_mode & WAIT_SLEEP_MODE ) {
			if ( event->keysym.sym == SDLK_RETURN || event->keysym.sym == SDLK_KP_ENTER || event->keysym.sym == SDLK_SPACE ) skip_to_wait = 1;
		}
#endif

    if ((event_mode & WAIT_TEXT_MODE ||
         usewheel_flag && event_mode & WAIT_BUTTON_MODE||
         system_menu_mode == SYSTEM_LOOKBACK) &&
        (!getcursor_flag && event->keysym.sym == SDLK_UP ||
         event->keysym.sym == SDLK_k)){
        current_button_state.button = -2;
        volatile_button_state.button = -2;
        if (event_mode & WAIT_TEXT_MODE) system_menu_mode = SYSTEM_LOOKBACK;
    }
    else if ((enable_wheeldown_advance_flag && event_mode & WAIT_TEXT_MODE ||
              usewheel_flag && event_mode & WAIT_BUTTON_MODE||
              system_menu_mode == SYSTEM_LOOKBACK) &&
             (!getcursor_flag && event->keysym.sym == SDLK_DOWN ||
              event->keysym.sym == SDLK_j)){
        if (event_mode & WAIT_TEXT_MODE){
            current_button_state.button = 0;
            volatile_button_state.button = 0;
        }
        else{
            current_button_state.button = -3;
            volatile_button_state.button = -3;
        }
    }
}

void ONScripterLabel::timerEvent( void )
{
  timerEventTop:

    int ret;

    if ( event_mode & WAIT_TIMER_MODE ){
        int duration = proceedAnimation();

        if ( duration == 0 ||
             ( remaining_time >= 0 &&
               remaining_time-duration <= 0 ) ){

            bool end_flag = true;
            bool loop_flag = false;
            if ( remaining_time >= 0 ){
                remaining_time = -1;
                if ( event_mode & WAIT_VOICE_MODE && wave_sample[0] ){
                    end_flag = false;
                    if ( duration > 0 ){
                        resetRemainingTime( duration );
                        advancePhase( duration );
                    }
                }
                else{
                    loop_flag = true;
                    if ( automode_flag )
                        current_button_state.button = 0;
                    else if ( usewheel_flag )
                        current_button_state.button = -5;
                    else
                        current_button_state.button = -2;
                }
            }

            if ( end_flag &&
                 event_mode & (WAIT_INPUT_MODE | WAIT_BUTTON_MODE) &&
                 ( clickstr_state == CLICK_WAIT ||
                   clickstr_state == CLICK_NEWPAGE ) ){
                playClickVoice();
                stopAnimation( clickstr_state );
            }

            if ( end_flag || duration == 0 )
                event_mode &= ~WAIT_TIMER_MODE;
            if ( loop_flag ) goto timerEventTop;
        }
        else{
            if ( remaining_time > 0 )
                remaining_time -= duration;
            resetRemainingTime( duration );
            advancePhase( duration );
        }
    }
    else if ( event_mode & EFFECT_EVENT_MODE ){
        char *current = script_h.getCurrent();
        ret = this->parseLine();

        if ( ret & RET_CONTINUE ){
            if ( ret == RET_CONTINUE ){
                readToken(); // skip tailing \0 and mark kidoku
            }
            if ( effect_blank == 0 || effect_counter == 0 ) goto timerEventTop;
            startTimer( effect_blank );
        }
        else{
            script_h.setCurrent( current );
            readToken();
            advancePhase();
        }

        #ifdef SDL2
            // if ( !ctrl_pressed_status )
                // SDL_Delay( FPS_TO_FRAMETIME( 83 ) );
        #endif

        return;
    }
    else{
        if ( system_menu_mode != SYSTEM_NULL ||
             ( event_mode & WAIT_INPUT_MODE &&
               volatile_button_state.button == -1 ) ){
            if ( !system_menu_enter_flag )
                event_mode |= WAIT_TIMER_MODE;
            executeSystemCall();
        }
        else
            executeLabel();
    }
    volatile_button_state.button = 0;
}

/* **************************************** *
 * Event loop
 * **************************************** */
int ONScripterLabel::eventLoop()
{
    SDL_Event event, tmp_event;

    advancePhase();

#ifdef __SWITCH__
    thread_data = (ThreadData *)malloc( sizeof(ThreadData) );

    thread_data->break_flag = false;
    thread_data->current_mode = &event_mode;

    thread_id = SDL_CreateThread( switch_getinput, "Switch mitm process", thread_data );
#endif

#ifdef SDL2
    while (1) {
      while ( SDL_PollEvent(&event) ) {
#else
    while ( SDL_WaitEvent(&event) ) {
#endif
        // ignore continous SDL_MOUSEMOTION
        while (event.type == SDL_MOUSEMOTION){
        #ifdef SDL2
            if ( SDL_PeepEvents( &tmp_event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT ) == 0 ) break;
            if (tmp_event.type != SDL_MOUSEMOTION) break;
            SDL_PeepEvents( &tmp_event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );
        #else
            if ( SDL_PeepEvents( &tmp_event, 1, SDL_PEEKEVENT, SDL_ALLEVENTS ) == 0 ) break;
            if (tmp_event.type != SDL_MOUSEMOTION) break;
            SDL_PeepEvents( &tmp_event, 1, SDL_GETEVENT, SDL_ALLEVENTS );
        #endif
            event = tmp_event;
        }

        switch (event.type) {
          case SDL_MOUSEMOTION:
            mouseMoveEvent( (SDL_MouseMotionEvent*)&event );
            break;

          case SDL_MOUSEBUTTONDOWN:
            if ( !btndown_flag ) break;
          case SDL_MOUSEBUTTONUP:
            mousePressEvent( (SDL_MouseButtonEvent*)&event );
            break;

        #ifdef __SWITCH__
          case SDL_FINGERMOTION:
            fingerMoveEvent( (SDL_TouchFingerEvent*)&event );
            break;
          
          case SDL_FINGERDOWN:
            fingerMoveEvent( (SDL_TouchFingerEvent*)&event );
            if ( !btndown_flag ) break;
          case SDL_FINGERUP:
            fingerPressEvent( (SDL_TouchFingerEvent*)&event );
            break;
        #endif
            

        #ifdef SDL2
          case SDL_MOUSEWHEEL:
            mouseWheelEvent( (SDL_MouseWheelEvent*)&event );
            break;
        #endif

          case SDL_JOYBUTTONDOWN:
            event.key.type = SDL_KEYDOWN;
            event.key.keysym.sym = transJoystickButton(event.jbutton.button);
            if(event.key.keysym.sym == SDLK_UNKNOWN)
                break;

          case SDL_KEYDOWN:
            event.key.keysym.sym = transKey(event.key.keysym.sym);
            keyDownEvent( (SDL_KeyboardEvent*)&event );
            if ( btndown_flag )
                keyPressEvent( (SDL_KeyboardEvent*)&event );
            break;

          case SDL_JOYBUTTONUP:
            event.key.type = SDL_KEYUP;
            event.key.keysym.sym = transJoystickButton(event.jbutton.button);
            if(event.key.keysym.sym == SDLK_UNKNOWN)
                break;

          case SDL_KEYUP:
            event.key.keysym.sym = transKey(event.key.keysym.sym);
            keyUpEvent( (SDL_KeyboardEvent*)&event );
            keyPressEvent( (SDL_KeyboardEvent*)&event );
            break;

          case SDL_JOYAXISMOTION:
          {
              SDL_KeyboardEvent ke = transJoystickAxis(event.jaxis);
              if (ke.keysym.sym != SDLK_UNKNOWN){
                  if (ke.type == SDL_KEYDOWN){
                      keyDownEvent( &ke );
                      if (btndown_flag)
                          keyPressEvent( &ke );
                  }
                  else if (ke.type == SDL_KEYUP){
                      keyUpEvent( &ke );
                      keyPressEvent( &ke );
                  }
              }
              break;
          }

          case ONS_TIMER_EVENT:
            timerEvent();
            break;

          case ONS_SOUND_EVENT:
          case ONS_CDAUDIO_EVENT:

// Just adds the case for ONS_FADE_EVENT.  This is part of our mp3 fadeout enablement patch.  Recommend for integration.  [Seung Park, 20060621]
#if defined(INSANI)
          case ONS_FADE_EVENT:
#endif

          case ONS_MIDI_EVENT:
          case ONS_MUSIC_EVENT:
            flushEventSub( event );
            break;

          case ONS_WAVE_EVENT:
            flushEventSub( event );
            //printf("ONS_WAVE_EVENT %d: %x %d %x\n", event.user.code, wave_sample[0], automode_flag, event_mode);
            if ( event.user.code != 0 ||
                 !(event_mode & WAIT_VOICE_MODE) ) break;

            if ( remaining_time <= 0 ){
                event_mode &= ~WAIT_VOICE_MODE;
                if ( automode_flag )
                    current_button_state.button = 0;
                else if ( usewheel_flag )
                    current_button_state.button = -5;
                else
                    current_button_state.button = -2;
                stopAnimation( clickstr_state );
                advancePhase();
            }
            break;

    #ifdef SDL2
          case SDL_WINDOWEVENT:
            if ( event.window.event == SDL_WINDOWEVENT_LEAVE ||
                 event.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
                 event.window.event == SDL_WINDOWEVENT_MINIMIZED )
                break;
            
            if ( event.window.event == SDL_WINDOWEVENT_EXPOSED ) {
                SDL_UpdateWindowSurface( window );
            }
    #else
          case SDL_ACTIVEEVENT:
            if ( !event.active.gain ) break;
          case SDL_VIDEOEXPOSE:
              SDL_UpdateRect( screen_surface, 0, 0, screen_width, screen_height );
    #endif
              break;

          case SDL_QUIT:
            endCommand();
            break;

          default:
            break;
        }

#ifdef SDL2
      }

      SDL_Delay( FPS_TO_FRAMETIME( 83 ) );
#endif

    }
    
    return -1;
}


