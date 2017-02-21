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
#include <errno.h>
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
#include <vita2d.h>
#include "appdb.h"
#include "font.h"

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

#define PAGE_ITEM_COUNT 20
#define SCREEN_ROW      27
#define ROW_HEIGHT      20
#define concat(str1, str2) \
    snprintf(buf, 256, "%s%s", str1, str2) ? buf : ""
vita2d_pgf* debug_font;
uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);
extern int errno;
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
	char buf[256];
	char *error = NULL;
	int ret = 0;
	ret = sqlite3_exec(db, sql, NULL, NULL, &error);
	if (error) {
		//printf("Failed to execute %s: %s\n", sql, error);
		drawText(13, concat("Failed to execute: ", error), white);
		sqlite3_free(error);
		goto fail;
	}
	return;
fail:
	sqlite3_close(db);
}

void do_uri_mod(void) {
	char buf[256];
	int ret;

	sqlite3 *db;
	ret = sqlite3_open(APP_DB, &db);
	if (ret) {
		//printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		drawText(13, concat("Failed to open the database: ", sqlite3_errmsg(db)), white);
	}

	sql_simple_exec(db, "DELETE FROM tbl_uri WHERE titleId='TEMA00001'");
	sql_simple_exec(db, "INSERT INTO tbl_uri VALUES ('TEMA00001', '1', 'zip', NULL)");

	sqlite3_close(db);
	db = NULL;

	return;
}

int chkIfDirExistCommon(){
	 int e;
	    struct stat sb;
	    char *name = "ux0:customtheme/c/common_theme";

	    e = stat(name, &sb);
	    printf("e=%d  errno=%d\n",e,errno);
	    if (e == 0)
	        {
	        if (sb.st_mode & S_IFDIR)
	            printf("%s is a directory.\n",name);
	        if (sb.st_mode & S_IFREG)
	            printf("%s is a regular file.\n",name);
	        // etc.
	        }
	    else
	        {
	        printf("stat failed.\n");
	        if (errno = ENOENT)
	            {
	            printf("The directory does not exist. Creating new directory...\n");
	            // Add more flags to the mode if necessary.
	            e = sceIoMkdir("ux0:customtheme", 0777);
	            e = sceIoMkdir("ux0:customtheme/c", 0777);
	            e = sceIoMkdir(name, 0777);
	            if (e != 0)
	                {
	                printf("mkdir failed; errno=%d\n",errno);
	                }
	            else
	                {
	                printf("created the directory %s\n",name);
	                }
	            }
	        }
	    return 0;
}
int chkIfDirExistUnique(){
	 int e;
	    struct stat sb;
	    char *name = "ux0:customtheme/u";

	    e = stat(name, &sb);
	    printf("e=%d  errno=%d\n",e,errno);
	    if (e == 0)
	        {
	        if (sb.st_mode & S_IFDIR)
	            printf("%s is a directory.\n",name);
	        if (sb.st_mode & S_IFREG)
	            printf("%s is a regular file.\n",name);
	        // etc.
	        }
	    else
	        {
	        printf("stat failed.\n");
	        if (errno = ENOENT)
	            {
	            printf("The directory does not exist. Creating new directory...\n");
	            // Add more flags to the mode if necessary.
	            e = sceIoMkdir("ux0:customtheme", 0777);
	            e = sceIoMkdir("ux0:customtheme/u", 0777);
	            e = sceIoMkdir(name, 0777);
	            if (e != 0)
	                {
	                printf("mkdir failed; errno=%d\n",errno);
	                }
	            else
	                {
	                printf("created the directory %s\n",name);
	                }
	            }
	        }
	    return 0;
}

void insert_theme_to_db(char *queryinsert, char *querydelete) {
	char buf[256];
	int ret;


	sqlite3 *db;
	ret = sqlite3_open(APP_DB, &db);
	if (ret) {
		//printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		drawText(13, concat("Failed to open the database: ", sqlite3_errmsg(db)), white);
	}

	sql_simple_exec(db, querydelete);
	sql_simple_exec(db, queryinsert);
	sqlite3_close(db);
	db = NULL;
	drawText(17, "DONE!", white);
	return;
}

