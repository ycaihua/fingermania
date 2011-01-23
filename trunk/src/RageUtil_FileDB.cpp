#include "global.h"

#include "RageUtil_FileDB.h"
#include "RageUtil.h"
#include "RageLog.h"

void FileSet::GetFilesMatching(const RString& sBeginning_, const RString& sContaining_, const RString& sEnding_, vector<RString>& asOut, bool bOnlyDirs) const
{
	RString sBeginning = sBeginning_;
	sBeginning.MakeLower();
	RString sContaining = sContaining_;
	sContaining.MakeLower();
	RString sEnding = sEnding_;
	sEnding.MakeLower();

	set<File>::const_iterator i = files.lower_bound(File(sBeginning));
	for (; i != files.end(); ++i)
	{
		const File& f = *i;
		if (bOnlyDirs && !f.dir)
			continue;

		const RString& sPath = f.lname;

		if (sBeginning.size() > sPath.size())
			break;
		if (sPath.compare(0, sBeginning.size(), sBeginning))
			break;

		int end_pos = int(sPath.size() - int(sEnding.size()));

		if (end_pos < 0)
			continue;
		if (sPath.compare(end_pos, string::npos, sEnding))
			continue;

		if (!sContaining_.empty())
		{
			size_t pos = sPath.find(sContaining, sBeginning.size());
			if (pos == sPath.npos)
				continue;
			if (pos + sContaining.size() > unsigned(end_pos))
				continue;
		}

		asOut.push_back(f.name);
	}
}

void FileSet::GetFilesEqualTo(const RString& sStr, vector<RString>& asOut, bool bOnlyDirs) const
{
	set<File>::const_iterator i = files.find(File(sStr));
	if (i == files.end())
		return;

	if (bOnlyDirs && !i->dir)
		return;

	asOut.push_back(i->name);
}

RageFileManager::FileType FileSet::GetFileType(const RString& sPath) const
{
	set<File>::const_iterator i = files.find(File(sPath));
	if (i == files.end())
		return RageFileManager::TYPE_NONE;

	return i->dir ? RageFileManager::TYPE_DIR:RageFileManager::TYPE_FILE;
}

int FileSet::GetFileSize(const RString& sPath) const
{
	set<File>::const_iterator i = files.find(File(sPath));
	if (i == files.end())
		return -1;
	return i->size;
}

int FileSet::GetFileHash(const RString& sPath) const
{
	set<File>::const_iterator i = files.find(File(sPath));
	if (i == files.end())
		return -1;
	return i->hash + i->size;
}

static void SplitPath(RString sPath, RString& sDir, RString& sName)
{
	CollapsePath(sPath);
	if (sPath.Right(1) == "/")
		sPath.erase(sPath.size() - 1);

	size_t iSep = sPath.find_last_of('/');
	if (iSep == RString::npos)
	{
		sDir = "";
		sName = sPath;
	}
	else
	{
		sDir = sPath.substr(0, iSep + 1);
		sName = sPath.substr(iSep + 1);
	}
}

RageFileManager::FileType FilenameDB::GetFileType(const RString& sPath)
{
	ASSERT(!m_Mutex.IsLockedByThisThread());

	RString sDir, sName;
	SplitPath(sPath, sDir, sName);

	if (sName == "/")
		return RageFileManager::TYPE_DIR;

	const FileSet* fs = GetFileSet(sDir);
	RageFileManager::FileType ret = fs->GetFileType(sName);
	m_Mutex.Unlock();
	return ret;
}

int FilenameDB::GetFileSize(const RString& sPath)
{
	ASSERT(!m_Mutex.IsLockedByThisThread());

	RString sDir, sName;
	SplitPath(sPath, sDir, sName);

	const FileSet* fs = GetFileSet(sDir);
	int ret = fs->GetFileSize(sName);
	m_Mutex.Unlock();
	return ret;
}

int FilenameDB::GetFileHash(const RString& sPath)
{
	ASSERT(!m_Mutex.IsLockedByThisThread());

	RString sDir, sName;
	SplitPath(sPath, sDir, sName);

	const FileSet* fs = GetFileSet(sDir);
	int ret = fs->GetFileSize(sName);
	m_Mutex.Unlock();
	return ret;
}

bool FilenameDB::ResolvePath(RString& sPath)
{
	if (sPath == "/" || sPath == "")
		return;

	int iBegin = 0, iSize = -1;

	RString ret = "";
	const FileSet* fs = NULL;

	static const RString slash("/");
	while (1)
	{
		split(sPath, slash, iBegin, iSize, true);
		if (iBegin == (int)sPath.size())
			break;

		if (fs == NULL)
			fs = GetFileSet(ret);
		else
			m_Mutex.Lock();

		RString p = sPath.substr(iBegin, iSize);
		ASSERT_M(p.size() != 1 || p[0] != '.', sPath);
		ASSERT_M(p.size() != 2 || p[0] != '.' || p[1] != '.', sPath);
		set<File>::const_iterator it = fs->files.find(File(p));

		if (it == fs->files.end())
		{
			m_Mutex.Unlock();
			return false;
		}

		ret += "/" + it->name;

		fs = it->dirp;

		m_Mutex.Unlock();
	}

	if (sPath.size() && sPath[sPath.size() - 1] == '/')
		sPath = ret + "/";
	else
		sPath = ret;

	return true;
}

