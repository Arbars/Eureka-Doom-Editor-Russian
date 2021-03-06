//------------------------------------------------------------------------
//  BUILDING NODES / PLAY THE MAP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2016 Andrew Apted
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
#include "m_loadsave.h"
#include "w_wad.h"

#include "ui_window.h"

#include "bsp.h"


// config items
bool bsp_on_save	= true;
bool bsp_fast		= false;
bool bsp_warnings	= false;

int  bsp_split_factor	= DEFAULT_FACTOR;

bool bsp_gl_nodes		= true;
bool bsp_force_v5		= false;
bool bsp_force_zdoom	= false;
bool bsp_compressed		= false;


extern bool inhibit_node_build;


#define NODE_PROGRESS_COLOR  fl_color_cube(2,6,2)


class UI_NodeDialog : public Fl_Double_Window
{
public:
	Fl_Browser *browser;

	Fl_Progress *progress;

	Fl_Button * button;

	int cur_prog;
	char prog_label[64];

	bool finished;

	bool want_cancel;
	bool want_close;

public:
	UI_NodeDialog();
	virtual ~UI_NodeDialog();

	/* FLTK method */
	int handle(int event);

public:
	void SetProg(int perc);

	void Print(const char *str);

	void Finish_OK();
	void Finish_Cancel();
	void Finish_Error();

	bool WantCancel() const { return want_cancel; }
	bool WantClose()  const { return want_close;  }

private:
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
};


//
//  Callbacks
//

void UI_NodeDialog::close_callback(Fl_Widget *w, void *data)
{
	UI_NodeDialog * that = (UI_NodeDialog *)data;

	that->want_close = true;

	if (! that->finished)
		that->want_cancel = true;
}


void UI_NodeDialog::button_callback(Fl_Widget *w, void *data)
{
	UI_NodeDialog * that = (UI_NodeDialog *)data;

	if (that->finished)
		that->want_close = true;
	else
		that->want_cancel = true;
}


//
//  Constructor
//
UI_NodeDialog::UI_NodeDialog() :
	    Fl_Double_Window(400, 400, "Создание Нод"),
		cur_prog(-1),
		finished(false),
		want_cancel(false),
		want_close(false)
{
	size_range(w(), h());

	callback((Fl_Callback *) close_callback, this);

	color(WINDOW_BG, WINDOW_BG);


	browser = new Fl_Browser(0, 0, w(), h() - 100);


	Fl_Box * ptext = new Fl_Box(FL_NO_BOX, 10, h() - 80, 80, 20, "Прогресс:");
	(void) ptext;


	progress = new Fl_Progress(100, h() - 80, w() - 120, 20);
	progress->align(FL_ALIGN_INSIDE);
	progress->box(FL_FLAT_BOX);
	progress->color(FL_LIGHT2, NODE_PROGRESS_COLOR);

	progress->minimum(0.0);
	progress->maximum(100.0);
	progress->value(0.0);


	button = new Fl_Button(w() - 100, h() - 46, 80, 30, "Отменить");
	button->callback(button_callback, this);


	end();

	resizable(browser);
}


//
//  Destructor
//
UI_NodeDialog::~UI_NodeDialog()
{ }


int UI_NodeDialog::handle(int event)
{
	if (event == FL_KEYDOWN && Fl::event_key() == FL_Escape)
	{
		if (finished)
			want_close = true;
		else
			want_cancel = true;
		return 1;
	}

	return Fl_Double_Window::handle(event);
}


void UI_NodeDialog::SetProg(int perc)
{
	if (perc == cur_prog)
		return;

	cur_prog = perc;

	sprintf(prog_label, "%d%%", perc);

	progress->value(perc);
	progress->label(prog_label);

	Fl::check();
}


void UI_NodeDialog::Print(const char *str)
{
	// split lines

	static char buffer[256];

	snprintf(buffer, sizeof(buffer), "%s", str);
	buffer[sizeof(buffer) - 1] = 0;

	char * pos = buffer;
	char * next;

	while (pos && *pos)
	{
		next = strchr(pos, '\n');

		if (next) *next++ = 0;

		browser->add(pos);

		pos = next;
	}

	browser->bottomline(browser->size());

	Fl::check();
}


void UI_NodeDialog::Finish_OK()
{
	finished = true;

	button->label("Закрыть");

	progress->value(100);
	progress->label("Успешно");
	progress->color(FL_BLUE, FL_BLUE);
}

void UI_NodeDialog::Finish_Cancel()
{
	finished = true;

	button->label("Закрыть");

	progress->value(0);
	progress->label("Отменено");
	progress->color(FL_RED, FL_RED);
}

void UI_NodeDialog::Finish_Error()
{
	finished = true;

	button->label("Закрыть");

	progress->value(100);
	progress->label("ОШИБКА");
	progress->color(FL_RED, FL_RED);
}


//------------------------------------------------------------------------


static nodebuildinfo_t * nb_info;

static char message_buf[MSG_BUF_LEN];

static UI_NodeDialog * dialog;


