// CSBlinux.cpp : Linux-specific file instead of windows-specific CSBwin.cpp.
// Note: CSBTypes.h needs to be included after stdafx.h
#include "stdafx.h"
#include "UI.h"
#include "resource.h"
#include <stdio.h>
#include <unistd.h>

#include "CSBTypes.h"
#include "auto_mapper.h"

#include "Dispatch.h"
#include "Objects.h"
#include "data.h"


#define FONT_SIZE 14
#define DY (FONT_SIZE+6)

#define APPTITLE  "CSBwin"
#define APPVERSION  "15.7"
#define APPVERMINOR "v0" /* linux-only update*/

//extern gboolean noDirtyRect;
//extern SDL_Rect DirtyRect;

SDL_Semaphore *sem;

static bool cursorIsShowing;
extern bool RecordMenuOption;
extern bool DMRulesDesignOption;
extern bool RecordDesignOption;
extern bool extendedPortraits;
bool fullscreenActive;
extern bool simpleEncipher; //not used in Linux
extern unsigned char *encipheredDataFile; //not used in Linux

static SDL_Event evert;
bool sdlQuitPending = false;
bool timerInQueue = false;

int absMouseX;  // screen resolution; you must divide by screenSize
int absMouseY;  //    to get position on ST screen.


SDL_Window *sdlWindow = NULL;
SDL_Texture *sdlTexture = NULL;
SDL_Renderer *sdlRenderer = NULL;
SDL_Rect desktop;
int SDL_VIDEOEXPOSE = 0;

auto_mapper autoMapper;


void PostQuitMessage( int a ) {
    SDL_Event event;
//    FILE *f;
//    f = fopen("debug","a");
//    fprintf(f,"PostQuitMessage(0x%x)\n", a);
//    fclose(f);
    while (SDL_PollEvent(&event) != 0){};
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
    sdlQuitPending = true;
}

#define IDC_Timer 1

/*
static void DumpEvents(void)
{
  FILE *f;
  evert ev;
  f = fopen("debug","a");
  fprintf(f, "%s\n", SDL_GetError());
  while (SDL_PollEvent(&ev) != 0)
  {
    if (ev.type = SDL_USEREVENT)
    {
      fprintf(f, "SDL_USEREVENT type = %d\n", ev.user.code);
    }
    else
    {
      fprintf(f, "Event type= %d\n", ev.type);
    };
  };
  fclose(f);
}
*/

bool videoExposeWaiting = false;

static uint32_t __Timer_Callback(void *userdata, SDL_TimerID timerID, Uint32 interval)
{
  size_t code;
  code = (size_t)(userdata);
  uint32_t ticks = SDL_GetTicks();
  if (code == IDC_Timer)
  {
      SDL_WaitSemaphore(sem);
    if(timerInQueue)
    {
      // We will add an IDC_Timer only when there is nothing
      // else to do.
	SDL_SignalSemaphore(sem);
      return interval;
    }
    else
    {
      // The queue is empty.  We can add an IDC_Timer and
      // (perhaps) an IDC_VIDEOEXPOSE.
     SDL_Event ev;
      if (videoExposeWaiting)
      {
        ev.type = SDL_EVENT_USER;
        ev.user.code = IDC_VIDEOEXPOSE;
        if (SDL_PushEvent(&ev) < 0)
        {
        //           DumpEvents();
          die(0x66a9);
        };
        videoExposeWaiting = false;
      }
      {
        ev.type = SDL_EVENT_USER;
        ((SDL_UserEvent*) &ev)->code = IDC_Timer;
        if (SDL_PushEvent(&ev) < 0)
        {
          char line[80];
          UI_MessageBox(SDL_GetError(), "PushEvent Failed",MESSAGE_OK);
          die(0x66aa);
        };
        timerInQueue = true;
        videoExposeWaiting = false;
      }
      SDL_SignalSemaphore(sem);
      return interval;
    }
  }
  else
  {
    PostQuitMessage(0xbab3);
  };
  return interval; // to avoid warning about not returning anything
}
  


static void PushEvent(void *param)
{
  SDL_Event ev;
  {
    size_t code;
    code = (size_t)(param);
    if (code == IDC_VIDEOEXPOSE)
    {
      if (SDL_PeepEvents(&ev,1,SDL_PEEKEVENT,
                     SDL_EVENT_FIRST,SDL_EVENT_LAST) >0)
      {
        // We will add a IDC_VIDEOEXPOSE only when there is
        // nothing else to do.
        videoExposeWaiting = true;
        return;
      };
    };
  };
  ev.type = SDL_EVENT_USER;
  ((SDL_UserEvent*) &ev)->code = (size_t)(param);
  if (SDL_PushEvent(&ev) < 0)
  {
    char line[80];
    sprintf(line," code = %d", (size_t)(param));
    UI_MessageBox(SDL_GetError(), "PushEvent Failed",MESSAGE_OK);
    UI_Die();
  };
  return;
}

int WindowWidth = 320*4;
int WindowHeight = 200*4 + DY;
float st_X = 320.0 / WindowWidth;
float st_Y = 200.0 / (WindowHeight - 20);
static CSB_UI_MESSAGE csbMessage;

