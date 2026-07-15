/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorchangeplan.h"

ConductorChangePlan::Result ConductorChangePlan::executeAtomically(
	bool hasChanges,
	const Validation &validation,
	const Application &application)
{
	if (!hasChanges) return {Code::NoChanges, -1, -1};
	if (!validation || !application) {
		return {Code::InvalidTransform, -1, -1};
	}

	const Result check = validation();
	if (!check.canApply()) return check;
	return application()
		? Result{}
		: Result{Code::ApplyFailed, -1, -1};
}
