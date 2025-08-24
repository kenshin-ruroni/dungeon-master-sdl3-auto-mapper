/*
 * AutoMap.h
 *
 *  Created on: 27 sept. 2014
 *      Author: descourt
 */

#pragma once
#include <deque>
#include <cstdint>
#include <cmath>
#include <stddef.h>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_ttf.h>


#include "CSB.h"

#define PADDING_X 10
#define PADDING_Y 10
#define SIZE_OF_BOX 10
#define SIZE_OF_TILE 5 // une cellule est découpée en petits carres de cote tile_size_box
#define LIGHT_RADIUS 5

#define MAX_MONSTER_Y 5
#define MAX_MONSTER_X 5

enum class RoomContent {empty, open,visited, visible,stone, monster};

inline extern const char * MonsterType2Name(MONSTERTYPE);

struct Monster
{
	DB1 *_db1; // sa ref dans la DataBase globale
	int16_t x,y;
	bool isDead;
};

struct Room
{
	int16_t x, y;
	RoomContent content; // 0 if not in the current level
	// walls : 2 front
	//         1 left
	//         0 behind
	//         3 right
	bool walls[4]; // walls[0] = true -> there is a wall otherwise no wall
	bool containsAmonster;
	bool MonsterIsVisible;
};

struct Level
{

	int16_t sizeOfBox;
	Room *map; // ses cellules
	Monster* monsters; // les monstres
	uint16_t _numberOFMonsters;
	int16_t sizeX;
	int16_t sizeY;
	int16_t numberOfTile;
};

// rotations pour tenir du parametre facing
/*
 *             facing = 2 (rotation 0 rad)
 *                / \
 *                 |
 *  facing = 1 <--   --> facing = 3 (rotation map de -pi/2)
 *  (rotation      |
 *    map         \ /
 *    de +pi/2) facing = 0  (rotation map de - pi)
 */

static int32_t cosinus[4] = {-1,0,1,0};
static int32_t   sinus[4] = {0,1,0,-1};

	
struct auto_mapper {

// le level en cours
	int32_t _MaxlevelNumber;
	Room *rooms;

	Level *_levels; // le stock de levels deja visites

	uint32_t width = 600, height = 600;
	uint16_t _numberOfBoxX;
	uint16_t _numberOfBoxY;
	int16_t sizeOfBox;
	int16_t numberOfTile;

	int16_t _Radius, _Radius2;

	TTF_Font *font_18;
	TTF_Font *font_12;

	std::deque<char *> messages;

int number_of_boxes ;


	auto_mapper()
	{

	_MaxlevelNumber = -1; // pas de level loaded

	_levels = NULL;

	sizeOfBox = SIZE_OF_BOX;

	numberOfTile = SIZE_OF_BOX/SIZE_OF_TILE;

	number_of_tiles =  numberOfTile * numberOfTile;

	_numberOfBoxX = 20;
	_numberOfBoxY = 20;

	number_of_boxes = _numberOfBoxX * _numberOfBoxY;

	_partyX = _numberOfBoxX/2;
	_partyY = _numberOfBoxY/2;

	_Radius2 = LIGHT_RADIUS * LIGHT_RADIUS;

	
	facingTo = 2; // on regarde vers le nord par defaut
	previousFacing = 2;

	rooms = NULL;

	initializes = true;
	firstInitialisation = false;
	uint32_t ix = 0;

	uint32_t iy = 0;


	//for (uint16_t i = 0; i < number_of_boxes ; i ++)
	//{
	//	iy = i/_numberOfBoxX;
	//	ix = i - iy * _numberOfBoxX;
	//	draw_room(i,ix,iy);
	//}
	initializes = false;

	}

	SDL_Window *window;
	SDL_Texture *texture;
	TTF_TextEngine *ttf_text_engine;
	inline void initialize()
	{
		msg_str ="";
		if (!TTF_Init())
			{
				SDL_Log("error initializing SDL TTF %s \n",SDL_GetError());
				// Handle error...
			}
			if(	(font_18 = TTF_OpenFont("BruceForeverRegular.ttf", 18)) == NULL){
				SDL_Log(" auto mmapper font not found.");exit(-1);
			}
			if(	(font_12 = TTF_OpenFont("BruceForeverRegular.ttf", 12)) == NULL){
			SDL_Log(" auto mmapper font not found.");exit(-1);
			}
		SDL_CreateWindowAndRenderer("auto mapper",width,height,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY|SDL_WINDOW_MOUSE_CAPTURE,&window,&renderer);
		grid = SDL_CreateSurface(width,height,SDL_PIXELFORMAT_BGRA32);
		texture = SDL_CreateTextureFromSurface(renderer,grid);
		ttf_text_engine = 	TTF_CreateRendererTextEngine(renderer);
	render();

	}


	~auto_mapper()
	{
	// release grid
		SDL_DestroyRenderer(renderer);
		SDL_DestroySurface(grid);
		SDL_DestroyWindow(window);
		SDL_DestroyTexture(texture);
	}




	void inline resize(uint32_t newnumberOfint32_tX, uint32_t newnumberOfBoxY, uint32_t newSizeOfBox);
	void inline scroll( uint32_t mouseX, uint32_t mouseY);


	int16_t ix,iy;

	inline void draw_all_rooms(){
		for (uint16_t i = 0; i < number_of_boxes ; i ++)
		{
			iy = i/_numberOfBoxX;
			ix = i - iy * _numberOfBoxX;
			draw_room(i,ix,iy);
		}
	}

	int number_of_tiles =  numberOfTile * numberOfTile;
	void inline zoom(float zoomFactor)
	{
		SDL_Window * w = SDL_GetMouseFocus();
		if ( w != window){
			return;
		}
		memset(grid->pixels,0,grid->pitch*grid->h);
		if ( zoomFactor > 0.)
		{
			sizeOfBox +=SIZE_OF_TILE;
		}
		else
		{
			sizeOfBox -=SIZE_OF_TILE;
		}

		if ( sizeOfBox < SIZE_OF_TILE)
		{
			sizeOfBox = SIZE_OF_TILE;
		}

		//SDL_FillSurfaceRect(grid, NULL, SDL_MapSurfaceRGBA(grid,0,0,0,255));
		//render_texture();

		numberOfTile = sizeOfBox/SIZE_OF_TILE;

		number_of_tiles =  numberOfTile * numberOfTile;

		draw_all_rooms();
		pan_to_party_position();
	}

	void inline pan_to_party_position()// center on box _partyX,_partyY
	{

		if ( rooms == NULL )
			return;

		compute_center_of_screen();
		draw_all_rooms();
		render();
		previousFacing = facingTo;
	} 

