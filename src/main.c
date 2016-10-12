/*
	VPKMirror
	Copyright (C) 2016, SMOKE

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <psp2/ctrl.h>
#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/ctrl.h>
#include <psp2/system_param.h>
#include <vita2d.h>
#include "main.h"
#include "init.h"
#include "net.h"
#include "sqlite3.h"
#include "../minizip/unzip.h"
#include <sys/stat.h>
#include <sys/types.h>

#define dir_delimter '/' //Estos 3 son para el unzip
#define MAX_FILENAME 512
#define READ_SIZE 8192

#define APP_DB "ur0:/shell/db/app.db"
#define installdir "ux0:test/theme.zip"
#define installdir2 "theme.zip"

int sceAppMgrGetAppParam(char *param);
char AppParam[1024];
int key_cache;

#define COLOR_BLACK      0xFF000000
#define COLOR_RED        0xFF0000FF
#define COLOR_BLUE       0xFF00FF00
#define COLOR_YELLOW     0xFF00FFFF
#define COLOR_GREEN      0xFFFF0000
#define COLOR_MAGENTA    0xFFFF00FF
#define COLOR_CYAN       0xFFFFFF00
#define COLOR_WHITE      0xFFFFFFFF
#define COLOR_GREY       0xFF808080
#define COLOR_DEFAULT_FG COLOR_WHITE
#define COLOR_DEFAULT_BG COLOR_BLACK
static uint32_t psvDebugScreenColorBg = COLOR_DEFAULT_BG;
static unsigned buttons[] = {
	SCE_CTRL_SELECT,
	SCE_CTRL_START,
	SCE_CTRL_UP,
	SCE_CTRL_RIGHT,
	SCE_CTRL_DOWN,
	SCE_CTRL_LEFT,
	SCE_CTRL_LTRIGGER,
	SCE_CTRL_RTRIGGER,
	SCE_CTRL_TRIANGLE,
	SCE_CTRL_CIRCLE,
	SCE_CTRL_CROSS,
	SCE_CTRL_SQUARE,
};

int get_key(void) {
	static unsigned prev = 0;
	SceCtrlData pad;
	while (1) {
		memset(&pad, 0, sizeof(pad));
		sceCtrlPeekBufferPositive(0, &pad, 1);
		unsigned new = prev ^ (pad.buttons & prev);
		prev = pad.buttons;
		for (int i = 0; i < sizeof(buttons)/sizeof(*buttons); ++i)
			if (new & buttons[i])
				return buttons[i];

		sceKernelDelayThread(1000); // 1ms
	}
}

void sql_simple_exec(sqlite3 *db, const char *sql) {
	char *error = NULL;
	int ret = 0;
	ret = sqlite3_exec(db, sql, NULL, NULL, &error);
	if (error) {
		printf("Failed to execute %s: %s\n", sql, error);
		sqlite3_free(error);
		goto fail;
	}
	return;
fail:
	sqlite3_close(db);
}

void do_uri_mod(void) {
	int ret;

	sqlite3 *db;
	ret = sqlite3_open(APP_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
	}

	sql_simple_exec(db, "DELETE FROM tbl_uri WHERE titleId='TEMA00001'");
	sql_simple_exec(db, "INSERT INTO tbl_uri VALUES ('TEMA00001', '1', 'zip', NULL)");

	sqlite3_close(db);
	db = NULL;

	return;
}

void insert_theme_to_db(char *queryinsert, char *querydelete) {
	int ret;


	sqlite3 *db;
	ret = sqlite3_open(APP_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
	}

	sql_simple_exec(db, querydelete);
	sql_simple_exec(db, queryinsert);
	sqlite3_close(db);
	db = NULL;
	printf("\nDONE!\n");
	return;
}

void delete_uniquetheme_db(char *querydelete) {
	int ret;


	sqlite3 *db;
	ret = sqlite3_open(APP_DB, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
	}

	sql_simple_exec(db, querydelete);

	sqlite3_close(db);
	db = NULL;

	return;
}

void delete_uniquetheme() {
#define commonThemepath "ux0:customtheme/c/commom_theme"
	netInit(); //SI NO PONGO ESTOS DOS TIRRAR ERROR LA FUNCION DONWLOAD
					httpInit();
				//Elimino todos los temas menos el que esta en la direccion de common

			char *sqldeletequery[256];
							strcpy(sqldeletequery, "DELETE FROM tbl_theme WHERE id NOT IN ('");
							strcat(sqldeletequery, commonThemepath); //install_dir es el que no tiene el / al final
							strcat(sqldeletequery, "')"); //borro todos los temas que no sean el commontheme
				printf("\nDeleting unique themes from database...\n");
				delete_uniquetheme_db(sqldeletequery);
				printf("\nDeleting unique themes from ux0:customtheme/u/...\n");
				removePath("ux0:customtheme/u", NULL); //borro la carpeta de temas unicos
				sceIoMkdir("ux0:customtheme/u", 0777);//creo de nuevo la carpeta de temas unicos
				printf("\nDONE!\n");

	printf("\nAuto exiting in 5 seconds...\n");

		sceKernelDelayThread(5 * 1000 * 1000);

		httpTerm();
		netTerm();
		finishSceAppUtil();

		sceKernelExitProcess(0);
}

void install_liveareacommontheme() {
	  char buffer [255];
	  int copy;
	  char patch[256];
	  char backup[256];
	printf("\nBacking up database file (app.db)\n");
	sprintf(patch, "ur0:shell/db/%s","app.db");
	sprintf(backup, "ur0:shell/db/%s_orig.db", "app");
	snprintf(buffer, 255, "Backing up database %s to %s...", patch, backup);
	printf("\nBacking up from %s to %s...\n", patch, backup);
	copy = copyfile(patch, backup);//AQUI YA TENGO MI BACKUP LISTO
#define commonThemepath "ux0:customtheme/c/commom_theme"

	/* grab app param from our custom uri
		   full app param looks like:
		   type=LAUNCH_APP_BY_URI&uri=zip:areainstall?theme.zip
		*/
		netInit();//SI NO PONGO ESTOS DOS TIRRAR ERROR LA FUNCION DONWLOAD
		httpInit();
		// get the part of the argument that we need
		char *file_name;
		file_name = strchr(AppParam, '?')+1; //obtengo el nombre del archivo a descargar

		// create url based off the file name
		char *file_url = malloc(512 * sizeof(char));
		snprintf(file_url, 512, "http://vita.xeron.co/themes/%s", file_name);

		// download file
		char *file_path = malloc(512 * sizeof(char)); //Es la direccion en el vita donde se descargo el zip
		snprintf(file_path, 512, "ux0:ptmp/%s", file_name);

		printf("\nDownloading %s...\n", file_name);
		download(file_url, file_path);


		char *install_dir = malloc(512 * sizeof(char));//Tiene que estar asi con el corchete o tira error
		char *install_dir2 = malloc(512 * sizeof(char));

							snprintf(install_dir, 512, "ux0:customtheme/c/%s",file_name); //le pongo el / al final del nombre de la carpeta para que ahi guarde los archivos descomprimidos

							     int length;
							     length = strlen(install_dir);//Get length of string
							     install_dir[length - 4] = '\0';//Hago null los ultimos 4 charectes que serian el ".zip" para crear un nombre de carpeta apropiado
							     snprintf(install_dir2, 512, "%s/",install_dir);

					printf("\nInstalling File...\n");
					installCommonTheme(file_path,"ux0:customtheme/c/commom_theme/");//Aqui descomprimo el zip en una carpeta para usarlo
					printf("\nDeleting temp files...\n");
					sceIoRemove(file_path); //Borro el zip de la carpeta ptmp


			//Guardo el nuevo tema en la base de datos
			char *sqlinsertquery[256];
			strcpy(sqlinsertquery, "INSERT INTO tbl_theme (id,packageImageFilePath,homePreviewFilePath,startPreviewFilePath,contentVer,size,type) VALUES ('");
			strcat(sqlinsertquery, commonThemepath); //no tiene el / al final
			strcat(sqlinsertquery, "', 'preview_thumbnail.png', 'preview_page.png', 'preview_lockscreen.png', '100', '0', '100'");
			strcat(sqlinsertquery, ")");

			char *sqldeletequery[256];
						strcpy(sqldeletequery, "DELETE FROM tbl_theme WHERE id ='");
						strcat(sqldeletequery, commonThemepath); //no tiene el / al final
						strcat(sqldeletequery, "'");
						printf("\nInserting theme path to database...\n");
			insert_theme_to_db(sqlinsertquery, sqldeletequery);
			printf("\nAuto exiting in 5 seconds...\n");

				sceKernelDelayThread(5 * 1000 * 1000);

				httpTerm();
				netTerm();
				finishSceAppUtil();

				sceKernelExitProcess(0);
}

