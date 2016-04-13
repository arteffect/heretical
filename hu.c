/* "heretical" - musicdisk code - SSG dec 97

to do's:
--------

 - kod optimizasyonu...

*/

#include <string.h>
#include <dir.h>
#include <math.h>

#include "mtypes.h"
#include "mikmod.h"

#include "h.h"

#define NO_ALLEGRO_SOUND

#include "allegro.h"

#include <sys/exceptn.h>

#define DATFILE         "hrtcl.dat"

#define WATER_DEPTH     5
#define WATER_MAXR      50
#define WATER_THICK     10

#define POWER0          "power0.wav"
#define POWER1          "power1.wav"
#define COMPUTER        "computer.wav"

#define COLOR_CONSOLE   206
#define COLOR_FONT      207
#define COLOR_HEADER    204
#define COLOR_VUMETER   205
#define COLOR_VUMETER_BACK 203
#define COLOR_INFO      COLOR_VUMETER

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

#define VUMETER_GAP     3

#define MAX_VOLUME      64

#define MAX_MOUSE_TAILS 20

typedef struct mouse_tail_info
{
  int x;
  int y;
  int level;
} mouse_tail_info;

typedef struct water_info
{
  int x;
  int y;
  int r;
  struct water_info *next;
} water_info;

typedef struct module_info
{
  char filename[13];
  char modname[31];
  struct module_info *next;
} module_info;

typedef struct RECT
{
  int x1,y1,x2,y2;
} RECT;

module_info *lastplayed=NULL;

int vumeter_channels = 0;
int songfinished = -1;

mouse_tail_info mouse_tails[MAX_MOUSE_TAILS];
int mouse_head=0;
int mouse_tail=0;
int mouse_lastx;
int mouse_lasty;

int lock_mouse=0;

static unsigned char xlat[255]=
  {0,32,133,192,15,132,5,255,255,255,139,77,8,139,9,131,121,4,0,15,132,165,
  1,0,0,139,70,40,139,64,20,3,64,65,66,67,68,69,133,71,72,73,74,139,76,77,
  78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,70,96,97,98,99,100,
  101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,
  119,120,121,59,254,61,70,0,224,33,34,35,36,37,38,39,40,41,42,43,44,45,46,
  47,48,49,50,51,52,53,54,55,56,57,58,0,75,126,8,0,116,126,8,0,241,10,3,0,
  33,0,0,0,64,126,8,0,0,176,10,0,37,146,1,0,159,198,2,0,183,4,0,0,130,2,0,
  0,180,129,4,0,144,10,0,0,56,125,8,0,68,125,8,0,20,147,2,0,56,125,8,0,38,
  198,2,0,183,4,0,0,70,2,0,0,120,128,4,0,248,129,1,0,58,224,1,0,9,0,0,0,88,
  125,8,0,9,0,0,0,120,128,4,0,0,0,0,0,84,0,0,0,180,129,4,0,128,129,8,0,8,
  103,2,0,183,4,1,0,204,137,1};

COLOR_MAP *maps[5];
BITMAP *mice[5];

SAMPLE *power0,*power1,*computer;

RECT diskc = {68,158,222,271};
RECT mainc = {332,63,555,270};
RECT vumeter = {88,370,230,423};
RECT powerbtn = {442,414,567,467};
RECT info    = {230,375,305,424};
RECT headerc = {76,57,209,93};

module_info *modlist = NULL;
module_info *focused = NULL;

water_info *waterlist_mainc = NULL;
water_info *waterlist_diskc = NULL;
water_info *waterlist_info  = NULL;

BITMAP *diskscr,*mouse,*header = NULL;
BITMAP *virtualscreen = NULL;
BITMAP *vs1=NULL,*vs2=NULL;
BITMAP *console = NULL;
BITMAP *con2 = NULL;

static UNIMOD *mordor = NULL;

static volatile int canupdate=0;

int curscreenpos = 0;
int font_height = 0;

int power = 0;
int crit = 0;
int conx = 0;
int cony = 0;

int sintable[180];

volatile int mouselock = 0;

char device[40]="";

DATAFILE *hudata;

void error(char *msg)
{
  allegro_exit();
  puts(msg);
  exit(1);
}