	void inline initialize_monsters(DB1 *mp, int32_t x , int32_t y)
	{
		Monster *m = NULL;
		Level * level =  (Level *) (_levels +_currentLevel);

		if ( level->monsters == NULL )
		{
			level->_numberOFMonsters = 1;
			level->monsters = (Monster *) malloc( sizeof(Monster) );
			(*(level->monsters))._db1 = mp;
			(*(level->monsters)).x = x;
			(*(level->monsters)).y = y;
			(*(level->monsters)).isDead = false;
		}
		else
		{
			// on connait deja cette ref ?
			bool found = false;
			for (uint16_t i = 0; i < level->_numberOFMonsters; i++)
			{
				m = (Monster *) (level->monsters + i);
				if ( m->_db1 == mp )
				{
					m->x = x;
					m->y = y;
					found = true;
					break;
				}
			}
			if ( !found ) // il n'est pas encore repertorie
			{
				// on augmente la taille de monsters
				level->_numberOFMonsters ++;
				level->monsters = (Monster *) realloc(level->monsters,level->_numberOFMonsters * sizeof(Monster) );
				// on recupere le slot a la fin
				m = (Monster *) (level->monsters + level->_numberOFMonsters - 1);
				// on met les donnees du slot 0 dedans
				m->x = ((Monster *) (level->monsters))->x;
				m->y = ((Monster *) (level->monsters))->y;
				m->_db1 = ((Monster *) (level->monsters))->_db1;
				// on place dans le slot 0 les donnees du monstre en cours
				m = (Monster *) (level->monsters);
				m->x = x;
				m->y = y;
				m->_db1 = mp;
				m->isDead = false;
			}
		}

	}
	void inline update_monsters(int16_t x,int16_t y)
{
	if ( _levels == NULL )
		return;
	//on rend invisible tous les monstres que le groupe ne peut pas voir
	m = NULL;
	DB1 *monsterFromDB = NULL;


	Level *level = (Level *) (_levels + _currentLevel);
	bool forceInvisible = false;
	bool AyAyAy =false;
	int16_t monsterX = 0, monsterY = 0;
	int16_t dY ,dX;

	for (uint16_t i = 0; i < level->_numberOFMonsters; i++)
	{
		forceInvisible = true;
		m = (Monster *) (level->monsters + i);

		if ( m->isDead )
			continue;

		monsterFromDB = m->_db1;
		/*
		//on recupere ses coordonnees pour test de coherence
		monsterX = monsterFromDB->destX();
		monsterY = monsterFromDB->destY();

		if ( m->x != monsterX) // pas bon ça on doit rectifier
		{
			//on modifie le contenu de la salle correspondante
			rooms[m->x + _numberOfBoxX * m->y].containsAmonster = false;
			rooms[m->x + _numberOfBoxX * m->y].MonsterIsVisible = false;
			//
			rooms[monsterX + _numberOfBoxX * monsterY].containsAmonster = true;
			rooms[monsterX + _numberOfBoxX * monsterY].MonsterIsVisible = false;
			AyAyAy = true;
		}
		if ( m->y != monsterY) // pas bon ça on doit rectifier
		{
			//on modifie le contenu de la salle correspondante
			rooms[m->x + _numberOfBoxX * m->y].containsAmonster = false;
			rooms[m->x + _numberOfBoxX * m->y].MonsterIsVisible = false;
			//
			rooms[monsterX + _numberOfBoxX * monsterY].containsAmonster = true;
			rooms[monsterX + _numberOfBoxX * monsterY].MonsterIsVisible = false;
			AyAyAy = true;
		}
		if ( AyAyAy )
		{
			draw_room(m->x + _numberOfBoxX * m->y, m->x,m->y);
			draw_room(monsterX + _numberOfBoxX * monsterY,monsterX,monsterY);
			m->x = monsterX;
			m->y = monsterY;
			AyAyAy =false;
		}
		*/
		// on compute les distances algebriques relatives
		dX = m->x - x;
		dY = m->y - y;

	  // le monstre doit etre en avant du groupe pour etre vu et dans un carre 3*4
	  switch( facingTo )
	  {
	  case 0:
		  if ( dY < 0 && dY >= -3 && abs(dX) <= 1)
		  {
			  forceInvisible = false;
		  }
		  break;
	  case 1:
		  if ( dX > 0 && dX <= 3 && abs(dY) <= 1)
		  {
			  forceInvisible = false;
		  }
		  break;
	  case 2:
		  if ( dY > 0 && dY <=3 && abs(dX) <= 1 )
		  {
			  forceInvisible = false;
		  }
		  break;
	  case 3:
		  if ( dX < 0 && dX >= -3 && abs(dY) <= 1)
		  {
			  forceInvisible = false;
		  }
	  }
	  //on regarde maintenant si il n'y a pas d'obstacles entre le groupe et le monstre
	  //
	if ( TestDirectPathOpen(m->x,m->y,x,y,StoneOrClosedFalseWall) == 0 || TestDirectPathOpen(m->x,m->y,x,y,BlockedTypeAHacked) == 0 )
	{
		forceInvisible = true;
	}

	  rooms[m->x + _numberOfBoxX * m->y].containsAmonster = true;
	  rooms[m->x + _numberOfBoxX * m->y].MonsterIsVisible = !forceInvisible;
	  draw_room(m->x + _numberOfBoxX * m->y,m->x,m->y);
	}
	pan_to_party_position();
}
	#define MAX_MESSAGES 4

	const char text[256]={0};
	std::string msg_str;
	inline void register_monster(char *monster_name,int16_t x,int16_t y)
	{
		if ( messages.size() >= MAX_MESSAGES){
			messages.pop_back();
		}
		snprintf((char *)text,256,"a %s has been found nearby at position (%i,%i) \n",monster_name,x,y);
		messages.emplace_front(text);
		msg_str ="";
		for (auto i = messages.begin();i!=messages.end();i++){
			msg_str+= *i;
		}
	}