static void __resize_screen( ui32 w, i32 h ) {
#if 1
    if (w < 640 || h < 400+DY) {
	WindowWidth = 640;
	WindowHeight = 400+DY;
    } else {
	WindowWidth = w;
	WindowHeight = h;
    }
    st_X = 320.0/WindowWidth;
    st_Y = 200.0/WindowHeight;
    SDL_SetWindowSize(sdlWindow,WindowWidth,WindowHeight);
    csbMessage.type=UIM_REDRAW_ENTIRE_SCREEN;
    if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
    {
	PostQuitMessage(0x24);
    }
#else
  evert ev;
#if defined SDL12
  //ev.type = SDL_VIDEORESIZE;
  //((SDL_ResizeEvent*)&ev) -> w = w;
  //((SDL_ResizeEvent*)&ev) -> h = h;
#elif defined SDL20
  ev.type = IDC_VIDEORESIZE;
  ev.user.data1 = (void*)((size_t)w);
  ev.user.data2 = (void*)((size_t)h);
#else
  xxxError
#endif
  SDL_PushEvent(&ev);
#endif
}
/*
* There IS a videoexpose event in linux, but apparently SDL
* wants to handle those for itself... so we cannot use 'invalidate'
* the obvious way. UI_Invalidate just marks a request to
* update the screen... LIN_Invalidate is called from vbl and pretends
* to be the OS that grants the update.
*/

bool GetVideoRectangle(i32, RECT *);
static bool pending_update = true;

void UI_Invalidate(bool erase/*=false*/) {
  pending_update = true;
}
void LIN_Invalidate()
{// *screen, x, y, w, h
  if( pending_update ) {
    PushEvent((void*)(IDC_VIDEOEXPOSE) );
  }
}




#define MAX_LOADSTRING 100



i32 trace=-1;
#ifdef _DYN_WINSIZE
//i32 screenSize=2;
#else
//i32 screenSize=2;
#endif

extern bool BeginRecordOK;
extern bool ItemsRemain;
extern bool PlayfileIsOpen(void);
extern bool RecordfileIsOpen(void);
extern bool TimerTraceActive;
extern bool AttackTraceActive;
extern bool AITraceActive;
extern bool PlaybackCommandOption;
extern unsigned char *encipheredDataFile;// Oh?

ui32 TImER=0;
extern i32 NoSpeedLimit;
extern i32 GameMode;
extern i32 MostRecentlyAdjustedSkills[2];
extern i32 LatestSkillValues[2];
extern XLATETYPE latestScanType;
extern XLATETYPE latestCharType;
extern i32 latestCharp1;
extern i32 latestScanp1;
extern i32 latestScanp2;
extern i32 latestScanXlate;
extern i32 latestCharXlate;
const char *helpMessage = "CSBlinux will need the orginal datafiles "
      "for either Chaos Strikes Back,\n"
      "or Dungeon Master in your current working directory.\n"
      "If the files are located somewhere else try\n --help for some usefull comandlinde arguments.\n"
      "\nMake sure that you have these files\n"
      "(and that they are spelled with lower case)\n"
      "   dungeon.dat\n"
      "   hcsb.dat\n"
      "   hcsb.hct\n"
      "   mini.dat\n"
      "   graphics.dat\n"
      "   config.linux";

/* This is a bad place for these id's */

void MTRACE(const char *msg)
{
  if (trace < 0) return;
  FILE *f = GETFILE(trace);
  fprintf(f, "%s", msg);
  fflush(f);
}

void UI_GetCursorPos(i32 *_x, i32 *_y)
{
  *_x = X_TO_CSB(absMouseX,screenSize);
  *_y = Y_TO_CSB(absMouseY,screenSize);
}



char szCSBVersion[] = "CSB for Windows/Linux Version " __DATE__;
int WindowX = 0;
int WindowY = 0;
bool fullscreenRequested = false;


i32 line = 0;

// Global Variables:
SDL_Surface *WND = NULL; //defined in UI.h
SDL_TimerID timer;

#ifdef _DYN_WINSIZE
HermesHandle from_palette;
HermesHandle to_palette;
HermesHandle converter;
HermesFormat* from_format;
#else
SDL_Surface *SCRAP = NULL;
#endif


/* Free the global allocated data and finish the main loop */
void cbAppDestroy(void)
{
  /* Close SDL (prevents the ticks to continue running) */
  //printf("\nQuitting...\n");
    Cleanup(true);
    doneCaches();
    SDL_RemoveTimer(timer);
    SDL_DestroySemaphore(sem);
    sem = NULL;
  SDL_Quit ();
    exit(0);
}

void g_log(const char *, int, const char *, ...)
{
    die(0x3512);
}

extern bool resetWhatToDo,resetgamesetup,resetprisondoor,resettag1,resettag2;

