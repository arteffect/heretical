/* unused code in hu */

/*
#define C64_BORDER      238
#define C64_CURSOR      239
#define C64_TEXT        240
#define C64_BACKGROUND  102
*/

/*void mycallback(int pos)
{
  RGB r;
  r.r = random();
  r.g = random();
  r.b = random();
  set_color(C64_BORDER,&r);
}

void put_centered(char *s)
{
  textout(screen,font,s,(SCREEN_WIDTH-text_length(font,s))/2,(SCREEN_HEIGHT-text_height(font))/2,1);
}

void c64_cursor(int put)
{
  rectfill(screen,(conx*8)+20,(cony*8)+20,(conx*8)+27,(cony*8)+27,put?C64_CURSOR:C64_BACKGROUND);
}

void c64_out(char *s)
{
  unsigned char c;
  unsigned char tmp[40];
  strcpy(tmp,s);
  for (c=0; s[c]; c++) tmp[c]=xlat[(unsigned char)s[c]];
  tmp[strlen(s)]=0;
  textout(screen,font,tmp,(conx*8)+20,(cony*8)+20,C64_TEXT);
  cony++;
}

void c64_cursor_handler()
{
  RGB r,r1,r2;
  get_color(C64_CURSOR,&r);
  get_color(C64_BORDER,&r1);
  get_color(C64_BACKGROUND,&r2);
  if (memcmp(&r,&r1,sizeof(RGB))==0) set_color(C64_CURSOR,&r2);
    else set_color(C64_CURSOR,&r1);
}

void c64_key(char *s)
{
  unsigned char c;
  unsigned char fuck[2];
  fuck[1]=0;
  c64_cursor(TRUE);
  rest(2000);
  for (c=0; s[c]; c++) {
    c64_cursor(TRUE);
    rest(150);
    fuck[0]=xlat[(unsigned char)s[c]];
    textout(screen,font,fuck,20+(conx*8),20+(cony*8),C64_TEXT);
    conx++;
  }
  c64_cursor(FALSE);
  cony++;
  conx=0;
}

void build_sintable()
{
  int i;
  for (i=0; i < 512; i++) {
    sintable[i] = sin(i*(2*M_PI)/512)*16384;
    costable[i] = cos(i*(2*M_PI)/512)*16384;
  }
}

void c64_init()
{
  FONT *c64font;
  RGB r;
  c64font=malloc(sizeof(FONT));
  c64font->dat.dat_8x8=hudata[C64_FONT].dat;
  c64font->height=8;
  if (set_gfx_mode(GFX_MODEX,360,240,0,0)) error("init_modex: god has just failed");
  set_palette(hudata[CBM_PCX].dat);
  get_color(C64_BORDER,&r);
  set_color(C64_TEXT,&r);
  rectfill(screen,0,0,360,19,C64_BORDER);
  rectfill(screen,0,20,19,219,C64_BORDER);
  rectfill(screen,340,20,360,219,C64_BORDER);
  rectfill(screen,0,220,360,240,C64_BORDER);
  rectfill(screen,20,20,339,219,C64_BACKGROUND);
  font=c64font;
  text_mode(C64_BACKGROUND);
  conx=0;
  cony=1;
  c64_out("    **** commodore 64 basic v2 ****");
  cony++;
  c64_out(" 64k ram system  38911 basic bytes free");
  cony++;
  c64_out("ready.");
  install_int_ex(c64_cursor_handler,BPS_TO_TIMER(2));
  c64_key("load\"*\",8,1");
  cony++;
  c64_out("searching for *");
  build_sintable();
  rest(2000);
  c64_out("loading");
  get_color(C64_TEXT,&r);
  set_color(C64_BORDER,&r);
  c64_out("ready.");
  c64_key("run");
  rest(1000);
  remove_int(c64_cursor_handler);
  clear_keybuf();
}*/

