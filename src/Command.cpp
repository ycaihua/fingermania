#include "global.h"
#include "Command.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "arch/Dialog/Dialog.h"
#include "Foreach.h"

RString Command::GetName() const
{
	if (m_vsArgs.empty())
		return RString();
	RString s = m_vArgs[0];
	Trim(s);
	return s;
}

Command::Arg Command::GetArg(unsigned index) const
{
	Arg a;
	if (index < m_vsArgs.size())
		a.s = m_vsArgs[index];
	return a;
}

void Command::Load(const RString& sCommand)
{
	m_vsArgs.clear();
	split(sCommand, ",", m_vsArgs, false);
}

RString Command::GetOriginalCommandString() const
{
	return join(",", m_vsArgs);
}

static void SplitWithQuotes(const RString sSource, const char Delimitor, vector<RString>& asOut, const bool bIgnoreEmpty)
{
	if (sSource.empty())
		return;

	size_t startpos = 0;
	do
	{
		size_t pos = startpos;
		while (pos < sSource.size())
		{
			if (sSource[pos] == Delimitor)
				break;

			if (sSource[pos] == '"' || sSource[pos] == '\'')
			{
				pos = sSource.find(sSource[pos], pos + 1);
				if (pos == string::npos)
					pos = sSource.size();
				else
					++pos;
			}
			else
				++pos;
		}

		if (pos - startpos > 0 || !bIgnoreEmpty)
		{
			if (startpos == 0 && pos - startpos == sSource.size())
				asOut.push_back(sSource);
			else
			{
				const RString AddCString = sSource.substr( startpos, pos-startpos );
				asOut.push_back( AddCString );
			}
		}

		startpos = pos + 1;
	} while (startpos <= sSource.size());
}

RString Commands::GetOriginalCommandString() const
{
	RString s;
	FOREACH_CONST(Command, v, c)
		s += c->GetOriginalCommandString();
	return s;
}

void ParseCommands(const RString& sCommands, Commands& vCommandOut)
{
	vector<RString> vsCommands;
	SplitWithQuotes(sCommands, ";", vsCommands, true);
	vCommandsOut.resize(vsCommands.size());

	for (unsigned i = 0; i < vsCommands.size(); i++)
	{
		Command& cmd = vCommandOut.v[i];
		cmd.Load(vsCommands[i]);
	}
}

Commands ParseCommands(const RString& sCommands)
{
	Commands vCommands;
	ParseCommands(sCommands, vCommands);
	return vCommands;
}