void FilenameDB::GetFilesMatching(const RString& sDir, const RString& sBeginning, const RString& sContaining, const RString& sEnding, vector<RString>& asOut, bool bOnlyDirs)
{
	ASSERT(!m_Mutex.IsLockedByThisThread());

	const FileSet* fs = GetFileSet(sDir);
	fs->GetFilesMatching(sBeginning, sContaining, sEnding, asOut, bOnlyDirs);
	m_Mutex.Unlock();
}

void FilenameDB::GetFilesEqualTo(const RString& sDir, const RString& sFile, vector<RString>& asOut, bool bOnlyDirs)
{
	ASSERT(!m_Mutex.IsLockedByThisThread());

	const FileSet* fs = GetFileSet(sDir);
	fs->GetFilesEqualTo(sFile, asOut, bOnlyDirs);
	m_Mutex.Unlock();
}

void FilenameDB::GetFilesSimpleMatch(const RString& sDir, const RString& sMask, vector<RString>& asOut, bool bOnlyDirs)
{
	size_t first_pos = sMask.find_first_of('*');
	if (first_pos == sMask.npos)
	{
		GetFilesEqualTo(sDir, sMask, asOut, bOnlyDirs);
		return;
	}

	size_t second_pos = sMask.find_first_of('*', first_pos + 1);
	if (second_pos == sMask.npos)
	{
		GetFilesMatching(sDir, sMask.substr(0, first_pos), RString(), sMask.substr(first_pos + 1), asOut, bOnlyDirs);
		return;
	}

	GetFilesMatching(sDir,
		sMask.substr(0, first_pos),
		sMask.substr(first_pos + 1, second_pos - first_pos - 1),
		sMask.substr(second_pos + 1), asOut, bOnlyDirs);
}

FileSet* FilenameDB::GetFileSet(const RString& sDir_, bool bCreate)
{
	RString sDir = sDir_;

	if (bCreate && m_Mutex.IsLockedByThisThread() && LOG)
		LOG_Warn("FilenameDB::GetFileSet: m_Mutex was locked");

	sDir.Replace("\\", "/");
	sDir.Replace("//", "/");

	if (sDir == "")
		sDir = "/";

	RString sLower = sDir;
	sLower.MakeLower();

	m_Mutex.Lock();

	while(1)
	{
		map<RString, FileSet*>::iterator i = dirs.find(sLower);
		if (!bCreate)
		{
			if (i == dirs.end())
				return NULL;
			return i->second;
		}

		if (i == dirs.end())
			break;

		FileSet* pFileSet = i->second;
		if (!pFileSet->m_bFilled)
		{
			m_Mutex.Wait();

			continue;
		}

		if (ExpireSeconds == -1 || pFileSet->age.PeekDeltaTime() < ExpireSeconds)
		{
			return pFileSet;
		}

		this->DelFileSet(i);
		break;
	}

	FileSet* pRet = new FileSet;
	pRet->m_bFilled = false;
	dirs[sLower] = pRet;

	m_Mutex.Unlock();
	ASSERT(!m_Mutex.IsLockedByThisThread());
	PopulateFileSet(*pRet, sDir);

	FileSet** pParentDirp = NULL;
	if (sDir != "/")
	{
		RString sParent = Dirname(sDir);
		if (sParent == "./")
			sParent = "";

		FileSet* pParent = GetFileSet(sParent);
		if (pParent != NULL)
		{
			set<File>::iterator it = pParent->files.find(File(Basename(sDir)));
			if (it != pParent->files.end())
				pParentDirp = const_cast<FileSet**>(&it->dirp);
		}
	}
	else
	{
		m_Mutex.Lock();
	}

	if (pParentDirp != NULL)
		*pParentDirp = pRet;

	pRet->age.Touch();
	pRet->m_bFilled = true;

	m_Mutex.Broadcast();

	return pRet;
}

void FilenameDB::AddFile(const RString& sPath_, int iSize, int iHash, void* pPriv)
{
	RString sPath(sPath_);

	if (sPath == "" || sPath == "/")
		return;

	if (sPath[0] != '/')
		sPath = "/" + sPath;

	vector<RString> asParts;
	split(sPath, "/", asParts, false);

	vector<RString>::const_iterator begin = asParts.begin();
	vector<RString>::const_iterator end = asParts.end();

	bool IsDir = false;
	if (sPath[sPath.size() - 1] != '/')
		IsDir = false;
	else
		--end;

	++begin;

	do
	{
		RString dir = "/" + join("/", begin, end - 1);
		if (dir != "/")
			dir += "/";
		const RString &fn = *(end - 1);
		FileSet* fs = GetFileSet(dir);
		ASSERT(m_Mutex.IsLockedByThisThread());

		File &f = const_cast<File&>(*fs->files.insert( fn ).first);
		f.dir = IsDir;
		if (!IsDir)
		{
			f.size = iSize;
			f.hash = iHash;
			f.priv = pPriv;
		}
		m_Mutex.Unlock();
		IsDir = true;
		--end;
	} while (begin != end);
}

