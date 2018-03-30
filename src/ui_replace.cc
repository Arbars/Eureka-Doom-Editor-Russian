//------------------------------------------------------------------------
//  FIND AND REPLACE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2015-2016 Andrew Apted
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

#include "e_path.h"  // GoToObject
#include "e_main.h"  // Selection_Clear
#include "m_config.h"	// gui_scheme
#include "m_game.h"
#include "r_render.h"
#include "w_rawdef.h"
#include "w_texture.h"


class number_group_c
{
	// This represents a small group of numbers and number ranges,
	// which the user can type into the Match input box.
#define NUMBER_GROUP_MAX	40

private:
	int size;

	int ranges[NUMBER_GROUP_MAX][2];

	bool everything;

public:
	number_group_c() : size(0), everything(false)
	{ }

	~number_group_c()
	{ }

	void clear()
	{
		size = 0;
		everything = false;
	}

	bool is_single() const
	{
		return (size == 1) && (ranges[0][0] == ranges[0][1]);
	}

	bool is_everything() const
	{
		return everything;
	}

	int grab_first() const
	{
		if (size == 0)
			return 0;

		return ranges[0][0];
	}

	void insert(int low, int high)
	{
		// overflow is silently ignored
		if (size >= NUMBER_GROUP_MAX)
			return;

		// TODO : try to merge with existing range

		ranges[size][0] = low;
		ranges[size][1] = high;

		size++;
	}

	bool get(int num) const
	{
		for (int i = 0 ; i < size ; i++)
		{
			if (ranges[i][0] <= num && num <= ranges[i][1])
				return true;
		}

		return false;
	}

	//
	// Parse a string like "1,3-5,9" and add the numbers (or ranges)
	// to this group.  Returns false for malformed strings.
	// An empty string is considered invalid.
	//
	bool ParseString(const char *str)
	{
		char *endptr;

		for (;;)
		{
			// support an asterix to mean everything
			// (useful when using filters)
			if (*str == '*')
			{
				insert(INT_MIN, INT_MAX);
				everything = true;
				return true;
			}

			int low  = (int)strtol(str, &endptr, 0 /* allow hex */);
			int high = low;

			if (endptr == str)
				return false;

			str = endptr;

			while (isspace(*str))
				str++;

			// check for range
			if (*str == '-' || (str[0] == '.' && str[1] == '.'))
			{
				str += (*str == '-') ? 1 : 2;

				while (isspace(*str))
					str++;

				high = (int)strtol(str, &endptr, 0 /* allow hex */);

				if (endptr == str)
					return false;

				str = endptr;

				// valid range?
				if (high < low)
					return false;

				while (isspace(*str))
					str++;
			}

			insert(low, high);

			if (*str == 0)
				return true;  // OK //

			// valid separator?
			if (*str == ',' || *str == '/' || *str == '|')
				str++;
			else
				return false;
		}
	}
};


//------------------------------------------------------------------------

class UI_TripleCheckButton : public Fl_Group
{
	/* this button has three states to represent how a search should
	   check against a boolean value:

	     1. want value to be FALSE   (show with red 'X')
	     2. want value to be TRUE    (show with green tick)
	     3. don't care about value   (show with black '?')
	*/

private:
	int _value;	  // -1, 0, +1

	Fl_Button * false_but;
	Fl_Button *  true_but;
	Fl_Button * other_but;

	void Update()
	{
		false_but->hide();
		 true_but->hide();
		other_but->hide();

		if (_value < 0)
			false_but->show();
		else if (_value > 0)
			true_but->show();
		else
			other_but->show();

		redraw();
	}

	void BumpValue()
	{
		_value = (_value  < 0) ? 0 : (_value == 0) ? 1 : -1;
		Update();
	}

	static void button_callback(Fl_Widget *w, void *data)
	{
		UI_TripleCheckButton *G = (UI_TripleCheckButton *)data;

		G->BumpValue();
		G->do_callback();
	}

public:
	UI_TripleCheckButton(int X, int Y, int W, int H, const char *label = NULL) :
		Fl_Group(X, Y, W, H),
		_value(0)
	{
		if (label)
		{
			Fl_Box *box = new Fl_Box(FL_NO_BOX, X, Y, W, H, label);
			box->align(FL_ALIGN_LEFT);
		}

		false_but = new Fl_Button(X, Y, W, H, "N");
		false_but->labelcolor(FL_RED);
		false_but->labelsize(H*2/3);
		false_but->callback(button_callback, this);

		true_but = new Fl_Button(X, Y, W, H, "Y");
		true_but->labelfont(FL_HELVETICA_BOLD);
		true_but->labelcolor(fl_rgb_color(0, 176, 0));
		true_but->labelsize(H*2/3);
		true_but->callback(button_callback, this);

		other_but = new Fl_Button(X, Y, W, H, "-");
		other_but->labelsize(H*3/4);
		other_but->callback(button_callback, this);

		end();

		resizable(NULL);

		Update();
	}

