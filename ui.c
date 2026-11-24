
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*
 * THE LAST SURVIVOR X360
 * Interfaz 2D: estructura inicial del menu.
 * El dibujo en pantalla se conectara desde main.c.
 */

typedef enum {
    MENU_NEW_GAME,
    MENU_CONTINUE,
    MENU_LOAD,
    MENU_SETTINGS,
    MENU_EXIT,
    MENU_COUNT
} MenuOption;

static const char *menu_items[MENU_COUNT] = {
    "NUEVA PARTIDA",
    "CONTINUAR",
    "CARGAR PARTIDA",
    "AJUSTES",
    "SALIR DEL JUEGO"
};

static MenuOption selected_option = MENU_NEW_GAME;

void ui_menu_move(int direction)
{
    int option = (int)selected_option + direction;

    if (option < 0)
        option = MENU_COUNT - 1;

    if (option >= MENU_COUNT)
        option = 0;

    selected_option = (MenuOption)option;
}

int ui_menu_selection(void)
{
    return (int)selected_option;
}

const char *ui_menu_text(int option)
{
    if (option < 0 || option >= MENU_COUNT)
        return "";

    return menu_items[option];
}
