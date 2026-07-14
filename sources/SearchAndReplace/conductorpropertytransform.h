/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORPROPERTYTRANSFORM_H
#define CONDUCTORPROPERTYTRANSFORM_H

#include "../conductorproperties.h"
#include "advancedreplacestruct.h"

inline QString conductorPropertyEraseText()
{
	return QStringLiteral("XXXXXXXXXXXXXXXXXXX");
}

ConductorProperties applyConductorPropertyPatch(
		const ConductorProperties &original,
		const ConductorProperties &patch);
ConductorProperties applyAdvancedConductorReplacement(
		const ConductorProperties &original,
		const advancedReplaceStruct &replacement);
ConductorProperties transformConductorProperties(
		const ConductorProperties &original,
		const ConductorProperties &patch,
		const advancedReplaceStruct &advanced,
		bool includePropertyPatch,
		bool includeAdvancedReplacement);

#endif // CONDUCTORPROPERTYTRANSFORM_H
