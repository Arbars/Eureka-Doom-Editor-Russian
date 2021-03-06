//------------------------------------------------------------------------
//  MENUS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "main.h"
#include "ui_window.h"

#include "e_main.h"
#include "m_files.h"
#include "m_loadsave.h"


//------------------------------------------------------------------------
//  FILE MENU
//------------------------------------------------------------------------

static void file_do_new_project(Fl_Widget *w, void * data)
{
	ExecuteCommand("NewProject");
}

static void file_do_manage_project(Fl_Widget *w, void * data)
{
	ExecuteCommand("ManageProject");
}

static void file_do_open(Fl_Widget *w, void * data)
{
	ExecuteCommand("OpenMap");
}

static void file_do_save(Fl_Widget *w, void * data)
{
	ExecuteCommand("SaveMap");
}

static void file_do_export(Fl_Widget *w, void * data)
{
	ExecuteCommand("ExportMap");
}

static void file_do_fresh_map(Fl_Widget *w, void * data)
{
	ExecuteCommand("FreshMap");
}

static void file_do_copy_map(Fl_Widget *w, void * data)
{
	ExecuteCommand("CopyMap");
}

static void file_do_rename(Fl_Widget *w, void * data)
{
	ExecuteCommand("RenameMap");
}

static void file_do_delete(Fl_Widget *w, void * data)
{
	ExecuteCommand("DeleteMap");
}

static void file_do_prefs(Fl_Widget *w, void * data)
{
	ExecuteCommand("PreferenceDialog");
}

static void file_do_build_nodes(Fl_Widget *w, void * data)
{
	ExecuteCommand("BuildAllNodes");
}

static void file_do_test_map(Fl_Widget *w, void * data)
{
	ExecuteCommand("TestMap");
}

static void file_do_load_given(Fl_Widget *w, void *data)
{
	const char *filename = (const char *) data;

	int given_idx = M_FindGivenFile(filename);

	if (given_idx >= 0)
		last_given_file = given_idx;

	OpenFileMap(filename);
}

static void file_do_load_recent(Fl_Widget *w, void *data)
{
	M_OpenRecentFromMenu(data);
}

static void file_do_quit(Fl_Widget *w, void * data)
{
	ExecuteCommand("Quit");
}


//------------------------------------------------------------------------
//  EDIT MENU
//------------------------------------------------------------------------

static void edit_do_undo(Fl_Widget *w, void * data)
{
	ExecuteCommand("Undo");
}

static void edit_do_redo(Fl_Widget *w, void * data)
{
	ExecuteCommand("Redo");
}

static void edit_do_cut(Fl_Widget *w, void * data)
{
	ExecuteCommand("Clipboard_Cut");
}

static void edit_do_copy(Fl_Widget *w, void * data)
{
	ExecuteCommand("Clipboard_Copy");
}

static void edit_do_paste(Fl_Widget *w, void * data)
{
	ExecuteCommand("Clipboard_Paste");
}

static void edit_do_delete(Fl_Widget *w, void * data)
{
	ExecuteCommand("Delete");
}

static void edit_do_select_all(Fl_Widget *w, void * data)
{
	ExecuteCommand("SelectAll");
}

static void edit_do_unselect_all(Fl_Widget *w, void * data)
{
	ExecuteCommand("UnselectAll");
}

static void edit_do_invert_sel(Fl_Widget *w, void * data)
{
	ExecuteCommand("InvertSelection");
}

static void edit_do_last_sel(Fl_Widget *w, void * data)
{
	ExecuteCommand("LastSelection");
}

static void edit_do_op_menu(Fl_Widget *w, void * data)
{
	ExecuteCommand("OperationMenu");
}

static void edit_do_move(Fl_Widget *w, void * data)
{
	ExecuteCommand("MoveObjectsDialog");
}

static void edit_do_scale(Fl_Widget *w, void * data)
{
	ExecuteCommand("ScaleObjectsDialog");
}

static void edit_do_rotate(Fl_Widget *w, void * data)
{
	ExecuteCommand("RotateObjectsDialog");
}

