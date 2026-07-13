/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef ELEMENTCOLLECTIONROLES_H
#define ELEMENTCOLLECTIONROLES_H

#include <Qt>

namespace ElementCollectionRoles
{
	enum Role {
		SearchTextRole = Qt::UserRole + 1,
		CollectionPathRole,
		IsElementRole
	};
}

#endif // ELEMENTCOLLECTIONROLES_H
