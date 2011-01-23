#include "global.h"
#include "DateTime.h"
#include "RageUtil.h"
#include "EnumHelper.h"
#include "LuaManager.h"
#include "LocalizedString.h"

DateTime::DateTime()
{
	Init();
}

void DateTime::Init()
{
	ZERO(*this);
}

bool DateTime::operator<(const DateTime& other) const
{
#define COMPARE(v) if (v != other.v) return v < other.v;
	COMPARE(tm_year);
	COMPARE(tm_mon);
	COMPARE(tm_mday);
	COMPARE(tm_hour);
	COMPARE(tm_min);
	COMPARE(tm_sec);
#undef COMPARE
	return false;
}

bool DateTime::operator==(const DateTime& other) const
{
#define COMPARE(x) if (x != other.x) return false;
	COMPARE(tm_year);
	COMPARE(tm_mon);
	COMPARE(tm_mday);
	COMPARE(tm_hour);
	COMPARE(tm_min);
	COMPARE(tm_sec);
#undef COMPARE
	return true;
}

DateTime DateTime::GetNowDateTime()
{
	time_t now = time(NULL);
	tm tNow;
	localtime_r(&now, &tNow);
	DateTime dtNow;
#define COPY_M(v) dtNow.v = tNow.v;
	COPY_M(tm_year);
	COPY_M(tm_mon);
	COPY_M(tm_mday);
	COPY_M(tm_hour);
	COPY_M(tm_min);
	COPY_M(tm_sec);
#undef COPY_M
	return dtNow;
}

DateTime DateTime::GetNowDate()
{
	DateTime tNow = GetNowDateTime();
	tNow.StripTime();
	return tNow;
}

void DateTime::StripTime()
{
	tm_hour = 0;
	tm_min = 0;
	tm_sec = 0;
}

RString DateTime::GetString() const
{
	RString s = ssprintf("%d-%02d-%02d",
		tm_year + 1900,
		tm_mon + 1,
		tm_mday);

	if (tm_hour != 0 ||
		tm_min != 0 ||
		tm_sec != 0)
	{
		s += ssprintf("%02d:%02d:%02d",
			tm_hour,
			tm_min,
			tm_sec);
	}

	return s;
}

bool DateTime::FromString(const RString sDateTime)
{
	Init();

	int ret;
	ret = sscanf(sDateTime, "%d-%d-%d %d:%d:%d",
		&tm_year,
		&tm_mon,
		&tm_mday,
		&tm_hour,
		&tm_min,
		&tm_sec);
	if (ret == 6)
		goto success;

	ret = sscanf(sDateTime, "%d-%d-%d",
		&tm_year,
		&tm_mon,
		&tm_mday);
	if (ret == 3)
		goto success;

	return false;

success:
	tm_year -= 1900;
	tm_mon -= 1;
	return true;
}

RString DayInYearToString(int iDayInYear)
{
	return ssprintf("DayInYear%03d", iDayInYear);
}

int StringToDayInYear(RString sDayInYear)
{
	int iDayInYear;
	if (sscanf(sDayInYear, "DayInYear%d", &iDayInYear) != 1)
		return -1;
	return iDayInYear;
}

static const RString LAST_DAYS_NAME[NUM_LAST_DAYS] = 
{
	"TODAY",
	"Yesterday",
	"Day2Ago",
	"Day3Ago",
	"Day4Ago",
	"Day5Ago",
	"Day6Ago",
};

RString LastDayToString(int iLastDayIndex)
{
	return LAST_DAYS_NAME[iLastDayIndex];
}

static const char* DAY_OF_WEEK_TO_NAME[DAYS_IN_WEEK] =
{
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
};

RString DayOfWeekToString(int iDayOfWeekIndex)
{
	return DAY_OF_WEEK_TO_NAME[iDayOfWeekIndex];
}

RString HourInDayToString(int iHourInDayIndex)
{
	return ssprintf("Hour%02d", iHourInDayIndex);
}

static const char* MonthNames[] = 
{
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December",
};
XToString(Month);
XToLocalizedString(Month);
LuaXType(Month);

RString LastWeekToString(int iLastWeekIndex)
{
	switch(iLastWeekIndex)
	{
	case 0:
		return "ThisWeek";
		break;
	case 1:
		return "LastWeek";
		break;
	default:
		return ssprintf("Week%02dAgo", iLastWeekIndex);
		break;
	}
}

RString LastDayToLocalizedString(int iLastDayIndex)
{
	RString s = LastDayToString(iLastDayIndex);
	s.Replace("Day", "");
	s.Replace("Ago", " Ago");
	return s;
}

RString LastWeekToLocalizedString(int iLastWeekIndex)
{
	RString s = LastWeekToString(iLastWeekIndex);
	s.Replace("Week", "");
	s.Replace("Ago", " Ago");
	return s;
}

RString HourInDayToLocalizedString(int iHourIndex)
{
	int iBeginHour = iHourIndex;
	iBeginHour--;
	wrap(iBeginHour, 24);
	iBeginHour++;

	return ssprintf("%02d:00+", iBeginHour);
}

tm AddDays(tm start, int iDaysToMove)
{
	time_t seconds = mktime(&start);
	seconds += iDaysToMove * 60 * 60 * 24;

	tm time;
	localtime_r(&seconds, &time);
	return time;
}

tm GetYesterday(tm start)
{
	return AddDays(start, -1);
}

int GetDayOfWeek(tm time)
{
	int iDayOfWeek = time.tm_wday;
	ASSERT(iDayOfWeek < DAYS_IN_WEEK);
	return iDayOfWeek;
}

tm GetNextSunday(tm start)
{
	return AddDays(start, DAYS_IN_WEEK - GetDayOfWeek(start));
}

tm GetDayInYearAndYear(int iDayInYearIndex, int iYear)
{
	tm when;
	ZERO(when);
	when.tm_mon = 0;
	when.tm_mday = iDayInYearIndex + 1;
	when.tm_year = iYear - 1900;
	time_t then = mktime(&when);

	localtime_r(&then, &when);
	return when;
}

LuaFunction(MonthToString, MonthToString(Enum::Check<Month>(L, 1)));
LuaFunction( MonthToLocalizedString, MonthToLocalizedString( Enum::Check<Month>(L, 1) ) );
LuaFunction(MonthOfYear, GetLocalTime().tm_mon);
LuaFunction(DayOfMonth, GetLocalTime().tm_mday);
LuaFunction(Hour, GetLocalTime().tm_hour);
LuaFunction(Minute, GetLocalTime().tm_min);
LuaFunction(Second, GetLocalTime().tm_sec);
LuaFunction(Year, GetLocalTime().tm_year + 1900);
LuaFunction(Weekday, GetLocalTime().tm_wday);
LuaFunction(DayOfYear, GetLocalTime().tm_yday);

