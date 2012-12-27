// $RCSfile$     $Date$     $Revision$
// Copyright (c) 2009 Krodo
// Part of Bylins http://www.mud.ru

#include <sstream>
#include "obj.hpp"
#include "utils.h"
#include "pk.h"
#include "celebrates.hpp"
#include "cache.hpp"
#include "screen.h"
#include "comm.h"
#include "char.hpp"
#include "db.h"
#include "constants.h"

extern void print_obj_affects(CHAR_DATA *ch, const obj_affected_type &affect);
extern void tascii(int *pointer, int num_planes, char *ascii);

id_to_set_info_map obj_data::set_table;

namespace
{

// ������ ������ ����� ����� ��� ������������ �������� ��������
typedef std::vector<OBJ_DATA *> PurgedObjList;
PurgedObjList purged_obj_list;

} // namespace

obj_data::obj_data()
{
	this->zero_init();
	caching::obj_cache.add(this);
}

obj_data::obj_data(const obj_data& other)
{
	*this = other;
	caching::obj_cache.add(this);
}

obj_data::~obj_data()
{
	if (!purged_)
	{
		this->purge(true);
	}
}

/**
 * ��. Character::zero_init()
 */
void obj_data::zero_init()
{
	uid = 0;
	item_number = NOTHING;
	in_room = NOWHERE;
	aliases = NULL;
	description = NULL;
	short_description = NULL;
	action_description = NULL;
	ex_description = NULL;
	carried_by = NULL;
	worn_by = NULL;
	worn_on = NOWHERE;
	in_obj = NULL;
	contains = NULL;
	id = 0;
	proto_script = NULL;
	script = NULL;
	next_content = NULL;
	next = NULL;
	room_was_in = NOWHERE;
	max_in_world = 0;
	skills = NULL;
	serial_num_ = 0;
	timer_ = 0;
	manual_mort_req_ = -1;
	purged_ = false;
	ilevel_ = 0;

	memset(&obj_flags, 0, sizeof(obj_flag_data));

	for (int i = 0; i < 6; i++)
	{
		PNames[i] = NULL;
	}
}

/**
 * ��. Character::purge()
 */
void obj_data::purge(bool destructor)
{
	if (purged_)
	{
		log("SYSERROR: double purge (%s:%d)", __FILE__, __LINE__);
		return;
	}

	caching::obj_cache.remove(this);
	//��. ����������� � ��������� BloodyInfo �� pk.cpp
	bloody::remove_obj(this);
	//weak_ptr ��� �� ��� ������ � ����
	Celebrates::remove_from_obj_lists(this->uid);

	if (!destructor)
	{
		// �������� ���
		this->zero_init();
		// ����������� ������������ �� ������������ ����
		purged_ = true;
		// ���������� � ������ ��������� ������ ����������
		purged_obj_list.push_back(this);
	}
}

bool obj_data::purged() const
{
	return purged_;
}

int obj_data::get_serial_num()
{
	return serial_num_;
}

void obj_data::set_serial_num(int num)
{
	serial_num_ = num;
}

const std::string obj_data::activate_obj(const activation& __act)
{
	if (item_number >= 0)
	{
		obj_flags.affects = __act.get_affects();
		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			affected[i] = __act.get_affected_i(i);

		int weight = __act.get_weight();
		if (weight > 0)
			obj_flags.weight = weight;

		if (obj_flags.type_flag == ITEM_WEAPON)
		{
			int nsides, ndices;
			__act.get_dices(ndices, nsides);
			// ���� ����� �������� �� ��, ��������������� �� ��� ���������.
			if (ndices > 0 && nsides > 0)
			{
				obj_flags.value[1] = ndices;
				obj_flags.value[2] = nsides;
			}
		}

		// ���������� ������.
		if (__act.has_skills())
		{
			// � ���� �������� � �������� skills ��������� �� ���� � ��� ��
			// ������. � � ����������. ������� ��� ���� ��������� �����,
			// ���� ��� ������� "������������" ����� ��� ����� �������.
			// ������, ������������� � ����, �������� ������ ������ ��������.
			skills = new std::map<int, int>;
			__act.get_skills(*skills);
		}

		return __act.get_actmsg() + "\n" + __act.get_room_actmsg();
	}
	else
		return "\n";
}