	Monster *m = NULL;
	Level * level = NULL;
	bool monster_is_visible_by_party ;
	bool monster_is_in_vicinity ;
	void inline update_positions_of_nearby_monsters(const char *monster_name, DB1 *monster, int16_t x,int16_t y)
	{
		monster_is_visible_by_party = false;
		m = NULL;
		Level * level =  (Level *) (_levels +_currentLevel);
		int16_t oldX = x,oldY = y;

		bool wasVisible = false;

		bool found = false;

		if ( level->monsters == NULL )
		{
			level->_numberOFMonsters = 1;
			level->monsters = (Monster *) malloc( sizeof(Monster) );
			(*(level->monsters))._db1 = monster;
			(*(level->monsters)).x = x;
			(*(level->monsters)).y = y;
			// on indique que la salle x,y contient le monstre
			rooms[x + _numberOfBoxX * y].containsAmonster = true;
		}
		else
		{
			for (uint16_t i = 0; i < level->_numberOFMonsters; i++)
			{
				m = (Monster *) (level->monsters + i);
				if ( m->_db1 == monster )
				{
					found = true;
					break;
				}
			}
		}

		if ( !found )
		{
			// on augmente la taille du buffer
			level->_numberOFMonsters ++;
			level->monsters = (Monster *) realloc(level->monsters,level->_numberOFMonsters * sizeof(Monster) );
			// on recupere le slot a la fin
			m = (Monster *) (level->monsters + level->_numberOFMonsters - 1);
			// on met les donnees du slot 0 dedans
			m->x = ((Monster *) (level->monsters))->x;
			m->y = ((Monster *) (level->monsters))->y;
			m->_db1 = ((Monster *) (level->monsters))->_db1;
			// on place dans le slot 0 les donnees du monstre en cours
			m = (Monster *) (level->monsters);
			m->x = x;
			m->y = y;
			m->_db1 = monster;
			rooms[x + _numberOfBoxX * y].containsAmonster = true;
			oldX = oldY = 0;
		}
		if ( found)
		{
			
			// WARNING le monstre peut avoir bouge entre temps
			// il faut en tenir compte

					// le monstre a bouge ???
					if (  m->x != x || m->y != y )
					{

						// oui on l'enleve de la salle ou il etait
						rooms[ m->x + _numberOfBoxX * m->y].containsAmonster = false;
						wasVisible = rooms[ m->x + _numberOfBoxX * m->y].MonsterIsVisible ;
						rooms[ m->x + _numberOfBoxX * m->y].MonsterIsVisible = false;
						//RoomContent tmp = rooms[ m->x + _numberOfBoxX * m->y].content;
						//rooms[ m->x + _numberOfBoxX * m->y].content = monster;
						//draw_room(m->x + _numberOfBoxX * m->y, m->x,m->y);
						// on replace son ancien contenu pour la maj
						//rooms[ m->x + _numberOfBoxX * m->y].content = tmp;
						draw_room(m->x + _numberOfBoxX * m->y, m->x,m->y);
						// on maj ses coordonnees
						m->x = x;
						m->y = y;
					}
					// on indique que la salle x,y contient le monstre
					rooms[x + _numberOfBoxX * y].containsAmonster = true;
					monster_is_in_vicinity = std::abs(m->x - _partyX) <= MAX_MONSTER_X && std::abs(m->y - _partyY) <= MAX_MONSTER_Y; 
					monster_is_visible_by_party = TestDirectPathOpen(m->x,m->y,_partyX,_partyY,StoneOrClosedFalseWall) != 0 || TestDirectPathOpen(m->x,m->y,_partyX,_partyY,BlockedTypeAHacked) != 0;		
					rooms[x + _numberOfBoxX * y].MonsterIsVisible = monster_is_visible_by_party && monster_is_in_vicinity;
					draw_room(x + _numberOfBoxX * y,x,y);
	
	//		// le monstre ne peut etre visible
	//		if ( !rooms[x + _numberOfBoxX * y].MonsterIsVisible )
	//		{
	//			//c'est bon il ne l'est pas on retourne
	//			return;
	//		}
	//		rooms[x + _numberOfBoxX * y].MonsterIsVisible = false;
	//		draw_room(x + _numberOfBoxX * y,x,y);
	//		DrawOnScreen();

			register_monster((char *)monster_name,x,y);
			pan_to_party_position();
			return;
		}



		rooms[x + _numberOfBoxX * y].MonsterIsVisible = monster_is_visible_by_party && monster_is_in_vicinity;
	
		// le monstre a bouge ???
		if (  m->x != x || m->y != y )
		{
			oldX = m->x;
			oldY = m->y;
			// oui on l'enleve de la salle ou il etait
			rooms[ m->x + _numberOfBoxX * m->y].containsAmonster = false;
			rooms[ m->x + _numberOfBoxX * m->y].MonsterIsVisible = false;
			//RoomContent tmp = rooms[ m->x + _numberOfBoxX * m->y].content;
			//rooms[ m->x + _numberOfBoxX * m->y].content = monster;
			//draw_room(m->x + _numberOfBoxX * m->y, m->x,m->y);
			// on replace son ancien contenu pour la maj
			//rooms[ m->x + _numberOfBoxX * m->y].content = tmp;
			draw_room(m->x + _numberOfBoxX * m->y, m->x,m->y);
			// on maj ses coordonnees
			m->x = x;
			m->y = y;
			//rooms[x + _numberOfBoxX * y].containsAmonster = true;
			//rooms[x + _numberOfBoxX * y].MonsterIsVisible = !forceInvisible;
		}

//				if ( rooms[ m->x + _numberOfBoxX * m->y].MonsterIsVisible == false)
//				{
//					rooms[ m->x + _numberOfBoxX * m->y].MonsterIsVisible = true;
//					draw_room(m->x + _numberOfBoxX * m->y, m->x,m->y);
//					DrawOnScreen();
//				}

		// on indique que la salle x,y contient le monstre
		rooms[m->x + _numberOfBoxX * m->y].containsAmonster = true;

		draw_room(x + _numberOfBoxX * y,x,y);
		pan_to_party_position();

}




	void inline look_around_for_nearby_monsters(int16_t x, int16_t y)
	{
		RN obj ;



		int16_t dY = y - _partyY;
		int16_t dX = x - _partyX;

		bool forceInvisible = true;

		//
		DB4 *monsterAsDB4;
		DB1 *monsterAsDB1;
		RN monster;
		for ( int16_t dx =  -MAX_MONSTER_X;dx <= MAX_MONSTER_X;dx++)
		{
			for ( int16_t dy =  -MAX_MONSTER_Y;dy <= MAX_MONSTER_Y;dy++)
			{
				monster = FindFirstMonster(x+dx,y+dy);
				monsterAsDB1 = (DB1 *)GetRecordAddressDB1(monster);
				if ( monsterAsDB1 == NULL)
				{
					continue;
				}
				monsterAsDB4 = (DB4 *)GetRecordAddressDB4(monster);
				const char *name = MonsterType2Name(monsterAsDB4->monsterType());
				update_positions_of_nearby_monsters(name,monsterAsDB1,x+dx,y+dy);
			}

		}
		pan_to_party_position();
		render();
	}

	void inline remove_monster_from_level(int16_t level, DB1 *monsterAsDB1, int16_t mapX, int16_t mapY)
	{

	if ( level > _MaxlevelNumber )
		return;

	Level * l =  (Level *) (_levels +level);

	Monster * monsters = l->monsters;
	m = NULL;

	for (uint16_t i = 0; i < l->_numberOFMonsters; i++)
	{
		m = (Monster *) monsters + i;
		if ( m->_db1 == monsterAsDB1 )
		{
			rooms[mapX+ l->sizeX * mapY].containsAmonster = false;
			rooms[mapX+ l->sizeX * mapY].MonsterIsVisible = false;
			m->isDead = true;
			if ( level == _currentLevel)
			{
				draw_room(mapX+ l->sizeX * mapY,mapX,mapY);
				render();
			}
			break;
		}
	}


	}


	void inline set_facing(int16_t facing)
	{
		previousFacing = facingTo;
		facingTo = facing;
		// on efface la surface grid
		//
		SDL_SetRenderDrawColor(renderer,0,0,0,0);
		memset(grid->pixels,0,grid->pitch*grid->h);
		SDL_RenderClear(renderer);
		render_texture();
		pan_to_party_position();
	}

	int16_t _partyX, _partyY;
	int16_t facingTo, previousFacing;

	uint32_t *monsterPositions; // linearized monsters positions i = iX + numberOfBoxX * iY
	uint32_t numberOfMonsters;

	Room *boxes; // the boxes

	bool initializes, firstInitialisation;

	SDL_Renderer *renderer;
	SDL_Texture *grid_texture;
	SDL_Surface *grid;

	SDL_Window *WND; // the window on which the grid is displayed

	int16_t _xc,_yc; // centre du screen

	int32_t _currentLevel;

