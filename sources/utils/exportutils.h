/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef EXPORT_UTILS_H
#define EXPORT_UTILS_H

#include <QByteArray>
#include <QString>

namespace ExportUtils {

	QString csvField(QString value, QChar separator = QLatin1Char(';'));
	QString normalizedPathKey(
		const QString &base_directory,
		const QString &path);
	bool writeTextAtomically(
		const QString &file_path,
		const QString &contents,
		QString *error_message = nullptr);
	bool writeBytesAtomically(
		const QString &file_path,
		const QByteArray &contents,
		QString *error_message = nullptr);

}

#endif