const std::string obj_data::deactivate_obj(const activation& __act)
{
	if (item_number >= 0)
	{
		obj_flags.affects = obj_proto[item_number]->obj_flags.affects;
		for (int i = 0; i < MAX_OBJ_AFFECT; i++)
			affected[i] = obj_proto[item_number]->affected[i];

		obj_flags.weight = obj_proto[item_number]->obj_flags.weight;

		if (obj_flags.type_flag == ITEM_WEAPON)
		{
			obj_flags.value[1] = obj_proto[item_number]->obj_flags.value[1];
			obj_flags.value[2] = obj_proto[item_number]->obj_flags.value[2];
		}

		// ������������ ������.
		if (__act.has_skills())
		{
			// ��� ��������� �� ��������� ����� ������ � ��������. ���
			// ����� ����� �������.
			delete skills;
			skills = obj_proto[item_number]->skills;
		}

		return __act.get_deactmsg() + "\n" + __act.get_room_deactmsg();
	}
	else
		return "\n";
}

void obj_data::set_skill(int skill_num, int percent)
{
	if (skills)
	{
		std::map<int, int>::iterator skill = skills->find(skill_num);
		if (skill == skills->end())
		{
			if (percent != 0)
				skills->insert(std::make_pair(skill_num, percent));
		}
		else
		{
			if (percent != 0)
				skill->second = percent;
			else
				skills->erase(skill);
		}
	}
	else
	{
		if (percent != 0)
		{
			skills = new std::map<int, int>;
			skills->insert(std::make_pair(skill_num, percent));
		}
	}
}

int obj_data::get_skill(int skill_num) const
{
	if (skills)
	{
		std::map<int, int>::iterator skill = skills->find(skill_num);
		if (skill != skills->end())
			return skill->second;
		else
			return 0;
	}
	else
	{
		return 0;
	}
}

/**
 * @warning ��������������, ��� __out_skills.empty() == true.
 */
void obj_data::get_skills(std::map<int, int>& out_skills) const
{
	if (skills)
		out_skills.insert(skills->begin(), skills->end());
}

bool obj_data::has_skills() const
{
	if (skills)
		return !skills->empty();
	else
		return false;
}

void obj_data::set_timer(int timer)
{
	timer_ = MAX(0, timer);
}

int obj_data::get_timer() const
{
	return timer_;
}

/**
* �������� �������� ������ (��� ������ ����������� ����� ������� �� ����).
* ������ ������� ����� ������ ��������� ������ �� ���������� �������.
* \param time �� ������� 1.
*/
void obj_data::dec_timer(int time)
{
	if (time > 0)
	{
		timer_ -= time;
		if (!timed_spell.empty())
		{
			timed_spell.dec_timer(this, time);
		}
	}
}

int obj_data::get_manual_mort_req() const
{
	return manual_mort_req_;
}

void obj_data::set_manual_mort_req(int param)
{
	manual_mort_req_ = param;
}

unsigned obj_data::get_ilevel() const
{
	return ilevel_;
}

void obj_data::set_ilevel(unsigned ilvl)
{
	ilevel_ = ilvl;
}