	virtual ~UI_TripleCheckButton()
	{ }

public:
	int value() const { return _value; }

	void value(int new_value)
	{
		_value = new_value;
		Update();
	}
};


//------------------------------------------------------------------------


#define HIDE_BG  (gui_scheme == 2 ? FL_DARK3 : FL_DARK1)


UI_FindAndReplace::UI_FindAndReplace(int X, int Y, int W, int H) :
	Fl_Group(X, Y, W, H, NULL),
	find_numbers(new number_group_c),
	 tag_numbers(new number_group_c),
	cur_obj(OBJ_THINGS, -1)
{
	box(FL_FLAT_BOX);

	color(WINDOW_BG, WINDOW_BG);


	/* ---- FIND AREA ---- */

	Fl_Group *grp1 = new Fl_Group(X, Y, W, 210);
	grp1->box(FL_UP_BOX);
	{
		Fl_Button *hide_button = new Fl_Button(X + 8, Y + 12, 22, 22, "X");
		hide_button->color(HIDE_BG, HIDE_BG);
		hide_button->labelsize(14);
		hide_button->callback(hide_callback, this);


		Fl_Box *title = new Fl_Box(X + 60, Y + 10, W - 70, 30, "Найти и заменить");
		title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		title->labelsize(18+KF*4);


		what = new Fl_Choice(X+60, Y+46, W - 120, 33);
		what->textsize(17);
		what->add("Объекты|Текстуры линий|Текстуры секторов|Линии по типу|Секторы по типу");
		what->value(0);
		what->callback(what_kind_callback, this);

		UpdateWhatColor();


		find_match = new Fl_Input(X+75, Y+95, 135, 25, "Совп.: ");
		find_match->when(FL_WHEN_CHANGED);
		find_match->callback(find_match_callback, this);

		find_desc = new Fl_Output(X+75, Y+125, 135, 25, "Расш.: ");

		find_pic = new UI_Pic(X+225, Y+95, 64, 64, "Выбрать");
		find_pic->callback((Fl_Callback *)choose_callback, this);
///		find_pic->AllowHighlight(true);

		find_but = new Fl_Button(X+50, Y+165, 90, 30, "Найти");
		find_but->labelfont(FL_HELVETICA_BOLD);
		find_but->callback(find_but_callback, this);

		select_all_but = new Fl_Button(X+160, Y+165, 105, 30, "Выбрать все");
		select_all_but->callback(select_all_callback, this);
	}
	grp1->end();


	/* ---- REPLACE AREA ---- */

	Fl_Group *grp2 = new Fl_Group(X, Y + 214, W, 132);
	grp2->box(FL_UP_BOX);
	{
		rep_value = new Fl_Input(X+75, Y+230, 135, 25, "Новый: ");
		rep_value->when(FL_WHEN_CHANGED);
		rep_value->callback(rep_value_callback, this);

		rep_desc = new Fl_Output(X+75, Y+260, 135, 25, "Расш.: ");

		rep_pic = new UI_Pic(X+225, Y+230, 64, 64, "Выбрать");
		rep_pic->callback((Fl_Callback *)choose_callback, this);
///		rep_pic->AllowHighlight(true);

		apply_but = new Fl_Button(X+45, Y+300, 90, 30, "Заменить");
		apply_but->labelfont(FL_HELVETICA_BOLD);
		apply_but->callback(apply_but_callback, this);

		replace_all_but = new Fl_Button(X+160, Y+300, 105, 30, "Заменить все");
		replace_all_but->callback(replace_all_callback, this);
	}
	grp2->end();


	/* ---- FILTER AREA ---- */

	Fl_Group *grp3 = new Fl_Group(X, Y + 350, W, H - 350);
	grp3->box(FL_UP_BOX);
	{
		filter_toggle = new Fl_Toggle_Button(X+15, Y+356, 30, 30, "v");
		filter_toggle->labelsize(16);
		filter_toggle->color(FL_DARK3, FL_DARK3);
		filter_toggle->callback(filter_toggle_callback, this);
		filter_toggle->clear_visible_focus();

		Fl_Box *f_text = new Fl_Box(X+60, Y+356, 200, 30, "Фильтр поиска");
		f_text->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		f_text->labelsize(16);

		filter_group = new Fl_Group(X, Y+391, W, H-391);
		{
			// common stuff
			restrict_to_sel = new Fl_Check_Button(X+10, Y+390, 80, 22, " Ограничить выбор");

			previous_sel = new selection_c();

			tag_input = new Fl_Input(X+117, Y+417, 130, 24, "Совпадение тегов:");
			tag_input->when(FL_WHEN_CHANGED);
			tag_input->callback(tag_input_callback, this);

			// thing stuff
			o_easy   = new UI_TripleCheckButton(X+105, Y+444, 28, 26, "easy: ");
			o_medium = new UI_TripleCheckButton(X+105, Y+474, 28, 26, "medium: ");
			o_hard   = new UI_TripleCheckButton(X+105, Y+504, 28, 26, "hard: ");

			o_sp     = new UI_TripleCheckButton(X+220, Y+444, 28, 26, "sp: ");
			o_coop   = new UI_TripleCheckButton(X+220, Y+474, 28, 26, "coop: ");
			o_dm     = new UI_TripleCheckButton(X+220, Y+504, 28, 26, "dm: ");

			// sector stuff
			o_floors   = new Fl_Check_Button(X+45, Y+448, 80, 22, " полы");
			o_ceilings = new Fl_Check_Button(X+45, Y+470, 80, 22, " потолки");
			o_skies    = new Fl_Check_Button(X+45, Y+492, 80, 22, " небо");

			// linedef stuff
			o_lowers  = new Fl_Check_Button(X+45, Y+448, 80, 22, " нижние");
			o_uppers  = new Fl_Check_Button(X+45, Y+470, 80, 22, " верхние");
			o_rails   = new Fl_Check_Button(X+45, Y+492, 80, 22, " ровны");

			o_one_sided = new Fl_Check_Button(X+155, Y+448, 80, 22, " односторонние");
			o_two_sided = new Fl_Check_Button(X+155, Y+470, 80, 22, " двусторонние");
		}
		filter_group->end();
		filter_group->hide();

		UpdateWhatFilters();
	}
	grp3->end();


	grp3->resizable(NULL);
	resizable(grp3);

	end();


	Clear();
}


