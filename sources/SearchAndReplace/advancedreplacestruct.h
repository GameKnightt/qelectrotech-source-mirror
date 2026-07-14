/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef ADVANCEDREPLACESTRUCT_H
#define ADVANCEDREPLACESTRUCT_H

#include <QString>

struct advancedReplaceStruct
{
	// 0: diagram, 1: element, 2: conductor, 3: independent text
	int who = -1;
	QString what;
	QString search;
	QString replace;
};

#endif // ADVANCEDREPLACESTRUCT_H