void Process_SDL_MOUSEMOTION(
           bool& cursorIsShowing)
{
  ("SDL_MOUSEMOTION\n");
  SDL_MouseMotionEvent *e = (SDL_MouseMotionEvent*) &evert;
  int x, y;
  x = e->x * st_X;
  y = (e->y - DY) * st_Y;
  if (y < 0) y = 0;

  absMouseX = x;
  absMouseY = y;
  int32_t st_mouseX = X_TO_CSB(absMouseX,screenSize);
  int32_t st_mouseY = Y_TO_CSB(absMouseY,screenSize);



  if (GameMode == 1)
  {
    if (cursorIsShowing )
    {
      cursorIsShowing = false;
      SDL_HideCursor();
    };
  }
  else
  {
    if (!cursorIsShowing)
    {
      cursorIsShowing = true;
      SDL_ShowCursor();
    };
  };
//  {
//    FILE *f;
//    static int prevX, prevY;
//    if ((e->x != prevX) || (e->y != prevY))
//    {
//      f = fopen("/run/shm/debug","a");
//      fprintf(f, "Movement e->x=%d(%d), e->y=%d(%d); set mouseX=%d mouseY=%d showing=%d\n",
//                  e->x, e->xrel, e->y, e->yrel, absMouseX, absMouseY, cursorIsShowing);
//      fclose(f);
//    };
//  };
}

void Process_ecode_IDC_Normal(void)
{
  //MTRACE("IDC_Normal\n");
  __resize_screen( 320, 240 );
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_NORMAL;
  csbMessage.p2 = 2; //2-(screenSize==1); //new value
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(1);
  };
}

void Process_ecode_IDC_Timer(void)
{
//  MTRACE("IDC_Timer\n");
  if(GameMode != 1)
  {
    LIN_Invalidate();
  }
  SDL_WaitSemaphore(sem);
  timerInQueue = false;
  SDL_SignalSemaphore(sem);
       //One-at-a-time, please
       //The problem was that under certain circumstances too
       //many timer events get queued and the SDL queue gets
       //full and keystrokes get discarded!
  csbMessage.type=UIM_TIMER;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(2);
  };
}

void Process_ecode_IDC_VIDEOEXPOSE(void)
{
  /*
   * BEGIN the evil VIDEOEXPOSE
   */
  //MTRACE("IDC_VIDEOEXPOSE\n");
  //printf("IDC_VIDEOEXPOSE\n");
  line++;
  line = line%10;
  //        memset( SCR, line, 40 );
  //        SDL_BlitSurface(SCR, NULL, WND, NULL );
  switch(line)
  {
    case 0:
      //        g_print("vblint = %u",VBLInterruptCount);
      break;
    case 1:
      //        g_print("chkvbl = %u",CheckVBLCount);
      break;
    case 2:
    //        g_print("STblt = %u",STBLTCount);
      break;
    case 3:
    //  g_print("Time = %u",GameTime);
      break;
    case 4:
    //  g_print(
    //         "Skill[%d]=0x%08x      ",
    //          MostRecentlyAdjustedSkills[0],
    //          LatestSkillValues[0]);
      break;
    case 5:
    //  g_print(
    //          "Skill[%d]=0x%08x      ",
    //          MostRecentlyAdjustedSkills[1],
    //          LatestSkillValues[1]);
      break;
    case 6:
      switch (latestCharType)
      {
        case TYPEIGNORED:
    //      g_print( "%04x key --> Ignored                         ", latestCharp1);
          break;
        case TYPEKEY:
    //      g_print( "%04x key --> Translated %08x", latestCharp1, latestCharXlate);
          break;
      };
      break;
    case 7:
      switch (latestScanType)
      {
        case TYPESCAN:
        case TYPEIGNORED:
    //      g_print( "%08x mscan --> Ignored                        ",latestScanp1);
          break;
        case TYPEMSCANL:
    //      g_print( "%08x mscan --> Translated to %08x L",latestScanp1,latestScanXlate);
          break;
        case TYPEMSCANR:
    //      g_print( "%08x mscan --> Translated to %08x R",latestScanp1,latestScanXlate);
          break;
        default:
    //      g_print("                                              ");
          break;
      };
      break;
    case 8:
      switch (latestScanType)
      {
        case TYPEMSCANL:
        case TYPEIGNORED:
        case TYPEMSCANR:
    //      g_print( "%08x scan --> Ignored                        ",latestScanp2);
          break;
        case TYPESCAN:
    //      g_print( "%08x scan --> Translated to %08x", latestScanp2,latestScanXlate);
          break;
        default:
    //      g_print("                                              ");
          break;
      };
      break;
    case 9:
    //  g_print("Total Moves = %d",totalMoveCount);
      break;
  };
  //TextOut(hdc,325,25+15*line,msg,strlen(msg));
  //        g_warning(msg);
  /*
   * This message should be sent when the screen has been erased?
   *
   */
  /*
    csbMessage.type=UIM_REDRAW_ENTIRE_SCREEN;
    if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
    {
      PostQuitMessage(0);
      break;
    };
  */

  /*
   * Finally, write to the screen!
   */
  csbMessage.type=UIM_PAINT;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(3);
  };
}

void Process_ecode_IDC_Double(void)
{
  MTRACE("IDC_Double\n");
  __resize_screen( 320*2, 200*2);
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_DOUBLE;
  csbMessage.p2 = 2; //2-(screenSize==2); //new value
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(4);
  };
}

void Process_ecode_IDC_Triple(void)
{
  MTRACE("IDC_Triple\n");
  __resize_screen( 320*3, 200*3 );
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_TRIPLE;
  csbMessage.p2 = 2; //2-(screenSize==3); //new value
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(5);
  };
}

