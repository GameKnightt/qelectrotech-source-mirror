/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "applicationtheme.h"

#include <QApplication>
#include <QtMath>

QColor ApplicationTheme::accentColor()
{
	return QColor(QStringLiteral("#176b5b"));
}

QColor ApplicationTheme::blend(
		const QColor &foreground,
		const QColor &background,
		qreal foreground_ratio)
{
	const qreal ratio = qBound<qreal>(0.0, foreground_ratio, 1.0);
	const auto channel = [ratio](int foreground_value, int background_value) {
		return qRound(
				foreground_value * ratio
				+ background_value * (1.0 - ratio));
	};
	return QColor(
			channel(foreground.red(), background.red()),
			channel(foreground.green(), background.green()),
			channel(foreground.blue(), background.blue()));
}

QPalette ApplicationTheme::accentedPalette(const QPalette &base_palette)
{
	QPalette palette(base_palette);
	const QColor accent = accentColor();
	const QColor window = base_palette.color(QPalette::Window);
	const QColor link_accent =
		window.lightness() < 128 ? accent.lighter(155) : accent;
	const QColor disabled_accent = blend(accent, window, 0.38);
	const QColor on_accent(Qt::white);

	for (QPalette::ColorGroup group : {
			QPalette::Active,
			QPalette::Inactive}) {
		palette.setColor(group, QPalette::Highlight, accent);
		palette.setColor(group, QPalette::HighlightedText, on_accent);
		palette.setColor(group, QPalette::Link, link_accent);
		palette.setColor(
			group,
			QPalette::LinkVisited,
			link_accent.darker(110));
	}
	palette.setColor(QPalette::Disabled, QPalette::Highlight, disabled_accent);
	palette.setColor(
			QPalette::Disabled,
			QPalette::HighlightedText,
			base_palette.color(QPalette::Disabled, QPalette::Text));

	return palette;
}

