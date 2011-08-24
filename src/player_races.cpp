// $RCSfile$     $Date$     $Revision$
// Part of Bylins http://www.mud.ru

#include <algorithm>
#include <sstream>

#include "conf.h"
#include "sysdep.h"
#include "utils.h"
#include "comm.h"
//#include "db.h"
//#include "dg_scripts.h"
//#include "char.hpp"
//#include "handler.h"

#include "player_races.hpp"
#include "pugixml.hpp"

PlayerKinListType PlayerRace::PlayerKinList;

void LoadRace(pugi::xml_node RaceNode, PlayerKinPtr KinPtr);
void LoadKin(pugi::xml_node KinNode);

PlayerKin::PlayerKin()
{
//Create kin
};

//������� ����� _���_ � ����������� �� ��� ����� �������� �� �����
void LoadRace(pugi::xml_node RaceNode, PlayerKinPtr KinPtr)
{
	pugi::xml_node CurNode;
	PlayerRacePtr TmpRace(new PlayerRace);

	TmpRace->SetRaceNum(RaceNode.attribute("racenum").as_int());
    TmpRace->SetEnabledFlag(RaceNode.attribute("enabled").as_bool());
	TmpRace->SetRaceMenuStr(RaceNode.child("menu").child_value());
	CurNode = RaceNode.child("racename");
	TmpRace->SetRaceItName(CurNode.child("itname").child_value());
	TmpRace->SetRaceHeName(CurNode.child("hename").child_value());
	TmpRace->SetRaceSheName(CurNode.child("shename").child_value());
	TmpRace->SetRacePluralName(CurNode.child("pluralname").child_value());
	//Add race features
	for (CurNode = RaceNode.child("feature"); CurNode; CurNode = CurNode.next_sibling("feature"))
	{
		TmpRace->AddRaceFeature(CurNode.attribute("featnum").as_int());
	}
    //��������� ������� ����� "��������� �� ����" ����� ����������
	for (CurNode = RaceNode.child("birthplace"); CurNode; CurNode = CurNode.next_sibling("birthplace"))
	{
		TmpRace->AddRaceBirthPlace(CurNode.attribute("id").as_int());
	}
	//Add new race in list
	KinPtr->PlayerRaceList.push_back(TmpRace);
}

//������� ����� ���� � ��������� �� ���� ���������� �� ������
void LoadKin(pugi::xml_node KinNode)
{
	pugi::xml_node CurNode;
	PlayerKinPtr TmpKin(new PlayerKin);

	//Parse kin's parameters
	TmpKin->KinNum = KinNode.attribute("kinnum").as_int();
    TmpKin->Enabled = KinNode.attribute("enabled").as_bool();
	CurNode = KinNode.child("menu");
	TmpKin->KinMenuStr = CurNode.child_value();
	CurNode = KinNode.child("kinname");
	TmpKin->KinItName = CurNode.child("itname").child_value();
	TmpKin->KinHeName = CurNode.child("hename").child_value();
	TmpKin->KinSheName = CurNode.child("shename").child_value();
	TmpKin->KinPluralName = CurNode.child("pluralname").child_value();

	//Parce kin races
	CurNode = KinNode.child("kinraces");
	for (CurNode = CurNode.child("race"); CurNode; CurNode = CurNode.next_sibling("race"))
	{
		LoadRace(CurNode, TmpKin);
	}
	//Add new kin in kin list
	PlayerRace::PlayerKinList.push_back(TmpKin);
}

//�������� ���������� ����� � ���
void PlayerRace::Load(const char *PathToFile)
{
	char buf[MAX_INPUT_LENGTH];
	pugi::xml_document Doc;
	pugi::xml_node KinList, KinNode;
	pugi::xml_parse_result Result;

	Result = Doc.load_file(PathToFile);
	if (!Result)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...%s", Result.description());
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}

	KinList = Doc.child("races");
	if (!KinList)
	{
		snprintf(buf, MAX_STRING_LENGTH, "...players races reading fail");
		mudlog(buf, CMP, LVL_IMMORT, SYSLOG, TRUE);
		return;
	}
	for (KinNode = KinList.child("kin"); KinNode; KinNode = KinNode.next_sibling("kin"))
	{
		LoadKin(KinNode);
	}
}

// ���������� ����� ������� ����������� � ������
void PlayerRace::AddRaceFeature(int feat)
{
	std::vector<int>::iterator RaceFeature = find(_RaceFeatureList.begin(), _RaceFeatureList.end(), feat);
	if (RaceFeature == _RaceFeatureList.end())
		_RaceFeatureList.push_back(feat);
};

// ���������� ������ ����� �������� ����������
void PlayerRace::AddRaceBirthPlace(int id)
{
	std::vector<int>::iterator BirthPlace = find(_RaceBirthPlaceList.begin(), _RaceBirthPlaceList.end(), id);
	if (BirthPlace == _RaceBirthPlaceList.end())
		_RaceBirthPlaceList.push_back(id);
};