void Process_ecode_IDC_Quadruple(void)
{
  MTRACE("IDC_Quadruple\n");
  __resize_screen( 320*4, 200*4 );
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_QUADRUPLE;
  csbMessage.p2 = 2; //2-(screenSize==4); //new value
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(6);
  };
}

void Process_ecode_IDC_Quintuple(void)
{
  MTRACE("IDC_Quintuple\n");
  __resize_screen( 320*5, 200*5 );
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_QUINTUPLE;
  csbMessage.p2 = 2; //2-(screenSize==4); //new value
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(7);
  };
}

void Process_ecode_IDC_Sextuple(void)
{
  MTRACE("IDC_Sextuple\n");
  __resize_screen( 320*6, 200*6 );
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_SEXTUPLE;
  csbMessage.p2 = 2; //2-(screenSize==4); //new value
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(8);
  };
}

void Process_ecode_IDC_QuickPlay(void)
{
  MTRACE("IDC_QuickPlay\n");
  if (!PlayfileIsOpen()) return;
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_QUICKPLAY;
  csbMessage.p2 = (NoSpeedLimit!=0) ? 0 : 1;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(9);
  };
}

void Process_ecode_IDM_Glacial(void)
{
  MTRACE("IDM_Glacial\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_CLOCK;
  csbMessage.p2 = SPEED_GLACIAL;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0xa);
  };
}

void Process_ecode_IDM_Molasses(void)
{
  MTRACE("IDM_Molasses\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_CLOCK;
  csbMessage.p2 = SPEED_MOLASSES;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0xb);
  };
}

void Process_ecode_IDM_VerySlow(void)
{
  MTRACE("IDM_VerySlow\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_CLOCK;
  csbMessage.p2 = SPEED_VERYSLOW;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0xc);
  };
}

void Process_ecode_IDM_Slow(void)
{
  MTRACE("IDM_Slow\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_CLOCK;
  csbMessage.p2 = SPEED_SLOW;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0xd);
  };
}

void Process_ecode_IDM_Normal(void)
{
  MTRACE("IDM_Normal\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_CLOCK;
  csbMessage.p2 = SPEED_NORMAL;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0xe);
  };
}

void Process_ecode_IDM_Fast(void)
{
  MTRACE("IDM_Fast\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_CLOCK;
  csbMessage.p2 = SPEED_FAST;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0xf);
  };
}

void Process_ecode_IDM_Quick(void)
{
  MTRACE("IDM_Quick\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_CLOCK;
  csbMessage.p2 = SPEED_QUICK;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x10);
  };
}

void Process_ecode_IDM_PlayerClock(void)
{
  MTRACE("IDM_PlayerClock\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_PLAYERCLOCK;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x11);
  };
}

void Process_ecode_IDM_ExtraTicks(void)
{
  MTRACE("IDM_ExtraTicks\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_EXTRATICKS;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x12);
  };
}

void Process_ecode_IDC_Record(void)
{
  MTRACE("IDC_Record\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_RECORD;
  csbMessage.p2 = 1;
  if (RecordMenuOption) csbMessage.p2 = 0;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x13);
  };
}

void Process_ecode_IDC_TimerTrace(void)
{
  MTRACE("IDC_TimerTrace\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_TIMERTRACE;
  csbMessage.p2 = 1;
  if (TimerTraceActive) csbMessage.p2 = 0;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x14);
  };
}

void Process_ecode_IDM_DMRules(void)
{
   MTRACE("IDM_DMRULES\n");
   csbMessage.type = UIM_SETOPTION;
   csbMessage.p1 = OPT_DMRULES;
   if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
   {
     PostQuitMessage(0x15);
   };
}

void Process_ecode_IDC_AttackTrace(void)
{
  MTRACE("IDC_AttackTrace\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_ATTACKTRACE;
  csbMessage.p2 = 1;
  if (AttackTraceActive) csbMessage.p2 = 0;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x16);
  };
}

void Process_ecode_IDC_AITrace(void)
{
  MTRACE("IDC_AITrace\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_AITRACE;
  csbMessage.p2 = 1;
  if (AITraceActive) csbMessage.p2 = 0;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x17);
  };
}

void Process_ecode_IDC_ItemsRemaining(void)
{
  MTRACE("IDC_ItemsRemaining\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_ITEMSREMAINING;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x18);
  };
}

void Process_ecode_IDC_NonCSBItemsRemaining(void)
{
  MTRACE("IDC_NonCSBItemsRemaining\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_NONCSBITEMSREMAINING;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x19);
  };
}

void Process_ecode_IDC_Playback(void)
{
  MTRACE("IDC_Playback\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_PLAYBACK;
  csbMessage.p2 = 1;
  if (PlayfileIsOpen()) csbMessage.p2 = 0;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x1a);
  };
}

void Process_ecode_IDC_DispatchTrace(void)
{
  MTRACE("IDC_DispatchTrace\n");
  if (trace  >= 0 )
  {
    CLOSE(trace);
    trace = -1;
  }
  else
  {
    trace = CREATE("trace.log","w", true);
  };
}