void install_liveareatheme() {
	  char buffer [255];
	  int copy;
	  char patch[256];
	  char backup[256];
	printf("\nBacking up database file (app.db)\n");
	sprintf(patch, "ur0:shell/db/%s","app.db");
	sprintf(backup, "ur0:shell/db/%s_orig.db", "app");
	snprintf(buffer, 255, "Backing up database %s to %s...", patch, backup);
	printf("\nBacking up from %s to %s...\n", patch, backup);
	copy = copyfile(patch, backup);//AQUI YA TENGO MI BACKUP LISTO

	/* grab app param from our custom uri
		   full app param looks like:
		   type=LAUNCH_APP_BY_URI&uri=zip:areainstall?theme.zip
		*/

		netInit();//SI NO PONGO ESTOS DOS TIRRAR ERROR LA FUNCION DONWLOAD
		httpInit();
		// get the part of the argument that we need
		char *file_name;
		file_name = strchr(AppParam, '?')+1; //obtengo el nombre del archivo a descargar

		// create url based off the file name
		char *file_url = malloc(512 * sizeof(char));
		snprintf(file_url, 512, "http://vita.xeron.co/themes/%s", file_name);

		// download file

		char *file_path = malloc(512 * sizeof(char)); //Es la direccion en el vita donde se descargo el zip
		snprintf(file_path, 512, "ux0:ptmp/%s", file_name);

		printf("\nDownloading %s...\n", file_name);
		download(file_url, file_path);



		char *install_dir = malloc(512 * sizeof(char));//Tiene que estar asi con el corchete o tira error
		char *install_dir2 = malloc(512 * sizeof(char));

							snprintf(install_dir, 512, "ux0:customtheme/u/%s",file_name); //le pongo el / al final del nombre de la carpeta para que ahi guarde los archivos descomprimidos

							     int length;
							     length = strlen(install_dir);//Get length of string
							     install_dir[length - 4] = '\0';//Hago null los ultimos 4 charectes que serian el ".zip" para crear un nombre de carpeta apropiado
							     snprintf(install_dir2, 512, "%s/",install_dir);

					printf("\nInstalling File...\n");
					installTheme(file_path,install_dir2);//Aqui descomprimo el zip en una carpeta para usarlo
					printf("\nDeleting temp files...\n");
					sceIoRemove(file_path); //Borro el zip de la carpeta ptmp

			//Guardo el nuevo tema en la base de datos
			char *sqlinsertquery[256];
			strcpy(sqlinsertquery, "INSERT INTO tbl_theme (id,packageImageFilePath,homePreviewFilePath,startPreviewFilePath,contentVer,size,type) VALUES ('");
			strcat(sqlinsertquery, install_dir); //install_dir es el que no tiene el / al final
			strcat(sqlinsertquery, "', 'preview_thumbnail.png', 'preview_page.png', 'preview_lockscreen.png', '100', '0', '100'");
			strcat(sqlinsertquery, ")");

			char *sqldeletequery[256];
						strcpy(sqldeletequery, "DELETE FROM tbl_theme WHERE id ='");
						strcat(sqldeletequery, install_dir); //install_dir es el que no tiene el / al final
						strcat(sqldeletequery, "'");
						printf("\nInserting theme path to database...\n");
			insert_theme_to_db(sqlinsertquery, sqldeletequery);

			printf("\nAuto exiting in 5 seconds...\n");

				sceKernelDelayThread(5 * 1000 * 1000);

				httpTerm();
				netTerm();
				finishSceAppUtil();

				sceKernelExitProcess(0);
}