UI_FindAndReplace::~UI_FindAndReplace()
{ }


void UI_FindAndReplace::hide_callback(Fl_Widget *w, void *data)
{
	main_win->HideSpecialPanel();
}


void UI_FindAndReplace::UpdateWhatColor()
{
	switch (what->value())
	{
		case 0: /* Things      */ what->color(THING_MODE_COL); break;
		case 1: /* Line Tex    */ what->color(LINE_MODE_COL); break;
		case 2: /* Sector Flat */ what->color(SECTOR_MODE_COL); break;
		case 3: /* Line Type   */ what->color(FL_GREEN); break;
		case 4: /* Sector Type */ what->color(fl_rgb_color(255,160,0)); break;
	}
}



void UI_FindAndReplace::UpdateWhatFilters()
{
	int x = what->value();

#define SHOW_WIDGET_IF(w, test)  \
	if (test) (w)->show(); else (w)->hide();

	// common stuff
	if (x == 0 && Level_format != MAPF_Hexen)
	{
		tag_input->deactivate();
		tag_input->value("");
	}
	else
	{
		tag_input->activate();
	}

	// thing stuff
	SHOW_WIDGET_IF(o_easy,   x == 0);
	SHOW_WIDGET_IF(o_medium, x == 0);
	SHOW_WIDGET_IF(o_hard,   x == 0);

	SHOW_WIDGET_IF(o_sp,     x == 0);
	SHOW_WIDGET_IF(o_coop,   x == 0);
	SHOW_WIDGET_IF(o_dm,     x == 0);

	// sector stuff
	SHOW_WIDGET_IF(o_floors,   x == 2);
	SHOW_WIDGET_IF(o_ceilings, x == 2);
	SHOW_WIDGET_IF(o_skies,    x == 2);

	// linedef stuff
	SHOW_WIDGET_IF(o_lowers, x == 1);
	SHOW_WIDGET_IF(o_uppers, x == 1);
	SHOW_WIDGET_IF(o_rails,  x == 1);

	SHOW_WIDGET_IF(o_one_sided, x == 1 || x == 3);
	SHOW_WIDGET_IF(o_two_sided, x == 1 || x == 3);

#undef SHOW_WIDGET_IF

	// vanilla DOOM : always hide SP and COOP flags
	if (x == 0 && ! game_info.coop_dm_flags && Level_format != MAPF_Hexen)
	{
		  o_sp->hide();
		o_coop->hide();
	}
}


void UI_FindAndReplace::rawShowFilter(int value)
{
	if (value)
	{
		filter_toggle->label("^");
		filter_group->show();
	}
	else
	{
		filter_toggle->label("v");
		filter_group->hide();
	}
}


void UI_FindAndReplace::filter_toggle_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	Fl_Toggle_Button *toggle = (Fl_Toggle_Button *)w;

	box->rawShowFilter(toggle->value());
}


