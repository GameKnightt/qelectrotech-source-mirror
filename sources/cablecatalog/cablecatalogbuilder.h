/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef CABLECATALOGBUILDER_H
#define CABLECATALOGBUILDER_H

#include "cablecatalogtypes.h"

class CableCatalogBuilder
{
	public:
		static CableCatalogSnapshot build(
				const QVector<CableConductorSnapshot> &conductors);
		static QString cableIdentity(const QString &reference);
		static QString fuzzyReference(const QString &reference);
		static bool naturalLessThan(const QString &left, const QString &right);
};

#endif // CABLECATALOGBUILDER_H