static const char *build_ErrorString(build_result_e ret)
{
	switch (ret)
	{
		case BUILD_OK: return "OK";

		// building was cancelled
		case BUILD_Cancelled: return "Cancelled by User";

		// the WAD file was corrupt / empty / bad filename
		case BUILD_BadFile: return "Bad File";

		// file errors
		case BUILD_ReadError:  return "Read Error";
		case BUILD_WriteError: return "Write Error";

		default: return "Unknown Error";
	}
}


void GB_PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsnprintf(message_buf, MSG_BUF_LEN, str, args);
	va_end(args);

	message_buf[MSG_BUF_LEN-1] = 0;

	if (dialog)
		dialog->Print(message_buf);

	LogPrintf("BSP: %s", message_buf);
}


static void PrepareInfo(nodebuildinfo_t *info)
{
	info->factor	= CLAMP(1, bsp_split_factor, 31);

	info->gl_nodes	= bsp_gl_nodes;
	info->fast		= bsp_fast;
	info->warnings	= bsp_warnings;

	info->force_v5			= bsp_force_v5;
	info->force_xnod		= bsp_force_zdoom;
	info->force_compress	= bsp_compressed;

	info->total_failed_maps    = 0;
	info->total_warnings       = 0;
	info->total_minor_warnings = 0;

	// clear cancelled flag
	info->cancelled = false;
}


static build_result_e BuildAllNodes(nodebuildinfo_t *info)
{
	LogPrintf("\n");

	// sanity check

	SYS_ASSERT(1 <= info->factor && info->factor <= 32);

	int num_levels = edit_wad->LevelCount();
	SYS_ASSERT(num_levels > 0);

	GB_PrintMsg("\n");

	dialog->SetProg(0);

	build_result_e ret;

	// loop over each level in the wad
	for (int n = 0 ; n < num_levels ; n++)
	{
		ret = AJBSP_BuildLevel(info, n);

		if (ret != BUILD_OK)
			break;

		dialog->SetProg(100 * (n + 1) / num_levels);

		Fl::check();

		if (dialog->WantCancel())
		{
			nb_info->cancelled = true;
		}
	}

	if (ret == BUILD_OK)
	{
		GB_PrintMsg("\n");
		GB_PrintMsg("Всего определено карт: %d\n", info->total_failed_maps);
		GB_PrintMsg("Всего ошибок: %d серьъёзных, %d мелких\n", info->total_warnings,
					info->total_minor_warnings);
	}
	else if (ret == BUILD_Cancelled)
	{
		GB_PrintMsg("\n");
		GB_PrintMsg("Создание ОТМЕНЕНО.\n\n");
	}
	else
	{
		// build nodes failed
		GB_PrintMsg("\n");
		GB_PrintMsg("Создание НЕ УДАЛОСЬ: %s\n", build_ErrorString(ret));
		GB_PrintMsg("Причина: %s\n\n", nb_info->message);
	}

	return ret;
}


void BuildNodesAfterSave(short lev_idx)
{
	dialog = NULL;

	nb_info = new nodebuildinfo_t;

	PrepareInfo(nb_info);

	build_result_e ret = AJBSP_BuildLevel(nb_info, lev_idx);

	// TODO : maybe print # of serious/minor warnings

	if (ret != BUILD_OK)
		LogPrintf("НОДЫ НЕ СТРОЯТСЯ (WTF!?).\n");

	delete nb_info;
}


void CMD_BuildAllNodes()
{
	if (! edit_wad)
	{
		DLG_Notify("Невозможно создать ноды во время редактирования PWAD.");
		return;
	}

	if (edit_wad->IsReadOnly())
	{
		DLG_Notify("Невозможно создать ноды в файле со статусом \"только для чтения\".");
		return;
	}

	if (MadeChanges)
	{
		if (DLG_Confirm("Отменить|&Сохранить",
		                "Имеются несохраненные изменения, Вы хотите сохранить их сейчас "
						"и затем создать все ноды?") <= 0)
		{
			return;
		}

		inhibit_node_build = true;

		bool save_result = M_SaveMap();

		inhibit_node_build = false;

		// user cancelled the save?
		if (! save_result)
			return;
	}


	// this probably cannot happen, but check anyway
	if (edit_wad->LevelCount() == 0)
	{
		DLG_Notify("Невозможно построить ноды: уровни не обнаружены!");
		return;
	}


	dialog = new UI_NodeDialog();

	dialog->set_modal();
	dialog->show();

	Fl::check();


	nb_info = new nodebuildinfo_t;

	PrepareInfo(nb_info);

	build_result_e ret = BuildAllNodes(nb_info);


	if (ret == BUILD_OK)
	{
		dialog->Finish_OK();

		Status_Set("Ноды созданы");
	}
	else if (nb_info->cancelled)
	{
		dialog->Finish_Cancel();

		Status_Set("Отменео");
	}
	else
	{
		dialog->Finish_Error();

		Status_Set("Ошибка создания нодов");
	}

	while (! dialog->WantClose())
	{
		Fl::wait(0.2);
	}

	delete nb_info; nb_info = NULL;
	delete dialog;  dialog = NULL;


	return;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
