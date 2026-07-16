/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/ui/applicationtheme.h"

#include <QApplication>
#include <QtTest>

#include <cmath>

namespace {

qreal linearChannel(int channel)
{
	const qreal value = channel / 255.0;
	return value <= 0.04045
		? value / 12.92
		: std::pow((value + 0.055) / 1.055, 2.4);
}

qreal luminance(const QColor &color)
{
	return 0.2126 * linearChannel(color.red())
		+ 0.7152 * linearChannel(color.green())
		+ 0.0722 * linearChannel(color.blue());
}

qreal contrastRatio(const QColor &first, const QColor &second)
{
	const qreal lighter = qMax(luminance(first), luminance(second));
	const qreal darker = qMin(luminance(first), luminance(second));
	return (lighter + 0.05) / (darker + 0.05);
}

}

class ApplicationThemeTest : public QObject
{
	Q_OBJECT

	private slots:
		void accentMeetsTextContrastRequirement();
		void paletteKeepsSurfacesAndAddsSemanticAccent();
		void systemPaletteModePreservesTheSystemHighlight();
		void styleSheetUsesConsistentFocusAndPrimaryActionRules();
		void applyPreservesCustomRulesAndRemovesLegacyInvalidSyntax();
};

void ApplicationThemeTest::accentMeetsTextContrastRequirement()
{
	QVERIFY(
		contrastRatio(ApplicationTheme::accentColor(), QColor(Qt::white))
		>= 4.5);
}

void ApplicationThemeTest::paletteKeepsSurfacesAndAddsSemanticAccent()
{
	QPalette base;
	base.setColor(QPalette::Window, QColor(QStringLiteral("#f4f4f4")));
	base.setColor(QPalette::Button, QColor(QStringLiteral("#eeeeee")));
	const QPalette themed = ApplicationTheme::accentedPalette(base);

	QCOMPARE(themed.color(QPalette::Window), base.color(QPalette::Window));
	QCOMPARE(themed.color(QPalette::Button), base.color(QPalette::Button));
	QCOMPARE(
		themed.color(QPalette::Active, QPalette::Highlight),
		ApplicationTheme::accentColor());
	QCOMPARE(
		themed.color(QPalette::Inactive, QPalette::Highlight),
		ApplicationTheme::accentColor());
	QCOMPARE(
		themed.color(QPalette::Active, QPalette::HighlightedText),
		QColor(Qt::white));
	QVERIFY(
		themed.color(QPalette::Disabled, QPalette::Highlight)
		!= ApplicationTheme::accentColor());
}

void ApplicationThemeTest::systemPaletteModePreservesTheSystemHighlight()
{
	QPalette base;
	const QColor system_highlight(QStringLiteral("#a13fc4"));
	base.setColor(QPalette::Active, QPalette::Highlight, system_highlight);
	const QPalette original_palette = qApp->palette();
	const QString original_style_sheet = qApp->styleSheet();

	ApplicationTheme::apply(qApp, base, QString(), false);
	QCOMPARE(
		qApp->palette().color(QPalette::Active, QPalette::Highlight),
		system_highlight);
	QVERIFY(!qApp->styleSheet().contains(ApplicationTheme::accentColor().name()));

	qApp->setPalette(original_palette);
	qApp->setStyleSheet(original_style_sheet);
}

void ApplicationThemeTest::styleSheetUsesConsistentFocusAndPrimaryActionRules()
{
	const QString style_sheet =
		ApplicationTheme::styleSheet(QApplication::palette());

	QVERIFY(style_sheet.contains(QStringLiteral(
		"QPushButton[primaryAction=\"true\"]")));
	QVERIFY(style_sheet.contains(QStringLiteral("QLineEdit:focus")));
	QVERIFY(style_sheet.contains(QStringLiteral("QAbstractItemView:focus")));
	QVERIFY(!style_sheet.contains(QStringLiteral("outline: 0")));
	QVERIFY(style_sheet.contains(QStringLiteral("QTabBar::tab:selected")));
	QVERIFY(style_sheet.contains(QStringLiteral("QDockWidget::title")));
	QVERIFY(style_sheet.contains(ApplicationTheme::accentColor().name()));
	QVERIFY(!style_sheet.contains(QStringLiteral("->")));
}

void ApplicationThemeTest::applyPreservesCustomRulesAndRemovesLegacyInvalidSyntax()
{
	const QPalette original_palette = QApplication::palette();
	const QString original_style_sheet = qApp->styleSheet();
	const QString custom_rule =
		QStringLiteral("QWidget#customRuleTarget { padding: 1px; }");

	ApplicationTheme::apply(qApp, original_palette, custom_rule);
	QVERIFY(qApp->styleSheet().endsWith(custom_rule));
	QVERIFY(qApp->styleSheet().contains(QStringLiteral("primaryAction")));
	QVERIFY(!qApp->styleSheet().contains(QStringLiteral("background-color ->")));

	qApp->setPalette(original_palette);
	qApp->setStyleSheet(original_style_sheet);
}

QTEST_MAIN(ApplicationThemeTest)

#include "tst_applicationtheme.moc"