void playsample(SAMPLE *s, int voice, int hz)
{
  if (s==NULL) return;
  MD_PlayStart();
  MD_VoiceSetVolume(voice,64);
  MD_VoiceSetPanning(voice,0);
  MD_VoiceSetFrequency(voice,hz);
  MD_VoicePlay(voice,s->handle,0,s->length,0,0,s->flags);
}

void paint_vscreen(void);
void update_vumeter(void);

void defaultplayer(void)
{
  if (!mordor) return;
  MP_HandleTick();
  MD_SetBPM(mp_bpm);
  update_vumeter();
}

void tickhandler(void)
{
  MD_Update();
}

static END_OF_FUNCTION(tickhandler);

void reset_audio()
{
  int i;
  for (i=0; i<md_numchn;i++) mp_audio[i].volume=0;
}

void draw_cursor(int color)
{
  rectfill(console,conx,cony,conx+8,cony+font_height,color);
}

void con_cls()
{
  conx = 0;
  cony = 0;
  clear_to_color(console,COLOR_CONSOLE);
  draw_cursor(COLOR_CONSOLE);
}

void con_nl(void)
{
  conx = 0;
  cony += font_height;
  if (cony+font_height >= console->h) {
    blit(console, console, 0, font_height, 0, 0, console->w, console->h-font_height);
    rectfill(console, 0,console->h-font_height,console->w,console->h,COLOR_CONSOLE);
    cony -= font_height;
  }
}

void check_mouse(void);

water_info *add_water(water_info *list, int x, int y)
{
  water_info *temp=malloc(sizeof(struct water_info));
  temp->x=x;
  temp->y=y;
  temp->r=2;
  temp->next=list;
  return temp;
}

void add_mouse()
{
  mouse_tails[mouse_tail].x = mouse_x;
  mouse_tails[mouse_tail].y = mouse_y;
  mouse_tails[mouse_tail].level = 4;
  mouse_tail=(mouse_tail+1)%MAX_MOUSE_TAILS;
}

void draw_water(BITMAP *source, BITMAP *dest, int srcx, int srcy, int destx, int desty, int star)
{
  register int r,xp,yp,x1,y1,z;
  for (r=star; r<(star+WATER_THICK); r++) {
    z=sintable[r%180];
    for (x1=0; x1<r; x1++) {
      y1=sqrt((r*r)-(x1*x1));
      xp=(x1*WATER_DEPTH)/z;
      yp=(y1*WATER_DEPTH)/z;
      putpixel(dest,x1+destx,y1+desty, getpixel(source,xp+srcx,yp+srcy));
      putpixel(dest,destx-x1,y1+desty, getpixel(source,srcx-xp,yp+srcy));
      putpixel(dest,destx-x1,desty-y1, getpixel(source,srcx-xp,srcy-yp));
      putpixel(dest,x1+destx,desty-y1, getpixel(source,x1+srcx,srcy-yp));
    }
  }
}

void outc(char *c)
{
  draw_cursor(COLOR_CONSOLE);
  if (c[0]==10) {
    con_nl();
  } else {
    text_mode(COLOR_CONSOLE);
    textout(console, font, c, conx, cony, COLOR_FONT);
    conx+=text_length(font, c);
    if (conx+8 >= console->w) {
      con_nl();
    }
  }
  draw_cursor(COLOR_FONT);
  rest(10);
  check_mouse();
}

void out(char *s)
{
  char temp[2];
  temp[1] = 0;
  while (s[0] != 0) {
    temp[0] = s[0];
    outc(temp);
    s++;
  }
}

void outln(char *s)
{
  out(s);
  draw_cursor(COLOR_CONSOLE);
  con_nl();
  draw_cursor(COLOR_FONT);
}

int inrect(RECT r)
{
  if ((mouse_x>=r.x1)&&(mouse_x<=r.x2)&&(mouse_y>=r.y1)&&(mouse_y<=r.y2)) {
    return -1;
  }
  return 0;
}