void UI_FindAndReplace::what_kind_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	obj_type_e prev_type = box->cur_obj.type;

	bool want_descs = true;

	switch (box->what->value())
	{
		case 0: box->cur_obj.type = OBJ_THINGS; break;
		case 1: box->cur_obj.type = OBJ_LINEDEFS; want_descs = false; break;
		case 2: box->cur_obj.type = OBJ_SECTORS;  want_descs = false; break;
		case 3: box->cur_obj.type = OBJ_LINEDEFS; break;
		case 4: box->cur_obj.type = OBJ_SECTORS; break;

		default: break;
	}

	// only clear everything when type changes
	if (prev_type != box->cur_obj.type)
	{
		box->Clear();
	}

	box->UpdateWhatColor();
	box->UpdateWhatFilters();

	if (want_descs)
	{
		box->find_desc->activate();
		box-> rep_desc->activate();
	}
	else
	{
		box->find_desc->deactivate();
		box-> rep_desc->deactivate();
	}
}


void UI_FindAndReplace::Open()
{
	show();

	WhatFromEditMode();

	// this will do a Clear() for us
	what->do_callback();

	Fl::focus(find_match);

	UnselectPics();
}


void UI_FindAndReplace::Clear()
{
	cur_obj.clear();

	find_match->value("");
	find_desc->value("");
	find_but->label("Найти");
	find_pic->Clear();

	rep_value->value("");
	rep_desc->value("");
	rep_pic->Clear();

	UnselectPics();

	find_but->deactivate();
	select_all_but->deactivate();

	apply_but->deactivate();
	replace_all_but->deactivate();

	filter_toggle->value(0);
	filter_toggle->do_callback();

	ResetFilters();
}


void UI_FindAndReplace::ResetFilters()
{
	tag_input->value("");
	tag_numbers->clear();

	restrict_to_sel->value(0);

	// thing filters
	o_easy  ->value(0);
	o_medium->value(0);
	o_hard  ->value(0);

	o_sp  ->value(0);
	o_coop->value(0);
	o_dm  ->value(0);

	ComputeFlagMask();

	// sector filters
	o_floors  ->value(1);
	o_ceilings->value(1);
	o_skies   ->value(1);

	// linedef filters
	o_lowers->value(1);
	o_uppers->value(1);
	o_rails ->value(1);

	o_one_sided->value(1);
	o_two_sided->value(1);
}


bool UI_FindAndReplace::WhatFromEditMode()
{
	switch (edit.mode)
	{
		case OBJ_THINGS:   what->value(0); return true;
		case OBJ_LINEDEFS: what->value(1); return true;
		case OBJ_SECTORS:  what->value(2); return true;

		default: return false;
	}
}


void UI_FindAndReplace::find_but_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->FindNext();
}


void UI_FindAndReplace::select_all_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->DoAll(false /* replace */);
}


void UI_FindAndReplace::apply_but_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->DoReplace();
}


void UI_FindAndReplace::replace_all_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	box->DoAll(true /* replace */);
}


void UI_FindAndReplace::find_match_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	bool is_valid = box->CheckInput(box->find_match, box->find_desc,
									box->find_pic, box->find_numbers);

	if (is_valid)
	{
		box->find_but->activate();
		box->select_all_but->activate();

		box->find_match->textcolor(FL_FOREGROUND_COLOR);
		box->find_match->redraw();
	}
	else
	{
		box->find_but->deactivate();
		box->select_all_but->deactivate();

		box->find_match->textcolor(FL_RED);
		box->find_match->redraw();
	}

	// update Replace section too
	box->rep_value->do_callback();
}


void UI_FindAndReplace::rep_value_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	bool is_valid = box->CheckInput(box->rep_value, box->rep_desc, box->rep_pic);

	if (is_valid)
	{
		box->rep_value->textcolor(FL_FOREGROUND_COLOR);
		box->rep_value->redraw();
	}
	else
	{
		box->rep_value->textcolor(FL_RED);
		box->rep_value->redraw();
	}

	bool is_usable = (is_valid && box->find_but->active());

	if (is_usable)
		box->replace_all_but->activate();
	else
		box->replace_all_but->deactivate();

	// require an found object too before 'Replace' button can be used

	if (box->cur_obj.is_nil())
		is_usable = false;

	if (is_usable)
		box->apply_but->activate();
	else
		box->apply_but->deactivate();
}


bool UI_FindAndReplace::CheckInput(Fl_Input *w, Fl_Output *desc, UI_Pic *pic, number_group_c *num_grp)
{
	const char *inp_text = w->value();

	if (strlen(inp_text) == 0)
	{
		desc->value("");
		 pic->Clear();
		return false;
	}


	// Line Textures
	if (what->value() == 1)
	{
		pic->GetTex(inp_text);
		return true;
	}

	// Sector Flats
	if (what->value() == 2)
	{
		pic->GetFlat(inp_text);
		return true;
	}


	// for numeric types, parse the number(s) and/or ranges

	int type_num = 0;

	if (! num_grp)
	{
		char *endptr;
		type_num = strtol(inp_text, &endptr, 0 /* allow hex */);

		// just check the number is valid
		if (*endptr != 0)
		{
			desc->value("(parse error)");
			return false;
		}
	}
	else
	{
		num_grp->clear();

		if (! num_grp->ParseString(inp_text))
		{
			desc->value("(parse error)");
			return false;
		}

		if (num_grp->is_everything())
		{
			desc->value("(everything)");
			return true;
		}
		else if (! num_grp->is_single())
		{
			desc->value("(multi-match)");
			return true;
		}

		type_num = num_grp->grab_first();
	}


	switch (what->value())
	{
		case 0: // Things
		{
			const thingtype_t *info = M_GetThingType(type_num);
			desc->value(info->desc);
			 pic->GetSprite(type_num, FL_DARK2);
			break;
		}

		case 3: // Lines by Type
		{
			const linetype_t *info = M_GetLineType(type_num);
			desc->value(info->desc);
			break;
		}

		case 4: // Lines by Type
		{
			const sectortype_t * info = M_GetSectorType(type_num);
			desc->value(info->desc);
			break;
		}

		default: break;
	}

	return true;
}