static void edit_do_mirror_horiz(Fl_Widget *w, void * data)
{
	ExecuteCommand("Mirror", "horiz");
}

static void edit_do_mirror_vert(Fl_Widget *w, void * data)
{
	ExecuteCommand("Mirror", "vert");
}


//------------------------------------------------------------------------
//  VIEW MENU
//------------------------------------------------------------------------

static void view_do_zoom_in(Fl_Widget *w, void * data)
{
	ExecuteCommand("Zoom", "+1", "/center");
}

static void view_do_zoom_out(Fl_Widget *w, void * data)
{
	ExecuteCommand("Zoom", "-1", "/center");
}

static void view_do_whole_map(Fl_Widget *w, void * data)
{
	ExecuteCommand("ZoomWholeMap");
}

static void view_do_whole_selection(Fl_Widget *w, void * data)
{
	ExecuteCommand("ZoomSelection");
}

static void view_do_camera_pos(Fl_Widget *w, void * data)
{
	ExecuteCommand("GoToCamera");
}

static void view_do_toggle_3d(Fl_Widget *w, void * data)
{
	ExecuteCommand("Toggle", "3d");
}

static void view_do_object_nums(Fl_Widget *w, void * data)
{
	ExecuteCommand("Toggle", "obj_nums");
}

static void view_do_sprites(Fl_Widget *w, void * data)
{
	ExecuteCommand("Toggle", "sprites");
}

static void view_do_gamma(Fl_Widget *w, void * data)
{
	ExecuteCommand("Toggle", "gamma");
}

static void view_do_sector_render(Fl_Widget *w, void * data)
{
	const Fl_Menu_Item *item = ((Fl_Menu_*)w)->mvalue();

	if (strcmp(item->text, "Floors") == 0)
		edit.sector_render_mode = SREND_Floor;
	else if (strcmp(item->text, "Ceilings") == 0)
		edit.sector_render_mode = SREND_Ceiling;
	else if (strcmp(item->text, "Lighting") == 0)
		edit.sector_render_mode = SREND_Lighting;
	else if (strcmp(item->text, "Sound") == 0)
		edit.sector_render_mode = SREND_SoundProp;
	else
		edit.sector_render_mode = SREND_Nothing;

	main_win->redraw();
}

static void view_do_default_props(Fl_Widget *w, void * data)
{
	ExecuteCommand("DefaultProps");
}

static void view_do_find(Fl_Widget *w, void * data)
{
	ExecuteCommand("FindDialog");
}

static void view_do_next(Fl_Widget *w, void * data)
{
	ExecuteCommand("FindNext");
}

static void view_do_jump(Fl_Widget *w, void * data)
{
	ExecuteCommand("JumpToObject");
}


//------------------------------------------------------------------------
//  BROWSER MENU
//------------------------------------------------------------------------

static void browser_do_textures(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "T");
}

static void browser_do_flats(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "F");
}

static void browser_do_things(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "O");
}

static void browser_do_lines(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "L");
}

static void browser_do_sectors(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "S");
}

static void browser_do_gen_types(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "G");
}

static void browser_do_recent_tex(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "T", "/recent");
}

static void browser_do_recent_flats(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "F", "/recent");
}

static void browser_do_recent_things(Fl_Widget *w, void * data)
{
	ExecuteCommand("BrowserMode", "O", "/recent");
}

static void browser_hide(Fl_Widget *w, void * data)
{
	ExecuteCommand("Set", "browser", "0");
}


//------------------------------------------------------------------------
//  CHECK MENU
//------------------------------------------------------------------------

static void checks_do_all(Fl_Widget *w, void * data)
{
	ExecuteCommand("MapCheck", "all");
}

static void checks_do_major(Fl_Widget *w, void * data)
{
	ExecuteCommand("MapCheck", "major");
}

static void checks_do_vertices(Fl_Widget *w, void * data)
{
	ExecuteCommand("MapCheck", "vertices");
}

static void checks_do_sectors(Fl_Widget *w, void * data)
{
	ExecuteCommand("MapCheck", "sectors");
}

static void checks_do_linedefs(Fl_Widget *w, void * data)
{
	ExecuteCommand("MapCheck", "linedefs");
}