void Process_ecode_IDM_GraphicTrace(void)
{
  MTRACE("IDC_GraphicTrace\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_GRAPHICTRACE;
  csbMessage.p2 = 1;
  if (TimerTraceActive) csbMessage.p2 = 0;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x1b);
  };
}

void Process_ecode_IDC_DSATrace(void)
{
  MTRACE("IDC_DSATrace\n");
  csbMessage.type = UIM_SETOPTION;
  csbMessage.p1 = OPT_DSATRACE;
  csbMessage.p2 = 1;
  if (DSATraceActive) csbMessage.p2 = 0;
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x1c);
  };
}

void Process_SDL_USEREVENT(void)
{
 // MTRACE("sdl_userevent->");
  SDL_UserEvent *e = (SDL_UserEvent*) &evert;
  switch( e->code )
  {// the menu (and timer)
    case IDC_Normal:        Process_ecode_IDC_Normal();        break;
    case IDC_Timer:         Process_ecode_IDC_Timer();         break;
    case IDC_VIDEOEXPOSE:   Process_ecode_IDC_VIDEOEXPOSE();   break;
    case IDC_Double:        Process_ecode_IDC_Double();        break;
    case IDC_Triple:        Process_ecode_IDC_Triple();        break;
    case IDC_Quadruple:     Process_ecode_IDC_Quadruple();     break;
    case IDC_Quintuple:     Process_ecode_IDC_Quintuple();     break;
    case IDC_Sextuple:      Process_ecode_IDC_Sextuple();      break;
    case IDC_QuickPlay:     Process_ecode_IDC_QuickPlay();     break;
    case IDM_Glacial:       Process_ecode_IDM_Glacial();       break;
    case IDM_Molasses:      Process_ecode_IDM_Molasses();      break;
    case IDM_VerySlow:      Process_ecode_IDM_VerySlow();      break;
    case IDM_Slow:          Process_ecode_IDM_Slow();          break;
    case IDM_Normal:        Process_ecode_IDM_Normal();        break;
    case IDM_Fast:          Process_ecode_IDM_Fast();          break;
    case IDM_Quick:         Process_ecode_IDM_Quick();         break;
    case IDM_PlayerClock:   Process_ecode_IDM_PlayerClock();   break;
    case IDM_ExtraTicks:    Process_ecode_IDM_ExtraTicks();    break;
    case IDC_Record:        Process_ecode_IDC_Record();        break;
    case IDC_TimerTrace:    Process_ecode_IDC_TimerTrace();    break;
    case IDM_DMRULES:       Process_ecode_IDM_DMRules();       break;
    case IDC_AttackTrace:   Process_ecode_IDC_AttackTrace();   break;
    case IDC_AITrace:       Process_ecode_IDC_AITrace();       break;
    case IDC_ItemsRemaining:Process_ecode_IDC_ItemsRemaining();break;
    case IDC_Playback:      Process_ecode_IDC_Playback();      break;
    case IDC_DispatchTrace: Process_ecode_IDC_DispatchTrace(); break;
    case IDM_GraphicTrace:  Process_ecode_IDM_GraphicTrace();  break;
    case IDC_DSATrace:      Process_ecode_IDC_DSATrace();      break;
    case IDC_NonCSBItemsRemaining:
                            Process_ecode_IDC_NonCSBItemsRemaining();
                                                               break;
    default:
      printf("Unknown SDL_USEREVENT\n");
      MTRACE("Unknown SDL_USEREVENT\n");
      //return DefWindowProc(hWnd, message, wParam, lParam);
      break;
  };
}

void Process_SDL_MOUSEBUTTONDOWN(void)
{
  MTRACE("SDL_MOUSEBUTTONDOWN->");
  {
    SDL_MouseButtonEvent  *e = (SDL_MouseButtonEvent*) &evert;
    switch(e->button)
    {// btn down, left or right
      case SDL_BUTTON_LEFT:
        MTRACE("SDL_BUTTON_LEFT\n");
        //printf("left button @%d ", (ui32)UI_GetSystemTime());
        csbMessage.type=UIM_LEFT_BUTTON_DOWN;
        csbMessage.p1 = absMouseX;
        csbMessage.p2 = absMouseY;
        if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
        {
          PostQuitMessage(0x1d);
        };
        break;
      case SDL_BUTTON_RIGHT:
        MTRACE("SDL_BUTTON_RIGHT\n");
        //printf("right ");
        csbMessage.type=UIM_RIGHT_BUTTON_DOWN;
        csbMessage.p1 = absMouseX; // = e->x;  // horizontal position of cursor
        csbMessage.p2 = absMouseY; // = e->y;    // vertical position of cursor
        if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
        {
          PostQuitMessage(0x1e);
        };
        break;
    };
  };
  //printf("at (%x, %x)\n", X_TO_CSB(absMouseX,screenSize),Y_TO_CSB(absMouseX,screenSize));
}

void Process_SDL_WHEEL_EVENT(){
MTRACE("SDL_WHEEL_EVENT");
  {// btn up, left or right
    SDL_MouseWheelEvent  *e = (SDL_MouseWheelEvent*) &evert;
    int32_t y = e->y > 0 ? 1:0;
    switch(y)
    {
      case 1:
       autoMapper.zoom(10);
        break;
      case 0:
       autoMapper.zoom(-10);
        break;
    };
  };

}

