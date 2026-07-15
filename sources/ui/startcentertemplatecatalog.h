/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef START_CENTER_TEMPLATE_CATALOG_H
#define START_CENTER_TEMPLATE_CATALOG_H

#include <QString>
#include <QStringList>
#include <QVector>

struct StartCenterTemplateEntry
{
	QString id;
	QString title;
	QString discipline;
	QString description;
	QString file_name;
	int folio_count = 0;
};

class StartCenterTemplateCatalog
{
	public:
		explicit StartCenterTemplateCatalog(
			const QStringList &roots = defaultRoots());

		static QVector<StartCenterTemplateEntry> curatedEntries();
		static QStringList defaultRoots();

		void setRoots(const QStringList &roots);
		QStringList roots() const;
		QString resolvedPath(const QString &id) const;
		bool isAvailable(const QString &id) const;

	private:
		QString safePathUnderRoot(
			const QString &root,
			const QString &file_name) const;

		QStringList m_roots;
};

#endif