	SDL_FRect rect = {};
	void inline draw_a_box(int16_t x1, int16_t y1,int x2, int16_t y2,uint8_t r,uint8_t g, uint8_t b,uint8_t a)
	{
		// 2 triangles to draw a box
		float falpha = float(a)/255.;
		SDL_SetRenderDrawColor(renderer,(uint8_t)(float(r)*falpha),(uint8_t)(float(g)*falpha),(uint8_t)(float(b)*falpha),(uint8_t)(float(a)*falpha));
		int w = 	std::max((int)x1,(int)x2) - std::min((int)x1,(int)x2);
		int h = 	std::max((int)y1,(int)y2) - std::min((int)y1,(int)y2);
		vertices[0] = {.position={(float)x1,(float)y1},.color = {(float)r/255.f *falpha,(float)g/255.f*falpha,(float)b/255.f*falpha,(float)a/255.f*falpha}};
		vertices[1] = {.position={(float)(x1 + w),(float)y1},.color = {(float)r/255.f*falpha,(float)g/255.f*falpha,(float)b/255.f*falpha,(float)a/255.f*falpha}};
		vertices[2] = {.position={(float)(x1+w),(float)(y1+h)},.color = {(float)r/255.f*falpha,(float)g/255.f*falpha,(float)b/255.f*falpha,(float)a/255.f*falpha}};
		vertices[3] = {.position={(float)x1,(float)y1},.color = {(float)r/255.f *falpha,(float)g/255.f*falpha,(float)b/255.f*falpha,(float)a/255.f*falpha}};
		vertices[4] = {.position={(float)(x1+w),(float)(y1+h)},.color = {(float)r/255.f*falpha,(float)g/255.f*falpha,(float)b/255.f*falpha,(float)a/255.f*falpha}};
		vertices[5] = {.position={(float)x1,(float)(y1+h)},.color = {(float)r/255.f*falpha,(float)g/255.f*falpha,(float)b/255.f*falpha,(float)a/255.f*falpha}};
		SDL_RenderGeometry(renderer,NULL,vertices,6,NULL,0);

		//SDL_SetRenderDrawColor(renderer,(uint8_t)(float(a)*falpha),(uint8_t)(float(a)*falpha),(uint8_t)(float(a)*falpha),(uint8_t)(float(a)*falpha));
		//SDL_RenderRect(renderer,&rect);

	}

	 SDL_Vertex vertices[6];
	void inline draw_a_filled_triangle(int16_t x1, int16_t y1,int x2, int16_t y2,int x3, int16_t y3,uint8_t r,uint8_t g, uint8_t b,uint8_t a)
	{
		vertices[0] = {.position={(float)x1,(float)y1},.color = {(float)r/255.f,(float)g/255.f,(float)b/255.f,(float)a/255.f}};
		vertices[1] = {.position={(float)x2,(float)y2},.color = {(float)r/255.f,(float)g/255.f,(float)b/255.f,(float)a/255.f}};
		vertices[2] = {.position={(float)x3,(float)y3},.color = {(float)r/255.f,(float)g/255.f,(float)b/255.f,(float)a/255.f}};

		SDL_RenderGeometry(renderer,NULL,vertices,3,NULL,0);
	}