void Process_SDL_MOUSEBUTTONUP(void)
{
  MTRACE("SDL_MOUSEBUTTONUP->");
  {// btn up, left or right
    SDL_MouseButtonEvent  *e = (SDL_MouseButtonEvent*) &evert;
    switch(e->button)
    {
      case SDL_BUTTON_RIGHT:
        MTRACE("SDL_BUTTON_RIGHT\n");
        csbMessage.type=UIM_RIGHT_BUTTON_UP;
        csbMessage.p1 = absMouseX; // = e->x;  // horizontal position of cursor
        csbMessage.p2 = absMouseY; // = e->y; // vertical position of cursor
        if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
        {
          PostQuitMessage(0x1f);
        };
        break;
      case SDL_BUTTON_LEFT:
        MTRACE("SDL_BUTTON_LEFT\n");
        csbMessage.type=UIM_LEFT_BUTTON_UP;
        csbMessage.p1 = absMouseX; // = e->x;   // horizontal position of cursor
        csbMessage.p2 = absMouseY; // = e->y;  // vertical position of cursor
        if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
        {
          PostQuitMessage(0x20);
        };
        break;
    };
  };
}

void Process_SDL_KEYDOWN(void)
{
//  static int prevp1 = -1;
  MTRACE("SDL_KEYDOWN\n");
  {
    /*SDL_KeyboardEvent *e = (SDL_KeyboardEvent*) &evert;
    if(e->keysym->sym==SDLK_LEFT)
    printf("left ");
    if else(e->keysym->sym==SDLK_RIGHT)
    printf("right ");
    */
    csbMessage.type=UIM_KEYDOWN;
    csbMessage.p1 =  evert.key.key; //virtual key
    csbMessage.p2 = evert.key.scancode; //scancode
    //printf("key press %x %x mod %x\n",csbMessage.p1,csbMessage.p2,evert.key.keysym.mod);
    //printf("Key 0x%x pressed @%d\n",(int)csbMessage.p1, (ui32)UI_GetSystemTime());
//    if ((csbMessage.p1 == 3) && (prevp1 == 3))
//    {
//      PostQuitMessage(0);
//      return;
//    };
//    prevp1 = csbMessage.p1;
    if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
    {
      PostQuitMessage(0x21);
    };
  };
  {
    static int prevKeyUp = -1;
    csbMessage.type=UIM_CHAR;
    csbMessage.p1 = evert.key.key;
    if (evert.key.key == SDLK_LCTRL) return;
    if (evert.key.key == SDLK_RCTRL) return;
    //{
    //  char line[80];
    //  sprintf(line, "p1=%d; mod=%d", csbMessage.p1, evert.key.keysym.mod);
    //  UI_MessageBox(line, "KEYUP", MESSAGE_OK);
    //};
    if (evert.key.mod &  (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL))
    {
      if (csbMessage.p1 == SDLK_C)
      {
        if (prevKeyUp == 3)
        {
          PostQuitMessage(0x22);
          return;
        };
        csbMessage.p1 = 3; //Control-c
      };
    };
    if ((evert.key.mod & (SDL_KMOD_ALT)) && evert.key.key == SDLK_RETURN) {
	static int oldw, oldh;
	fullscreenActive = !fullscreenActive;
	    
	SDL_SetWindowFullscreen(sdlWindow,fullscreenActive);
    }

    prevKeyUp = csbMessage.p1;
    if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
    {
      PostQuitMessage(0x23);
    };
  };
}
void Process_SDL_KEYUP(void)
{
  MTRACE("SDL_KEYUP\n");
}

inline void Process_SDL_WINDOWEVENT(void)
{
  switch(evert.type)
  {
    case SDL_EVENT_WINDOW_SHOWN:
      break;
    case SDL_EVENT_WINDOW_HIDDEN:
      break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
      break;
    case SDL_EVENT_WINDOW_MOVED:
      break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      __resize_screen( evert.window.data1, evert.window.data2 );
      break;
    case SDL_EVENT_WINDOW_EXPOSED:
      csbMessage.type=UIM_REDRAW_ENTIRE_SCREEN;
      if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
      {
	      PostQuitMessage(0x24);
      }
      break;
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
      break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
      break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
      break;
    case SDL_EVENT_WINDOW_MINIMIZED:
      break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
      break;
    case SDL_EVENT_WINDOW_RESTORED:
      break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      PostQuitMessage(1);
      break;
  };
}


static int drawn = 0;
void post_render();


	 SDL_AudioStream *audio_stream;

	 MIX_Mixer *sdl3_mixer;


	 SDL_AudioDeviceID audio_device_id;


  SDL_AudioSpec opening_doors_audio_spec;
	SDL_AudioSpec audio_spec;

