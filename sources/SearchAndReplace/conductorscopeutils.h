/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORSCOPEUTILS_H
#define CONDUCTORSCOPEUTILS_H

#include <QSet>
#include <QVector>

template<typename T>
QVector<T> takeUnvisitedConductorScopeMembers(
	const QVector<T> &members,
	QSet<T> &visited)
{
	QVector<T> unique_members;
	unique_members.reserve(members.size());
	QSet<T> seen_in_scope;
	for (const T &member : members)
	{
		if (seen_in_scope.contains(member)) continue;
		seen_in_scope.insert(member);
		if (visited.contains(member)) continue;
		visited.insert(member);
		unique_members.append(member);
	}
	return unique_members;
}

#endif // CONDUCTORSCOPEUTILS_H