void delete_seltheme(char *themepath) {
#define commonThemepath "ux0:customtheme/c/common_theme"
	netInit(); //SI NO PONGO ESTOS DOS TIRRAR ERROR LA FUNCION DONWLOAD
					httpInit();
				//Elimino todos los temas menos el que esta en la direccion de common

			char *sqldeletequery[256];
							strcpy(sqldeletequery, "DELETE FROM tbl_theme WHERE id = '");
							strcat(sqldeletequery, themepath); //es el path donde se encuentra el tema a borrar
							strcat(sqldeletequery, "'"); //
				drawText(4,"Deleting theme from database...", white);
				//printf("\nDeleting theme from database...\n");
				delete_uniquetheme_db(sqldeletequery);
				//printf("\nDeleting theme folder...\n");
				drawText(6,"Deleting theme folder...", white);
				removePath(themepath, NULL); //borro la carpeta de temas unicos
				//sceIoMkdir(themepath, 0777);//creo de nuevo la carpeta de temas unicos
				drawText(8, "DONE!", white);
				//printf("\nDONE!\n");

	//printf("\nAuto exiting in 5 seconds...\n");
				drawText(10, "Auto exiting in 5 seconds...", white);
		sceKernelDelayThread(5 * 1000 * 1000);

		httpTerm();
		netTerm();
		finishSceAppUtil();

		sceKernelExitProcess(0);
}

void delete_uniquetheme_db(char *querydelete) {
	char buf[256];
	int ret;


	sqlite3 *db;
	ret = sqlite3_open(APP_DB, &db);
	if (ret) {
		//printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		drawText(13, concat("Failed to open the database: ", sqlite3_errmsg(db)), white);
	}

	sql_simple_exec(db, querydelete);

	sqlite3_close(db);
	db = NULL;

	return;
}

void delete_uniquetheme() {
	char buf[256];
		clearScreen();
#define commonThemepath "ux0:customtheme/c/common_theme"
	netInit(); //SI NO PONGO ESTOS DOS TIRRAR ERROR LA FUNCION DONWLOAD
					httpInit();
				//Elimino todos los temas menos el que esta en la direccion de common

			char *sqldeletequery[256];
							strcpy(sqldeletequery, "DELETE FROM tbl_theme WHERE id NOT IN ('");
							strcat(sqldeletequery, commonThemepath); //install_dir es el que no tiene el / al final
							strcat(sqldeletequery, "')"); //borro todos los temas que no sean el commontheme
				//printf("\nDeleting unique themes from database...\n");
				drawText(1, "Deleting Unique Themes From Database...", white);
				delete_uniquetheme_db(sqldeletequery);
				drawText(3, "Deleting unique themes from ux0:customtheme/u/...", white);
				//printf("\nDeleting unique themes from ux0:customtheme/u/...\n");
				removePath("ux0:customtheme/u", NULL); //borro la carpeta de temas unicos
				sceIoMkdir("ux0:customtheme/u", 0777);//creo de nuevo la carpeta de temas unicos
				//printf("\nDONE!\n");
				drawText(5, "DONE!", white);
	//printf("\nAuto exiting in 5 seconds...\n");
	drawText(7, "Auto exiting in 5 seconds...", white);
		sceKernelDelayThread(5 * 1000 * 1000);

		httpTerm();
		netTerm();
		finishSceAppUtil();

		sceKernelExitProcess(0);
}

void install_liveareacommontheme() {
	char buf[256];
		clearScreen();
	  char buffer [255];
	  int copy;
	  char patch[256];
	  char backup[256];
	//printf("\nBacking up database file (app.db)\n");
	drawText(1, "Backing up database file (app.db)", white);
	sprintf(patch, "ur0:shell/db/%s","app.db");
	sprintf(backup, "ur0:shell/db/%s_orig.db", "app");
	snprintf(buffer, 255, "Backing up database %s to %s...", patch, backup);
	drawText(3, buffer, white);
	//printf("\nBacking up from %s to %s...\n", patch, backup);
	copy = copyfile(patch, backup);//AQUI YA TENGO MI BACKUP LISTO
#define commonThemepath "ux0:customtheme/c/common_theme"
	chkIfDirExistCommon();
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
		snprintf(file_url, 512, "http://vstema.com/themes/%s", file_name);

		// download file
		char *file_path = malloc(512 * sizeof(char)); //Es la direccion en el vita donde se descargo el zip
		snprintf(file_path, 512, "ux0:ptmp/%s", file_name);

		//printf("\nDownloading %s...\n", file_name);
		drawText(5, concat("Downloading: ", file_name), white);
		download(file_url, file_path);


		char *install_dir = malloc(512 * sizeof(char));//Tiene que estar asi con el corchete o tira error
		char *install_dir2 = malloc(512 * sizeof(char));

							snprintf(install_dir, 512, "ux0:customtheme/c/%s",file_name); //le pongo el / al final del nombre de la carpeta para que ahi guarde los archivos descomprimidos

							     int length;
							     length = strlen(install_dir);//Get length of string
							     install_dir[length - 4] = '\0';//Hago null los ultimos 4 charectes que serian el ".zip" para crear un nombre de carpeta apropiado
							     snprintf(install_dir2, 512, "%s/",install_dir);

					//printf("\nInstalling File...\n");
					drawText(7, "Installing Theme...", white);
					installCommonTheme(file_path,"ux0:customtheme/c/common_theme/");//Aqui descomprimo el zip en una carpeta para usarlo
					//printf("\nDeleting temp files...\n");
					drawText(13, "Deleting temp files...", white);
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
						//printf("\nInserting theme path to database...\n");
						drawText(15, "Inserting theme path to database...", white);
			insert_theme_to_db(sqlinsertquery, sqldeletequery);
			//printf("\nAuto exiting in 5 seconds...\n");
			drawText(19, "Auto exiting in 5 seconds...", white);
				sceKernelDelayThread(5 * 1000 * 1000);

				httpTerm();
				netTerm();
				finishSceAppUtil();

				sceKernelExitProcess(0);
}