void UI_FindAndReplace::tag_input_callback(Fl_Widget *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	bool is_valid = box->CheckNumberInput(box->tag_input, box->tag_numbers);

	if (is_valid)
	{
		box->tag_input->textcolor(FL_FOREGROUND_COLOR);
		box->tag_input->redraw();
	}
	else
	{
		// uhhh, cannot disable all the search buttons [ too hard ]
		// so..... showing red is the all we can do....

		box->tag_input->textcolor(FL_RED);
		box->tag_input->redraw();
	}
}


bool UI_FindAndReplace::CheckNumberInput(Fl_Input *w, number_group_c *num_grp)
{
	num_grp->clear();

	// an empty string will mean match everything
	if (w->size() == 0)
		return true;

	if (num_grp->ParseString(w->value()))
		return true;

	return false;
}


//------------------------------------------------------------------------


char UI_FindAndReplace::GetKind()
{
	// these letters are same as the Browser uses

	int v = what->value();

	if (v < 0 || v >= 5)
		return '?';

	const char *kinds = "OTFLS";

	return kinds[v];
}


bool UI_FindAndReplace::ClipboardOp(char what)
{
	// hmmm, review this
	fl_beep();
	return true;
}


void UI_FindAndReplace::BrowsedItem(char kind, int number, const char *name, int e_state)
{
	if (kind != GetKind())
	{
		fl_beep();
		return;
	}

	// determine which box the user intended
	int sel_pics =	find_pic->Selected() ? 1 :
					 rep_pic->Selected() ? 2 : 0;

	if (sel_pics == 0)
	{
		sel_pics =	(Fl::focus() == find_match || Fl::focus() == find_desc) ? 1 :
					(Fl::focus() ==  rep_value || Fl::focus() ==  rep_desc) ? 2 : 0;
	}

	if (sel_pics == 0)
	{
		fl_beep();
		return;
	}

	bool is_replace = (sel_pics == 2);


	char append = 0;

	// only append when SHIFT key was pressed
	if ((e_state & FL_SHIFT) && !is_replace)
	{
		append = ',';
	}

	// insert the chosen item

	Fl_Input *inp = is_replace ? rep_value : find_match;

	if (kind == 'T' || kind == 'F')
	{
		InsertName(inp, append, name);
	}
	else
	{
		// already present?
		if (! is_replace && find_numbers->get(number))
			return;

		InsertNumber(inp, append, number);
	}
}


void UI_FindAndReplace::InsertName(Fl_Input *inp, char append, const char *name)
{
	if (append)
	{
		int len = inp->size();

		// insert a separator, unless user has already put one there
		if (NeedSeparator(inp))
		{
			char buf[4];
			buf[0] = append;
			buf[1] = 0;

			inp->replace(len, len, buf);
			len += 1;
		}

		inp->replace(len, len, name);
	}
	else
	{
		inp->value(name);
	}

	inp->do_callback();

	Fl::focus(inp);
	inp->redraw();
}

void UI_FindAndReplace::InsertNumber(Fl_Input *inp, char append, int number)
{
	char buf[256];

	sprintf(buf, "%d", number);

	InsertName(inp, append, buf);
}


bool UI_FindAndReplace::NeedSeparator(Fl_Input *inp) const
{
	const char *str = inp->value();

	// nothing but whitespace?  --> no need
	while (isspace(*str))
		str++;

	if (str[0] == 0)
		return false;

	// ends with a punctuation symbol?  --> no need
	int p = (int)strlen(str) - 1;

	while (p >= 0 && isspace(str[p]))
		p--;

	if (p >= 0)
	{
		if (str[p] == '_') return true;
		if (str[p] == '*') return true;

		if (ispunct(str[p]))
			return false;
	}

	return true;
}


void UI_FindAndReplace::UnselectPics()
{
	find_pic->Selected(false);
	 rep_pic->Selected(false);
}