static void checks_do_things(Fl_Widget *w, void * data)
{
	ExecuteCommand("MapCheck", "things");
}

static void checks_do_textures(Fl_Widget *w, void * data)
{
	ExecuteCommand("MapCheck", "textures");
}

static void checks_do_tags(Fl_Widget *w, void * data)
{
	ExecuteCommand("MapCheck", "tags");
}


//------------------------------------------------------------------------
//  HELP MENU
//------------------------------------------------------------------------

static void help_do_logs(Fl_Widget *w, void * data)
{
	ExecuteCommand("LogViewer");
}

static void help_do_online_docs(Fl_Widget *w, void * data)
{
	ExecuteCommand("Documentation");
}

static void help_do_about(Fl_Widget *w, void * data)
{
	ExecuteCommand("AboutDialog");
}


//------------------------------------------------------------------------

#define M_GIVEN_FILES	"&Полученные файлы"
#define M_RECENT_FILES	"&Последние файлы"


#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] =
{
	{ "&Файл", 0, 0, 0, FL_SUBMENU },

		{ "&Новый проект   ",   FL_COMMAND + 'n', FCAL file_do_new_project },
		{ "&Настроить проект  ", FL_COMMAND + 'm', FCAL file_do_manage_project },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Открыть карту",  FL_COMMAND + 'o', FCAL file_do_open },
		{ M_GIVEN_FILES, 0, 0, 0, FL_SUBMENU|FL_MENU_INACTIVE },
			{ 0 },
		{ M_RECENT_FILES, 0, 0, 0, FL_SUBMENU|FL_MENU_INACTIVE },
			{ 0 },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Сохранить карту",    FL_COMMAND + 's', FCAL file_do_save },
		{ "&Экспортировать карту",  FL_COMMAND + 'e', FCAL file_do_export },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Обновить карту",    0, FCAL file_do_fresh_map },
		{ "&Скопировать карту",    0,  FCAL file_do_copy_map },
		{ "Переименовать карту",   0,  FCAL file_do_rename },
		{ "Удалить карту",   0,  FCAL file_do_delete },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Протестировать",     FL_COMMAND + 't', FCAL file_do_test_map },
		{ "&Создать все ноды",  FL_COMMAND + 'b', FCAL file_do_build_nodes },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Настройки",      FL_COMMAND + 'p', FCAL file_do_prefs },
		{ "&Выйти",             FL_COMMAND + 'q', FCAL file_do_quit },
		{ 0 },

	{ "&Изменить", 0, 0, 0, FL_SUBMENU },

		{ "&Отменить",   FL_COMMAND + 'z',  FCAL edit_do_undo },
		{ "&Вернуть",   FL_COMMAND + 'y',  FCAL edit_do_redo },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Вы&резать",     FL_COMMAND + 'x', FCAL edit_do_cut },
		{ "&Скопировать",    FL_COMMAND + 'c', FCAL edit_do_copy },
		{ "&Вставить",   FL_COMMAND + 'v', FCAL edit_do_paste },
		{ "&Удалить",  FL_Delete,        FCAL edit_do_delete },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Выделить &Всё",       FL_COMMAND + 'a', FCAL edit_do_select_all },
		{ "Отменить выделение",      FL_COMMAND + 'u', FCAL edit_do_unselect_all },
		{ "&Отзеркалить выделение", FL_COMMAND + 'i', FCAL edit_do_invert_sel },
		{ "&Последнее выделение",   FL_COMMAND + 'l', FCAL edit_do_last_sel },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Меню действий",   FL_F+1, FCAL edit_do_op_menu },
		{ "&Сдвинуть объект...",  FL_F+2, FCAL edit_do_move },
		{ "&Изменить размер...", FL_F+3, FCAL edit_do_scale },
		{ "Повернуть объект...", FL_F+4, FCAL edit_do_rotate },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Отразить &Горизонтально",  0, FCAL edit_do_mirror_horiz },
		{ "Отразить &Вертикально",    0, FCAL edit_do_mirror_vert },
		{ 0 },

	{ "&Вид", 0, 0, 0, FL_SUBMENU },

		// Note: FL_Tab cannot be used as a shortcut here, as it
		//       invokes FLTK's hard-coded navigation stuff.

		{ "Сменить &3D вид",           0, FCAL view_do_toggle_3d },
		{ "Сменить С&прайты",     FL_F+10, FCAL view_do_sprites },
		{ "Сменить &Гамму",       FL_F+11, FCAL view_do_gamma },
		{ "Сменить номера объектов",  FL_F+12, FCAL view_do_object_nums },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Отрисовка секторов", 0, 0, 0, FL_SUBMENU },
			{ "Nothing",   0, FCAL view_do_sector_render },
			{ "Потолок",    0, FCAL view_do_sector_render },
			{ "Пол",  0, FCAL view_do_sector_render },
			{ "Освещение",  0, FCAL view_do_sector_render },
			{ "Звук",     0, FCAL view_do_sector_render },
			{ 0 },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Приблизить",      0, FCAL view_do_zoom_in },
		{ "Отдалить",     0, FCAL view_do_zoom_out },
		{ "Общий вид карты",    0, FCAL view_do_whole_map },
		{ "Общий вид выделенного", 0, FCAL view_do_whole_selection },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Исходные настройки  ",  FL_COMMAND + 'd', FCAL view_do_default_props },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Найти и заменить",   FL_COMMAND + 'f', FCAL view_do_find },
		{ "Искать далее",        FL_COMMAND + 'g', FCAL view_do_next },
		{ "Вернуться к камере",     0, FCAL view_do_camera_pos },
		{ "&Перейти к объекту",   0, FCAL view_do_jump },
		{ 0 },

	{ "&Каталог", 0, 0, 0, FL_SUBMENU },

		{ "&Текстуры",     FL_F+5, FCAL browser_do_textures },
		{ "&Плоскости",        FL_F+6, FCAL browser_do_flats },
		{ "Объекты",       FL_F+7, FCAL browser_do_things },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Триггеры линий",     0, FCAL browser_do_lines },
		{ "&Типы секторов",      0, FCAL browser_do_sectors },
		{ "&Общие типы", 0, FCAL browser_do_gen_types },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Последние текстуры", 0, FCAL browser_do_recent_tex },
		{ "Последние плоскости",     0, FCAL browser_do_recent_flats },
		{ "Последние объекты",    0, FCAL browser_do_recent_things },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Скрыть",            0, FCAL browser_hide },
		{ 0 },

	{ "&Проверить", 0, 0, 0, FL_SUBMENU },

		{ "&ВСЁ",           FL_F+9, FCAL checks_do_all },
		{ "&Основное  ",      0, FCAL checks_do_major },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Вершины",     0, FCAL checks_do_vertices },
		{ "&Сектора",      0, FCAL checks_do_sectors },
		{ "&Линии",     0, FCAL checks_do_linedefs },
		{ "&Объекты",       0, FCAL checks_do_things },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Текстуры",     0, FCAL checks_do_textures },
		{ "Теги",         0, FCAL checks_do_tags },
		{ 0 },

	{ "&Помощь", 0, 0, 0, FL_SUBMENU },
		{ "&Онлайн документация...",    0, FCAL help_do_online_docs },
		{ "&Просмотр логов....",     0, FCAL help_do_logs },
		{ "&О программе...",   0, FCAL help_do_about },
		{ 0 },

	{ 0 }
};


