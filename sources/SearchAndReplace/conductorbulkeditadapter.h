/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORBULKEDITADAPTER_H
#define CONDUCTORBULKEDITADAPTER_H

#include "conductorbulkeditmodel.h"

class ConductorChangePlan;

QVector<ConductorBulkEditModel::Row> conductorBulkEditRows(
	const ConductorChangePlan &plan);

#endif // CONDUCTORBULKEDITADAPTER_H