int obj_data::get_mort_req() const
{
	if (manual_mort_req_ >= 0)
	{
		return manual_mort_req_;
	}
	else if (ilevel_ > 30)
	{
		return 9;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

namespace
{

const float SQRT_MOD = 1.7095;
const int AFF_SHIELD_MOD = 30;
const int AFF_BLINK_MOD = 10;

float count_affect_weight(OBJ_DATA *obj, int num, int mod)
{
	float weight = 0;

	switch(num)
	{
	case APPLY_STR:
		weight = mod * 5.0;
		break;
	case APPLY_DEX:
		weight = mod * 10.0;
		break;
	case APPLY_INT:
		weight = mod * 10.0;
		break;
	case APPLY_WIS:
		weight = mod * 10.0;
		break;
	case APPLY_CON:
		weight = mod * 10.0;
		break;
	case APPLY_CHA:
		weight = mod * 10.0;
		break;
	case APPLY_HIT:
		weight = mod * 0.2;
		break;
	case APPLY_AC:
		weight = mod * -1.0;
		break;
	case APPLY_HITROLL:
		weight = mod * 3.3;
		break;
	case APPLY_DAMROLL:
		weight = mod * 3.3;
		break;
	case APPLY_SAVING_WILL:
		weight = mod * -1.0;
		break;
	case APPLY_SAVING_CRITICAL:
		weight = mod * -1.0;
		break;
	case APPLY_SAVING_STABILITY:
		weight = mod * -1.0;
		break;
	case APPLY_SAVING_REFLEX:
		weight = mod * -1.0;
		break;
	case APPLY_CAST_SUCCESS:
		weight = mod * 1.0;
		break;
	case APPLY_MORALE:
		weight = mod * 2.0;
		break;
	case APPLY_INITIATIVE:
		weight = mod * 1.0;
		break;
	case APPLY_ABSORBE:
		weight = mod * 1.0;
		break;
	}

	return weight;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

namespace ObjSystem
{

bool is_armor_type(const OBJ_DATA *obj)
{
	switch GET_OBJ_TYPE(obj)
	{
	case ITEM_ARMOR:
	case ITEM_ARMOR_LIGHT:
	case ITEM_ARMOR_MEDIAN:
	case ITEM_ARMOR_HEAVY:
		return true;
	}
	return false;
}

/**
* ��. CharacterSystem::release_purged_list()
*/
void release_purged_list()
{
	for (PurgedObjList::iterator i = purged_obj_list.begin();
		i != purged_obj_list.end(); ++i)
	{
		delete *i;
	}
	purged_obj_list.clear();
}

bool is_mob_item(OBJ_DATA *obj)
{
	if (IS_OBJ_NO(obj, ITEM_NO_MALE)
		&& IS_OBJ_NO(obj, ITEM_NO_FEMALE)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ITEM_NO_MONO)
		&& IS_OBJ_NO(obj, ITEM_NO_POLY)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ITEM_NO_RUSICHI)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ITEM_NO_CLERIC)
		&& IS_OBJ_NO(obj, ITEM_NO_THIEF)
		&& IS_OBJ_NO(obj, ITEM_NO_WARRIOR)
		&& IS_OBJ_NO(obj, ITEM_NO_ASSASINE)
		&& IS_OBJ_NO(obj, ITEM_NO_GUARD)
		&& IS_OBJ_NO(obj, ITEM_NO_PALADINE)
		&& IS_OBJ_NO(obj, ITEM_NO_RANGER)
		&& IS_OBJ_NO(obj, ITEM_NO_SMITH)
		&& IS_OBJ_NO(obj, ITEM_NO_MERCHANT)
		&& IS_OBJ_NO(obj, ITEM_NO_DRUID)
		&& IS_OBJ_NO(obj, ITEM_NO_BATTLEMAGE)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMMAGE)
		&& IS_OBJ_NO(obj, ITEM_NO_DEFENDERMAGE)
		&& IS_OBJ_NO(obj, ITEM_NO_NECROMANCER)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_NO(obj, ITEM_NO_SEVERANE)
		&& IS_OBJ_NO(obj, ITEM_NO_POLANE)
		&& IS_OBJ_NO(obj, ITEM_NO_KRIVICHI)
		&& IS_OBJ_NO(obj, ITEM_NO_VATICHI)
		&& IS_OBJ_NO(obj, ITEM_NO_VELANE)
		&& IS_OBJ_NO(obj, ITEM_NO_DREVLANE)
		&& IS_OBJ_NO(obj, ITEM_NO_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_MALE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_FEMALE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_MONO)
		&& IS_OBJ_ANTI(obj, ITEM_AN_POLY)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_RUSICHI)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_CLERIC)
		&& IS_OBJ_ANTI(obj, ITEM_AN_THIEF)
		&& IS_OBJ_ANTI(obj, ITEM_AN_WARRIOR)
		&& IS_OBJ_ANTI(obj, ITEM_AN_ASSASINE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_GUARD)
		&& IS_OBJ_ANTI(obj, ITEM_AN_PALADINE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_RANGER)
		&& IS_OBJ_ANTI(obj, ITEM_AN_SMITH)
		&& IS_OBJ_ANTI(obj, ITEM_AN_MERCHANT)
		&& IS_OBJ_ANTI(obj, ITEM_AN_DRUID)
		&& IS_OBJ_ANTI(obj, ITEM_AN_BATTLEMAGE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMMAGE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_DEFENDERMAGE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_NECROMANCER)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}
	if (IS_OBJ_ANTI(obj, ITEM_AN_SEVERANE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_POLANE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_KRIVICHI)
		&& IS_OBJ_ANTI(obj, ITEM_AN_VATICHI)
		&& IS_OBJ_ANTI(obj, ITEM_AN_VELANE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_DREVLANE)
		&& IS_OBJ_ANTI(obj, ITEM_AN_CHARMICE))
	{
		return true;
	}

	return false;
}

