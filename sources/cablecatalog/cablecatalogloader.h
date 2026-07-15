/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef CABLECATALOGLOADER_H
#define CABLECATALOGLOADER_H

#include "cablecatalogtypes.h"

#include <QHash>
#include <QPointer>

class Conductor;
class QETProject;

struct CableCatalogLoadResult
{
	CableCatalogSnapshot snapshot;
	QHash<quint64, QPointer<Conductor>> live_targets;
	bool ok = true;
	QString diagnostic;
};

class CableCatalogLoader
{
	public:
		static CableCatalogLoadResult load(QETProject *project);
};

#endif // CABLECATALOGLOADER_H