	void inline draw_room(uint16_t i,  uint32_t ix,uint32_t iy)
	{

		if ( rooms == NULL )
			return;

		static int16_t xc,yc;

		// on se place sur la ieme Room
		//
		Room *room = (Room *) (rooms + i );
		//printf( "size of Room = %i \n",sizeof(Room) );
		//printf("room + %u = 0X%X   room[%u] = 0X%X \n",  i , (uintptr_t) room, i, (uintptr_t) &(rooms[i]) );



		int16_t x,y;


			// on centre sur la position courante dans le dongeon
		static	int16_t x0 = width/2 ;
		static 	int16_t y0 = height/2 ;

			// coordonnees des points de la salle par rapport a la salle courante
			//
			int16_t X0 = - ix * sizeOfBox + _partyX * sizeOfBox; // coordonnee X lower left

			int16_t Y0 = -iy * sizeOfBox + _partyY* sizeOfBox;   // coordonnee Y lower left

			int16_t X1 = X0 + sizeOfBox; // coordonnee X lower right
			int16_t Y1 = Y0; 			  // coordonnee Y lower right

			int16_t X2 = X0 + sizeOfBox; // coordonnee X upper right
			int16_t Y2 = Y0 - sizeOfBox; // coordonnee Y lower right


			int16_t X3 = X0; 			  // coordonnee X upper left
			int16_t Y3 = Y0 - sizeOfBox; // coordonnee Y upper left

			//on applique la rotation
	//		if ( facingTo != previousFacing || initializes )
	//		{
				int16_t XP0 = cosinus[facingTo] * X0 - sinus[facingTo] * Y0;
				int16_t YP0 = sinus[facingTo] * X0 +  cosinus[facingTo] * Y0;

				room->x = x0 +  XP0 ;
				room->y = y0 +  YP0 ;

				int16_t XP1 = cosinus[facingTo] * X1 - sinus[facingTo] * Y1 ;
				int16_t YP1 = sinus[facingTo] * X1 + cosinus[facingTo] * Y1 ;

				int16_t XP2 = cosinus[facingTo] * X2 - sinus[facingTo] * Y2 ;
				int16_t YP2 = sinus[facingTo] * X2 + cosinus[facingTo] * Y2 ;


				int16_t XP3 = cosinus[facingTo] * X3 - sinus[facingTo] * Y3 ;
				int16_t YP3 = sinus[facingTo] * X3 + cosinus[facingTo] * Y3 ;

	//		}
	//		else
	//		{
	//			room->x = x0 +  deltaX ;
	//			room->y = y0 +  deltaY ;
	//		}



			if ( room->content == RoomContent::empty && !room->containsAmonster && !room->MonsterIsVisible )
			{
				draw_a_box(x0+XP3,y0+YP3,x0+XP1,y0+YP1,0,0,0,255);
				
			}


		xc = (XP1+XP3)/2 ;
		yc = (YP1+YP3)/2 ;

		uint8_t r = 50,g = 50,b = 50, a = 255;

		int16_t deltaX = (ix - _partyX) * sizeOfBox;
		int16_t deltaY = (iy - _partyY) * sizeOfBox;

		// on baisse le canal alpha selon la distance à la cellule courante
		// 		float alpha = 255./(1. + 0.025f* (std::powf( _partyX-x0+XP3 ,2) + std::powf( _partyY-y0+YP3 ,2)) );

			a = 255./( 1 + 0.0002 * std::sqrt((float) (deltaX*deltaX+deltaY*deltaY) ) );
			r = 190;
			b = 190;
			g = 190;
		bool test = room->content == RoomContent::visited ||
									 room->content == RoomContent::stone ||
									 room->content == RoomContent::visible ||
									 room->containsAmonster ;

		if ( test )
		{
			// on vide
			draw_a_box(x0+XP3,y0+YP3,x0+XP1,y0+YP1,0,0,0,a);
			uint8_t rr = 0,gg = 0,bb = 0;

			if ( room->content == RoomContent::visible )
			{
				rr = 243, gg = 132, bb = 0;
			}

			if ( room->content == RoomContent::visited )
			{
				rr = 0, gg = 132, bb = 0;
			}

			if ( room->content == RoomContent::stone )
			{
				rr = gg = bb = 190;
			}

			if ( room->containsAmonster && room->MonsterIsVisible )
			{
				rr = 255; gg = 0;bb = 0;
			}
			int16_t deltaX = abs(XP3 - XP1)/ numberOfTile;// (numberOfTile - 1);
			int16_t deltaY = abs(YP3 - YP1)/ numberOfTile ; // (numberOfTile - 1);
			uint16_t j,k;
			uint8_t  alpha;
			int16_t xp1,yp1,xp2,yp2;
			int16_t lpx = std::min(XP3,XP1);
			int16_t lpy = std::min(YP3,YP1);
			for (uint16_t l = 0; l < number_of_tiles;l++)
			{
				k = l/numberOfTile;
				j = l - k * numberOfTile;
				xp1 = x0+lpx+j*deltaX;
				yp1 = y0+lpy + k*deltaY;
				xp2 = x0+lpx+(j+1)*deltaX;
				yp2 = y0+lpy+(k+1)*deltaY;
				//une gaussienne de lumière de rayon le 1/10 du max radius de la map centrée sur la cellule en cours
				//alpha = 255/( 1 + exp(  0.1 * ( ((xp1-_xc)*(xp1-_xc) + (yp1-_yc)*(yp1-_yc)  )/_Radius2 ))  );
				// source quadratique
				//alpha = 255/( 1 + (  0.25 * ( ((xp1-_xc)*(xp1-_xc) + (yp1-_yc)*(yp1-_yc)  )/_Radius2 ))  );
				alpha = 255/( 1 + ( 0.01f/(1.0f+number_of_tiles) *  ( ((xp1-_xc)*(xp1-_xc) + (yp1-_yc)*(yp1-_yc)  )/_Radius2 ))  );
				draw_a_box(xp1,yp1,xp2,yp2,rr,gg,bb,alpha);
				//DrawOnScreen();
			}

		}

		if ( ix == (uint32_t) _partyX && iy == (uint32_t) _partyY)
		{
			// une flèche centree sur la salle en cours

			r = 100;g=250;b = 200;
			a = 255;
			draw_a_box(x0+XP3,y0+YP3,x0+XP1,y0+YP1,0,255,0,255);
			// on fait une rotation opposee par rapport au centre de la salle en cours

			// centre de la salle :



			int16_t x1 = (XP3+XP0)/2 - xc;
			int16_t y1 = (YP3+YP0)/2 - yc;

			int16_t xp1 = cosinus[facingTo] * x1 + sinus[facingTo] * y1;
			int16_t yp1 = -sinus[facingTo] * x1 + cosinus[facingTo] * y1;

			int16_t x2 = (XP2+XP1)/2 -xc;
			int16_t y2 = (YP2+YP1)/2 -yc;

			int16_t xp2 = cosinus[facingTo] * x2 + sinus[facingTo] * y2;
			int16_t yp2 = -sinus[facingTo] * x2 + cosinus[facingTo] * y2;

			int16_t x3 = (XP2+XP3)/2 - xc;
			int16_t y3 = (YP2+YP3)/2 - yc;

			int16_t xp3 = cosinus[facingTo] * x3 + sinus[facingTo] * y3;
			int16_t yp3 = -sinus[facingTo] * x3 + cosinus[facingTo] * y3;

			draw_a_filled_triangle(
								x0 + xc + xp1, y0 + yc + yp1,
								x0 + xc + xp2, y0 + yc + yp2 ,
								x0 + xc + xp3, y0 + yc + yp3,
								100, 100, 100, 255);


		}

		/*
		SDL_SetRenderDrawColor(renderer,r,g,b,a);
		if ( room->walls[0] ) // south wall ?
		{
			SDL_RenderLine(renderer, room->x,room->y,x0 + XP1 ,y0 + YP1);
		}
		if ( room->walls[1] ) // left wall ?
		{
			SDL_RenderLine(renderer,room->x,room->y, x0 + XP3 , y0 + YP3);
		}
		if ( room->walls[2] ) // north wall ?
		{
			SDL_RenderLine(renderer, x0 + XP3,y0 + YP3, x0 + XP2,y0 + YP2);
		}
		if ( room->walls[3] ) // right wall ?
		{
			SDL_RenderLine(renderer,x0 + XP1 ,y0 + YP1,x0 + XP2,y0 + YP2);
		}
		*/

	//	if ( room->containsAmonster )
	//	{
	//		if ( room->MonsterIsVisible )
	//		{
	//			boxRGBA(grid,x0+XP3,y0+YP3,x0+XP1,y0+YP1,255,0,0,255);
	//		}
	//		else
	//		{
	//			uint8_t  rr = 0,gg = 0,bb = 0;
	//			if ( room->content == visible )
	//			{
	//				rr = 243, gg = 132, bb = 0;
	//			}
	//
	//			if ( room->content == visited )
	//			{
	//				rr = 0, gg = 132, bb = 0;
	//			}
	//
	//			if ( room->content == stone )
	//			{
	//				rr = gg = bb = 190;
	//			}
	//			//boxRGBA(grid,x0+XP3,y0+YP3,x0+XP1,y0+YP1,rr,gg,bb,255);
	//			int16_t deltaX = abs(XP3 - XP1)/ numberOfTile;// (numberOfTile - 1);
	//			int16_t deltaY = abs(YP3 - YP1)/ numberOfTile ; // (numberOfTile - 1);
	//			int16_t j,k;
	//			uint8_t  alpha;
	//			int16_t xp1,yp1,xp2,yp2;
	//			int16_t lpx = std::min(XP3,XP1);
	//			int16_t lpy = std::min(YP3,YP1);
	//			for (int16_t l = 0; l < numberOfTile * numberOfTile;l++)
	//			{
	//				k = l/numberOfTile;
	//				j = l - k * numberOfTile;
	//				xp1 = x0+lpx+j*deltaX;
	//				yp1 = y0+lpy + k*deltaY;
	//				xp2 = x0+lpx+(j+1)*deltaX;
	//				yp2 = y0+lpy+(k+1)*deltaY;
	//				//une gaussienne de lumière de rayon le 1/10 du max radius de la map centrée sur la cellule en cours
	//				//alpha = 255/( 1 + exp(  0.1 * ( ((xp1-_xc)*(xp1-_xc) + (yp1-_yc)*(yp1-_yc)  )/_Radius2 ))  );
	//				// source quadratique
	//				//alpha = 255/( 1 + (  0.25 * ( ((xp1-_xc)*(xp1-_xc) + (yp1-_yc)*(yp1-_yc)  )/_Radius2 ))  );
	//				alpha = 255/( 1 + ( 0.25f *  ( ((xp1-_xc)*(xp1-_xc) + (yp1-_yc)*(yp1-_yc)  )/_Radius2 ))  );
	//				boxRGBA(grid,xp1,yp1,xp2,yp2,rr,gg,bb,alpha);
	//				bool t =( room->content == visited) || ( room->content == visible );
	//				if ( t )
	//				{
	//					DrawOnScreen();
	//				}
	//			}
	//		}
	//	}

	//	if ( room->content == monster )// monster moved
	//	{
	//		boxRGBA(grid,x0+XP3,y0+YP3,x0+XP1,y0+YP1,0,0,0,255);
	//	}
	//	DrawOnScreen();
	}
	void inline compute_center_of_screen()
	{
		int16_t x0 = 0.5f * width ;
		int16_t y0 = 0.5f * height ;

		// coordonnees des points de la salle par rapport a la salle courante
		//
		int16_t X0 = 0; // coordonnee X lower left

		int16_t Y0 = 0;   // coordonnee Y lower left

		int16_t X1 = X0 + sizeOfBox; // coordonnee X lower right
		int16_t Y1 = Y0; 			  // coordonnee Y lower right

		int16_t X2 = X0 + sizeOfBox; // coordonnee X upper right
		int16_t Y2 = Y0 - sizeOfBox; // coordonnee Y lower right


		int16_t X3 = X0; 			  // coordonnee X upper left
		int16_t Y3 = Y0 - sizeOfBox; // coordonnee Y upper left

		//on applique la rotation
	//		if ( facingTo != previousFacing || initializes )
	//		{
			int16_t XP0 = cosinus[facingTo] * X0 - sinus[facingTo] * Y0;
			int16_t YP0 = sinus[facingTo] * X0 +  cosinus[facingTo] * Y0;


			int16_t XP1 = cosinus[facingTo] * X1 - sinus[facingTo] * Y1 ;
			int16_t YP1 = sinus[facingTo] * X1 + cosinus[facingTo] * Y1 ;

			int16_t XP2 = cosinus[facingTo] * X2 - sinus[facingTo] * Y2 ;
			int16_t YP2 = sinus[facingTo] * X2 + cosinus[facingTo] * Y2 ;


			int16_t XP3 = cosinus[facingTo] * X3 - sinus[facingTo] * Y3 ;
			int16_t YP3 = sinus[facingTo] * X3 + cosinus[facingTo] * Y3 ;

	//		}
	//		else
	//		{
	//			room->x = x0 +  deltaX ;
	//			room->y = y0 +  deltaY ;
	//		}


	_xc = x0 + (XP1+XP3)/2 ;
	_yc = y0 + (YP1+YP3)/2 ;


	}
	void inline change_level(int32_t newLevel, int16_t facing, uint32_t nX, uint32_t nY)
{

	firstInitialisation = false;

	if ( _MaxlevelNumber == -1 )
	{
		_currentLevel = newLevel;
	}



	_numberOfBoxX = nX;
	_numberOfBoxY = nY;

	number_of_boxes = _numberOfBoxX * _numberOfBoxY;
	//sizeOfBox = SIZE_OF_BOX;

	facingTo = facing;

	_Radius2 = 1; //td::min(_numberOfBoxX,_numberOfBoxY)/2 ;

	_Radius2 *= _Radius2;

	// a t on deja stocke ce level ?
	if ( newLevel > _MaxlevelNumber )
	{

		_levels = (Level *) realloc( _levels, (newLevel + 1 ) * sizeof(Level) );

		rooms = (Room *)malloc(nX * nY * sizeof(Room) );
		memset(rooms,0, nX * nY * sizeof(Room) );
		_levels[newLevel].map = rooms; // on connecte le level et la map
		_levels[newLevel].monsters = NULL;
		_levels[newLevel]._numberOFMonsters = 0;
		_levels[newLevel].sizeX = _numberOfBoxX;
		_levels[newLevel].sizeY = _numberOfBoxY;
		_MaxlevelNumber = newLevel;

		// on sauvegarde le sizeofBox du level en cours
		_levels[newLevel].sizeOfBox = sizeOfBox;
		_levels[newLevel].numberOfTile = numberOfTile;
		// on reset le size of box
		//sizeOfBox = SIZE_OF_BOX;
		//numberOfTile = sizeOfBox/SIZE_OF_TILE;
	}
	else
	{
		// on sauvegarde le sizeofBox du level en cours
		_levels[_currentLevel].sizeOfBox = sizeOfBox;
		// oui on le recupere
		rooms = _levels[newLevel].map ;
		sizeOfBox = _levels[newLevel].sizeOfBox;
		numberOfTile = _levels[_currentLevel].numberOfTile;
		number_of_tiles = numberOfTile * numberOfTile;
	}

	_currentLevel = newLevel;
	pan_to_party_position();
}