void init_ilvl(OBJ_DATA *obj)
{
	if (is_mob_item(obj)
		|| OBJ_FLAGGED(obj, ITEM_SETSTUFF)
		|| obj->get_manual_mort_req() >= 0)
	{
		obj->set_ilevel(0);
		return;
	}

	float total_weight = 0.0;

	// ������� APPLY_x
	for (int k = 0; k < MAX_OBJ_AFFECT; k++)
	{
		if (obj->affected[k].location == 0) continue;

		// ������, ���� ���� ������ �������� � ���������� �����
		for (int kk = 0; kk < MAX_OBJ_AFFECT; kk++)
		{
			if (obj->affected[k].location == obj->affected[kk].location
				&& k != kk)
			{
				log("SYSERROR: double affect=%d, obj_vnum=%d",
					obj->affected[k].location, GET_OBJ_VNUM(obj));
				obj->set_ilevel(1000000);
				return;
			}
		}
		float weight = count_affect_weight(obj, obj->affected[k].location,
			obj->affected[k].modifier);
		total_weight += pow(weight, SQRT_MOD);
	}
	// ������� AFF_x ����� weapon_affect
	for (int m = 0; weapon_affect[m].aff_bitvector != -1; ++m)
	{
		if (weapon_affect[m].aff_bitvector == AFF_AIRSHIELD
			&& IS_SET(GET_OBJ_AFF(obj, weapon_affect[m].aff_pos), weapon_affect[m].aff_pos))
		{
			total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
		}
		else if (weapon_affect[m].aff_bitvector == AFF_FIRESHIELD
			&& IS_SET(GET_OBJ_AFF(obj, weapon_affect[m].aff_pos), weapon_affect[m].aff_pos))
		{
			total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
		}
		else if (weapon_affect[m].aff_bitvector == AFF_ICESHIELD
			&& IS_SET(GET_OBJ_AFF(obj, weapon_affect[m].aff_pos), weapon_affect[m].aff_pos))
		{
			total_weight += pow(AFF_SHIELD_MOD, SQRT_MOD);
		}
		else if (weapon_affect[m].aff_bitvector == AFF_BLINK
			&& IS_SET(GET_OBJ_AFF(obj, weapon_affect[m].aff_pos), weapon_affect[m].aff_pos))
		{
			total_weight += pow(AFF_BLINK_MOD, SQRT_MOD);
		}
	}

	obj->set_ilevel(ceil(pow(total_weight, 1/SQRT_MOD)));
}

void init_item_levels()
{
	for (std::vector <OBJ_DATA *>::iterator i = obj_proto.begin(),
		iend = obj_proto.end(); i != iend; ++i)
	{
		init_ilvl(*i);
	}
}

} // namespace ObjSystem

////////////////////////////////////////////////////////////////////////////////

AcquiredAffects::AcquiredAffects()
	: type_(0), weight_(0)
{
	affects_flags_ = clear_flags;
	extra_flags_ = clear_flags;
	no_flags_ = clear_flags;
}

AcquiredAffects::AcquiredAffects(OBJ_DATA *obj)
{
	name_ = GET_OBJ_PNAME(obj, 4) ? GET_OBJ_PNAME(obj, 4) : "<null>";
	type_ = ACQUIRED_ENCHANT;

	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != APPLY_NONE
			&& obj->affected[i].modifier != 0)
		{
			affected_.push_back(obj->affected[i]);
		}
	}

	affects_flags_ = GET_OBJ_AFFECTS(obj);
	extra_flags_ = obj->obj_flags.extra_flags;
	REMOVE_BIT(GET_FLAG(extra_flags_, ITEM_TICKTIMER), ITEM_TICKTIMER);
	no_flags_ = GET_OBJ_NO(obj);
	weight_ = GET_OBJ_VAL(obj, 0);
}