//��������� ��������� �� ���� PC
PlayerKinPtr PlayerRace::GetPlayerKin(int Kin)
{
	PlayerKinPtr KinPtr;
	for (PlayerKinListType::iterator it =  PlayerKinList.begin();it != PlayerKinList.end();++it)
		if ((*it)->KinNum == Kin)
			KinPtr = *it;
	return KinPtr;
};

//��������� ��������� �� ��� PC
PlayerRacePtr PlayerRace::GetPlayerRace(int Kin,int Race)
{
	PlayerRacePtr RacePtr;
	PlayerKinPtr KinPtr = PlayerRace::GetPlayerKin(Kin);

	if (KinPtr != NULL)
		for (PlayerRaceListType::iterator it = KinPtr->PlayerRaceList.begin();it != KinPtr->PlayerRaceList.end();++it)
			if ((*it)->_RaceNum == Race)
				RacePtr = *it;
	return RacePtr;
};

//�������� ������� � ������� ����+���� ����������� � ��������� �������
bool PlayerRace::FeatureCheck(int Kin,int Race,int Feat)
{
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Kin, Race);
	if (RacePtr == NULL)
		return false;
	std::vector<int>::iterator RaceFeature = find(RacePtr->_RaceFeatureList.begin(), RacePtr->_RaceFeatureList.end(), Feat);
	if (RaceFeature != RacePtr->_RaceFeatureList.end())
		return true;

	return false;
};

void PlayerRace::GetKinNamesList(CHAR_DATA *ch)
{
	//char buf[MAX_INPUT_LENGTH];
	//snprintf(buf, MAX_STRING_LENGTH, " %d \r\n", PlayerKinList[0]->PlayerRaceList[0]->GetFeatNum());
	//send_to_char(buf, ch);
	//for (PlayerKinListType::iterator it = PlayerKinList.begin();it != PlayerKinList.end();++it)
	//{
	//	snprintf(buf, MAX_STRING_LENGTH, " %s \r\n", (*it)->KinHeName.c_str());
	//	send_to_char(buf, ch);
	//}
	//test message
	//char buf33[MAX_INPUT_LENGTH];
	//snprintf(buf33, MAX_STRING_LENGTH, "!==!...%s", CurNode.child("shename").child_value());
	//mudlog(buf33, CMP, LVL_IMMORT, SYSLOG, TRUE);
}

//��������� ����� ������ ������������ ��� ���������� ����+����
std::vector<int> PlayerRace::GetRaceFeatures(int Kin,int Race)
{
	std::vector<int> RaceFeatures;
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Kin, Race);
	if (RacePtr != NULL) {
		RaceFeatures = RacePtr->_RaceFeatureList;
	} else {
		RaceFeatures.push_back(0);
	}
	return RaceFeatures;
}

//��������� ������ ���� �� ��������
int PlayerRace::GetKinNumByName(string KinName)
{
	for (PlayerKinListType::iterator it =  PlayerKinList.begin();it != PlayerKinList.end();++it)
		if ((*it)->KinMenuStr == KinName)
			return (*it)->KinNum;

	return RACE_UNDEFINED;
};

//��������� ������ ���� �� ��������
int PlayerRace::GetRaceNumByName(int Kin, string RaceName)
{
	PlayerKinPtr KinPtr = PlayerRace::GetPlayerKin(Kin);
	if (KinPtr != NULL)
		for (PlayerRaceListType::iterator it =  KinPtr->PlayerRaceList.begin();it != KinPtr->PlayerRaceList.end();++it)
			if ((*it)->_RaceMenuStr == RaceName)
				return (*it)->_RaceNum;

	return RACE_UNDEFINED;
};

//��������� �������� ���� �� ������ � ����
std::string PlayerRace::GetKinNameByNum(int KinNum, int Sex)
{
    for (PlayerKinListType::iterator it =  PlayerKinList.begin();it != PlayerKinList.end();++it)
        if ((*it)->KinNum == KinNum)
            switch (Sex)
            {
            case SEX_NEUTRAL:
                return PlayerRace::PlayerKinList[KinNum]->KinItName;
                break;
            case SEX_MALE:
                return PlayerRace::PlayerKinList[KinNum]->KinHeName;
                break;
            case SEX_FEMALE:
                return PlayerRace::PlayerKinList[KinNum]->KinSheName;
                break;
            case SEX_POLY:
                return PlayerRace::PlayerKinList[KinNum]->KinPluralName;
                break;
            default:
                return PlayerRace::PlayerKinList[KinNum]->KinHeName;
            }

	return KIN_NAME_UNDEFINED;
};

