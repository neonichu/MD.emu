#pragma once

#include <fs/sys.hh>
#include <util/basicString.h>
#include <util/collection/DLList.hh>

struct RecentGameInfo
{
	FsSys::cPath path;
	char name[256];

	bool operator ==(RecentGameInfo const& rhs) const
	{
		return string_equal(path, rhs.path);
	}
};

extern DLList<RecentGameInfo> recentGameList;

void recent_addGame(const char *fullPath, const char *name);