	void inline update_party_position( int16_t x, int16_t y)
	{
		if ( !firstInitialisation )
		{
			initializes = true;
		}

		if ( !initializes)
		{
			rooms[_partyX + _numberOfBoxX * _partyY].content = RoomContent::visited;
		}

		look_around_for_nearby_monsters(x,y);
		update_monsters(x,y);

		_partyX = x;
		_partyY = y;
		pan_to_party_position();

		if ( !firstInitialisation )
		{
			initializes = false;
			firstInitialisation = true;
		}
	}
	void inline initiliaze_position_of_party( int16_t x, int16_t y)
	{
		initializes = true;
		_partyX = x;
		_partyY = y;
		pan_to_party_position();
		initializes = false;
	}



	void inline analyze_rooms_around(int16_t x, int16_t y)
{
	// ok on regarde les roomtypes des rooms situées devant, derrière, gauche, droite

	// derrière x,y-1

	// current room

	int16_t i = x + _numberOfBoxX * y;
	Room *room = (Room *) (rooms + i );

	bool frontIllegal = false, rightIllegal = false, leftIllegal = false, behindIllegal = false,
			NEIllegal = false, SEIllegal = false, NWIllegal = false, SWIllegal = false;

	int16_t iFront,iLeft,iBehind,iRight,iNE,iSE,iNW,iSW;

	int32_t behind, front, left,right, NE, SE, SW,NW ;
	ROOMTYPE roomBehind, roomFront, roomLeft, roomRight,roomNE, roomSE, roomSW,roomNW  ;

	switch( facingTo )
	{
	case 2 :


		if ( y - 1 < 0 )
		{
			behindIllegal = true;
			SWIllegal = true;
			SEIllegal = true;
		}

		if ( x - 1 < 0 )
		{
			rightIllegal = true;
			NEIllegal = true;
			SEIllegal = true;
		}

		if ( y + 1 >= _numberOfBoxY )
		{
			frontIllegal = true;
			NEIllegal = true;
			NWIllegal = true;
		}

		if ( x + 1 >= _numberOfBoxX )
		{
			leftIllegal = true;
			NWIllegal = true;
		}

		behind = GetCellFlags(x, y - 1);
		iBehind = x + _numberOfBoxX * (y-1);

		front = GetCellFlags(x, y + 1);
		iFront  = x + _numberOfBoxX * (y+1);


		left = GetCellFlags(x+1, y);
		iLeft   = x+1 + _numberOfBoxX * y;

		right = GetCellFlags(x-1, y);
		iRight  = x-1 + _numberOfBoxX * y;

		NE = GetCellFlags(x - 1, y + 1);
		iNE = x-1 + _numberOfBoxX * (y+1);

		SE = GetCellFlags(x - 1, y -1);
		iSE = x-1 + _numberOfBoxX * (y-1);

		NW = GetCellFlags(x + 1, y + 1);
		iNW = x+1 + _numberOfBoxX * (y+1);

		SW = GetCellFlags(x + 1, y -1);
		iSW = x+1 + _numberOfBoxX * (y-1);

		break;
	case 0:
		front = GetCellFlags(x, y -1);   iFront  = x + _numberOfBoxX * (y-1);
		if ( y-1 < 0)
		{
			frontIllegal = true;
			NEIllegal = true;
			SEIllegal = true;
		}

		if ( x - 1 < 0 )
		{
			leftIllegal=true;
			NEIllegal = true;
			NWIllegal = true;
		}

		if ( y+1 >= _numberOfBoxY)
		{
			behindIllegal = true;
			NWIllegal = true;
			SWIllegal = true;
		}

		if ( x+1 >= _numberOfBoxX)
		{
			rightIllegal = true;
			SEIllegal = true;
			SWIllegal = true;
		}

		behind = GetCellFlags(x, y + 1);
		iBehind = x + _numberOfBoxX * (y+1);


		left = GetCellFlags(x-1, y);
		iLeft   = x-1 + _numberOfBoxX * y;


		right = GetCellFlags(x+1, y);
		iRight  = x+1 + _numberOfBoxX * y;


		NE = GetCellFlags(x + 1, y - 1);
		iNE = x+1 + _numberOfBoxX * (y-1);


		SE = GetCellFlags(x + 1, y + 1);
		iSE = x+1 + _numberOfBoxX * (y+1);


		NW = GetCellFlags(x - 1, y - 1);
		iNW = x-1 + _numberOfBoxX * (y-1);


		SW = GetCellFlags(x - 1, y + 1);
		iSW = x-1 + _numberOfBoxX * (y+1);


		break;
	case 1:
		front = GetCellFlags(x+1, y);    iFront  = x+1 + _numberOfBoxX * y;
		if ( x+1 >= _numberOfBoxX)
			frontIllegal = true;

		left = GetCellFlags(x, y -1);    iLeft   = x + _numberOfBoxX * (y-1);
		if (y-1 < 0 )
			leftIllegal = true;

		right = GetCellFlags(x, y + 1);  iRight  = x + _numberOfBoxX * (y+1);

		if (y+1 >= _numberOfBoxY)
			rightIllegal = true;

		behind = GetCellFlags(x-1, y);   iBehind = x-1 + _numberOfBoxX * y;
		if ( x-1 < 0)
			behindIllegal = true;

		NE = GetCellFlags(x + 1, y + 1); iNE = x+1 + _numberOfBoxX * (y+1);
		if ( x+1 >= _numberOfBoxX || y+1 >= _numberOfBoxY )
			NEIllegal = true;

		SE = GetCellFlags(x - 1, y + 1); iSE = x-1 + _numberOfBoxX * (y+1);
		if ( x-1 < 0 || y+1 >= _numberOfBoxY)
		SEIllegal = true;

		NW = GetCellFlags(x + 1, y - 1); iNW = x+1 + _numberOfBoxX * (y-1);
		if ( x+1 >= _numberOfBoxX || y-1 < 0)
		NWIllegal = true;

		SW = GetCellFlags(x - 1, y - 1); iSW = x-1 + _numberOfBoxX * (y-1);
		if ( x-1 < 0 || y-1 < 0)
			SWIllegal = true;

		break;
	case 3:
		front = GetCellFlags(x-1, y);    iFront  = x-1 + _numberOfBoxX * y;
		if(y-1 < 0)
			frontIllegal = true;

		left = GetCellFlags(x, y + 1);   iLeft   = x + _numberOfBoxX * (y+1);
		if (y+1 >= _numberOfBoxY )
			leftIllegal = true;

		right = GetCellFlags(x, y - 1);  iRight  = x + _numberOfBoxX * (y-1);
		if(y-1 < 0)
			rightIllegal = true;

		behind = GetCellFlags(x+1, y);   iBehind = x+1 + _numberOfBoxX * y;
		if ( x+1 >= _numberOfBoxX )
			behindIllegal = true;

		NE = GetCellFlags(x - 1, y - 1); iNE = x-1 + _numberOfBoxX * (y-1);
		if ( x-1 < 0 || y-1 < 0)
			NEIllegal = true;

		SE = GetCellFlags(x + 1, y - 1); iSE = x+1 + _numberOfBoxX * (y-1);
		if ( x+1 >= _numberOfBoxX || y-1 < 0)
			SEIllegal = true;

		NW = GetCellFlags(x - 1, y + 1); iNW = x-1 + _numberOfBoxX * (y+1);
		if ( x-1 < 0 || y+1 >= _numberOfBoxY)
		NWIllegal = true;

		SW = GetCellFlags(x + 1, y + 1); iSW = x+1 + _numberOfBoxX * (y+1);
		if ( x+1 >= _numberOfBoxX || y+1 >= _numberOfBoxY)
			SWIllegal = true;

		break;
	}

/*
 *
 * RoomType renvoie 0 si cellflag pointe vers une cellule en dehors
 *
 * de (0-width-1,0-height-1)
 */

	roomBehind = RoomType(behind);
	roomFront = RoomType(front);
	roomLeft = RoomType(left);
	roomRight = RoomType(right);
	roomNE = RoomType(NE);
	roomSE = RoomType(SE);
	roomNW = RoomType(NW);
	roomSW = RoomType(SW);

	switch( facingTo )
	{
	case 2:

		if ( roomFront == roomSTONE  )
		{
			room->walls[2] = true;

			if ( !frontIllegal)
			{
				rooms[iFront].walls[0] = true;
				rooms[iFront].content = RoomContent::stone;
			}
		}
		if ( roomFront == roomOPEN && !frontIllegal && rooms[iFront].content != RoomContent::visited )
		{
			rooms[iFront].content = RoomContent::visible;
		}
			// on ne peut voir qu'a gauche et a droite
			if ( roomLeft == roomSTONE   )
			{
				room->walls[1] = true;

				if ( !leftIllegal)
				{
					rooms[iLeft].walls[3] = true;
					rooms[iLeft].content = RoomContent::stone; // c'est un mur
				}
			}
			if ( !leftIllegal && roomLeft == roomOPEN &&  rooms[iLeft].content != RoomContent::visited)
			{
				rooms[iLeft].content = RoomContent::visible;
			}
			if ( roomRight == roomSTONE  )
			{
				room->walls[3] = true;

				if ( !rightIllegal)
				{
					rooms[iRight].walls[1] = true;
					rooms[iRight].content = RoomContent::stone; // c'est un mur
				}
			}
			if ( !rightIllegal && roomRight == roomOPEN &&  rooms[iRight].content != RoomContent::visited)
			{
				rooms[iRight].content = RoomContent::visible;
			}
			break;

	case 0:

		if ( roomFront == roomSTONE )
		{
			room->walls[0] = true;
			if  ( !frontIllegal)
			{
				rooms[iFront].walls[2] = true;
				rooms[iFront].content = RoomContent::stone;
			}
		}
		if ( !frontIllegal  && roomFront == roomOPEN &&  rooms[iFront].content != RoomContent::visited)
		{
			rooms[iFront].content = RoomContent::visible;
		}
		// on ne peut voir qu'a gauche et a droite
		if ( roomLeft == roomSTONE )
		{
			room->walls[3] = true;
			if ( !leftIllegal)
			{
				rooms[iLeft].walls[1] = true;
				rooms[iLeft].content = RoomContent::stone; // c'est un mur
			}
		}
		if ( !leftIllegal  && roomLeft == roomOPEN &&  rooms[iLeft].content != RoomContent::visited)
		{
			rooms[iLeft].content = RoomContent::visible;
		}
		if ( roomRight == roomSTONE   )
		{
			room->walls[1] = true;
			if ( !rightIllegal)
			{
				rooms[iRight].walls[3] = true;
				rooms[iRight].content = RoomContent::stone; // c'est un mur
			}
		}
		if ( !rightIllegal  && roomRight == roomOPEN &&  rooms[iRight].content != RoomContent::visited)
		{
			rooms[iRight].content = RoomContent::visible;
		}

		break;

	case 1:
		if ( roomFront == roomSTONE )
		{
			room->walls[1] = true;
			if ( !frontIllegal)
			{
				rooms[iFront].walls[3] = true;
				rooms[iFront].content = RoomContent::stone;
			}
		}
		if ( !frontIllegal  &&  roomFront == roomOPEN && rooms[iFront].content != RoomContent::visited)
		{
			rooms[iFront].content = RoomContent::visible;
		}
		// on ne peut voir qu'a gauche et a droite
		if ( roomLeft == roomSTONE && !leftIllegal  )
		{
			room->walls[0] = true;
			if ( !leftIllegal)
			{
				rooms[iLeft].walls[2] = true;
				rooms[iLeft].content = RoomContent::stone; // c'est un mur
			}
		}
		if ( !leftIllegal  &&  roomLeft == roomOPEN && rooms[iLeft].content != RoomContent::visited)
		{
			rooms[iLeft].content = RoomContent::visible;
		}
		if ( roomRight == roomSTONE )
		{
			room->walls[2] = true;
			if(!rightIllegal)
			{
				rooms[iRight].walls[0] = true;
				rooms[iRight].content = RoomContent::stone; // c'est un mur
			}
		}
		if ( !rightIllegal  &&  roomRight == roomOPEN && rooms[iRight].content != RoomContent::visited)
		{
			rooms[iRight].content = RoomContent::visible;
		}
		break;
	case 3:
		if ( roomFront == roomSTONE )
		{
			room->walls[3] = true;
			if ( !frontIllegal)
			{
			  rooms[iFront].walls[1] = true;
			  rooms[iFront].content = RoomContent::stone;
			}
		}
		if ( !frontIllegal  && roomFront == roomOPEN &&  rooms[iFront].content != RoomContent::visited)
		{
			rooms[iFront].content = RoomContent::visible;
		}
		// on ne peut voir qu'a gauche et a droite
		if ( roomLeft == roomSTONE )
		{
			room->walls[2] = true;
			if ( !leftIllegal)
			{
				rooms[iLeft].walls[0] = true;
				rooms[iLeft].content = RoomContent::stone; // c'est un mur
			}
		}
		if ( !leftIllegal  && roomLeft == roomOPEN &&  rooms[iLeft].content != RoomContent::visited)
		{
			rooms[iLeft].content = RoomContent::visible;
		}
		if ( roomRight == roomSTONE  )
		{
			room->walls[0] = true;
			if ( !rightIllegal)
			{
				rooms[iRight].walls[2] = true;
				rooms[iRight].content = RoomContent::stone; // c'est un mur
			}
		}
		if ( !rightIllegal  && roomRight == roomOPEN &&  rooms[iRight].content != RoomContent::visited )
		{
			rooms[iRight].content = RoomContent::visible;
		}
		break;
	}


	if ( !NEIllegal  && roomNE == roomSTONE &&  !(roomFront == roomSTONE && roomRight == roomSTONE) )
	{
		rooms[iNE].content = RoomContent::stone;
		rooms[iNE].walls[0] = rooms[iNE].walls[1] = rooms[iNE].walls[2] = rooms[iNE].walls[3] = true;
	}
	else if (!NEIllegal && roomNE == roomOPEN && rooms[iNE].content != RoomContent::visited )
	{
		rooms[iNE].content = RoomContent::visible;
	}
	if ( !NWIllegal &&  roomNW == roomSTONE && !(roomFront == roomSTONE && roomLeft == roomSTONE) )
	{
		rooms[iNW].content = RoomContent::stone;
		rooms[iNW].walls[0] = rooms[iNW].walls[1] = rooms[iNW].walls[2] = rooms[iNW].walls[3] = true;
	}
	else if (!NWIllegal &&  roomNW == roomOPEN && rooms[iNW].content != RoomContent::visited )
	{
		rooms[iNW].content = RoomContent::visible;
	}


	if ( !NWIllegal && roomFront == roomSTONE && roomLeft == roomSTONE &&  !(rooms[iNW].content == RoomContent::visited) && !(rooms[iNW].content ==RoomContent::visible) && !(rooms[iNW].content == RoomContent::stone))
	{
		rooms[iNW].content = RoomContent::empty;
	}

	if ( !NEIllegal && roomFront == roomSTONE && roomRight == roomSTONE && !(rooms[iNE].content == RoomContent::visited) && !(rooms[iNE].content == RoomContent::visible) && !(rooms[iNE].content == RoomContent::stone) )
	{
		rooms[iNE].content = RoomContent::empty;
	}

	compute_center_of_screen();
	draw_room(i, x,y );


	//DrawOnScreen();
}