void install_liveareatheme() {
	char buf[256];
		clearScreen();
	  char buffer [255];
	  int copy;
	  char patch[256];
	  char backup[256];
	//printf("\nBacking up database file (app.db)\n");
	drawText(1, "Backing up database file (app.db)", white);
	sprintf(patch, "ur0:shell/db/%s","app.db");
	sprintf(backup, "ur0:shell/db/%s_orig.db", "app");
	snprintf(buffer, 255, "Backing up database %s to %s...", patch, backup);
	drawText(3, buffer, white);
	//printf("\nBacking up from %s to %s...\n", patch, backup);
	copy = copyfile(patch, backup);//AQUI YA TENGO MI BACKUP LISTO
	chkIfDirExistUnique();
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
		snprintf(file_url, 512, "http://vstema.com/themes/%s", file_name);

		// download file

		char *file_path = malloc(512 * sizeof(char)); //Es la direccion en el vita donde se descargo el zip
		snprintf(file_path, 512, "ux0:ptmp/%s", file_name);

		//printf("\nDownloading %s...\n", file_name);
		drawText(5, concat("Downloading: ", file_name), white);
		download(file_url, file_path);



		char *install_dir = malloc(512 * sizeof(char));//Tiene que estar asi con el corchete o tira error
		char *install_dir2 = malloc(512 * sizeof(char));

							snprintf(install_dir, 512, "ux0:customtheme/u/%s",file_name); //le pongo el / al final del nombre de la carpeta para que ahi guarde los archivos descomprimidos

							     int length;
							     length = strlen(install_dir);//Get length of string
							     install_dir[length - 4] = '\0';//Hago null los ultimos 4 charectes que serian el ".zip" para crear un nombre de carpeta apropiado
							     snprintf(install_dir2, 512, "%s/",install_dir);

					//printf("\nInstalling File...\n");
					drawText(7, "Installing Theme...", white);
					installTheme(file_path,install_dir2);//Aqui descomprimo el zip en una carpeta para usarlo
					//printf("\nDeleting temp files...\n");
					drawText(13, "Deleting temp files...", white);
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
						drawText(15, "Inserting theme path to database...", white);
						//printf("\nInserting theme path to database...\n");
			insert_theme_to_db(sqlinsertquery, sqldeletequery);

			drawText(19, "Auto exiting in 5 seconds...", white);
			//printf("\nAuto exiting in 5 seconds...\n");

				sceKernelDelayThread(5 * 1000 * 1000);

				httpTerm();
				netTerm();
				finishSceAppUtil();

				sceKernelExitProcess(0);
}