void UI_FindAndReplace::choose_callback(UI_Pic *w, void *data)
{
	UI_FindAndReplace *box = (UI_FindAndReplace *)data;

	// ensure corresponding input widget has the focus

	if (w == box->find_pic)
	{
		box->find_pic->Selected(! box->find_pic->Selected());

		if (box->find_pic->Selected())
		{
			main_win->BrowserMode(box->GetKind());

			Fl::focus(box->find_match);
			box->find_match->redraw();

			box->rep_pic->Selected(false);
		}
	}
	else
	{
		box->rep_pic->Selected(! box->rep_pic->Selected());

		if (box->rep_pic->Selected())
		{
			main_win->BrowserMode(box->GetKind());

			Fl::focus(box->rep_value);
			box->rep_value->redraw();

			box->find_pic->Selected(false);
		}
	}

	Render3D_ClearSelection();
}


//------------------------------------------------------------------------


bool UI_FindAndReplace::FindNext()
{
	// this can happen via CTRL-G shortcut (View / Go to next)
	if (strlen(find_match->value()) == 0)
	{
		Beep("Не найдено активных!");
		return false;
	}

	ComputeFlagMask();

	if (cur_obj.type != edit.mode)
	{
		Editor_ChangeMode_Raw(cur_obj.type);
	}


	bool is_first = cur_obj.is_nil();

	// grab the selection *once*, on the first find
	if (is_first)
	{
		previous_sel->change_type(edit.mode);
		previous_sel->merge(*edit.Selected);

		// catch a common user mistake
		if (filter_toggle->value() && restrict_to_sel->value() &&
			edit.Selected->empty())
		{
			Beep("ПУСТОЕ ВЫДЕЛЕНИЕ!");
			return false;
		}
	}

	Selection_Clear();


	int start_at = cur_obj.is_nil() ? 0 : (cur_obj.num + 1);
	int total    = NumObjects(cur_obj.type);

	for (int idx = start_at ; idx < total ; idx++)
	{
		if (! MatchesObject(idx))
			continue;

		if (! Filter_PrevSel(idx))
			continue;

		// found!
		{
			cur_obj.num = idx;

			if (is_first)
			{
				find_but->label("Next");
				rep_value->do_callback();
			}

			GoToObject(cur_obj);

			Status_Set("Найден #%d", idx);
			return true;
		}
	}

	// nothing (else) was found

	cur_obj.clear();

	find_but->label("Find");
	rep_value->do_callback();

	if (is_first)
		Beep("Ничего не найдено");
	else
		Beep("Больше не найдено");

	return false;
}


void UI_FindAndReplace::DoReplace()
{
	// sanity check  [ should never happen ]
	if (strlen(find_match->value()) == 0 ||
		strlen( rep_value->value()) == 0)
	{
		Beep("Неверная замена");
		return;
	}

	// this generally can't happen either
	if (cur_obj.is_nil())
	{
		Beep("Нет объекта для замены!");
		return;
	}

	int replace_tex_id = BA_InternaliseString(NormalizeTex(rep_value->value()));

	BA_Begin();

	ApplyReplace(cur_obj.num, replace_tex_id);

	BA_Message("replacement in %s #%d", NameForObjectType(cur_obj.type), cur_obj.num);

	BA_End();

	// move onto next object
	FindNext();
}


bool UI_FindAndReplace::MatchesObject(int idx)
{
	switch (what->value())
	{
		case 0: // Things
			return Match_Thing(idx);

		case 1: // LineDefs (texturing)
			return Match_LineDef(idx);

		case 2: // Sectors (texturing)
			return Match_Sector(idx);

		case 3: // Lines by Type
			return Match_LineType(idx);

		case 4: // Sectors by Type
			return Match_SectorType(idx);

		default: return false;
	}
}


void UI_FindAndReplace::ApplyReplace(int idx, int new_tex)
{
	SYS_ASSERT(idx >= 0);

	switch (what->value())
	{
		case 0: // Things
			Replace_Thing(idx);
			break;

		case 1: // LineDefs (texturing)
			Replace_LineDef(idx, new_tex);
			break;

		case 2: // Sectors (texturing)
			Replace_Sector(idx, new_tex);
			break;

		case 3: // Lines by Type
			Replace_LineType(idx);
			break;

		case 4: // Sectors by Type
			Replace_SectorType(idx);
			break;

		default: break;
	}
}