	TTF_Text *ttf_text;
	TTF_Text *msg_text;
	void inline render()
	{
		// Load a font

		SDL_Color text_color = {255, 255, 0,255};
		SDL_Rect textLocation = { 10, 10, 0, 0 };

		sprintf((char *)text, " location Room At (%u,%u) \n level #%i",_partyX,_partyY, _currentLevel );
		ttf_text = TTF_CreateText(ttf_text_engine,font_18,text,strlen(text));
		msg_text = TTF_CreateText(ttf_text_engine,font_12,msg_str.c_str(),msg_str.size());
		TTF_SetTextColor(ttf_text,255,255,0,255);
		SDL_RenderLine(renderer, 1, 1,width - 1, 1 );
		SDL_RenderLine(renderer, 1, 1 , 1, height - 1 );
		SDL_RenderLine(renderer,  1,height - 1 ,  height - 1 ,height -1 );
		SDL_RenderLine(renderer, width -1,  height- 1, width -1, 1 );
		TTF_DrawRendererText(ttf_text,10,10);
		TTF_DrawRendererText(msg_text,10,height-MAX_MESSAGES * 12);
		render_texture();
		TTF_DestroyText(ttf_text);
		TTF_DestroyText(msg_text);
	}

	void inline render_texture(){
		SDL_UpdateTexture(texture,NULL,grid->pixels,grid->pitch);
		SDL_RenderPresent(renderer);
		SDL_SetRenderDrawColor(renderer,0,0,0,0);
		SDL_RenderClear(renderer);
	}

};


