#include "global.h"
#include "Bookkeeper.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "IniFile.h"
#include "GameConstantsAndTypes.h"
#include "SongManager.h"
#include "RageFile.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include <ctime>

Bookkeeper* BOOKKEEPER = NULL;

static const RString COINS_DAT = "Save/Coins.xml";

Bookkeeper::Bookkeeper()
{
	ClearAll();

	ReadFromDisk();
}

Bookkeeper::~Bookkeeper()
{
	WriteToDisk();
}

#define WARN_AND_RETURN {LOG->Warn("Error parsing at %s:%d", __FILE__, __LINE__); return;}

void Bookkeeper::ClearAll()
{
	m_mapCoinsForHour.clear();
}

bool Bookkeeper::Date::operator < (const Date& rhs) const
{
	if (m_iYear != rhs.m_iYear)
		return m_iYear < rhs.m_iYear;
	if (m_iDayOfYear != rhs.m_iDayOfYear)
		return m_iDayOfYear < rhs.m_iDayOfYear;
	return m_iHour < rhs.m_iHour;
}

void Bookkeeper::Date::Set(time_t t)
{
	tm ltime;
	localtime_r(&t, &ltime);

	Set(ltime);
}

void Bookkeeper::Date::Set(tm pTime)
{
	m_iHour = pTime.tm_hour;
	m_iDayOfYear = pTime.tm_yday;
	m_iYear = pTime.tm_year + 1900;
}

void Bookkeeper::LoadFromNode(const XNode* pNode)
{
	if (pNode->GetName() != "Bookkeeping")
	{
		LOG->Warn("Error loading bookkeeping: unexpected \"%s\"", pNode->GetName().c_str());
		return;
	}

	const XNode* pData = pNode->GetChild("Data");
	if (pData == NULL)
	{
		LOG->Warn("Error loading bookkeeping: Data node missing");
		return;
	}

	FOREACH_CONST_Child(pData, day)
	{
		Date d;
		if (!day->GetAttrValue("Hour", d.m_iHour) ||
			!day->GetAttrValue("Day", d.m_iDayOfYear) ||
			!day->GetAttrValue("Year", d.m_iYear))
		{
			LOG->Warn("Incomplete date field");
			continue;
		}

		int iCoins;
		day->GetTextValue(iCoins);

		m_mapCoinsForHour[d] = iCoins;
	}
}

XNode* Bookkeeper::CreateNode() const
{
	XNode* xml = new XNode("Bookkeeping");

	{
		XNode* pData = xml->AppendChild("Data");

		for (map<Date, int>::const_iterator it = m_mapCoinsForHour.begin(); it != m_mapCoinsForHour.end(); ++it)
		{
			int iCoins = it->second;
			XNode* pDay = pData->AppendChild("Coins", iCoins);

			const Date& d = it->first;
			pDay->AppendAttr("Hour", d.m_iHour);
			pDay->AppendAttr("Day", d.m_iDayOfYear);
			pDay->AppendAttr("Year", d.m_iYear);
		}
	}

	return xml;
}

void Bookkeeper::ReadFromDisk()
{
	if (!IsAFile(COINS_DAT))
		return;

	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, COINS_DAT))
		return;

	LoadFromNode(&xml);
}

void Bookeeper::WriteToDisk()
{
	RageFile f;
	if (!f.Open(COINS_DAT, RageFile::WRITE | RageFile::SLOW_FLUSH))
	{
		LOG->Warn("Couldn't open file \"%s\" for writing: %s", COINS_DAT.c_str(), f.GetError().c_str());
		return;
	}

	auto_ptr<XNode> xml(CreateNode());
	XmlFileUtil::SaveToFile(xml.get(), f);
}

void Bookkeeper::CoinsInserted()
{
	Date d;
	d.Set(time(NULL));

	++m_mapCoinsForHour[d];
}

int Bookkeeper::GetNumCoinsInRange(map<Date, int>::const_iterator begin, map<Date, int>::const_iterator end) const
{
	int iCoins = 0;

	while (begin != end)
	{
		iCoins += begin->second;
		++begin;
	}

	return iCoins;
}

int Bookkeeper::GetNumCoins(Date beginning, Date ending) const
{
	return GetNumCoinsInRange(m_mapCoinsForHour.lower_bound(beginning),
		m_mapCoinsForHour.lower_bound(ending));
}

int Bookkeeper::GetCoinsTotal() const
{
	return GetNumCoinsInRange(m_mapCoinsForHour.begin(), m_mapCoinsForHour.end());
}

void Bookkeeper::GetCoinsLastDays(int coins[NUM_LAST_DAYS]) const
{
	time_t lOldTime = time(NULL);
	tm time;
	localtime_r(&lOldTime, &time);

	time.tm_hour = 0;

	for (int i = 0; i < NUM_LAST_DAYS; i++)
	{
		tm EndTime = AddDays(time, +1);
		coins[i] = GetNumCoins(time, EndTime);
		time = GetYesterday(time);
	}
}

int Bookkeeper::GetCoinsLastWeeks(int coins[NUM_LAST_WEEKS]) const
{
	time_t lOldTime = time(NULL);
	tm time;
	localtime_r(&lOldTime, &time);

	time = GetNextSunday(time);
	time = GetYesterday(time);

	for (int w = 0; w < NUM_LAST_WEEKS; w++)
	{
		tm StartTime = AddDays(time, -DAYS_IN_WEEK);
		coins[w] = GetNumCoins(StartTime, time);
		time = StartTime;
	}
}

void Bookkeeper::GetCoinsByDayOfWeek(int coins[DAYS_IN_WEEK]) const
{
	for (int i = 0; i < DAYS_IN_WEEK; i++)
		coins[i] = 0;

	for (map<Date, int>::const_iterator it = m_mapCoinsForHour.begin(); it != m_mapCoinsForHour.end(); ++it)
	{
		const Date& d = it->first;
		int iDayOfWeek = GetDayInYearAndYear(d.m_iDayOfYear, d.m_iYear).tm_wday;
		coins[iDayOfWeek] += it->second;
	}
}

void Bookkeeper::GetCoinsByHour(int coins[HOURS_IN_DAY]) const
{
	memset(coins, 0, sizeof(int) * HOURS_IN_DAY);
	for (map<Date, int>::const_iterator it = m_mapCoinsForHour.begin(); it != m_mapCoinsForHour.end(); ++it)
	{
		const Date& d = it->first;

		if (d.m_iHour >= HOURS_IN_DAY)
		{
			LOG->Warn("Hour %i >= %i", d.m_iHour, HOURS_IN_DAY);
			continue;
		}

		coins[d.m_iHour] += it->second;
	}
}