void UI_FindAndReplace::DoAll(bool replace)
{
	if (strlen(find_match->value()) == 0)
	{
		Beep("не найдено активных!");
		return;
	}

	// catch a common user mistake
	if (filter_toggle->value() && restrict_to_sel->value() &&
		edit.Selected->empty())
	{
		Beep("ВЫБОР ПУСТОТЫ!");
		return;
	}

	ComputeFlagMask();

	if (cur_obj.type != edit.mode)
		Editor_ChangeMode_Raw(cur_obj.type);

	int replace_tex_id = 0;

	if (replace)
	{
		replace_tex_id = BA_InternaliseString(NormalizeTex(rep_value->value()));

		BA_Begin();
	}

	// we select objects even in REPLACE mode
	// (gives the user a visual indication that stuff was done)

	previous_sel->change_type(edit.mode);
	previous_sel->merge(*edit.Selected);

	// this clears the selection
	edit.Selected->change_type(edit.mode);

	int total = NumObjects(cur_obj.type);
	int count = 0;

	for (int idx = 0 ; idx < total ; idx++)
	{
		if (! MatchesObject(idx))
			continue;

		if (! Filter_PrevSel(idx))
			continue;

		count++;

		if (replace)
			ApplyReplace(idx, replace_tex_id);

		edit.Selected->set(idx);
	}

	if (count == 0)
		Beep("Ничего не найдено");
	else
		Status_Set("Найдено %d объектов", count);

	if (replace)
	{
		BA_MessageForSel("replacement in", edit.Selected);

		BA_End();
	}

	if (count > 0)
	{
		GoToSelection();
		edit.error_mode = true;
	}

	if (replace)
	{
		cur_obj.clear();
		rep_value->do_callback();
	}

	RedrawMap();
}


//------------------------------------------------------------------------
//    MATCHING METHODS
//------------------------------------------------------------------------

bool UI_FindAndReplace::Match_Thing(int idx)
{
	const Thing *T = Things[idx];

	if (! find_numbers->get(T->type))
		return false;

	// skill/mode flag filter
	if ((T->options & options_mask) != options_value)
		return false;

	if (Level_format == MAPF_Hexen && ! Filter_Tag(T->tid))
		return false;

	return true;
}


bool UI_FindAndReplace::Match_LineDef(int idx)
{
	const LineDef *L = LineDefs[idx];

	if (! Filter_Tag(L->tag) || ! Filter_Sides(L))
		return false;

	const char *pattern = find_match->value();

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		const SideDef *SD = (pass == 0) ? L->Right() : L->Left();

		if (! SD)
			continue;

		const char *L_tex = SD->LowerTex();
		const char *U_tex = SD->UpperTex();
		const char *R_tex = SD->MidTex();

		if (! L->TwoSided())
		{
			L_tex = R_tex;
			R_tex = U_tex = NULL;
		}

		if (!filter_toggle->value() || o_lowers->value())
			if (L_tex && Pattern_Match(L_tex, pattern))
				return true;

		if (!filter_toggle->value() || o_uppers->value())
			if (U_tex && Pattern_Match(U_tex, pattern))
				return true;

		if (!filter_toggle->value() || o_rails->value())
			if (R_tex && Pattern_Match(R_tex, pattern, true /* is_rail */))
				return true;
	}

	return false;
}


bool UI_FindAndReplace::Match_Sector(int idx)
{
	const Sector *SEC = Sectors[idx];

	if (! Filter_Tag(SEC->tag))
		return false;

	const char *pattern = find_match->value();

	if (!filter_toggle->value() || o_floors->value())
		if (Pattern_Match(SEC->FloorTex(), pattern))
			return true;

	const char *ceil_tex = SEC->CeilTex();

	if (!filter_toggle->value() || (!is_sky(ceil_tex) && o_ceilings->value())
								|| ( is_sky(ceil_tex) && o_skies->value()) )
		if (Pattern_Match(ceil_tex, pattern))
			return true;

	return false;
}


bool UI_FindAndReplace::Match_LineType(int idx)
{
	const LineDef *L = LineDefs[idx];

	if (! find_numbers->get(L->type))
		return false;

	if (! Filter_Tag(L->tag) || ! Filter_Sides(L))
		return false;

	return true;
}


bool UI_FindAndReplace::Match_SectorType(int idx)
{
	const Sector *SEC = Sectors[idx];

	if (! find_numbers->get(SEC->type))
		return false;

	if (! Filter_Tag(SEC->tag))
		return false;

	return true;
}


bool UI_FindAndReplace::Filter_PrevSel(int idx)
{
	if (! filter_toggle->value())
		return true;

	if (! restrict_to_sel->value())
		return true;

	return previous_sel->get(idx);
}


bool UI_FindAndReplace::Filter_Tag(int tag)
{
	if (! filter_toggle->value())
		return true;

	// an empty string means everything (same as '*')
	if (tag_input->size() == 0)
		return true;

	return tag_numbers->get(tag);
}


bool UI_FindAndReplace::Filter_Sides(const LineDef *L)
{
	if (filter_toggle->value())
	{
		if (! o_one_sided->value() && L->OneSided())
			return false;

		if (! o_two_sided->value() && L->TwoSided())
			return false;
	}

	return true;
}


