/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef APPLICATION_THEME_H
#define APPLICATION_THEME_H

#include <QColor>
#include <QPalette>
#include <QString>

class QApplication;

/**
	@brief Central semantic theme for the Qt Widgets interface.

	The class keeps the native/system surfaces and typography, then adds a
	small accessible accent and consistent control spacing. It deliberately
	avoids replacing the platform style or document-rendering colours.
*/
class ApplicationTheme final
{
	public:
		static QColor accentColor();
		static QPalette accentedPalette(const QPalette &base_palette);
		static QString styleSheet(
				const QPalette &palette,
				bool use_custom_accent = true);
		static void apply(
				QApplication *application,
				const QPalette &base_palette,
				const QString &custom_style_sheet = QString(),
				bool use_custom_accent = true);

	private:
		static QColor blend(
				const QColor &foreground,
				const QColor &background,
				qreal foreground_ratio);
};

#endif // APPLICATION_THEME_H
