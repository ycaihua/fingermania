#include "global.h"
#include "CsvFile.h"
#include "RageUtil.h"
#include "RageFile.h"
#include "RageLog.h"
#include "Foreach.h"

CsvFile::CsvFile()
{
}

bool CsvFile::ReadFile(const RString& sPath)
{
	m_sPath = sPath;
	CHECKPOINT_M(ssprintf("Reading '%s'", m_sPath.c_str()));

	RageFile f;
	if (!f.Open(m_sPath))
	{
		LOG->Trace("Reading '%s' failed: %s", m_sPath.c_str(), f.GetError().c_str());
		m_sError = f.GetError();
		return 0;
	}

	return ReadFile(f);
}

bool CsvFile::ReadFile(RageFileBasic& f)
{
	m_vvs.clear();

	while (1)
	{
		RString line;
		switch(f.GetLine(line))
		{
		case -1:
			m_sError = f.GetError();
			return false;
		case 0:
			return true;
		}

		utf8_remove_bom(line);

		vector<RString> vs;
		while (!line.empty())
		{
			if (line[0] == '\"')
			{
				line.erase(line.begin());
				RString::size_type iEnd = 0;
				do
				{
					iEnd = line.find('\"', iEnd);
					if (iEnd == line.npos)
					{
						iEnd = line.size() - 1;
						break;
					}

					if (line.size() > iEnd + 1 && line[iEnd + 1] == '\"')
						iEnd = iEnd + 2;
					else
						break;
				}
				while(true);

				RString sValue = line;
				sValue = sValue.Left(iEnd);
				vs.push_back(sValue);

				line.erase(line.begin(), line.begin() + iEnd);

				if (!line.empty() && line[0] == '\"')
					line.erase(line.begin());
			}
			else
			{
				RString::size_type iEnd = line.find(',');
				if (iEnd == line.npos)
					iEnd = line.size();

				RString sValue = line;
				sValue = sValue.Left(iEnd);
				vs.push_back(sValue);

				line.erase(line.begin(), line.begin() + iEnd);
			}

			if (!line.empty() && line[0] == ',')
				line.erase(line.begin());
		}

		m_vvs.push_back(vs);
	}
}

bool CsvFile::WriteFile(const RString& sPath) const
{
	RageFile f;
	if (!f.Open(sPath, RageFile::WRITE))
	{
		LOG->Trace("Writing '%s' failed: %s", sPath.c_str(), f.GetError().c_str());
		m_sError = f.GetError();
		return false;
	}

	return CsvFile::WriteFile(f);
}

bool CsvFile::WriteFile(RageFileBasic& f) const
{
	FOREACH_CONST(StringVector, m_vvs, line)
	{
		RString sLine;
		FOREACH_CONST(RString, *line, value)
		{
			RString sValue = *value;
			sVal.Replace("\"", "\"\"");
			sLine += "\"" + sVal + "\"";
			if (value != line->end() - 1)
				sLine += ",";
		}
		if (f.PutLine(sLine) == -1)
		{
			m_sError = f.GetError();
			return false;
		}
	}
	return true;
}