void install_vitashelltheme() {
	/* grab app param from our custom uri
	   full app param looks like:
	   type=LAUNCH_APP_BY_URI&uri=zip:shellinstall?theme.zip
	*/

	netInit();//SI NO PONGO ESTOS DOS TIRRAR ERROR LA FUNCION DONWLOAD
	httpInit();
	// get the part of the argument that we need
	char *file_name;
	file_name = strchr(AppParam, '?')+1; //obtengo el nombre del archivo a descargar

	// create url based off the file name
	char *file_url = malloc(512 * sizeof(char));
	snprintf(file_url, 512, "http://vita.xeron.co/themes/%s", file_name);

	// download file
	//printf("Downloading %s...\n", file_url);
	char *file_path = malloc(512 * sizeof(char)); //Es la direccion en el vita donde se descargo el zip
	snprintf(file_path, 512, "ux0:ptmp/%s", file_name);

	printf("\nDownloading %s...\n", file_name);
	download(file_url, file_path);


	char *install_dir = malloc(512 * sizeof(char));//Tiene que estar asi con el corchete o tira error
	char *install_dir2 = malloc(512 * sizeof(char));
	//file_name = strrchr(tempAppParan, '/')+1;
		//				printf("\nFile name: %s...", AppParam); //Muestro el nombre del archivo
	//					sceKernelDelayThread(5* 1000*1000);
						snprintf(install_dir, 512, "ux0:VitaShell/theme/%s",file_name); //le pongo el / al final del nombre de la carpeta para que ahi guarde los archivos descomprimidos
						     int length;
						     length = strlen(install_dir);//Get length of string
						     install_dir[length - 4] = '\0';//Hago null los ultimos 4 charectes que serian el ".zip" para crear un nombre de carpeta apropiado
						     snprintf(install_dir2, 512, "%s/",install_dir);

											//sceKernelDelayThread(5* 1000*1000);

				printf("\nInstalling File...\n");
				installTheme(file_path,install_dir2);//Aqui descomprimo el zip en una carpeta para usarlo
				printf("\nDeleting temp files...\n");
				sceIoRemove(file_path); //Borro el zip de la carpeta ptmp

				//sceKernelDelayThread(5* 1000*1000);
				printf("\nAuto exiting in 5 seconds...\n");

					sceKernelDelayThread(5 * 1000 * 1000);

					httpTerm();
					netTerm();
					finishSceAppUtil();

					sceKernelExitProcess(0);
}