/********************** MAIN **********************/
int main(int argc, char* argv[])
{
  
	setenv("SDL_VIDEODRIVER", "x11", 1);
	setenv("TIMIDITY_CFG", "/etc/timidity/timidity.cfg", 1);

  folderParentName=(char*)".";
  folderName=(char*)".";
  root = (char*)".";
  /* Parse commandline arguments.
   * Eat all arguments I recognize. Otherwise Gnome kills me.
   */
  printf("%s\n", APPTITLE  " " APPVERSION APPVERMINOR );
        versionSignature = Signature(szCSBVersion);
  if( UI_ProcessOption(argv,argc) == false )
  {
      exit(0);
  }
  if( !(folderParentName&&folderName&&root) )
  {
    root=(char*)"./";
  }

  //    ***** Init the Aplication enviroment ******


//    ***** Initialize defaults, Video and Audio *****
   	if ( !SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_VIDEO))
	{
		SDL_Log("Unable to init SDL: %s", SDL_GetError() );
	}

	//Initialize SDL_mixer with our chosen audio settings

	const char*cfg_timidity = SDL_getenv("TIMIDITY_CFG");

	if(!MIX_Init()) {
		printf("Unable to initialize audio: %s\n", SDL_GetError());
		exit(1);
	}

// See all precise audio info here : http://dmweb.free.fr/community/documentation/file-formats/data-files/#data-6
// Normal samples are supposed to be at 5486, but I get sometimes interruptions while a door is opening
	// and they don't happen with 5200 Hz
  	opening_doors_audio_spec ={
			.format = SDL_AUDIO_U8,
			.channels = 1,
			.freq = 4237
		};

	audio_spec ={
			.format = SDL_AUDIO_U8,
			.channels = 1,
			.freq = 5200
		};

  //  int samples_frames;
  //SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&audio_spec,&samples_frames);

		audio_device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec);

		if (audio_device_id == 0)
		{
			printf("Could not open audio: %s\n", SDL_GetError());
			exit(-1);
		}
	if ( (sdl3_mixer = MIX_CreateMixerDevice(audio_device_id,&audio_spec)) == NULL)
	{
			SDL_Log(" mixer creation failed %s", SDL_GetError());
			exit(2);
	}

	if ( (audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec, NULL, NULL)) == NULL)
	{
			SDL_Log(" audio stream creation failed %s", SDL_GetError());
			exit(2);
	}

	// start audio stream
	SDL_ResumeAudioDevice(audio_device_id);
	SDL_ResumeAudioStreamDevice(audio_stream);

	printf("SDL %i successfully initialized.\n",SDL_GetVersion());