QString ApplicationTheme::styleSheet(
		const QPalette &palette,
		bool use_custom_accent)
{
	const QColor accent = use_custom_accent
		? accentColor()
		: palette.color(QPalette::Active, QPalette::Highlight);
	const QColor window = palette.color(QPalette::Window);
	const QColor button = palette.color(QPalette::Button);
	const QColor border = palette.color(QPalette::Mid);
	const bool dark = window.lightness() < 128;
	const QColor accent_hover = dark
		? accent.lighter(132)
		: accent.darker(108);
	const QColor focus_accent = dark ? accent.lighter(155) : accent;
	const QColor accent_soft = blend(accent, window, dark ? 0.24 : 0.12);
	const QColor button_hover = blend(accent, button, dark ? 0.18 : 0.08);

	return QStringLiteral(
		"QPushButton {"
		" min-height: 28px; padding: 4px 12px;"
		" border: 1px solid %1; border-radius: 4px;"
		" background-color: palette(button); }"
		"QPushButton:hover {"
		" border-color: %2; background-color: %3; }"
		"QPushButton:focus { border: 2px solid %6; padding: 3px 11px; }"
		"QPushButton:default, QPushButton[primaryAction=\"true\"] {"
		" border-color: %4; background-color: %4;"
		" color: palette(highlighted-text); }"
		"QPushButton:default:hover, QPushButton[primaryAction=\"true\"]:hover {"
		" border-color: %2; background-color: %2; }"
		"QPushButton:disabled { color: palette(mid); }"

		"QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox,"
		" QDateEdit, QTimeEdit, QDateTimeEdit,"
		" QTextEdit, QPlainTextEdit {"
		" min-height: 26px; padding: 3px 6px;"
		" border: 1px solid %1; border-radius: 4px;"
		" background-color: palette(base);"
		" selection-background-color: %4;"
		" selection-color: palette(highlighted-text); }"
		"QLineEdit:focus, QComboBox:focus, QSpinBox:focus,"
		" QDoubleSpinBox:focus, QDateEdit:focus, QTimeEdit:focus,"
		" QDateTimeEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {"
		" border: 2px solid %6; padding: 2px 5px; }"

		"QAbstractItemView {"
		" border: 1px solid %1; border-radius: 4px;"
		" selection-background-color: %4;"
		" selection-color: palette(highlighted-text); }"
		"QAbstractItemView:focus { border: 2px solid %6; }"
		"QAbstractItemView::item { padding: 3px 5px; }"
		"QAbstractItemView::item:hover { background-color: %5; }"
		"QAbstractItemView::item:selected {"
		" background-color: %4; color: palette(highlighted-text); }"
		"QHeaderView::section {"
		" padding: 6px 8px; border: 0;"
		" border-right: 1px solid %1; border-bottom: 1px solid %1;"
		" background-color: palette(alternate-base); font-weight: 600; }"

		"QGroupBox {"
		" margin-top: 10px; padding: 10px 8px 8px 8px;"
		" border: 1px solid %1; border-radius: 5px; }"
		"QGroupBox::title {"
		" subcontrol-origin: margin; left: 10px; padding: 0 4px;"
		" color: palette(text); font-weight: 600; }"

		"QTabWidget::pane {"
		" border: 1px solid %1; border-radius: 4px; top: -1px; }"
		"QTabBar::tab {"
		" min-height: 26px; padding: 5px 11px;"
		" border-bottom: 2px solid transparent; }"
		"QTabBar::tab:hover { background-color: %5; }"
		"QTabBar::tab:selected {"
		" border-bottom-color: %6; color: %6; font-weight: 600; }"

		"QMenuBar::item { padding: 5px 8px; border-radius: 4px; }"
		"QMenuBar::item:selected { background-color: %5; }"
		"QMenu { padding: 4px; }"
		"QMenu::item { padding: 6px 24px 6px 28px; border-radius: 3px; }"
		"QMenu::item:selected {"
		" background-color: %4; color: palette(highlighted-text); }"
		"QMenu::separator { height: 1px; margin: 4px 8px; background: %1; }"

		"QToolButton { border: 1px solid transparent; border-radius: 4px; }"
		"QToolButton:hover { border-color: %1; background-color: %5; }"
		"QToolButton:checked { border-color: %4; background-color: %5; }"
		"QToolButton:focus { border: 2px solid %6; }"
		"QDockWidget::title {"
		" padding: 6px 8px; border-left: 3px solid %6;"
		" background-color: palette(alternate-base); font-weight: 600; }"
		"QStatusBar { border-top: 1px solid %1; }"

		"QListWidget#configPagesList {"
		" border: 0; border-right: 1px solid %1; border-radius: 0;"
		" background-color: palette(window); }"
		"QListWidget#configPagesList:focus {"
		" border: 0; border-right: 1px solid %1; }"
		"QListWidget#configPagesList::item {"
		" margin: 2px 8px; padding: 8px 10px; border-radius: 5px; }"
		"QListWidget#configPagesList::item:selected {"
		" background-color: %5; color: palette(text);"
		" border-left: 3px solid %6; font-weight: 600; }"

		"QFrame#onboardingAccentLine { background-color: %4; }"
		"QFrame[onboardingCard=\"true\"] {"
		" border: 1px solid %1; border-radius: 6px;"
		" background-color: palette(base); }"
		"QProgressBar#onboardingProgress {"
		" min-height: 7px; max-height: 7px; border: 0;"
		" border-radius: 3px; background-color: %5; text-align: center; }"
		"QProgressBar#onboardingProgress::chunk {"
		" border-radius: 3px; background-color: %6; }"

		"QToolTip {"
		" padding: 5px 7px; border: 1px solid %1; border-radius: 3px;"
		" background-color: palette(base); color: palette(text); }")
		.arg(
			border.name(QColor::HexRgb),
			accent_hover.name(QColor::HexRgb),
			button_hover.name(QColor::HexRgb),
			accent.name(QColor::HexRgb),
			accent_soft.name(QColor::HexRgb),
			focus_accent.name(QColor::HexRgb));
}

void ApplicationTheme::apply(
		QApplication *application,
		const QPalette &base_palette,
		const QString &custom_style_sheet,
		bool use_custom_accent)
{
	if (!application) return;

	const QPalette palette = use_custom_accent
		? accentedPalette(base_palette)
		: base_palette;
	application->setPalette(palette);
	QString combined_style_sheet = styleSheet(palette, use_custom_accent);
	const QString custom_rules = custom_style_sheet.trimmed();
	if (!custom_rules.isEmpty()) {
		combined_style_sheet.append(QLatin1Char('\n'));
		combined_style_sheet.append(custom_rules);
	}
	application->setStyleSheet(combined_style_sheet);
}