//��������� �������� ���� �� ������ � ����
std::string PlayerRace::GetRaceNameByNum(int KinNum, int RaceNum, int Sex)
{
    //static char out_str[MAX_STRING_LENGTH];
    //*out_str = '\0';
    //sprintf(out_str, "����� ��� %d %d", KinNum, RaceNum);
    //return out_str; //PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceHeName;
    PlayerKinPtr KinPtr;
	if ((KinNum > KIN_UNDEFINED) && (KinNum < PlayerRace::PlayerKinList.size()))
    {
        KinPtr = PlayerRace::PlayerKinList[KinNum];
        for (PlayerRaceListType::iterator it =  KinPtr->PlayerRaceList.begin();it != KinPtr->PlayerRaceList.end();++it)
            if ((*it)->_RaceNum == RaceNum)
                switch (Sex)
                {
                case SEX_NEUTRAL:
                    return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceItName;
                    break;
                case SEX_MALE:
                    return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceHeName;
                    break;
                case SEX_FEMALE:
                    return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceSheName;
                    break;
                case SEX_POLY:
                    return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RacePluralName;
                    break;
                default:
                    return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum]->_RaceHeName;
                };
    }

	return RACE_NAME_UNDEFINED;
};

//����� ������ ����� � ���� ����
std::string PlayerRace::ShowRacesMenu(int KinNum)
{
    std::ostringstream buffer;
    PlayerKinPtr KinPtr = PlayerRace::GetPlayerKin(KinNum);

    if (KinPtr != NULL)
        for (PlayerRaceListType::iterator it =  KinPtr->PlayerRaceList.begin();it != KinPtr->PlayerRaceList.end();++it)
            buffer << " " << (*it)->_RaceNum+1 << ") " << (*it)->_RaceMenuStr << "\r\n";

    return buffer.str();
};

//��������� ������� ���� � �������� �������
//���������� ����� ���� ��� "������������"
int PlayerRace::CheckRace(int KinNum, char *arg)
{
    int RaceNum = atoi(arg);
    //��������� ���� ����������� � ���� �� ������ �������� ShowRacesMenu
    //�� ��� ������� � ����������� _����_ ����� ������������ ������ ���� ��������� �����
    //��� ������ ������� (� ��������� �� �������). ����� ������ ���������, ��� ������� ��������� ��������.
    //�� ���� �� ������� �� ������������� �������+1 (��� � ���� ��������� � 1)
    //�� ��� ���������� ���� ����� _����_ � �� ������, ��� ������������ ��� ����� � �� ���������
    if (!RaceNum || (RaceNum < 1) ||
        (RaceNum > PlayerRace::PlayerKinList[KinNum]->PlayerRaceList.size()) ||
        !PlayerKinList[KinNum]->PlayerRaceList[RaceNum-1]->_Enabled)
        return RACE_UNDEFINED;
	if ((KinNum > RACE_UNDEFINED) && (KinNum < PlayerRace::PlayerKinList.size()))
        return PlayerRace::PlayerKinList[KinNum]->PlayerRaceList[RaceNum-1]->_RaceNum;

    return RACE_UNDEFINED;
};

//����� ������ ��� � ���� ����
std::string PlayerRace::ShowKinsMenu()
{
    std::ostringstream buffer;
    for (PlayerKinListType::iterator it =  PlayerKinList.begin();it != PlayerKinList.end();++it)
        buffer << " " << (*it)->KinNum+1 << ") " << (*it)->KinMenuStr << "\r\n";

    return buffer.str();
};

//��������� ������� ���� � ��������� �������
int PlayerRace::CheckKin(char *arg)
{
    int KinNum = atoi(arg);
    if (!KinNum || (KinNum < 1) ||
        (KinNum > PlayerRace::PlayerKinList.size()) ||
        !PlayerRace::PlayerKinList[KinNum-1]->Enabled)
        return KIN_UNDEFINED;

    return PlayerRace::PlayerKinList[KinNum-1]->KinNum;
};

std::vector<int> PlayerRace::GetRaceBirthPlaces(int Kin,int Race)
{
	std::vector<int> BirthPlaces;
	PlayerRacePtr RacePtr = PlayerRace::GetPlayerRace(Kin, Race);
	if (RacePtr != NULL)
		BirthPlaces = RacePtr->_RaceBirthPlaceList;

	return BirthPlaces;
}

int PlayerRace::CheckBirthPlace(int Kin, int Race, char *arg)
{
    int BirthPlaceNum = atoi(arg);
    if (BirthPlaceNum &&
        ((Kin > RACE_UNDEFINED) && (Kin < PlayerRace::PlayerKinList.size())) &&
        (Race > RACE_UNDEFINED) && (Race < PlayerRace::PlayerKinList[Kin]->PlayerRaceList.size()) &&
        ((BirthPlaceNum > 0) && (BirthPlaceNum <= PlayerRace::PlayerKinList[Kin]->PlayerRaceList[Race]->_RaceBirthPlaceList.size())))
        return PlayerRace::PlayerKinList[Kin]->PlayerRaceList[Race]->_RaceBirthPlaceList[BirthPlaceNum-1];

    return RACE_UNDEFINED;
};