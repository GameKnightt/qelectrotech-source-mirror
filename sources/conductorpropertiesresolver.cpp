/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorpropertiesresolver.h"

ConductorProperties resolveConductorProperties(
	const ConductorProperties &requested,
	const ConductorFormulaResolver &formulaResolver)
{
	ConductorProperties resolved = requested;
	if (resolved.m_formula.isEmpty()) return resolved;

	if (formulaResolver) {
		resolved.text = formulaResolver(resolved.m_formula);
	} else if (resolved.text.isEmpty()) {
		resolved.text = resolved.m_formula;
	}
	return resolved;
}
