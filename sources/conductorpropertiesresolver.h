/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORPROPERTIESRESOLVER_H
#define CONDUCTORPROPERTIESRESOLVER_H

#include "conductorproperties.h"

#include <functional>

using ConductorFormulaResolver = std::function<QString(const QString &)>;

ConductorProperties resolveConductorProperties(
		const ConductorProperties &requested,
		const ConductorFormulaResolver &formulaResolver);

#endif // CONDUCTORPROPERTIESRESOLVER_H