void UI_FindAndReplace::ComputeFlagMask()
{
	options_mask  = 0;
	options_value = 0;

	if (! filter_toggle->value())
	{
		// this will always succeed
		return;
	}

#define FLAG_FROM_WIDGET(w, mul, flag)  \
	if ((w)->value() != 0)  \
	{  \
		options_mask |= (flag);  \
		if ((w)->value() * (mul) > 0)  \
			options_value |= (flag);  \
	}

	FLAG_FROM_WIDGET(  o_easy, 1, MTF_Easy);
	FLAG_FROM_WIDGET(o_medium, 1, MTF_Medium);
	FLAG_FROM_WIDGET(  o_hard, 1, MTF_Hard);

	if (game_info.coop_dm_flags)
	{
		FLAG_FROM_WIDGET(  o_sp, -1, MTF_Not_SP);
		FLAG_FROM_WIDGET(o_coop, -1, MTF_Not_COOP);
		FLAG_FROM_WIDGET(  o_dm, -1, MTF_Not_DM);
	}
	else	// vanilla DOOM
	{
		FLAG_FROM_WIDGET(o_dm, 1, MTF_Not_SP);
	}

#undef FLAG_FROM_WIDGET
}


bool UI_FindAndReplace::Pattern_Match(const char *tex, const char *pattern, bool is_rail)
{
	// allow multiple names (simple patterns) separated by commas.
	// they can include '*' as a wildcard.

	char local_pat[256];
	int ofs = 0;

	for (;;)
	{
		if (*pattern == 0 || *pattern == ',' || *pattern == '/' || *pattern == '|')
		{
			local_pat[ofs] = 0;

			// do not match the empty rail texture against the "*" wildcard.
			// [ this is debatable, but I think this prevents making changes
			//   which the user really didn't want or expect ]
			if (is_rail && is_null_tex(tex) && local_pat[0] == '*')
			{
				// no match
			}
			else
			{
				if (fl_filename_match(tex, local_pat))
					return true;
			}

			if (*pattern == 0)
				return false;

			// begin new part, skip comma
			ofs = 0;
			pattern++;
		}

		// prevent overflow of the local buffer
		if (ofs + 4 > (int)sizeof(local_pat))
			return false;

		local_pat[ofs++] = *pattern++;
	}
}


//------------------------------------------------------------------------
//    REPLACE METHODS
//------------------------------------------------------------------------

void UI_FindAndReplace::Replace_Thing(int idx)
{
	int new_type = atoi(rep_value->value());

	BA_ChangeTH(idx, Thing::F_TYPE, new_type);
}


void UI_FindAndReplace::Replace_LineDef(int idx, int new_tex)
{
	const LineDef *L = LineDefs[idx];

	const char *pattern = find_match->value();

	for (int pass = 0 ; pass < 2 ; pass++)
	{
		int sd_num = (pass == 0) ? L->right : L->left;

		const SideDef *SD = (pass == 0) ? L->Right() : L->Left();

		if (! SD)
			continue;

		const char *L_tex = SD->LowerTex();
		const char *U_tex = SD->UpperTex();
		const char *R_tex = SD->MidTex();

		if (! L->TwoSided())
		{
			if (!filter_toggle->value() || o_lowers->value())
				if (R_tex && Pattern_Match(R_tex, pattern))
					BA_ChangeSD(sd_num, SideDef::F_MID_TEX, new_tex);

			continue;
		}

		if (!filter_toggle->value() || o_lowers->value())
			if (L_tex && Pattern_Match(L_tex, pattern))
				BA_ChangeSD(sd_num, SideDef::F_LOWER_TEX, new_tex);

		if (!filter_toggle->value() || o_uppers->value())
			if (U_tex && Pattern_Match(U_tex, pattern))
				BA_ChangeSD(sd_num, SideDef::F_UPPER_TEX, new_tex);

		if (!filter_toggle->value() || o_rails->value())
			if (R_tex && Pattern_Match(R_tex, pattern, true /* is_rail */))
				BA_ChangeSD(sd_num, SideDef::F_MID_TEX, new_tex);
	}
}


void UI_FindAndReplace::Replace_Sector(int idx, int new_tex)
{
	const Sector *SEC = Sectors[idx];

	const char *pattern = find_match->value();

	if (!filter_toggle->value() || o_floors->value())
		if (Pattern_Match(SEC->FloorTex(), pattern))
			BA_ChangeSEC(idx, Sector::F_FLOOR_TEX, new_tex);

	const char *ceil_tex = SEC->CeilTex();

	if (!filter_toggle->value() || (!is_sky(ceil_tex) && o_ceilings->value())
								|| ( is_sky(ceil_tex) && o_skies->value()) )
		if (Pattern_Match(ceil_tex, pattern))
			BA_ChangeSEC(idx, Sector::F_CEIL_TEX, new_tex);
}


void UI_FindAndReplace::Replace_LineType(int idx)
{
	int new_type = atoi(rep_value->value());

	BA_ChangeLD(idx, LineDef::F_TYPE, new_type);
}


void UI_FindAndReplace::Replace_SectorType(int idx)
{
	int new_type = atoi(rep_value->value());

	BA_ChangeSEC(idx, Sector::F_TYPE, new_type);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
