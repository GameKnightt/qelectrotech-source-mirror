/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef TERMINALSTRIPOVERVIEWLOADER_H
#define TERMINALSTRIPOVERVIEWLOADER_H

#include "terminalstripoverviewmodel.h"

class QETProject;

class TerminalStripOverviewLoader
{
	public:
		static QVector<TerminalStripOverviewRow> rowsForProject(QETProject *project);
};

#endif // TERMINALSTRIPOVERVIEWLOADER_H
