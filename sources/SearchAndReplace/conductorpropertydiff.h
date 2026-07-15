/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORPROPERTYDIFF_H
#define CONDUCTORPROPERTYDIFF_H

#include "../conductorproperties.h"

#include <QString>
#include <QVector>

struct ConductorPropertyChange
{
	QString key;
	QString label;
	QString before;
	QString after;
};

QVector<ConductorPropertyChange> conductorPropertyChanges(
		const ConductorProperties &before,
		const ConductorProperties &after);

#endif // CONDUCTORPROPERTYDIFF_H