void install_vitashelltheme() {
	char buf[256];
	clearScreen();
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
	snprintf(file_url, 512, "http://vstema.com/themes/%s", file_name);

	// download file
	//printf("Downloading %s...\n", file_url);
	char *file_path = malloc(512 * sizeof(char)); //Es la direccion en el vita donde se descargo el zip
	snprintf(file_path, 512, "ux0:ptmp/%s", file_name);

	//printf("\nDownloading %s...\n", file_name);
	drawText(1, concat("Downloading: ", file_name), white);
	download(file_url, file_path);


	char *install_dir = malloc(512 * sizeof(char));//Tiene que estar asi con el corchete o tira error
	char *install_dir2 = malloc(512 * sizeof(char));
	char * theme_name = malloc(512 * sizeof(char));
	//file_name = strrchr(tempAppParan, '/')+1;
		//				printf("\nFile name: %s...", AppParam); //Muestro el nombre del archivo
	//					sceKernelDelayThread(5* 1000*1000);
						snprintf(install_dir, 512, "ux0:VitaShell/theme/%s",file_name); //le pongo el / al final del nombre de la carpeta para que ahi guarde los archivos descomprimidos
						     int length;
						     length = strlen(install_dir);//Get length of string
						     install_dir[length - 4] = '\0';//Hago null los ultimos 4 charectes que serian el ".zip" para crear un nombre de carpeta apropiado
						     snprintf(install_dir2, 512, "%s/",install_dir);

											//sceKernelDelayThread(5* 1000*1000);

				//printf("\nInstalling File...\n");
				drawText(3, "Installing File...", white);
				installTheme(file_path,install_dir2);//Aqui descomprimo el zip en una carpeta para usarlo
				drawText(5, "Deleting temp files...", white);
				//printf("\nDeleting temp files...\n");
				sceIoRemove(file_path); //Borro el zip de la carpeta ptmp

				//Editanto THEME.txt
								FILE *fp; // creates a pointer to a file
								char var;
								fp = fopen("ux0:VitaShell/theme/theme.txt","w+"); //opens file with read-write permissions
								if(fp!=NULL) {
								    //fscanf(fp,"%d",&var);      //read contents of file
								}
								length = strlen(file_name);
								file_name[length - 4] = '\0';
								snprintf(theme_name, 512, "%s/",file_name);
								fprintf(fp,"THEME_NAME =\"%s\"",theme_name);         //write contents to file
								fclose(fp); //close the file after you are done

				//sceKernelDelayThread(5* 1000*1000);
				drawText(7, "Auto exiting in 5 seconds...", white);
				//printf("\nAuto exiting in 5 seconds...\n");

					sceKernelDelayThread(5 * 1000 * 1000);

					httpTerm();
					netTerm();
					finishSceAppUtil();

					sceKernelExitProcess(0);
}




const char *ICON_CIRCLE = "\xe2\x97\x8b";
const char *ICON_CROSS = "\xe2\x95\xb3";
const char *ICON_SQUARE = "\xe2\x96\xa1";
const char *ICON_TRIANGLE = "\xe2\x96\xb3";
const char *ICON_UPDOWN = "\xe2\x86\x95";

int enter_button = 0;
int SCE_CTRL_ENTER;
int SCE_CTRL_CANCEL;
char ICON_ENTER[4];
char ICON_CANCEL[4];