// ****************************** the DISPLAY ****************************************************
  {

    int flags;
    screenSize = 1;
    flags = 0;
    if (fullscreenRequested)
    {
      flags =  SDL_WINDOW_FULLSCREEN;
    };
    flags |= SDL_WINDOW_RESIZABLE;
    SDL_GetDisplayBounds(0,&desktop);

  sdlWindow = SDL_CreateWindow("dungeon Master SDL3", WindowWidth, WindowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE);
	sdlRenderer = SDL_CreateRenderer(sdlWindow,NULL);
	sdlTexture = SDL_CreateTexture(sdlRenderer,SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,320,200);


  autoMapper.initialize();
	
  SDL_ShowCursor();

	const char *error = SDL_GetError();
	if ( strlen(error) > 0 ){
	printf("error: %s\n", SDL_GetError());
	}
/*
    if ((sdlWindow = SDL_CreateWindow(
                   szCSBVersion,
                   SDL_WINDOWPOS_CENTERED,
                   SDL_WINDOWPOS_CENTERED,
                   WindowWidth,WindowHeight,
                   flags)) == NULL)
    {
      UI_MessageBox(SDL_GetError(),
                    "Failed to create window",
                    MESSAGE_OK);
      die(0xe9a3);
    };
    SDL_Surface *sf = SDL_LoadBMP("lsb.bmp");
    if (sf) {
	SDL_SetWindowIcon(sdlWindow,sf);
	SDL_FreeSurface(sf);
    }
    SDL_VIDEOEXPOSE = SDL_RegisterEvents(1);
    if ((sdlRenderer = SDL_CreateRenderer(
                   sdlWindow,
                   -1,
                   0)) == NULL)
    {
      UI_MessageBox(SDL_GetError(),
                    "Failed to create Renderer",
                    MESSAGE_OK);
      die(0x519b);
    };
    // I disable this because of imgui, the game picture doesn't take the whole window anymore, so it's better to convert the coordinates ourselves
    // SDL_RenderSetLogicalSize(sdlRenderer,320,200);
    if ((sdlTexture = SDL_CreateTexture(
                   sdlRenderer,
                   SDL_PIXELFORMAT_RGB565,
                   SDL_TEXTUREACCESS_STREAMING,
                   320,200)) == NULL)
    {
      UI_MessageBox(SDL_GetError(),
                    "Failed to create texture",
                    MESSAGE_OK);
      die(0xff53);
    };
    */


  };

  SDL_ShowCursor();
  cursorIsShowing = true;
  if(fullscreenRequested)
  {
    fullscreenActive = true;
  }
  UI_Initialize_sounds();


  /*
   * Initialize the display in a 640x480 8-bit palettized mode,
   * requesting a software surface, or anything else....
   */

  /* Do the window-cha-cha. */
  speedTable[SPEED_GLACIAL].vblPerTick = 1000;
  speedTable[SPEED_MOLASSES].vblPerTick = 55;
  speedTable[SPEED_VERYSLOW].vblPerTick = 33;
  speedTable[SPEED_SLOW].vblPerTick = 22;
  speedTable[SPEED_NORMAL].vblPerTick = 15;
  speedTable[SPEED_FAST].vblPerTick = 11;
  speedTable[SPEED_QUICK].vblPerTick = 7;
  speedTable[SPEED_FTL].vblPerTick = 5;

  volumeTable[VOLUME_FULL].divisor = 1;
  volumeTable[VOLUME_HALF].divisor = 2;
  volumeTable[VOLUME_QUARTER].divisor = 4;
  volumeTable[VOLUME_EIGHTH].divisor = 8;
  volumeTable[VOLUME_OFF].divisor = 65536;

  MTRACE("*** First initialization ***\n");
  csbMessage.type=UIM_INITIALIZE;
  /* Parse config.linux */
  if (CSBUI(&csbMessage) != UI_STATUS_NORMAL)
  {
    PostQuitMessage(0x25);
  };

  /* Setup a 50ms timer. */
  sem = SDL_CreateSemaphore(1);
  timer = SDL_AddTimer(TImER?TImER:10, __Timer_Callback, (void*)(IDC_Timer));

  //SDL_WM_GrabInput(SDL_GRAB_ON);

  /********************************************
   *
   *       Main Execution Loop
   *
   ********************************************
   */
  while (true)
  {

#ifdef USE_OLD_GTK
    gtk_main_iteration_do(FALSE); // This solution will always cost CPU instead of gtk_main()
#endif //USE_OLD_GTK
    // evert = app.WaitEvent();
    if (SDL_PollEvent(&evert) == 0)
    {
      SDL_WaitSemaphore(sem);
	    timerInQueue=false;
	    SDL_SignalSemaphore(sem);
      if(SDL_WaitEvent(&evert) == 0) {
	      printf("SDL_WaitEvent error %s\n",SDL_GetError());
	      continue;
      }
    }

    // uint32_t tick = SDL_GetTicks();
    // printf("event type %x ticks %d\n",evert.type,tick);
  
    /* Listen for 'quick buttons' here. */
    /* Hail to the Great Message Struct! */
    MTRACE("msg=");
    bool processed = false;
    switch( evert.type )
    {
      case SDL_EVENT_QUIT:
        MTRACE("SDL_QUIT\n");
        cbAppDestroy();
        break;
      case SDL_EVENT_MOUSE_MOTION:     Process_SDL_MOUSEMOTION(
                                         cursorIsShowing);
                                break;
      case SDL_EVENT_USER:       Process_SDL_USEREVENT();    processed = true;   break;// __timer_loopback
      case SDL_EVENT_MOUSE_BUTTON_DOWN: Process_SDL_MOUSEBUTTONDOWN(); processed = true;break;
      case SDL_EVENT_MOUSE_BUTTON_UP:   Process_SDL_MOUSEBUTTONUP();  processed = true; break;
      case SDL_EVENT_KEY_DOWN:         Process_SDL_KEYDOWN();    processed = true;     break;
      case SDL_EVENT_KEY_UP:           Process_SDL_KEYUP();      processed = true;     break;
      case SDL_EVENT_MOUSE_WHEEL:       Process_SDL_WHEEL_EVENT();   processed = true;  break;
      /*
      case SDL_ACTIVEEVENT:
        {
          if ( evert.active.state & SDL_APPACTIVE )
          {
            if ( evert.active.gain )
            {
              printfalll("App activated\n");
            }
            else
            {
              printf("App iconified\n");
            };
          };
        };
      */

    }; /* Eof Great Event Switch */
    if ( !processed){
          Process_SDL_WINDOWEVENT();  
        }
  } /* Eof Lord Message Loop */

  Cleanup(true);
  doneCaches();
  printf("Quiting SDL.\n");

  /* Shutdown all subsystems */
  SDL_Quit();
  printf("Quiting....\n");
  return (0);
}

extern void ItemsRemaining(i32 mode); // CSBUI.cpp
extern const char *listing_title; // LinCSBUI.cpp
extern LISTING *listing; // AsciiDump.cpp
extern bool resetstartcsb;
bool show_listing;
extern i32 lastTime;
extern bool skipLogo;

extern i32 numState;

// The only test so far directly on nostaliga_mode : recharging ability of vi altars in Mouse.cpp
bool monsters_vulnerable_on_attack = true,nostalgia_mode = false;

    SDL_FRect r;
void post_render() {
    drawn = 1;


	r.y = DY;
	r.x = 0;
	r.w = WindowWidth;
	r.h = WindowHeight - DY;

    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);

    double ratio = r.w*1.0/r.h;
    if (abs(ratio - 32/20.0) > 1e-5) {
	if (ratio > 32/20.0) {
	    r.w = r.h*32/20.0;
	    r.x = (WindowWidth-r.w)/2;
	} else {
	    r.h = r.w*20/32.0;
	    r.y = (WindowHeight - r.h)/2;
	}
    }
    if (d.NumGraphic>0 && SDL_RenderTexture(sdlRenderer,
		sdlTexture,
		NULL,
		&r) < 0) {
	printf("rendercopy error %s\n",SDL_GetError());
    }

    SDL_RenderPresent(sdlRenderer);
}
