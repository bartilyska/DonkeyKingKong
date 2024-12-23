#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH 640 //szerokosc ekranu
#define SCREEN_HEIGHT 480 //wysokosc ekranu
#define EXTRA_BOUND 0.95 //przyblizenie dokladnej lokalizacji postaci i beczek
#define HALF_BITMAP 14 //pol rozmiaru bitmapy
#define SPEED 70.0 //predkosc postaci
#define PLATFORM_HEIGHT 10 //wysokosc platformy
#define PLATFORMS 5 //liczba platform
#define LADDERS 5 //liczba drabin
#define LADDER_WIDTH 20 //szerokosc drabiny
#define GRAVITY 175 //predkosc grawitacyjna(jak mocno ciagnie postac w dol)
#define JUMP_DROP 310 //jak szybko postac ma spadac w trakcie skoku
#define JUMP_HEIGHT 350 //predkosc pocztkowa skoku 
#define MAPS 3 //liczba map
#define START_MENU 50 //x i y prawego gornego rogu menu
#define END_BOX 150//dlugosc boku prostokata na x ktory przenosi do nastepnego etapu
#define HALL_OF_FAME 5//liczba osob ktore beda sie wyswietlac na liscie
#define BARRELS 10 //liczba beczek na ekranie
#define GRA 0 //bez menu czyli gra
#define MENU 1 //menuglowne
#define SAVE 2 //menu do zapisu
#define DEAD 3 //menu po smierci
#define BARREL_SPEED 120 //predkosc beczek na x na y gravity
#define BARREL_SPAWN_RATE 3 //beczki sie pojawiaja co 3 sekundy
#define BARREL_ANIM_TIME 0.4 // animacja beczek dzieje sie co polowe tego czasu

// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};


// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};
//rysuje drabine podobnie jak prostokat
void DrawLadder(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	for (i = x; i < x + 4; i++)
		DrawLine(screen, i, y + 1, k - 2, 0, 1, fillColor);
	for (i = x + 16; i < x + 20; i++)
		DrawLine(screen, i, y + 1, k - 2, 0, 1, fillColor);
	for (i = y; i < y + k - 1; i++)
	{
		if (i < y + k - 10)
		{
			for (int j = i; j < i + 4; j++)
				DrawLine(screen, x + 1, j, l - 2, 1, 0, fillColor);
		}
		i += 10;
	}
}
struct elements
{
	SDL_Event event;
	SDL_Surface* screen, * charset, * marioprawo, * mariolewo, * marioskok, * mariodrabina;
	SDL_Surface* mario, * trophy, * heart, * kong, * kongnormal, * kongmove, * barrel, * barrel1, * showbarrel;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
};
void FreeSDL(elements* sdl)
{
	SDL_FreeSurface(sdl->charset);
	SDL_FreeSurface(sdl->marioprawo);
	SDL_FreeSurface(sdl->mariolewo);
	SDL_FreeSurface(sdl->marioskok);
	SDL_FreeSurface(sdl->mariodrabina);
	SDL_FreeSurface(sdl->mario);
	SDL_FreeSurface(sdl->heart);
	SDL_FreeSurface(sdl->kong);
	SDL_FreeSurface(sdl->kongnormal);
	SDL_FreeSurface(sdl->kongmove);
	SDL_FreeSurface(sdl->screen);
	SDL_FreeSurface(sdl->trophy);
	SDL_FreeSurface(sdl->barrel);
	SDL_FreeSurface(sdl->barrel1);
	SDL_FreeSurface(sdl->showbarrel);
	SDL_DestroyTexture(sdl->scrtex);
	SDL_DestroyRenderer(sdl->renderer);
	SDL_DestroyWindow(sdl->window);
	SDL_Quit();
}
void initializeBitMaps(elements* sdl)
{
	// wczytanie obrazka cs8x8.bmp
	sdl->charset = SDL_LoadBMP("./sprites/cs8x8.bmp");
	if (sdl->charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->charset, true, 0x000000);

	sdl->marioprawo = SDL_LoadBMP("./sprites/mario.bmp");
	if (sdl->marioprawo == NULL) {
		printf("SDL_LoadBMP(mario.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->marioprawo, true, 0xFFFFFF);

	sdl->mariolewo = SDL_LoadBMP("./sprites/mariolewo.bmp");
	if (sdl->mariolewo == NULL) {
		printf("SDL_LoadBMP(mariolewo.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->mariolewo, true, 0xFFFFFF);


	sdl->marioskok = SDL_LoadBMP("./sprites/marioskok.bmp");
	if (sdl->marioskok == NULL) {
		printf("SDL_LoadBMP(marioskok.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->marioskok, true, 0xFFFFFF);

	sdl->mariodrabina = SDL_LoadBMP("./sprites/mariodrabina.bmp");
	if (sdl->mariodrabina == NULL) {
		printf("SDL_LoadBMP(mariodrabina.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->mariodrabina, true, 0xFFFFFF);

	sdl->trophy = SDL_LoadBMP("./sprites/trophy.bmp");
	if (sdl->trophy == NULL) {
		printf("SDL_LoadBMP(trophy.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->trophy, true, 0xFFFFFF);

	sdl->heart = SDL_LoadBMP("./sprites/serce.bmp");
	if (sdl->heart == NULL) {
		printf("SDL_LoadBMP(serce.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->heart, true, 0xFFFFFF);

	sdl->barrel = SDL_LoadBMP("./sprites/beczka.bmp");
	if (sdl->barrel == NULL) {
		printf("SDL_LoadBMP(beczka.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->barrel, true, 0xFFFFFF);

	sdl->barrel1 = SDL_LoadBMP("./sprites/beczka2.bmp");
	if (sdl->barrel1 == NULL) {
		printf("SDL_LoadBMP(beczka2.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->barrel1, true, 0xFFFFFF);

	sdl->kongnormal = SDL_LoadBMP("./sprites/kingkong2.bmp");
	if (sdl->kongnormal == NULL) {
		printf("SDL_LoadBMP(kingkong.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->kongnormal, true, 0xFFFFFF);

	sdl->kongmove = SDL_LoadBMP("./sprites/kingkong.bmp");
	if (sdl->kongmove == NULL) {
		printf("SDL_LoadBMP(kingkong.bmp) error: %s\n", SDL_GetError());
		FreeSDL(sdl);
		exit(0);
	};
	SDL_SetColorKey(sdl->kongmove, true, 0xFFFFFF);
	sdl->showbarrel = sdl->barrel1;
	sdl->kong = sdl->kongnormal;
	sdl->mario = sdl->marioprawo;
}
void initializeSDL(elements* sdl)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(0);
	}
	int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdl->window, &sdl->renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		exit(0);
	};
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(sdl->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(sdl->renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(sdl->window, "Projekt King Donkey");
	sdl->screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	sdl->scrtex = SDL_CreateTexture(sdl->renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_ShowCursor(SDL_DISABLE);

	initializeBitMaps(sdl);
}

struct cols
{
	int czarny, zielony, czerwony, niebieski, podloga, drabina;
};
void initializeColors(elements* sdl, cols* colors)
{
	colors->czarny = SDL_MapRGB(sdl->screen->format, 0x00, 0x00, 0x00);
	colors->zielony = SDL_MapRGB(sdl->screen->format, 0x00, 0xFF, 0x00);
	colors->czerwony = SDL_MapRGB(sdl->screen->format, 0xFF, 0x00, 0x00);
	colors->niebieski = SDL_MapRGB(sdl->screen->format, 0x0b, 0x9a, 0xaa);
	colors->podloga = SDL_MapRGB(sdl->screen->format, 0xb2, 0x12, 0x3d);
	colors->drabina = SDL_MapRGB(sdl->screen->format, 0x0e, 0xcd, 0xcc);
}
struct params
{
	int t1, quit, frames;
	double worldTime, fpsTimer, fps, distancex, distancey, speedx, speedy, gravity, speedjump, timetoshowpoints;
	int moverigth, moveleft, moveup, movedown, jump;
	int menu, lifes, points, showtrophy; //menu=0-gra menu=1-glowne menu=2-koncowkowe
	int showpoints; //przechowuje zmienna do pokazywania punktow na ekran
	double barrelx[BARRELS], barrely[BARRELS], barrelspeedx[BARRELS], barrelspeedy[BARRELS], timetofall, timebarrelanim;
	int barreljumped[BARRELS]; // czy przeskoczona zeby nie liczylo za duzo razy pkt
	char nick[100];
};
struct staticThings
{
	int map; //ktora mapa
	int startplatformx[PLATFORMS]; //x,y gornego prawego prostokata
	int startplatformy[PLATFORMS];
	int lengthplatform[PLATFORMS];
	int startladderx[LADDERS];
	int startladdery[LADDERS];
	int lengthladder[LADDERS];
	int startx, starty; //pozycja startowa etapow
	int endx, endy; //koncowa pozycja etapow
	int trophyx, trophyy; //pozycja trofeum i czy pokazywac
	int kongx, kongy;//pozycja konga
	double barrelx, barrely;//pozycje beczek startowe
};
void zeroStatic(staticThings* things)
{
	things->map = 0;
	for (int i = 0; i < PLATFORMS; i++)
	{
		things->startplatformx[i] = 0;
		things->startplatformy[i] = 0;
		things->lengthplatform[i] = 0;
	}
	for (int i = 0; i < LADDERS; i++)
	{
		things->startladderx[i] = 0;
		things->startladdery[i] = 0;
		things->lengthladder[i] = 0;
	}
	things->startx = 0;
	things->starty = 0;
	things->endx = 0;
	things->endy = 0;
	things->trophyx = 0;
	things->trophyy = 0;
	things->kongx = 0;
	things->kongy = 0;
	things->barrelx = 0;
	things->barrely = 0;
}
//pierwszy wiersz - 1 mapa,drugi druga. Ustawienie pozycji elementow map
void initializeStatic(staticThings* things, int k)
{
	things->map = k;
	int platformx[MAPS][PLATFORMS] = {
	{0,300,150,390,100},
	{0,200,0,200,0},
	{60,240,390,550,0}
	};
	int platformy[MAPS][PLATFORMS] = {
	{SCREEN_HEIGHT - 10,SCREEN_HEIGHT - 60,SCREEN_HEIGHT - 150,SCREEN_HEIGHT - 250,SCREEN_HEIGHT - 340},
	{SCREEN_HEIGHT - 10,SCREEN_HEIGHT - 100,SCREEN_HEIGHT - 190,SCREEN_HEIGHT - 280,SCREEN_HEIGHT - 330},
	{SCREEN_HEIGHT - 50,SCREEN_HEIGHT - 135,SCREEN_HEIGHT - 220,SCREEN_HEIGHT - 265,SCREEN_HEIGHT - 380}
	};
	int lengthplatform[MAPS][PLATFORMS] = {
	{SCREEN_WIDTH - 50,SCREEN_WIDTH - 440,SCREEN_WIDTH - 350,SCREEN_WIDTH - 390,SCREEN_WIDTH - 160},
	{SCREEN_WIDTH - 100,SCREEN_WIDTH - 200,SCREEN_WIDTH - 100,SCREEN_WIDTH - 200,SCREEN_WIDTH - 120},
	{SCREEN_WIDTH - 400,SCREEN_WIDTH - 400,SCREEN_WIDTH - 420,SCREEN_WIDTH - 550,SCREEN_WIDTH - 100}
	};

	for (int i = 0; i < PLATFORMS; i++)
	{
		things->startplatformx[i] = platformx[things->map][i];
		things->startplatformy[i] = platformy[things->map][i];
		things->lengthplatform[i] = lengthplatform[things->map][i];
	}
	int ladderx[MAPS][LADDERS] = {
	{360,450,300,420,510},
	{400,250,300,230,510},
	{265,430,590,200,465}
	};
	int laddery[MAPS][LADDERS] = {
	{ 420,420,SCREEN_HEIGHT - 150,SCREEN_HEIGHT - 250,SCREEN_HEIGHT - 340},
	{ SCREEN_HEIGHT - 100,SCREEN_HEIGHT - 100,SCREEN_HEIGHT - 190,SCREEN_HEIGHT - 280,SCREEN_HEIGHT - 280},
	{ SCREEN_HEIGHT - 135,SCREEN_HEIGHT - 220,SCREEN_HEIGHT - 265,SCREEN_HEIGHT - 380,SCREEN_HEIGHT - 380}
	};
	int lengthladder[MAPS][LADDERS] = {
	{50,50,90,100,90},
	{90,90,90,90,90},
	{85,85,45,100,90}
	};

	for (int i = 0; i < LADDERS; i++)
	{
		things->startladderx[i] = ladderx[things->map][i];
		things->startladdery[i] = laddery[things->map][i];
		things->lengthladder[i] = lengthladder[things->map][i];
	}
	int startx[MAPS] = { 20,20,80 };
	int starty[MAPS] = { 470 - HALF_BITMAP,470 - HALF_BITMAP,430 - HALF_BITMAP };
	things->startx = startx[things->map];
	things->starty = starty[things->map];
	int endx[MAPS] = { 100,0,0 };
	int endy[MAPS] = { SCREEN_HEIGHT - 340 - HALF_BITMAP,SCREEN_HEIGHT - 330 - HALF_BITMAP,SCREEN_HEIGHT - 380 - HALF_BITMAP };
	things->endx = endx[things->map];
	things->endy = endy[things->map];
	int trophyx[MAPS] = { 160,600,197 + HALF_BITMAP };
	int trophyy[MAPS] = { SCREEN_HEIGHT - 150 - HALF_BITMAP,SCREEN_HEIGHT - 100 - HALF_BITMAP,190 };
	things->trophyx = trophyx[things->map];
	things->trophyy = trophyy[things->map];
	int kongx[MAPS] = { 130,30,30 };
	int kongy[MAPS] = { SCREEN_HEIGHT - 365 ,SCREEN_HEIGHT - 355,SCREEN_HEIGHT - 405 };
	things->kongx = kongx[things->map];
	things->kongy = kongy[things->map];
	int barrelx[MAPS] = { 170,70,70 };
	int barrely[MAPS] = { SCREEN_HEIGHT - 340 - HALF_BITMAP ,SCREEN_HEIGHT - 330 - HALF_BITMAP ,SCREEN_HEIGHT - 380 - HALF_BITMAP };
	things->barrelx = barrelx[things->map];
	things->barrely = barrely[things->map];
}
void zeroGame(params* game)
{
	game->frames = 0;
	game->fpsTimer = 0;
	game->fps = 0;
	game->quit = 0;
	game->worldTime = 0;
	game->distancex = 0;
	game->distancey = 0;
	game->speedx = 0;
	game->speedy = 0;
	game->moverigth = 0;
	game->moveleft = 0;
	game->moveup = 0;
	game->movedown = 0;
	game->jump = 0;
	game->speedjump = 0;
	game->t1 = 0;
	game->menu = GRA;
	game->gravity = 0;
	game->lifes = 0;
	game->showtrophy = 0;
	strcpy(game->nick, "");
	game->timetoshowpoints = 0;
	game->showpoints = 0;
	for (int i = 0; i < BARRELS; i++)
	{
		game->barrelx[i] = 0;
		game->barrely[i] = 0;
		game->barrelspeedx[i] = 0;
		game->barrelspeedy[i] = 0;
		game->barreljumped[i] = 0;
	}
	game->timetofall = 0;
	game->timebarrelanim = 0;
}
void startGame(params* game, staticThings* things)
{
	game->frames = 0;
	game->fpsTimer = 0;
	game->fps = 0;
	game->quit = 0;
	game->worldTime = 0;
	game->distancex = things->startx;
	game->distancey = things->starty;
	game->speedx = 0;
	game->speedy = 0;
	game->moverigth = 0;
	game->moveleft = 0;
	game->moveup = 0;
	game->movedown = 0;
	game->jump = 0;
	game->speedjump = 0;
	game->t1 = SDL_GetTicks();
	game->menu = GRA;
	game->gravity = 0;
	game->lifes = 3;
	game->points = 0;
	game->showtrophy = 1;
	game->timetoshowpoints = 0;
	for (int i = 0; i < BARRELS; i++)
	{
		game->barrelx[i] = things->barrelx;
		game->barrely[i] = things->barrely;
		game->barrelspeedx[i] = 0;
		game->barrelspeedy[i] = 0;
		game->barreljumped[i] = 0;
	}
	game->timetofall = 0.9;
	game->timebarrelanim = BARREL_ANIM_TIME;
}
void continueGame(params* game, staticThings* things)
{
	game->distancex = things->startx;
	game->distancey = things->starty;
	game->speedx = 0;
	game->speedy = 0;
	game->moverigth = 0;
	game->moveleft = 0;
	game->moveup = 0;
	game->movedown = 0;
	game->jump = 0;
	game->speedjump = 0;
	game->gravity = 0;
	game->showtrophy = 1;
	game->timetoshowpoints = 0;
	for (int i = 0; i < BARRELS; i++)
	{
		game->barrelx[i] = things->barrelx;
		game->barrely[i] = things->barrely;
		game->barrelspeedx[i] = 0;
		game->barrelspeedy[i] = 0;
		game->barreljumped[i] = 0;
	}
	game->timetofall = 0.9;
	game->timebarrelanim = BARREL_ANIM_TIME;
}
//ustawia grawitacje
void onPlatform(params* game, cols* colors, elements* sdl, staticThings* things)
{
	int ok = 0;
	for (int i = 0; i < PLATFORMS; i++) {

		double lowerBoundy = things->startplatformy[i] - EXTRA_BOUND;
		double upperBoundy = things->startplatformy[i] + EXTRA_BOUND;
		double lowerBoundx = things->startplatformx[i] - EXTRA_BOUND;
		double upperBoundx = things->startplatformx[i] + EXTRA_BOUND + things->lengthplatform[i];

		if (game->distancey + HALF_BITMAP >= lowerBoundy && game->distancey + HALF_BITMAP <= upperBoundy &&
			game->distancex + HALF_BITMAP >= lowerBoundx && game->distancex - HALF_BITMAP <= upperBoundx)
		{ //sprawdzanie i ustawianie grawitacji dla postaci
			game->gravity = 0;
			ok = 1;
		}
	}
	if (ok == 0)
		game->gravity = GRAVITY;
}
int onPlatformcheck(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < PLATFORMS; i++) {

		double lowerBoundy = things->startplatformy[i] - EXTRA_BOUND;
		double upperBoundy = things->startplatformy[i] + EXTRA_BOUND;
		double lowerBoundx = things->startplatformx[i] - EXTRA_BOUND;
		double upperBoundx = things->startplatformx[i] + EXTRA_BOUND + things->lengthplatform[i];

		if (game->distancey + HALF_BITMAP >= lowerBoundy && game->distancey + HALF_BITMAP <= upperBoundy &&
			game->distancex + HALF_BITMAP >= lowerBoundx && game->distancex - HALF_BITMAP <= upperBoundx)
		{
			return 1;
		}
	}
	return 0;
}
//potrzebne zwiekszenie zakresu do animacji
int onPlatformcheckEdit(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < PLATFORMS; i++) {

		double lowerBoundy = things->startplatformy[i] - 5 * EXTRA_BOUND;
		double upperBoundy = things->startplatformy[i] + 5 * EXTRA_BOUND;
		double lowerBoundx = things->startplatformx[i] - EXTRA_BOUND;
		double upperBoundx = things->startplatformx[i] + EXTRA_BOUND + things->lengthplatform[i];

		if (game->distancey + HALF_BITMAP >= lowerBoundy && game->distancey + HALF_BITMAP <= upperBoundy &&
			game->distancex + HALF_BITMAP >= lowerBoundx && game->distancex - HALF_BITMAP <= upperBoundx)
		{
			return 1;
		}
	}
	return 0;
}
//nastawia grawitacje
void onLadder(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < LADDERS; i++) {

		double lowerBoundy = things->startladdery[i] - EXTRA_BOUND;
		double upperBoundy = things->startladdery[i] + EXTRA_BOUND + things->lengthladder[i];
		double lowerBoundx = things->startladderx[i] - EXTRA_BOUND;
		double upperBoundx = things->startladderx[i] + EXTRA_BOUND + LADDER_WIDTH;

		if (game->distancey + HALF_BITMAP >= lowerBoundy && game->distancey + HALF_BITMAP <= upperBoundy &&
			game->distancex >= lowerBoundx && game->distancex <= upperBoundx)
		{
			game->gravity = 0;
		}
	}
}
int onLaddercheck(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < LADDERS; i++) {

		double lowerBoundy = things->startladdery[i] - EXTRA_BOUND;
		double upperBoundy = things->startladdery[i] + EXTRA_BOUND + things->lengthladder[i];
		double lowerBoundx = things->startladderx[i] - EXTRA_BOUND;
		double upperBoundx = things->startladderx[i] + EXTRA_BOUND + LADDER_WIDTH;

		if (game->distancey + HALF_BITMAP >= lowerBoundy && game->distancey + HALF_BITMAP <= upperBoundy &&
			game->distancex >= lowerBoundx && game->distancex <= upperBoundx)
		{
			return 1;
		}
	}
	return 0;
}
//potrzebna by nie wpadala postac pod platforme
int onBottomLaddercheck(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < LADDERS; i++) {

		double lowerBoundy = things->startladdery[i] - EXTRA_BOUND + things->lengthladder[i];
		double upperBoundy = things->startladdery[i] + EXTRA_BOUND + things->lengthladder[i];
		double lowerBoundx = things->startladderx[i] - EXTRA_BOUND;
		double upperBoundx = things->startladderx[i] + EXTRA_BOUND + LADDER_WIDTH;

		if (game->distancey + HALF_BITMAP >= lowerBoundy && game->distancey + HALF_BITMAP <= upperBoundy &&
			game->distancex >= lowerBoundx && game->distancex <= upperBoundx)
		{
			return 1;
		}
	}
	return 0;
}
//potrzebna by nie blokowac skokow z samej gory drabinki
int onTopLaddercheck(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < LADDERS; i++) {

		double lowerBoundy = things->startladdery[i] - EXTRA_BOUND;
		double upperBoundy = things->startladdery[i] + EXTRA_BOUND;
		double lowerBoundx = things->startladderx[i] - EXTRA_BOUND;
		double upperBoundx = things->startladderx[i] + EXTRA_BOUND + LADDER_WIDTH;

		if (game->distancey + HALF_BITMAP >= lowerBoundy && game->distancey + HALF_BITMAP <= upperBoundy &&
			game->distancex >= lowerBoundx && game->distancex <= upperBoundx)
		{
			return 1;
		}
	}
	return 0;
}
//powiekszenie zakresu w celu unikniecia zbyt czestej lagow zmiany animacji
int onTopLaddercheckEdit(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < LADDERS; i++) {

		double lowerBoundy = things->startladdery[i] - 5 * EXTRA_BOUND;
		double upperBoundy = things->startladdery[i] + 5 * EXTRA_BOUND;
		double lowerBoundx = things->startladderx[i] - EXTRA_BOUND;
		double upperBoundx = things->startladderx[i] + EXTRA_BOUND + LADDER_WIDTH;

		if (game->distancey + HALF_BITMAP >= lowerBoundy && game->distancey + HALF_BITMAP <= upperBoundy &&
			game->distancex >= lowerBoundx && game->distancex <= upperBoundx)
		{
			return 1;
		}
	}
	return 0;
}
int hitPlatformcheck(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < PLATFORMS; i++) {

		double lowerBoundy = things->startplatformy[i] - EXTRA_BOUND + PLATFORM_HEIGHT;
		double upperBoundy = things->startplatformy[i] + EXTRA_BOUND + PLATFORM_HEIGHT;
		double lowerBoundx = things->startplatformx[i] - EXTRA_BOUND;
		double upperBoundx = things->startplatformx[i] + EXTRA_BOUND + things->lengthplatform[i];

		if (game->distancey - HALF_BITMAP >= lowerBoundy && game->distancey - HALF_BITMAP <= upperBoundy &&
			game->distancex + HALF_BITMAP >= lowerBoundx && game->distancex - HALF_BITMAP <= upperBoundx)
		{
			return 1;
		}
	}
	return 0;
}
//zmiana kierunku ruchu beczek przy wyjezdzie za ekran i 
//powrot gdy spadna ponizej ekranu
void barrelsChangeDirection(params* game, cols* colors, elements* sdl, staticThings* things)
{
	for (int i = 0; i < BARRELS; i++)
	{
		int ok = 0;
		for (int j = 0; j < PLATFORMS; j++)
		{
			double lowerBoundy = things->startplatformy[j] - EXTRA_BOUND;
			double upperBoundy = things->startplatformy[j] + 3 * EXTRA_BOUND;
			double lowerBoundx = things->startplatformx[j] - EXTRA_BOUND;
			double upperBoundx = things->startplatformx[j] + EXTRA_BOUND + things->lengthplatform[j];
			if (game->barrely[i] + HALF_BITMAP >= lowerBoundy && game->barrely[i] + HALF_BITMAP <= upperBoundy &&
				game->barrelx[i] + HALF_BITMAP >= lowerBoundx && game->barrelx[i] - HALF_BITMAP <= upperBoundx)
			{
				game->barrelspeedy[i] = 0;
				ok = 1;
				break;
			}
		}
		if (ok == 0)
			game->barrelspeedy[i] = GRAVITY;
	}
	for (int i = 0; i < BARRELS; i++)
	{
		if (game->barrelx[i] <= 0 && game->barrelspeedx[i] < 0)
		{
			game->barrelspeedx[i] = BARREL_SPEED;
		}
		else if (game->barrelx[i] >= SCREEN_WIDTH && game->barrelspeedx[i] > 0)
		{
			game->barrelspeedx[i] = -BARREL_SPEED;
		}
		if (game->barrely[i] > SCREEN_HEIGHT)
		{
			game->barrelx[i] = things->barrelx;
			game->barrely[i] = things->barrely;
			game->barrelspeedx[i] = 0;
			game->barreljumped[i] = 0;// gdy beczka wraca to mozna znowu ja przeskoczyc
		}
	}
}
void setSpeed(params* game, cols* colors, elements* sdl, staticThings* things)
{
	if (game->moverigth == 1 && game->moveleft == 1) game->speedx = 0;
	else if (game->moverigth == 1) game->speedx = SPEED;
	else if (game->moveleft == 1) game->speedx = -SPEED;
	else game->speedx = 0;
	if (onPlatformcheck(game, colors, sdl, things) && game->jump == 1 && game->speedjump < GRAVITY)
	{
		game->speedjump = -JUMP_HEIGHT;
	}
	if (onLaddercheck(game, colors, sdl, things))
	{
		if (!onTopLaddercheck(game, colors, sdl, things))game->speedjump = 0;
		if (game->moveup == 1 && game->movedown == 1) game->speedy = 0;
		else if (game->moveup == 1) game->speedy = -0.7 * SPEED;
		else if (game->movedown == 1 && !onBottomLaddercheck(game, colors, sdl, things))
			game->speedy = 0.7 * SPEED;
		else game->speedy = 0;
	}
	else
	{
		game->speedy = 0;
	}
	if (hitPlatformcheck(game, colors, sdl, things) && game->speedjump < 0)
	{
		game->speedjump = GRAVITY - 1;
	}
}
//odpowiednie ustawianie bitmap 
//funkcje "Edit" sa w celu unikniecia bledow animacji
void setBitmap(params* game, cols* colors, elements* sdl, staticThings* things)
{
	if (game->speedx > 0) sdl->mario = sdl->marioprawo;
	if (game->speedx < 0)  sdl->mario = sdl->mariolewo;
	if (!onPlatformcheckEdit(game, colors, sdl, things) && !onTopLaddercheckEdit(game, colors, sdl, things))
		sdl->mario = sdl->marioskok;
	if (game->speedy != 0 || (onLaddercheck(game, colors, sdl, things) && !onBottomLaddercheck(game, colors, sdl, things)
		&& !onTopLaddercheck(game, colors, sdl, things) && game->speedy == 0))
		sdl->mario = sdl->mariodrabina;
	if (sdl->mario == sdl->marioskok && onPlatformcheck(game, colors, sdl, things))
		sdl->mario = sdl->marioprawo; //zeby odbugowac postac ze skoku po spadnieciu
	if (game->timetofall > 0 && game->timetofall < 0.9)
	{
		sdl->kong = sdl->kongmove; //zmiana animacji na 0.9s przed wyrzuceniem beczki
	}
	else sdl->kong = sdl->kongnormal;
	if (game->timebarrelanim > 0 && game->timebarrelanim < BARREL_ANIM_TIME / 2)
	{
		sdl->showbarrel = sdl->barrel;
	}
	else sdl->showbarrel = sdl->barrel1;
}
void collectPoints(params* game, cols* colors, elements* sdl, staticThings* things)
{
	if (game->distancex > things->trophyx - HALF_BITMAP && game->distancex < things->trophyx + HALF_BITMAP &&
		game->distancey > things->trophyy - HALF_BITMAP - 10 && game->distancey < things->trophyy + HALF_BITMAP + 10 &&
		game->showtrophy == 1)
	{
		game->points += 800;
		game->showpoints = 800;
		game->timetoshowpoints = 1.0;
		game->showtrophy = 0; //zeby nie pokazywac juz zebranego trofeum
	}
	for (int i = 0; i < BARRELS; i++)
	{
		if (game->distancey < game->barrely[i] && game->distancey>game->barrely[i] - 50 &&
			game->distancex > game->barrelx[i] - HALF_BITMAP && game->distancex < game->barrelx[i] + HALF_BITMAP
			&& game->speedjump < 0 && game->barreljumped[i] == 0)
		{
			game->points += 100;
			game->showpoints = 100;
			game->timetoshowpoints = 0.5;
			game->barreljumped[i] = 1; //oznacza ze beczka zostala juz przeskoczona
		}
	}
}
//restrart niektorych parametrow po smierci postaci
void AfterDead(params* game, staticThings* things)
{
	game->distancex = things->startx;
	game->distancey = things->starty;
	game->speedx = 0;
	game->speedy = 0;
	game->moverigth = 0;
	game->moveleft = 0;
	game->moveup = 0;
	game->movedown = 0;
	game->jump = 0;
	game->speedjump = 0;
	game->gravity = 0;
	game->timetoshowpoints = 0;
	game->lifes--;
	for (int i = 0; i < BARRELS; i++)
	{
		game->barrelx[i] = things->barrelx;
		game->barrely[i] = things->barrely;
		game->barrelspeedx[i] = 0;
		game->barrelspeedy[i] = 0;
		game->barreljumped[i] = 0;
	}
	game->timetofall = 0.9;
}
void isEnd(params* game, cols* colors, elements* sdl, staticThings* things)
{
	if (game->distancey < things->endy && game->distancex>things->endx && game->distancex < things->endx + END_BOX)
	{
		things->map++;
		game->points += 1000;
		if (things->map < 3) //gdy jest dostepna kolejna mapa to ja zaladuj
		{
			initializeStatic(things, things->map);
			continueGame(game, things);
			sdl->mario = sdl->marioprawo;
		}
		else // przejdz do zapisu wyniku
		{
			game->menu = SAVE;
			sdl->mario = sdl->marioprawo;
		}
	}
	else if (game->distancex + HALF_BITMAP < 0 || game->distancex - HALF_BITMAP > SCREEN_WIDTH ||
		game->distancey > 470 + EXTRA_BOUND)
	{ //warunek smierci przez bycie poza mapa
		AfterDead(game, things);
		game->menu = DEAD;
		sdl->mario = sdl->marioprawo;
	}
	for (int i = 0; i < BARRELS; i++)
	{ //lekko pozmniejszane by dokladniej wygladalo 
		if (game->distancex > game->barrelx[i] - 1.6 * HALF_BITMAP && game->distancex < game->barrelx[i] + 1.5 * HALF_BITMAP &&
			game->distancey > game->barrely[i] - 1.9 * HALF_BITMAP && game->distancey < game->barrely[i] + 1.9 * HALF_BITMAP)
		{
			AfterDead(game, things);
			game->menu = DEAD;
			sdl->mario = sdl->marioprawo;
			break;
		}
	}
	if (game->lifes == 0) //jak nie ma juz zyc to jednak przejdz do zapisu
	{
		game->menu = SAVE;
		sdl->mario = sdl->marioprawo;
	}

}
void ButtonsLoop(params* game, cols* colors, elements* sdl, staticThings* things)
{
	while (SDL_PollEvent(&sdl->event))
	{
		switch (sdl->event.type)
		{
		case SDL_KEYDOWN:
			switch (sdl->event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				game->quit = 1;
				break;
			case SDLK_RIGHT:
				game->moverigth = 1;
				break;
			case SDLK_LEFT:
				game->moveleft = 1;
				break;
			case SDLK_UP:
				game->moveup = 1;
				break;
			case SDLK_DOWN:
				game->movedown = 1;
				break;
			case SDLK_SPACE:
				game->jump = 1;
				break;
			case SDLK_n:
				startGame(game, things);
				break;
			case SDLK_1:
				initializeStatic(things, 0);
				startGame(game, things);
				sdl->mario = sdl->marioprawo;
				break;
			case SDLK_2:
				initializeStatic(things, 1);
				startGame(game, things);
				sdl->mario = sdl->marioprawo;
				break;
			case SDLK_3:
				initializeStatic(things, 2);
				startGame(game, things);
				sdl->mario = sdl->marioprawo;
				break;
			}
			break;
		case SDL_KEYUP:
			switch (sdl->event.key.keysym.sym) {
			case SDLK_RIGHT:
				game->moverigth = 0;
				break;
			case SDLK_LEFT:
				game->moveleft = 0;
				break;
			case SDLK_UP:
				game->moveup = 0;
				break;
			case SDLK_DOWN:
				game->movedown = 0;
				break;
			case SDLK_SPACE:
				game->jump = 0;
				break;
			}
			break;
		case SDL_QUIT:
			game->quit = 1;
			break;
		};
	};
	game->frames++;
}
void setBarrelSpeed(params* game, elements* sdl, cols* colors, staticThings* things)
{
	for (int i = 0; i < BARRELS; i++)
	{
		if (game->barrelspeedx[i] == 0) //ustaw predkosc tylko dla jednej dostepnej beczki
		{
			game->barrelspeedx[i] = BARREL_SPEED;
			break;
		}
	}
}
void countWithDelta(params* game, cols* colors, elements* sdl, staticThings* things)
{
	int t2 = SDL_GetTicks();
	// w tym momencie t2-t1 to czas w milisekundach,
	// jaki uplynal od ostatniego narysowania ekranu
	// delta to ten sam czas w sekundach
	double delta = (t2 - game->t1) * 0.001;
	game->t1 = t2;
	game->worldTime += delta;
	game->distancey += (game->speedjump + game->speedy + game->gravity) * delta;//przemieszczanie postaci na y
	game->distancex += game->speedx * delta;//przemieszczanie na x
	for (int i = 0; i < BARRELS; i++)
	{
		game->barrelx[i] += (game->barrelspeedx[i]) * delta;
		game->barrely[i] += (game->barrelspeedy[i]) * delta;
	}
	if (game->speedjump < 0)
		game->speedjump += delta * JUMP_DROP; //zmniejszanie w czasie mocy skoku
	else
		game->speedjump = 0;

	if (game->timetoshowpoints > 0) //zmniejszanie czasu wyswietlania punktow
	{
		game->timetoshowpoints = game->timetoshowpoints - delta;
	}
	else
		game->timetoshowpoints = 0;

	game->timetofall -= delta;
	if (game->timetofall < 0)
	{
		setBarrelSpeed(game, sdl, colors, things);
		game->timetofall = BARREL_SPAWN_RATE; //beczki generowac sie beda co 3 sekundy
	}
	game->timebarrelanim -= delta;
	if (game->timebarrelanim < 0)
		game->timebarrelanim += BARREL_ANIM_TIME;

	game->fpsTimer += delta;
	if (game->fpsTimer > 0.5) { //liczenie fps
		game->fps = game->frames * 2;
		game->frames = 0;
		game->fpsTimer -= 0.5;
	};
}
//rysuje statyczne rzeczy platformy, drabiny, gorna ramke
void drawOnScreen(params* game, cols* colors, elements* sdl, staticThings* things)
{
	SDL_FillRect(sdl->screen, NULL, colors->czarny);
	for (int i = 0; i < PLATFORMS; i++)
	{
		DrawRectangle(sdl->screen, things->startplatformx[i], things->startplatformy[i], things->lengthplatform[i],
			PLATFORM_HEIGHT, colors->podloga, colors->podloga);
	}
	for (int i = 0; i < LADDERS; i++)
	{
		DrawLadder(sdl->screen, things->startladderx[i], things->startladdery[i], LADDER_WIDTH,
			things->lengthladder[i], colors->drabina, colors->drabina);
	}
	char text[128];
	DrawRectangle(sdl->screen, 4, 4, SCREEN_WIDTH - 8, 36, colors->podloga, colors->niebieski);
	sprintf(text, "Bartosz Lyskanowski s198051, czas trwania = %.1lf s  %.0lf klatek / s", game->worldTime, game->fps);
	DrawString(sdl->screen, sdl->screen->w / 2 - strlen(text) * 8 / 2, 10, text, sdl->charset);
	sprintf(text, "Esc-wyjscie, n-nowa gra, Liczba punktow: %i, Zrealizowano:A,B,C,D,E,F,G", game->points);
	DrawString(sdl->screen, sdl->screen->w / 2 - strlen(text) * 8 / 2, 26, text, sdl->charset);
	if (game->showtrophy == 1) //jezeli nie zostalo zebrane trofeum to je wyswietl
		DrawSurface(sdl->screen, sdl->trophy, things->trophyx, things->trophyy);
	for (int i = 0; i < game->lifes; i++)
	{
		DrawSurface(sdl->screen, sdl->heart, 613 - 34 * i, 62); //pozycjonowanie serduszek
	}
	if (game->timetoshowpoints > 0)
	{
		sprintf(text, "%i", game->showpoints);//przyczepienie liczby punktow do glowy postaci
		DrawString(sdl->screen, game->distancex - 12, game->distancey - 30, text, sdl->charset);
	}
	DrawSurface(sdl->screen, sdl->kong, things->kongx, things->kongy);
	for (int i = 0; i < BARRELS; i++)
	{
		if (game->barrelspeedx[i] == 0) //nie animuj stojacych beczek
			DrawSurface(sdl->screen, sdl->barrel, game->barrelx[i], game->barrely[i]);
		else
			DrawSurface(sdl->screen, sdl->showbarrel, game->barrelx[i], game->barrely[i]);
	}
	DrawSurface(sdl->screen, sdl->mario, game->distancex, game->distancey);
}
struct players
{
	char name[30];
	int points;
};
void HallOfFame(players* person)
{
	FILE* plik;
	plik = fopen("wyniki.txt", "r");
	if (plik == NULL)
	{
		printf("Nie otwiera sie plik");
		exit(0);
	}
	int ile = 0;
	while (fscanf(plik, "%s %d", person[ile].name, &(person[ile].points)) == 2)
	{
		ile++;
	}
	while (ile < HALL_OF_FAME) //jezeli jest mniej niz odpowiednio osob to dorob kreski
	{
		strcpy(person[ile].name, "-------");
		person[ile].points = 0;
		ile++;
	}
	for (int i = 0; i < ile; i++) //bubble sort
	{
		for (int j = i + 1; j < ile; j++)
		{
			if (person[i].points < person[j].points)
			{
				int temp;
				char pom[30];
				temp = person[i].points;
				person[i].points = person[j].points;
				person[j].points = temp;
				strcpy(pom, person[i].name);
				strcpy(person[i].name, person[j].name);
				strcpy(person[j].name, pom);
			}
		}
	}
	fclose(plik);
}
void drawMenu(params* game, cols* colors, elements* sdl, staticThings* things, players* person)
{
	SDL_FillRect(sdl->screen, NULL, colors->czarny);
	DrawRectangle(sdl->screen, START_MENU, START_MENU, SCREEN_WIDTH - 2 * START_MENU, SCREEN_HEIGHT - 2 * START_MENU,
		colors->czerwony, colors->niebieski);
	char text[128];
	sprintf(text, "WELCOME TO DONKEY KONG");
	DrawString(sdl->screen, 4 * START_MENU + 40, START_MENU + 18, text, sdl->charset);
	sprintf(text, "WYBIERZ MAPE:");
	DrawString(sdl->screen, 4 * START_MENU + 65, START_MENU + 38, text, sdl->charset);
	sprintf(text, "1-MAPA NR 1");
	DrawString(sdl->screen, 4 * START_MENU + 67, START_MENU + 58, text, sdl->charset);
	sprintf(text, "2-MAPA NR 2");
	DrawString(sdl->screen, 4 * START_MENU + 67, START_MENU + 78, text, sdl->charset);
	sprintf(text, "3-MAPA NR 3");
	DrawString(sdl->screen, 4 * START_MENU + 67, START_MENU + 98, text, sdl->charset);
	DrawLine(sdl->screen, START_MENU, START_MENU + 118, SCREEN_WIDTH - 2 * START_MENU, 1, 0, colors->podloga);
	sprintf(text, "HALL OF FAME: ");
	DrawString(sdl->screen, 4 * START_MENU + 67, START_MENU + 138, text, sdl->charset);
	for (int i = 0; i < HALL_OF_FAME; i++)
	{
		sprintf(text, "%d: %s %d", i + 1, person[i].name, person[i].points);
		DrawString(sdl->screen, 4 * START_MENU + 67, START_MENU + 158 + 20 * i, text, sdl->charset);
	}
	DrawLine(sdl->screen, START_MENU, START_MENU + 278, SCREEN_WIDTH - 2 * START_MENU, 1, 0, colors->podloga);
	sprintf(text, "ESCAPE - WYJSCIE");
	DrawString(sdl->screen, 4 * START_MENU + 50, START_MENU + 298, text, sdl->charset);
	sprintf(text, "TWORCA");
	DrawString(sdl->screen, 4 * START_MENU + 90, START_MENU + 318, text, sdl->charset);
	sprintf(text, "Bartosz Lyskanowski s198051");
	DrawString(sdl->screen, 4 * START_MENU + 25, START_MENU + 338, text, sdl->charset);
}
void ButtonsMenu(params* game, cols* colors, elements* sdl, staticThings* things)
{
	while (SDL_PollEvent(&sdl->event))
	{
		switch (sdl->event.type)
		{
		case SDL_KEYDOWN:
			switch (sdl->event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				game->quit = 1;
				break;
			case SDLK_1:
				game->menu = GRA;
				things->map = 0;
				break;
			case SDLK_2:
				game->menu = GRA;
				things->map = 1;
				break;
			case SDLK_3:
				game->menu = GRA;
				things->map = 2;
				break;
			}
			break;
		case SDL_QUIT:
			game->quit = 1;
			break;
		};
	};
}
void drawEnding(params* game, cols* colors, elements* sdl, staticThings* things)
{
	SDL_FillRect(sdl->screen, NULL, colors->czarny);
	DrawRectangle(sdl->screen, 2 * START_MENU, 2 * START_MENU, SCREEN_WIDTH - 4 * START_MENU, SCREEN_HEIGHT - 4 * START_MENU,
		colors->czerwony, colors->niebieski);
	char text[128];
	sprintf(text, "KONIEC GRY!");
	DrawString(sdl->screen, 4 * START_MENU + 60, 2 * START_MENU + 28, text, sdl->charset);
	sprintf(text, "UDALO CI SIE ZDOBYC %i PUNKTOW", game->points);
	DrawString(sdl->screen, 4 * START_MENU, 2 * START_MENU + 58, text, sdl->charset);
	sprintf(text, "PODAJ SWOJ NICKNAME:");
	DrawString(sdl->screen, 4 * START_MENU + 35, 2 * START_MENU + 88, text, sdl->charset);
	sprintf(text, "%s", game->nick);
	DrawString(sdl->screen, 4 * START_MENU + 45, 2 * START_MENU + 118, text, sdl->charset);
	sprintf(text, "ENTER ZEBY ZATWIERDZIC");
	DrawString(sdl->screen, 4 * START_MENU + 25, 2 * START_MENU + 148, text, sdl->charset);
}
void safeToFile(params* game)
{
	FILE* plik;
	plik = fopen("wyniki.txt", "a");
	if (plik == NULL)
	{
		printf("Nie otwiera sie plik");
		exit(0);
	}
	fprintf(plik, "%s %d\n", game->nick, game->points);
	fclose(plik);
}
void ButtonsEnding(params* game, cols* colors, elements* sdl, staticThings* things, players* person)
{
	int dl;
	while (SDL_PollEvent(&sdl->event))
	{
		switch (sdl->event.type)
		{
		case SDL_KEYDOWN:
			switch (sdl->event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				game->quit = 1;
				break;
			case SDLK_BACKSPACE:
				dl = strlen(game->nick);
				if (dl > 0) {
					game->nick[dl - 1] = '\0';
				}
				break;
			case SDLK_RETURN:
				dl = strlen(game->nick);
				if (dl == 0)
					strcpy(game->nick, "NONAME"); //jezeli gracz nie poda nicku wpisz noname by nie rozwalic pliku
				safeToFile(game);
				HallOfFame(person); //zaaktualizuj hall of fame 
				game->menu = MENU;
				strcpy(game->nick, "");
				break;
			default:
				if ((sdl->event.key.keysym.sym >= SDLK_a && sdl->event.key.keysym.sym <= SDLK_z) ||
					(sdl->event.key.keysym.sym >= SDLK_0 && sdl->event.key.keysym.sym <= SDLK_9))
				{
					char letter;
					if (sdl->event.key.keysym.sym >= SDLK_a && sdl->event.key.keysym.sym <= SDLK_z)
						letter = (char)sdl->event.key.keysym.sym + 'A' - 'a'; //zamiana na wielkie litery
					else letter = (char)sdl->event.key.keysym.sym;
					dl = strlen(game->nick);
					if (dl < 20)
					{
						game->nick[dl] = letter;
						game->nick[dl + 1] = '\0';
					}
				}
			}
			break;
		case SDL_QUIT:
			game->quit = 1;
			break;
		};
	};
}
void deadMenu(params* game, cols* colors, elements* sdl, staticThings* things)
{
	SDL_FillRect(sdl->screen, NULL, colors->czarny);
	DrawRectangle(sdl->screen, 2 * START_MENU, 2 * START_MENU, SCREEN_WIDTH - 4 * START_MENU, SCREEN_HEIGHT - 4 * START_MENU,
		colors->czerwony, colors->niebieski);
	char text[128];
	sprintf(text, "UMARLES!");
	DrawString(sdl->screen, 4 * START_MENU + 80, 2 * START_MENU + 28, text, sdl->charset);
	sprintf(text, "NA RAZIE MASZ %i PUNKTOW", game->points);
	DrawString(sdl->screen, 4 * START_MENU, 2 * START_MENU + 58, text, sdl->charset);
	sprintf(text, "Czy chcesz kontynuowac? Enter");
	DrawString(sdl->screen, 4 * START_MENU, 2 * START_MENU + 88, text, sdl->charset);
	sprintf(text, "Czy chcesz wyjsc? Escape");
	DrawString(sdl->screen, 4 * START_MENU, 2 * START_MENU + 118, text, sdl->charset);
}
void ButtonsDead(params* game, cols* colors, elements* sdl, staticThings* things)
{
	while (SDL_PollEvent(&sdl->event))
	{
		switch (sdl->event.type)
		{
		case SDL_KEYDOWN:
			switch (sdl->event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				game->quit = 1;
				break;
			case SDLK_RETURN:
				game->t1 = SDL_GetTicks(); //zeby nie liczyc czasu gdy gra byla zapauzowana
				game->menu = GRA;
				break;
			}
			break;
		case SDL_QUIT:
			game->quit = 1;
			break;
		};
	};
}
void mainLoop(params* game, cols* colors, elements* sdl, staticThings* things, players* person)
{
	while (!game->quit)
	{
		if (game->menu == MENU)
		{
			drawMenu(game, colors, sdl, things, person);
			ButtonsMenu(game, colors, sdl, things);
			if (game->menu == GRA)
			{
				initializeStatic(things, things->map);
				startGame(game, things);
			}
		}
		else if (game->menu == SAVE)
		{
			drawEnding(game, colors, sdl, things);
			ButtonsEnding(game, colors, sdl, things, person);
		}
		else if (game->menu == GRA)
		{
			countWithDelta(game, colors, sdl, things);
			drawOnScreen(game, colors, sdl, things);//wyrzucanie na ekran
			ButtonsLoop(game, colors, sdl, things);//obsluga zdarzen o ile zaszly
			onPlatform(game, colors, sdl, things);
			barrelsChangeDirection(game, colors, sdl, things); //obsluga beczek zmiana kierunku, powrot na gore mapy
			onLadder(game, colors, sdl, things);
			setSpeed(game, colors, sdl, things);
			setBitmap(game, colors, sdl, things);
			collectPoints(game, colors, sdl, things);
			isEnd(game, colors, sdl, things);
		}
		else
		{
			deadMenu(game, colors, sdl, things);
			ButtonsDead(game, colors, sdl, things);
		}
		SDL_UpdateTexture(sdl->scrtex, NULL, sdl->screen->pixels, sdl->screen->pitch);
		//SDL_RenderClear(sdl->renderer);
		SDL_RenderCopy(sdl->renderer, sdl->scrtex, NULL, NULL);
		SDL_RenderPresent(sdl->renderer);
	};
	FreeSDL(sdl); // zwolnienie powierzchni
};
// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	elements sdl;
	initializeSDL(&sdl);
	cols colors;
	initializeColors(&sdl, &colors);
	staticThings things;
	zeroStatic(&things);
	params game;
	zeroGame(&game);
	game.menu = MENU;
	players person[100];
	HallOfFame(person);
	mainLoop(&game, &colors, &sdl, &things, person);
	return 0;
};