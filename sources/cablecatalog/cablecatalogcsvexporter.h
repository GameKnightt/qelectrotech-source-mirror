/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef CABLECATALOGCSVEXPORTER_H
#define CABLECATALOGCSVEXPORTER_H

#include "cablecatalogtypes.h"

class CableCatalogCsvExporter
{
	public:
		static QString toCsv(const CableCatalogSnapshot &snapshot,
						 bool include_unassigned = false);
};

#endif // CABLECATALOGCSVEXPORTER_H