void paint_info(void)
{
  RECT r;
  char tmp[32];
  extern unsigned char numrow;
  void infoout(char *s)
  {
    textout(virtualscreen,font,s,r.x1,r.y1,COLOR_INFO);
    r.y1+=font_height;
  }
  if (!power) return;
  r = info;
  text_mode(0);
  infoout(device);
  sprintf(tmp, "%dbit %dkHz", (md_mode&DMODE_16BITS)?16:8,
                                      md_mixfreq/1000);
  infoout(tmp);
  r.y1+=font_height;
  if (mordor&&(!MP_Ready())) {
    sprintf(tmp,"%2X/%2X %2X/%2X",mp_patpos,numrow,mp_sngpos,mordor->numpos);
    infoout(tmp);
  } else r.y1+=font_height;
  r.y1+=font_height;
  sprintf(tmp, "%s  %s",(MP_Ready()?"||":"> "),(md_mode&DMODE_STEREO)?"((o))":"  o");
  infoout(tmp);
}

void paint_diskc(void)
{
  struct module_info *temp;
  RECT r;
  r = diskc;
  for(temp=modlist; temp!=NULL; temp=temp->next) {
    r.y2 = r.y1+8;
    if (temp==focused) {
      text_mode(COLOR_FONT);
      textout(virtualscreen,font,temp->modname,r.x1,r.y1,0);
    } else {
      text_mode(COLOR_CONSOLE);
      textout(virtualscreen,font,temp->modname,r.x1,r.y1,COLOR_FONT);
    }
    r.y1 = r.y2+1;
  }
}

struct module_info *mouse_on_which_mod(void)
{
  RECT r;
  struct module_info *temp;
  r = diskc;
  for(temp=modlist; temp!=NULL; temp=temp->next) {
    r.y2 = r.y1+8;
    if (inrect(r)) return temp;
    r.y1 = r.y2+1;
  }
  return NULL;
}

extern MD_SampleUnload(SWORD i);

SAMPLE *si;

void addmod(char *filename, char *songname)
{
  struct module_info *temp;
  temp=malloc(sizeof(struct module_info));
  strcpy(temp->filename,filename);
  strcpy(temp->modname,(songname==NULL)?"<untitled>":songname);
  if (modlist!=NULL) {
    temp->next=modlist;
  } else {
    temp->next=NULL;
  }
  modlist = temp;
}

void xrectfill(x1,y1,x2,y2,color)
int x1,y1,x2,y2,color;
{
  register int y,c;
  for (y=y1; y<=y2; y++) {
    c=(y&1)?0:color;
    hline(virtualscreen,x1,y,x2,c);
  }
}

void update_vumeter()
{
  int c;
  if (songfinished) reset_audio();
  for (c=0; c<vumeter_channels; c++) {
    if (mp_audio[c].vudata > 0) {
      mp_audio[c].vudata-=1;
      if (mp_audio[c].vudata < 0) mp_audio[c].vudata=0;
    }
  }
}

void paint_vumeter()
{
  int vumeter_xsize;
  int vumeter_ysize=(vumeter.y2-vumeter.y1);
  int vol,barx,bary,c=0;
  if (!power) return;
  if (!vumeter_channels) return;
  vumeter_xsize=(vumeter.x2-vumeter.x1)/vumeter_channels;
  for (barx=vumeter.x1; c<vumeter_channels; c++) {
    vol = mp_audio[c].vudata;
    bary = vumeter_ysize-((vumeter_ysize*vol)/MAX_VOLUME);
    xrectfill(barx,vumeter.y1,barx+vumeter_xsize-VUMETER_GAP,vumeter.y1+bary,COLOR_VUMETER_BACK);
    xrectfill(barx,vumeter.y1+bary,barx+vumeter_xsize-VUMETER_GAP,vumeter.y2,COLOR_VUMETER);
    barx+=vumeter_xsize;
  }
}

void playmod(char *filename)
{
  char tmp[80];
  char *p;
  FILE *fi;
  unsigned char i;
  sprintf(tmp,"loading %s...",filename);
  out(tmp);
  MD_PlayStop();
  if (mordor) {
    reset_audio();
    while (TRUE) {
      for (i=0; i<md_numchn; i++) if (mp_audio[i].vudata > 0) continue;
      break;
    }
    ML_Free(mordor);
  }
  mordor = ML_LoadFN(filename);
  if (mordor==NULL) {
    outln("failed!?");
    return;
  }
  outln("ok");
  MP_Init(mordor);
  md_numchn=mordor->numchn;
  vumeter_channels=mordor->numchn;
  MD_PlayStart();
  songfinished = 0;
  strcpy(tmp,filename);
  p = strstr(tmp,".");
  strcpy(p,".txt");
  fi = fopen(tmp,"r");
  if (fi) {
    con_cls();
    while (!feof(fi)) {
      if (fgets(tmp,80,fi)==NULL) break;
      if (tmp[strlen(tmp)-1]=='\n') tmp[strlen(tmp)-1]=0;
      outln(tmp);
    }
    fclose(fi);
  }
}