void FilenameDB::DelFileSet(map<RString, FileSet*>::iterator dir)
{
	ASSERT(m_Mutex.IsLockedByThisThread());

	if (dir == dirs.end())
		return;

	FileSet* fs = dir->second;

	for (map<RString, FileSet*>::iterator it = dirs.begin(); it != dirs.end(); ++it)
	{
		FileSet* Clean = it->second;
		for (set<File>::iterator f = Clean->files.begin(); f != Clean->files.end(); ++f)
		{
			File& ff = (File&)*f;
			if (ff.dirp == fs)
				ff.dirp = NULL;
		}
	}

	delete fs;
	dirs.erase(dir);
}

void FilenameDB::DelFile(const RString& sPath)
{
	LockMut(m_Mutex);
	RString lower = sPath;
	lower.MakeLower();

	map<RString, FileSet*>::iterator fsi = dirs.find(lower);
	DelFileSet(fsi);

	RString Dir, Name;
	SplitPath(sPath, Dir, Name);
	FileSet* Parent = GetFileSet(Dir, false);
	if (Parent)
		Parent->files.erase(Name);
	m_Mutex.Unlock();
}

void FilenameDB::FlushDirCache(const RString& sDir)
{
	FileSet* pFileSet = NULL;
	m_Mutex.Lock();

	while(true)
	{
		if (dirs.empty())
			break;

		pFileSet = dirs.begin()->second;

		dirs.erase(dirs.begin());

		while (!pFileSet->m_bFilled)
			m_Mutex.Wait();
		delete pFileSet;
	}

#if 0
	{
		if (it != dirs.end())
		{
			pFileSet = it->second;
			dirs.erase(it);
			while (!pFileSet->m_bFilled)
				m_Mutex.Wait();
			delete pFileSet;

			if (sDir != "/")
			{
				RString sParent = Dirname(sDir);
				if (sParent == "./")
					sParent = "";
				sParent.MakeLower();
				it = dirs.find(sParent);
				if (it != dirs.end())
				{
					FileSet *pParent = it->second;
					set<File>::iterator fileit = pParent->files.find(File(Basename(sDir)));
					if (fileit != pParent->files.end())
						fileit->dirp = NULL;
				}
			}
		}
		else
		{
			LOG_Warn("Trying to flush an unknown directory %s.", sDir.c_str());
		}
	}
#endif

	m_Mutex.Unlock();
}

const File* FilenameDB::GetFile(const RString& sPath)
{
	if (m_Mutex.IsLockedByThisThread() && LOG)
		LOG->Warn("FilenameDB::GetFile: m_Mutex was locked");

	RString Dir, Name;
	SplitPath(sPath, Dir, Name);
	FileSet* fs = GetFileSet(Dir);

	set<File>::iterator it;
	it = fs->files.find(File(Name));
	if (it == fs->files.end())
		return NULL;

	return &*it;
}

void* FilenameDB::GetFilePriv(const RString& path)
{
	ASSERT(!m_Mutex.IsLockedByThisThread());

	const File* pFile = GetFile(path);
	void* pRet = NULL;
	if (pFile != NULL)
		pRet = pFile->priv;

	m_Mutex.Unlock();
	return pRet;
}

void FilenameDB::GetDirListing(const RString& sPath_, vector<RString>& asAddTo, bool bOnlyDirs, bool bReturnPathToo)
{
	RString sPath = sPath_;

	ASSERT(!sPath.empty());

	size_t pos = sPath.find_last_of('/');
	RString fn;
	if (pos == sPath.npos)
	{
		fn = sPath;
		sPath = "";
	}
	else
	{
		fn = sPath.substr(pos + 1);
		sPath = sPath.substr(0, pos + 1);
	}

	if (fn.size() == 0)
		fn = "*";

	unsigned iStart = asAddTo.size();
	GetFilesSimpleMatch(sPath, fn, asAddTo, bOnlyDirs);

	if (bReturnPathToo && iStart < asAddTo.size())
	{
		while (iStart < asAddTo.size())
		{
			asAddTo[iStart].insert(0, sPath);
			iStart++;
		}
	}
}

void FilenameDB::GetFileSetCopy(const RString& sDir, FileSet& out)
{
	FileSet* pFileSet = GetFileSet(sDir);
	out = *pFileSet;
	m_Mutex.Unlock();
}

void FilenameDB::CacheFile(const RString& sPath)
{
	LOG->Warn("Slow cache due to: %s", sPath.c_str());
	FlushDirCache(Dirname(sPath));
}
