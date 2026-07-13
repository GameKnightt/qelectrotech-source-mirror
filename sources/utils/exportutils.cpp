/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "exportutils.h"

#include <QDir>
#include <QFileDevice>
#include <QSaveFile>
#include <QTextStream>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

QString ExportUtils::csvField(QString value, QChar separator)
{
	if (value.contains(separator) || value.contains(QLatin1Char('"'))
		|| value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r')))
	{
		value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
		return QLatin1Char('"') + value + QLatin1Char('"');
	}
	return value;
}

QString ExportUtils::normalizedPathKey(
	const QString &base_directory,
	const QString &path)
{
	QString key = QDir::cleanPath(QDir(base_directory).absoluteFilePath(path));
#ifdef Q_OS_WIN
	key = key.toCaseFolded();
#endif
	return key;
}

bool ExportUtils::writeTextAtomically(
	const QString &file_path,
	const QString &contents,
	QString *error_message)
{
	if (error_message) error_message->clear();

	QSaveFile file(file_path);
	file.setDirectWriteFallback(false);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		if (error_message) *error_message = file.errorString();
		return false;
	}

	QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	stream.setCodec("UTF-8");
#else
	stream.setEncoding(QStringConverter::Utf8);
#endif
	stream.setGenerateByteOrderMark(false);
	stream << contents;
	stream.flush();
	if (stream.status() != QTextStream::Ok
		|| file.error() != QFileDevice::NoError)
	{
		if (error_message) *error_message = file.errorString();
		file.cancelWriting();
		return false;
	}

	if (!file.commit()) {
		if (error_message) *error_message = file.errorString();
		return false;
	}
	return true;
}