int main() {

	int key = 0; //Se usa para saber que boton presione y hacer la accion correspondiente
	again:

	//Inicializo screen
	psvDebugScreenInit();
	// clears screen with a given color
	psvDebugScreenClear(psvDebugScreenColorBg);
	initSceAppUtil();

		sceAppMgrGetAppParam(AppParam);
		int arg_len = strlen(AppParam);

	printf("Welcome to TEMA this application lets you download a theme\n");
	printf("for VitaShell or Live Area and install it for its use.\n");
	printf("PLEASE BE CAREFUL NOT TO INSTALL A VITASHELL THEME\n");
	printf("AS A LIVE AREA ONE.\n");
	printf("\n\n");
	if (arg_len == 0) {

		printf("Press X to install a VitaShell Theme.\n");
		printf("Press O to install a Live Area Theme.\n");
		//printf("Press TRIANGLE to install a Common Live Area Theme.\n");
		printf("Press SQUARE to delete all unique Live Area Themes.\n");
		printf("\n");

		key = get_key();
		switch (key) {
		case SCE_CTRL_CROSS:
				do_uri_mod();
				printf("This should now open the browser.\nFrom there you can click a link and it will return here.\n");
				sceKernelDelayThread(3 * 1000 * 1000); // 3 seconds
				sceAppMgrLaunchAppByUri(0xFFFFF, "http://vita.xeron.co/vitashell");
				sceKernelDelayThread(10000);
				sceAppMgrLaunchAppByUri(0xFFFFF, "http://vita.xeron.co/vitashell");
				sceKernelExitProcess(0);


			install_vitashelltheme();
			break;
		case SCE_CTRL_CIRCLE:
			do_uri_mod();
						printf("This should now open the browser.\nFrom there you can click a link and it will return here.\n");
						sceKernelDelayThread(3 * 1000 * 1000); // 3 seconds
						sceAppMgrLaunchAppByUri(0xFFFFF, "http://vita.xeron.co/livearea");
						sceKernelDelayThread(10000);
						sceAppMgrLaunchAppByUri(0xFFFFF, "http://vita.xeron.co/livearea");
						sceKernelExitProcess(0);
			install_liveareatheme();
			break;
		case SCE_CTRL_TRIANGLE:
			install_liveareacommontheme();
			break;
		case SCE_CTRL_SQUARE:
			//Doble confirmacion para extra seguridad

			//psvDebugScreenClear(psvDebugScreenColorBg);
			printf("\nAre you sure you want to delete ALL unique Live Area Themes?\n");
			printf("Press SELECT to confirm or START to cancel.\n");
			againDelete:
			key = get_key();
					switch (key) {
					case SCE_CTRL_SELECT:
						delete_uniquetheme();
						break;
					case SCE_CTRL_START:
						printf("\nUnique Live Area Themes not deleted.\n");
						printf("\nAuto exiting in 5 seconds...\n");
						sceKernelDelayThread(5 * 1000 * 1000);

						finishSceAppUtil();

						sceKernelExitProcess(0);
						break;
					default:
						printf("\nInvalid input, try again.\n\n");
						goto againDelete;
					}

					break;
		default:
			printf("Invalid input, try again.\n\n");
			goto again;
		}

		}

		char *menusel;
		char *menusel2;
		char *menusel3 = malloc(512 * sizeof(char));
		menusel = AppParam;
        menusel2 = strrchr(menusel, ':')+1;

        strncpy(menusel3, menusel2,1);

		sceKernelDelayThread(5* 1000*1000);
		if (strcmp(menusel3,"s")==0) {
			install_vitashelltheme();
		}
		else if (strcmp(menusel3,"a")==0) {
			again2:
			printf("\nPress SELECT to install the Live Area Theme as Unique.\n");
			printf("\nPress START to install the Live Area Theme as Common.\n\n");
			key = get_key();
					switch (key) {
					case SCE_CTRL_SELECT:
						install_liveareatheme();

						break;
					case SCE_CTRL_START:
						install_liveareacommontheme();

					break;
					default:
						printf("Invalid input, try again.\n\n");
									goto again2;
							}
		}


}