void boot()
{
  struct ffblk searchrec;
  __dpmi_free_mem_info info;
  char tmp[80];
  __dpmi_get_free_memory_information(&info);
  sprintf(tmp,"a/OS PC v1.0 - %ldMB vmem",info.largest_available_free_block_in_bytes/(1024*1024));
  outln(tmp);
/*  sprintf(tmp,"gfx driver: %s",gfx_driver->name);
  outln(tmp);*/
  if (windows_version) outln("WARNING: Windows detected. unstable.");
  if (md_driver==&drv_nos) {
    outln("WARNING: no audio devices found");
    outln("check your environment variables");
  } else {
    sprintf(tmp, "%s detected.", device);
    outln(tmp);
    sprintf(tmp, "init: %dbit %dkHz %s sound", (md_mode&DMODE_16BITS)?16:8,
                                        md_mixfreq/1000,
                                        (md_mode&DMODE_STEREO)?"stereo":"mono");
    outln(tmp);
  }
  out("booting \"HereticaL\"...");
  MD_PlayStop();
  if (findfirst("*.*",&searchrec,FA_RDONLY|FA_HIDDEN|FA_SYSTEM|FA_ARCH)==0) {
    do {
      mordor = ML_LoadFN(searchrec.ff_name);
      check_mouse();
      if (mordor) {
        addmod(searchrec.ff_name,mordor->songname);
        out(".");
        ML_Free(mordor);
      }
    } while(findnext(&searchrec)==0);
  }
  if (modlist==NULL) outln("no modules!?"); else {
    outln("ok");
  }
  outln("ready");
}

void set_power()
{
  RGB r;
  int dest;
  if (!power) {
    vumeter_channels=4;
    power=TRUE;
    playsample(power1,0,44100);
    r.r = 63;
    r.g = 0;
    r.b = 0;
    set_color(COLOR_HEADER,&r);
    get_color(COLOR_CONSOLE,&r);
    dest=r.g+10;
    while (r.g<dest) {
      r.g++;
      rest(50);
      set_color(COLOR_CONSOLE,&r);
    }
    playsample(computer,1,11025);
    boot();
  } else {
    power=FALSE;
    MD_PlayStop();
    ML_Free(mordor);
    mordor=NULL;
    playsample(power0,0,44100);
    con_cls();
    modlist = NULL;
    r.r = 0;
    r.g = 0;
    r.b = 0;
    set_color(COLOR_VUMETER,&r);
    get_color(COLOR_CONSOLE,&r);
    while (r.g>1) {
     r.g-=2;
     rest(100);
     set_color(COLOR_CONSOLE,&r);
    }
    conx = mainc.x1;
    cony = mainc.y1;

    exit(0);
  }
}

void check_mouse(void)
{
  if (mouse_b==1) {
    if (!lock_mouse) {
      lock_mouse++;
      if (inrect(mainc)) {
        waterlist_mainc = add_water(waterlist_mainc,mouse_x-mainc.x1,mouse_y-mainc.y1);
      } else if (inrect(diskc)) {
        waterlist_diskc = add_water(waterlist_diskc,mouse_x-diskc.x1,mouse_y-diskc.y1);
      } else if (inrect(info)) {
        waterlist_info = add_water(waterlist_info,mouse_x-info.x1,mouse_y-info.y1);
      }
    }
  } else if (lock_mouse) lock_mouse--;
  focused = mouse_on_which_mod();
  if (crit) return;

  if ((mouse_b == 1)) {
    if (inrect(powerbtn)) {
      set_power();
    }
    crit++;
    if (focused) {
      playmod(focused->filename);
      lastplayed = focused;
    }
    crit--;
  }
}

void paint_console()
{
  struct water_info *temp=waterlist_mainc;
  con2 = create_sub_bitmap(virtualscreen,mainc.x1,mainc.y1,console->w,console->h);
  blit(console,con2,0,0,0,0,console->w,console->h);
  while (temp) {
    draw_water(console,con2,temp->x,temp->y,temp->x,temp->y,temp->r);
    temp=temp->next;
  }
  destroy_bitmap(con2);
  con2=NULL;
}

