/* ************************************************************************
*   File: stuff.cpp                                     Part of Bylins    *
*  Usage: stuff load table handling                                       *
*                                                                         *
*  $Author$                                                          *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "utils.h"
#include "dg_scripts.h"
#include "stuff.hpp"
#include "char.hpp"
#include "room.hpp"
#include "corpse.hpp"

void oload_class::init()
{
	std::string cppstr;
	std::istringstream isstream;
	bool in_block = false;
	obj_vnum ovnum;
	mob_vnum mvnum;
	int oqty, lprob;

	clear();

	std::ifstream fp(LIB_MISC "stuff.lst");

	if (!fp)
	{
		cppstr = "oload_class:: Unable open input file !!!";
		mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	while (!fp.eof())
	{
		getline(fp, cppstr);

		if (cppstr.empty() || cppstr[0] == ';')
			continue;
		if (cppstr[0] == '#')
		{
			cppstr.erase(cppstr.begin());

			if (cppstr.empty())
			{
				cppstr = "oload_class:: Error in line '#' expected '#<RIGHT_obj_vnum>' !!!";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				in_block = false;
				continue;
			}

			isstream.str(cppstr);
			isstream >> std::noskipws >> ovnum;

			if (!isstream.eof() || real_object(ovnum) < 0)
			{
				isstream.clear();
				cppstr = "oload_class:: Error in line '#" + cppstr + "' expected '#<RIGHT_obj_vnum>' !!!";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				in_block = false;
				continue;
			}

			isstream.clear();

			in_block = true;
		}
		else if (in_block)
		{
			oqty = lprob = -1;

			isstream.str(cppstr);
			isstream >> std::skipws >> mvnum >> oqty >> lprob;

			if (lprob < 0 || lprob > MAX_LOAD_PROB || oqty < 0 || real_mobile(mvnum) < 0 || !isstream.eof())
			{
				isstream.clear();
				cppstr = "oload_class:: Error in line '" + cppstr + "'";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				cppstr = "oload_class:: \texpected '<RIGHT_mob_vnum>\t<0 <= obj_qty>\t<0 <= load_prob <= MAX_LOAD_PROB>' !!!";
				mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
				continue;
			}

			isstream.clear();

			add_elem(mvnum, ovnum, obj_load_info(oqty, lprob));
		}
		else
		{
			cppstr = "oload_class:: Error in line '" + cppstr + "' expected '#<RIGHT_obj_vnum>' !!!";
			mudlog(cppstr.c_str(), LGH, LVL_IMMORT, SYSLOG, TRUE);
		}
	}
}

oload_class oload_table;

obj_rnum ornum_by_info(const std::pair<obj_vnum, obj_load_info>& it)
{
	obj_rnum i;
	obj_rnum resutl_obj = number(1, MAX_LOAD_PROB) <= it.second.load_prob ?
						  (it.first >= 0 && (i = real_object(it.first)) >= 0 ?
						   (obj_index[i].stored + obj_index[i].number < it.second.obj_qty ?
							i :
							NOTHING) :
								   NOTHING) :
								  NOTHING;
	if (resutl_obj != NOTHING)
		log("Current load_prob: %d/%d, obj #%d (setload)", it.second.load_prob, MAX_LOAD_PROB, it.first);
	return resutl_obj;
}

/**
 * \param setload = true - ���� ����� ������� ����� �����
 *        setload = false - ���� ����� ������ ����
 */
void obj_to_corpse(OBJ_DATA *corpse, CHAR_DATA *ch, int rnum, bool setload)
{
	OBJ_DATA *o = read_object(rnum, REAL);
	if (!o)
	{
		log("SYSERROR: null from read_object rnum=%d (%s:%d)",
				rnum, __FILE__, __LINE__);
		return;
	}

	log("Load obj #%d by %s in room #%d (%s)",
			GET_OBJ_VNUM(o), GET_NAME(ch), GET_ROOM_VNUM(ch->in_room),
			setload ? "setload" : "globaldrop");

	if (!setload)
	{
		act("���� ������, ���� ������!", 0, ch, o, 0, TO_ROOM);
	}

	if (MOB_FLAGGED(ch, MOB_CORPSE))
	{
		obj_to_room(o, IN_ROOM(ch));
	}
	else
	{
		obj_to_obj(o, corpse);
	}
	if (!obj_decay(o))
	{
		if (o->in_room != NOWHERE)
		{
			act("�� ����� �����$U ������ $o.", FALSE, ch, o, 0, TO_ROOM);
		}
		load_otrigger(o);
	}
}

void obj_load_on_death(OBJ_DATA *corpse, CHAR_DATA *ch)
{
	/*
		for (oload_class::iterator iter = oload_table.begin(); iter != oload_table.end(); iter++)
			for (std::map<obj_vnum, obj_load_info>::iterator iter1 = iter->second.begin(); iter1 != iter->second.end(); iter1++)
				log("%d %d %d %d", iter->first, iter1->first, iter1->second.obj_qty, iter1->second.load_prob);
	*/

	if (ch == NULL || !IS_NPC(ch) || (!MOB_FLAGGED(ch, MOB_CORPSE) && corpse == NULL))
	{
		return;
	}

	if (GlobalDrop::check_mob(corpse, ch))
	{
		return;
	}

	oload_class::const_iterator p = oload_table.find(GET_MOB_VNUM(ch));

	if (p == oload_table.end())
	{
		return;
	}

	std::vector<obj_rnum> v(p->second.size());
	std::transform(p->second.begin(), p->second.end(), v.begin(), ornum_by_info);

	for (size_t i = 0; i < v.size(); i++)
	{
		if (v[i] >= 0)
		{
			obj_to_corpse(corpse, ch, v[i], true);
		}
	}
}