void drawText(uint32_t y, char* text, uint32_t color){
    int i;
    for (i=0;i<3;i++){
        vita2d_start_drawing();
        vita2d_pgf_draw_text(debug_font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text);
        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

void drawLoopText(uint32_t y, char *text, uint32_t color) {
    vita2d_pgf_draw_text(debug_font, 2, (y + 1) * ROW_HEIGHT, color, 1.0, text);
}

void clearScreen(){
    int i;
    for (i=0;i<3;i++){
        vita2d_start_drawing();
        vita2d_clear_screen();
        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

enum {
    INJECTOR_MAIN = 1,
    INJECTOR_TITLE_SELECT,
    INJECTOR_BACKUP_PATCH,
    INJECTOR_START_DUMPER,
    INJECTOR_RESTORE_PATCH,
    INJECTOR_EXIT,
};

int readBtn() {
    SceCtrlData pad = {0};
    static int old;
    int btn;

    sceCtrlPeekBufferPositive(0, &pad, 1);

    btn = pad.buttons & ~old;
    old = pad.buttons;
    return btn;
}

void print_game_list(appinfo *head, appinfo *tail, appinfo *curr) {
    appinfo *tmp = head;
    int i = 2;
    char buf[256];
    while (tmp) {
        snprintf(buf, 256, "%s", tmp->title_id); // Aqui es donde muestro el nombre de los archivos a borrar en el menu
        drawLoopText(i, buf, curr == tmp ? green : white);
        if (tmp == tail) {
            break;
        }
        tmp = tmp->next;
        i++;
    }
}


int injector_main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));

    int btn;
    char buf[256];
    char version_string[256];
    snprintf(version_string, 256, "LIVE AREA THEMES DELETION MENU");

    applist list = {0};

    int ret = get_applist(&list);
    if (ret < 0) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        snprintf(buf, 256, "Initialization error, %x", ret);
        drawText(0, buf, red);

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();

        while (readBtn());
        return -1;
    }
    appinfo *head, *tail, *curr;
    curr = head = tail = list.items;

    int i = 0;
    while (tail->next) {
        i++;
        if (i == PAGE_ITEM_COUNT) {
            break;
        }
        tail = tail->next;
    }

    int state = INJECTOR_MAIN;

    //cleanup_prev_inject(&list);

    while (1) {
        vita2d_start_drawing();
        vita2d_clear_screen();
        switch (state) {
            case INJECTOR_MAIN:
                drawLoopText(0, version_string, white);

                print_game_list(head, tail, curr);

                drawLoopText(24, concat(ICON_UPDOWN, " - Select Item"), white);
                drawLoopText(25, concat(ICON_ENTER, " - Delete Theme"), white);
                drawLoopText(26, concat(ICON_CANCEL, " - Exit"), white);

                btn = readBtn();
                if (btn & SCE_CTRL_ENTER) {
                    state = INJECTOR_TITLE_SELECT;
                    break;
                }
                if (btn & SCE_CTRL_CANCEL) {
                    state = INJECTOR_EXIT;
                    break;
                }
                if ((btn & SCE_CTRL_UP) && curr->prev) {
                    if (curr == head) {
                        head = head->prev;
                        tail = tail->prev;
                    }
                    curr = curr->prev;
                    break;
                }
                if ((btn & SCE_CTRL_DOWN) && curr->next) {
                    if (curr == tail) {
                        tail = tail->next;
                        head = head->next;
                    }
                    curr = curr->next;
                    break;
                }
                break;
            case INJECTOR_TITLE_SELECT:
            	//vita2d_start_drawing();
            	clearScreen();
            	vita2d_start_drawing();
            	drawText(0, version_string, white);
            	                snprintf(buf, 255, "Deleting theme: %s", curr->title_id);
            	                drawText(2, buf, white);
            	                //sceKernelDelayThread(5 * 1000 * 1000);
            	                delete_seltheme(curr->title_id);
                break;
            case INJECTOR_EXIT:
                vita2d_end_drawing();
                vita2d_wait_rendering_done();
                vita2d_swap_buffers();
                return 0;
        }

        vita2d_end_drawing();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}









int main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));

    debug_font = load_system_fonts();

	char buf[256];
	int key = 0; //Se usa para saber que boton presione y hacer la accion correspondiente
	again:
	clearScreen();
	//Inicializo screen
	psvDebugScreenInit();
	// clears screen with a given color
	psvDebugScreenClear(psvDebugScreenColorBg);
	initSceAppUtil();

		sceAppMgrGetAppParam(AppParam);
		int arg_len = strlen(AppParam);

	//printf("Welcome to TEMA this application lets you download a theme\n");
	//printf("for VitaShell or Live Area and install it for its use.\n");
	//printf("PLEASE BE CAREFUL NOT TO INSTALL A VITASHELL THEME\n");
	//printf("AS A LIVE AREA ONE.\n");
	//printf("\n\n");
    drawText(1, "Welcome to TEMA this application lets you download a theme\n", white);
    drawText(2, "for VitaShell or Live Area and install it for its use.", white);
    drawText(4, "PLEASE BE CAREFUL NOT TO INSTALL A VITASHELL THEME", white);
    drawText(5, "AS A LIVE AREA ONE.", white);
	if (arg_len == 0) {

		drawText(8, concat(ICON_CROSS, " - Install a Vita Shell Theme"), white);
		drawText(9, concat(ICON_CIRCLE, " - Install a Live Area Theme"), white);
		drawText(10, concat(ICON_TRIANGLE, " - Go to Live Area Themes Delete Menu"), white);
		drawText(11, concat(ICON_SQUARE, " - Delete ALL Unique Live Area Themes"), white);

		//printf("Press X to install a VitaShell Theme.\n");
		//printf("Press O to install a Live Area Theme.\n");
		//printf(concat(ICON_TRIANGLE, " - Delete Theme"));
		//printf("Press SQUARE to delete ALL unique Live Area Themes.\n");
		//printf("\n");

		key = get_key();
		switch (key) {
		case SCE_CTRL_TRIANGLE:

			    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button);
			    if (enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE) {
			        SCE_CTRL_ENTER = SCE_CTRL_CIRCLE;
			        SCE_CTRL_CANCEL = SCE_CTRL_CROSS;
			        strcpy(ICON_ENTER, ICON_CIRCLE);
			        strcpy(ICON_CANCEL, ICON_CROSS);
			    } else {
			        SCE_CTRL_ENTER = SCE_CTRL_CROSS;
			        SCE_CTRL_CANCEL = SCE_CTRL_CIRCLE;
			        strcpy(ICON_ENTER, ICON_CROSS);
			        strcpy(ICON_CANCEL, ICON_CIRCLE);
			    }
			injector_main();
			sceKernelExitProcess(0);
					break;
		case SCE_CTRL_CROSS:
			//clearScreen();
				do_uri_mod();
				//printf("This should now open the browser.\nFrom there you can click a link and it will return here.\n");
				drawText(13, "This should now open the browser.\nFrom there you can click a link and it will return here.", white);
				sceKernelDelayThread(3 * 1000 * 1000); // 3 seconds
				sceAppMgrLaunchAppByUri(0xFFFFF, "http://vstema.com/vitashell");
				sceKernelDelayThread(10000);
				sceAppMgrLaunchAppByUri(0xFFFFF, "http://vstema.com/vitashell");
				sceKernelExitProcess(0);


			install_vitashelltheme();
			break;
		case SCE_CTRL_CIRCLE:
			do_uri_mod();
						//printf("This should now open the browser.\nFrom there you can click a link and it will return here.\n");
			            drawText(13, "This should now open the browser.\nFrom there you can click a link and it will return here.", white);
						sceKernelDelayThread(3 * 1000 * 1000); // 3 seconds
						sceAppMgrLaunchAppByUri(0xFFFFF, "http://vstema.com/livearea");
						sceKernelDelayThread(10000);
						sceAppMgrLaunchAppByUri(0xFFFFF, "http://vstema.com/livearea");
						sceKernelExitProcess(0);
			install_liveareatheme();
			break;
		case SCE_CTRL_SQUARE:
			//Doble confirmacion para extra seguridad

			//psvDebugScreenClear(psvDebugScreenColorBg);
			//printf("\nAre you sure you want to delete ALL unique Live Area Themes?\n");
			//printf("Press SELECT to confirm or START to cancel.\n");
			drawText(13, "Are you sure you want to delete ALL unique Live Area Themes?", white);
			drawText(15, "Press SELECT to confirm or START to cancel.", white);
			againDelete:
			key = get_key();
					switch (key) {
					case SCE_CTRL_SELECT:
						delete_uniquetheme();
						break;
					case SCE_CTRL_START:
						clearScreen();
						//printf("\nUnique Live Area Themes not deleted.\n");
						//printf("\nAuto exiting in 5 seconds...\n");
						drawText(2, "Unique Live Area Themes not deleted.", white);
						drawText(4, "Auto exiting in 5 seconds...", white);
						sceKernelDelayThread(5 * 1000 * 1000);

						finishSceAppUtil();

						sceKernelExitProcess(0);
						break;
					default:
						//printf("\nInvalid input, try again.\n\n");
						drawText(16, "Invalid input, try again.", white);
						goto againDelete;
					}

					break;
		default:
			//printf("Invalid input, try again.\n\n");
			drawText(16, "Invalid input, try again.", white);
			goto again;
		}

		}

		char *menusel;
		char *menusel2;
		char *menusel3 = malloc(512 * sizeof(char));
		menusel = AppParam;
        menusel2 = strrchr(menusel, ':')+1;

        strncpy(menusel3, menusel2,1);

		//sceKernelDelayThread(5* 1000*1000);
		if (strcmp(menusel3,"s")==0) {
			install_vitashelltheme();
		}
		else if (strcmp(menusel3,"a")==0) {

			//printf("\nPress SELECT to install the Live Area Theme as Unique.\n");
			//printf("\nPress START to install the Live Area Theme as Common.\n\n");
			drawText(8, "Press SELECT to install the Live Area Theme as Unique.", white);
			drawText(9, "Press START to install the Live Area Theme as Common.", white);
			again2:
			key = get_key();
					switch (key) {
					case SCE_CTRL_SELECT:
						install_liveareatheme();

						break;
					case SCE_CTRL_START:
						install_liveareacommontheme();

					break;
					default:
						//printf("Invalid input, try again.\n\n");
						drawText(14, "Invalid input, try again.", white);
									goto again2;
							}
		}


}