void update_mice()
{
  int n;
  if ((mouse_lastx!=mouse_x)||(mouse_lasty!=mouse_y)) add_mouse();
  mouse_lastx=mouse_x;
  mouse_lasty=mouse_y;
  for(n=mouse_head; n!=mouse_tail; n=(n+1)%MAX_MOUSE_TAILS) {
    color_map = maps[mouse_tails[n].level];
    draw_trans_sprite(virtualscreen,mice[mouse_tails[n].level],mouse_tails[n].x,mouse_tails[n].y);
    mouse_tails[n].level--;
    if (mouse_tails[n].level==-1) {
      mouse_head=(n+1)%MAX_MOUSE_TAILS;
    }
  }
}

void update_water_mainc()
{
  struct water_info *temp=waterlist_mainc,*p=NULL;
  while (temp) {
    temp->r+=4;
    if (temp->r > WATER_MAXR) {
      if (temp==waterlist_mainc) {
        free(temp);
        waterlist_mainc=NULL;
        return;
      } else {
        p->next=temp->next;
        free(temp);
        temp=p->next;
        continue;
      }
    }
    p=temp;
    temp=temp->next;
  }
}

void update_water_diskc()
{
  struct water_info *temp=waterlist_diskc,*p=NULL;
  while (temp) {
    temp->r+=4;
    if (temp->r > WATER_MAXR) {
      if (temp==waterlist_diskc) {
        free(temp);
        waterlist_diskc=NULL;
        return;
      } else {
        p->next=temp->next;
        free(temp);
        temp=p->next;
        continue;
      }
    }
    p=temp;
    temp=temp->next;
  }
}

void update_water_info()
{
  struct water_info *temp=waterlist_info,*p=NULL;
  while (temp) {
    temp->r+=4;
    if (temp->r > WATER_MAXR) {
      if (temp==waterlist_info) {
        free(temp);
        waterlist_info=NULL;
        return;
      } else {
        p->next=temp->next;
        free(temp);
        temp=p->next;
        continue;
      }
    }
    p=temp;
    temp=temp->next;
  }
}

void paint_header()
{
  if (power) blit(header,virtualscreen,0,0,headerc.x1,headerc.y1,header->w,header->h);
}