void AcquiredAffects::print(CHAR_DATA *ch) const
{
	send_to_char(ch, "\r\n���������� %s :%s\r\n", name_.c_str(), CCCYN(ch, C_NRM));

	for (std::vector<obj_affected_type>::const_iterator i = affected_.begin(),
		iend = affected_.end(); i != iend; ++i)
	{
		print_obj_affects(ch, *i);
	}

	if (sprintbits(affects_flags_, weapon_affects, buf2, ","))
	{
		send_to_char(ch, "%s   �������: %s%s\r\n",
				CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	}

	if (sprintbits(extra_flags_, extra_bits, buf2, ","))
	{
		send_to_char(ch, "%s   �����������: %s%s\r\n",
				CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	}

	if (sprintbits(no_flags_, no_bits, buf2, ","))
	{
		send_to_char(ch, "%s   ��������: %s%s\r\n",
				CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	}

	if (weight_ != 0)
	{
		send_to_char(ch, "%s   %s ��� �������� �� %d%s\r\n", CCCYN(ch, C_NRM),
				weight_ > 0 ? "�����������" : "���������",
				abs(weight_), CCNRM(ch, C_NRM));
	}
}

void AcquiredAffects::apply_to_obj(OBJ_DATA *obj) const
{
	for (std::vector<obj_affected_type>::const_iterator i = affected_.begin(),
		iend = affected_.end(); i != iend; ++i)
	{
		for (int k = 0; k < MAX_OBJ_AFFECT; k++)
		{
			if (obj->affected[k].location == i->location)
			{
				obj->affected[k].modifier += i->modifier;
				break;
			}
			else if (obj->affected[k].location == APPLY_NONE)
			{
				obj->affected[k].location = i->location;
				obj->affected[k].modifier = i->modifier;
				break;
			}
		}
	}

	GET_FLAG(GET_OBJ_AFFECTS(obj), INT_ZERRO) |= GET_FLAG(affects_flags_, INT_ZERRO);
	GET_FLAG(GET_OBJ_AFFECTS(obj), INT_ONE) |= GET_FLAG(affects_flags_, INT_ONE);
	GET_FLAG(GET_OBJ_AFFECTS(obj), INT_TWO) |= GET_FLAG(affects_flags_, INT_TWO);
	GET_FLAG(GET_OBJ_AFFECTS(obj), INT_THREE) |= GET_FLAG(affects_flags_, INT_THREE);

	GET_FLAG(obj->obj_flags.extra_flags, INT_ZERRO) |= GET_FLAG(extra_flags_, INT_ZERRO);
	GET_FLAG(obj->obj_flags.extra_flags, INT_ONE) |= GET_FLAG(extra_flags_, INT_ONE);
	GET_FLAG(obj->obj_flags.extra_flags, INT_TWO) |= GET_FLAG(extra_flags_, INT_TWO);
	GET_FLAG(obj->obj_flags.extra_flags, INT_THREE) |= GET_FLAG(extra_flags_, INT_THREE);

	GET_FLAG(obj->obj_flags.no_flag, INT_ZERRO) |= GET_FLAG(no_flags_, INT_ZERRO);
	GET_FLAG(obj->obj_flags.no_flag, INT_ONE) |= GET_FLAG(no_flags_, INT_ONE);
	GET_FLAG(obj->obj_flags.no_flag, INT_TWO) |= GET_FLAG(no_flags_, INT_TWO);
	GET_FLAG(obj->obj_flags.no_flag, INT_THREE) |= GET_FLAG(no_flags_, INT_THREE);

	GET_OBJ_WEIGHT(obj) += weight_;
	if (GET_OBJ_WEIGHT(obj) <= 0)
	{
		GET_OBJ_WEIGHT(obj) = 1;
	}
}

int AcquiredAffects::get_type() const
{
	return type_;
}

std::string AcquiredAffects::print_to_file() const
{
	std::stringstream out;
	out << "Ench: I " << name_ << "\n" << " T "<< type_ << "\n";

	for (std::vector<obj_affected_type>::const_iterator i = affected_.begin(),
		iend = affected_.end(); i != iend; ++i)
	{
		out << " A " << i->location << " " << i->modifier << "\n";
	}

	*buf = '\0';
	tascii((int *) &affects_flags_, 4, buf);
	out << " F " << buf << "\n";

	*buf = '\0';
	tascii((int *) &extra_flags_, 4, buf);
	out << " E " << buf << "\n";

	*buf = '\0';
	tascii((int *) &no_flags_, 4, buf);
	out << " N " << buf << "\n";

	out << " W " << weight_ << "\n~\n";

	return out.str();
}

////////////////////////////////////////////////////////////////////////////////