//------------------------------------------------------------------------


#define MAX_PWAD_LIST  20

//
// allow the hard-coded menu shortcuts to be bind to other functions
// by the user, hence we remove those shortcuts when this happens. 
//
static void Menu_RemovedBoundKeys(Fl_Menu_Item *items)
{
	int total = items[0].size();  // includes {0} at end

	for (int i = 0 ; i < total ; i++)
	{
		if (! items[i].text)
			continue;

		int shortcut = items[i].shortcut_;
		if (! shortcut)
			continue;

		// convert to a Eureka key code
		keycode_t key = shortcut & FL_KEY_MASK;

			 if (shortcut & FL_COMMAND) key |= MOD_COMMAND;
		else if (shortcut & FL_META)    key |= MOD_META;
		else if (shortcut & FL_ALT)     key |= MOD_ALT;

		if (M_IsKeyBound(key, KCTX_General))
			items[i].shortcut_ = 0;
	}
}


static int Menu_FindItem(const Fl_Menu_Item *items, const char *text)
{
	int total = items[0].size();  // includes {0} at end

	for (int i = 0 ; i < total ; i++)
		if (items[i].text && y_stricmp(items[i].text, text) == 0)
			return i;

	return -1;  // not found
}


static void Menu_CopyItem(Fl_Menu_Item* &pos, const Fl_Menu_Item &orig)
{
	memcpy(pos, &orig, sizeof(orig));

	pos++;
}