void paint_vscreen()
{
  if (canupdate) return;
  canupdate++;
  blit(diskscr,virtualscreen,0,0,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
  paint_header();
  paint_diskc();
  paint_console();
  update_water_mainc();
  paint_vumeter();
  paint_info();
  update_mice();
  color_map = maps[4];
  draw_sprite(virtualscreen,mouse,mouse_x,mouse_y);
  if (curscreenpos==0) {
    scroll_screen(0,SCREEN_HEIGHT);
    virtualscreen=vs1;
    curscreenpos=SCREEN_HEIGHT;
  } else {
    scroll_screen(0,0);
    virtualscreen=vs2;
    curscreenpos=0;
  }
  canupdate--;
}

COLOR_MAP *create_map(int r,int g,int b)
{
  COLOR_MAP *map;
  PALETTE mypal;
  map = malloc(256*256);
  if (map==NULL) error("create_map: couldn't alloc 64k");
  memcpy(mypal,hudata[PAL_PCX].dat,sizeof(PALETTE));
  mypal[COLOR_VUMETER].r=63;
  mypal[COLOR_VUMETER].g=63;
  mypal[COLOR_VUMETER].b=63;
  mypal[COLOR_HEADER].r=63;
  mypal[COLOR_HEADER].g=63;
  mypal[COLOR_HEADER].b=63;
  create_trans_table(map,mypal,r,g,b,NULL);
  return map;
}

void init_sound(void)
{
	md_mixfreq      = 44100;
	md_dmabufsize   = 10000;
	md_mode         = DMODE_16BITS|DMODE_STEREO;
	md_device       = 0;
   md_numchn       = 4;

   mp_loop = 0;

	ML_RegisterLoader(&load_mod);
	ML_RegisterLoader(&load_xm);
	ML_RegisterLoader(&load_uni);

	MD_RegisterDriver(&drv_nos);
	MD_RegisterDriver(&drv_sb);
	MD_RegisterDriver(&drv_gus);
   MD_RegisterDriver(&drv_ss);

   MD_RegisterPlayer(defaultplayer);

   if (!MD_Init()) error("init_sound: something terrible has happened");

   LOCK_FUNCTION(tickhandler);
   install_int_ex(tickhandler, BPS_TO_TIMER(50));

   atexit(MD_Exit);

   if (md_driver==&drv_sb) strcpy(device, "SoundBlaster");
   else if (md_driver==&drv_gus) strcpy(device, "GUS");
   else if (md_driver==&drv_ss) strcpy(device, "SoundScape");
   else if (md_driver==&drv_nos) strcpy(device, "");

   power0=MW_LoadWavFN(POWER0);
   power1=MW_LoadWavFN(POWER1);
   computer=MW_LoadWavFN(COMPUTER);
}

BITMAP *create_small(BITMAP *bmp, int percent)
{
  BITMAP *newbmp;
  newbmp = create_bitmap((bmp->w*percent)/100,(bmp->h*percent)/100);
  if (newbmp==NULL) error("create_small: no more memory :P");
  stretch_blit(bmp, newbmp, 0,0,bmp->w,bmp->h,0,0,newbmp->w,newbmp->h);
  return newbmp;
}

void init_sintable()
{
  double f;
  int r;
  for (f=0.0; f<180.0; f++) {
    r = f;
    sintable[r]=WATER_DEPTH-((sin(f)*(WATER_DEPTH-1)));
  }
}

void init_all_gfx()
{
  RGB r;
  install_keyboard();
  if (set_gfx_mode(GFX_AUTODETECT,SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_WIDTH,SCREEN_HEIGHT*2))
    error("init_all_gfx: failed. 1MB VESA compliant SVGA required");
  set_palette(hudata[AFT4_PALETTE].dat);
  blit(hudata[AFT4_PCX].dat,screen,0,0,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);

  init_sintable();

  maps[4] = create_map(168,168,168);
  maps[3] = create_map(158,158,158);
  maps[2] = create_map(148,148,148);
  maps[1] = create_map(138,138,138);
  maps[0] = create_map(128,128,128);
  vs1 = create_sub_bitmap(screen,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
  vs2 = create_sub_bitmap(screen,0,SCREEN_HEIGHT,SCREEN_WIDTH,SCREEN_HEIGHT);
  virtualscreen = vs2;
  if (!(vs1||vs2)) error("init_all_gfx: non mem for vs1 or vs2");
  console = create_bitmap((mainc.x2-mainc.x1)+1,(mainc.y2-mainc.y1)+1);
  con_cls();
  mouse = hudata[MOUSE_PCX].dat;
  mice[4]=hudata[MOUSE_PCX].dat;
  mice[3]=create_small(mice[4],85);
  mice[2]=create_small(mice[4],65);
  mice[1]=create_small(mice[4],45);
  mice[0]=create_small(mice[4],25);
  diskscr = hudata[DISKSCR_PCX].dat;
  header = hudata[HEADER_PCX].dat;
  fade_out(2);
  clear(screen);
  set_palette(hudata[PAL_PCX].dat);
  r.r = 25;
  r.g = 20;
  r.b = 0;
  set_color(COLOR_VUMETER_BACK,&r);
  blit(diskscr,virtualscreen,0,0,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
  mouse_lastx = mouse_x;
  mouse_lasty = mouse_y;
  font = hudata[SYS_FONT].dat;
  font_height = text_height(font);
  install_int_ex(paint_vscreen,BPS_TO_TIMER(24));
  clear_keybuf();
}

void shutdown()
{
  puts("\"HereticaL\" musicdisk þ code: SSG þ gfx: Cori þ zax: Heretic\n\nan aRtEffECt production");
}

void init_datafile(void)
{
  allegro_init();
  install_timer();
  hudata = load_datafile(DATFILE);
  if (hudata == NULL) error("huh? couldn't read resources!?");
  if (!install_mouse()) error("mouse required");
  atexit(allegro_exit);
}

void init_startup()
{
  atexit(shutdown);
  __djgpp_set_ctrl_c(0);
}

void main(int argc, char *argv[])
{
  init_startup();
  init_datafile();
  init_sound();
  init_all_gfx();
  while (TRUE) {
    check_mouse();
    if (keypressed()) if ((readkey()>>8)==KEY_ESC) break;
  }
}
/**** eof ****/
