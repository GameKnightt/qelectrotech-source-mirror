/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "startcentertemplatecatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

StartCenterTemplateCatalog::StartCenterTemplateCatalog(
	const QStringList &roots) :
	m_roots(roots)
{
}

QVector<StartCenterTemplateEntry>
StartCenterTemplateCatalog::curatedEntries()
{
	return {
		{
			QStringLiteral("arduino_lcd"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog", "Arduino et écran LCD"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog", "Électronique"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog",
				"Découvrir un montage de commande et d'affichage."),
			QStringLiteral("ArduinoLCD.qet"),
			3
		},
		{
			QStringLiteral("grafcet"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog", "Séquence GRAFCET"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog", "Automatisme"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog",
				"Explorer une séquence d'automatisme sur plusieurs folios."),
			QStringLiteral("grafcet.qet"),
			3
		},
		{
			QStringLiteral("habitat_unifilaire"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog", "Habitat unifilaire"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog", "Résidentiel"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog",
				"Partir d'un schéma électrique d'habitation."),
			QStringLiteral("Habitat-Unifilaire.qet"),
			1
		},
		{
			QStringLiteral("industrial"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog", "Installation industrielle"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog", "Industriel"),
			QCoreApplication::translate(
				"StartCenterTemplateCatalog",
				"Parcourir un projet complet de 50 folios."),
			QStringLiteral("industrial.qet"),
			50
		}
	};
}

QStringList StartCenterTemplateCatalog::defaultRoots()
{
	const QString application_dir = QCoreApplication::applicationDirPath();
	return {
		QDir::cleanPath(application_dir + QStringLiteral("/examples")),
		QDir::cleanPath(application_dir + QStringLiteral("/../examples")),
		QDir::cleanPath(
			application_dir + QStringLiteral("/../share/qelectrotech/examples")),
		QDir::cleanPath(
			application_dir + QStringLiteral("/../Resources/examples"))
	};
}

void StartCenterTemplateCatalog::setRoots(const QStringList &roots)
{
	m_roots = roots;
}

QStringList StartCenterTemplateCatalog::roots() const
{
	return m_roots;
}

QString StartCenterTemplateCatalog::resolvedPath(const QString &id) const
{
	const auto entries = curatedEntries();
	for (const StartCenterTemplateEntry &entry : entries) {
		if (entry.id != id) continue;
		for (const QString &root : m_roots) {
			const QString path = safePathUnderRoot(root, entry.file_name);
			if (!path.isEmpty()) return path;
		}
		break;
	}
	return {};
}

bool StartCenterTemplateCatalog::isAvailable(const QString &id) const
{
	return !resolvedPath(id).isEmpty();
}

QString StartCenterTemplateCatalog::safePathUnderRoot(
	const QString &root,
	const QString &file_name) const
{
	if (root.isEmpty()
		|| file_name.isEmpty()
		|| QFileInfo(file_name).isAbsolute()
		|| file_name != QFileInfo(file_name).fileName()
		|| QFileInfo(file_name).suffix().compare(
			QStringLiteral("qet"), Qt::CaseInsensitive) != 0) {
		return {};
	}

	const QFileInfo root_info(root);
	const QString canonical_root = root_info.canonicalFilePath();
	if (canonical_root.isEmpty() || !root_info.isDir()) return {};

	const QFileInfo file_info(QDir(canonical_root).filePath(file_name));
	const QString canonical_file = file_info.canonicalFilePath();
	if (canonical_file.isEmpty()
		|| !file_info.isFile()
		|| !file_info.isReadable()) {
		return {};
	}

	const QString normalized_root = QDir::fromNativeSeparators(canonical_root);
	const QString normalized_file = QDir::fromNativeSeparators(canonical_file);
	const QString root_prefix = normalized_root + QLatin1Char('/');
#ifdef Q_OS_WIN
	const Qt::CaseSensitivity path_case = Qt::CaseInsensitive;
#else
	const Qt::CaseSensitivity path_case = Qt::CaseSensitive;
#endif
	if (!normalized_file.startsWith(root_prefix, path_case)) return {};
	return canonical_file;
}