static void Menu_AddItem(Fl_Menu_Item* &pos, const char *text,
						 Fl_Callback *cb, void *data, int flags)
{
	Fl_Menu_Item item;

	memset(&item, 0, sizeof(item));

	item.text = text;
	item.flags = flags;
	item.callback_ = cb;
	item.user_data_ = data;

	Menu_CopyItem(pos, item);
}


static Fl_Menu_Item * Menu_PopulateGivenFiles(Fl_Menu_Item *items)
{
	int count = (int)Pwad_list.size();

	if (count < 2)
		return items;

	// silently ignore excess pwads
	if (count > MAX_PWAD_LIST)
		count = MAX_PWAD_LIST;


	// find Given Files sub-menu and activate it
	int menu_pos = Menu_FindItem(items, M_GIVEN_FILES);

	if (menu_pos < 0)  // [should not happen]
		return items;

	items[menu_pos++].activate();


	// create new array
	int total = items[0].size();  // includes {0} at end

	Fl_Menu_Item * new_array = new Fl_Menu_Item[total + count];
	Fl_Menu_Item * pos = new_array;

	for (int i = 0 ; i < menu_pos ; i++)
		Menu_CopyItem(pos, items[i]);

	for (int k = 0 ; k < count ; k++)
	{
		const char *short_name = fl_filename_name(Pwad_list[k]);

		short_name = StringPrintf("%s%s%d:  %s", (k < 9) ? "  " : "",
								  (k < 9) ? "&" : "", 1+k, short_name);

		Menu_AddItem(pos, short_name,
					 FCAL file_do_load_given,
					 (void *) Pwad_list[k], 0);
	}

	for ( ; menu_pos < total ; menu_pos++)
		Menu_CopyItem(pos, items[menu_pos]);

	return new_array;
}


static Fl_Menu_Item * Menu_PopulateRecentFiles(Fl_Menu_Item *items, Fl_Callback *cb)
{
	int count = M_RecentCount();

	if (count < 1)
		return items;


	// find Recent Files sub-menu and activate it
	int menu_pos = Menu_FindItem(items, M_RECENT_FILES);

	if (menu_pos < 0)  // [should not happen]
		return items;

	items[menu_pos++].activate();


	// create new array
	int total = items[0].size();  // includes {0} at end

	Fl_Menu_Item * new_array = new Fl_Menu_Item[total + count];
	Fl_Menu_Item * pos = new_array;

	for (int i = 0 ; i < menu_pos ; i++)
		Menu_CopyItem(pos, items[i]);

	for (int k = 0 ; k < count ; k++)
	{
		char name_buf[256];

		M_RecentShortName(k, name_buf);

		void *data = M_RecentData(k);

		Menu_AddItem(pos, StringDup(name_buf), cb, data, 0);
	}

	for ( ; menu_pos < total ; menu_pos++)
		Menu_CopyItem(pos, items[menu_pos]);

	return new_array;
}


Fl_Sys_Menu_Bar * Menu_Create(int x, int y, int w, int h)
{
	Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);

#ifndef __APPLE__
	bar->textsize(KF_fonth);
#endif

	Fl_Menu_Item *items = menu_items;

	Menu_RemovedBoundKeys(items);

	items = Menu_PopulateGivenFiles(items);
	items = Menu_PopulateRecentFiles(items, FCAL file_do_load_recent);

	bar->menu(items);

	return bar;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
