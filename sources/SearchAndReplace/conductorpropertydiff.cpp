/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorpropertydiff.h"

#include <QCoreApplication>
#include <QLocale>

namespace {

class ConductorPropertyDiffTranslation
{
	Q_DECLARE_TR_FUNCTIONS(ConductorPropertyDiff)
};

QString textValue(const QString &value)
{
	return value.isEmpty()
		? ConductorPropertyDiffTranslation::tr("(vide)")
		: value;
}

QString boolValue(bool value)
{
	return value
		? ConductorPropertyDiffTranslation::tr("Oui")
		: ConductorPropertyDiffTranslation::tr("Non");
}

QString colorValue(const QColor &color)
{
	if (!color.isValid()) return ConductorPropertyDiffTranslation::tr("Aucune");
	return color.alpha() == 255
		? color.name(QColor::HexRgb).toUpper()
		: color.name(QColor::HexArgb).toUpper();
}

QString typeValue(ConductorProperties::ConductorType type)
{
	return type == ConductorProperties::Single
		? ConductorPropertyDiffTranslation::tr("Unifilaire")
		: ConductorPropertyDiffTranslation::tr("Multifilaire");
}

QString alignmentValue(Qt::Alignment alignment)
{
	if (alignment == Qt::AlignLeft) return ConductorPropertyDiffTranslation::tr("Gauche");
	if (alignment == Qt::AlignRight) return ConductorPropertyDiffTranslation::tr("Droite");
	if (alignment == Qt::AlignTop) return ConductorPropertyDiffTranslation::tr("Haut");
	if (alignment == Qt::AlignBottom) return ConductorPropertyDiffTranslation::tr("Bas");
	return ConductorPropertyDiffTranslation::tr("Par défaut");
}

QString penStyleValue(Qt::PenStyle style)
{
	switch (style) {
		case Qt::SolidLine: return ConductorPropertyDiffTranslation::tr("Trait plein");
		case Qt::DashLine: return ConductorPropertyDiffTranslation::tr("Pointillés");
		case Qt::DotLine: return ConductorPropertyDiffTranslation::tr("Points");
		case Qt::DashDotLine: return ConductorPropertyDiffTranslation::tr("Traits et points");
		case Qt::DashDotDotLine: return ConductorPropertyDiffTranslation::tr("Traits et deux points");
		case Qt::NoPen: return ConductorPropertyDiffTranslation::tr("Aucun");
		default: return QString::number(static_cast<int>(style));
	}
}

QString numberValue(double value)
{
	const QLocale locale;
	QString formatted = locale.toString(value, 'f', 2);
	while (formatted.endsWith(QLatin1Char('0'))) formatted.chop(1);
	if (formatted.endsWith(locale.decimalPoint())) formatted.chop(1);
	return formatted;
}

template<typename T, typename Formatter>
void appendChange(
	QVector<ConductorPropertyChange> &changes,
	const char *key,
	const char *label,
	const T &before,
	const T &after,
	Formatter formatter)
{
	if (before == after) return;
	changes.append({
		QString::fromLatin1(key),
		ConductorPropertyDiffTranslation::tr(label),
		formatter(before),
		formatter(after)});
}

}

QVector<ConductorPropertyChange> conductorPropertyChanges(
	const ConductorProperties &before,
	const ConductorProperties &after)
{
	QVector<ConductorPropertyChange> changes;
	changes.reserve(26);

	appendChange(changes, "type", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Type"), before.type, after.type, typeValue);
	appendChange(changes, "text", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Repère affiché"), before.text, after.text, textValue);
	appendChange(changes, "formula", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Formule"), before.m_formula, after.m_formula, textValue);
	appendChange(changes, "function", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Fonction"), before.m_function, after.m_function, textValue);
	appendChange(changes, "tension_protocol", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Tension / protocole"), before.m_tension_protocol, after.m_tension_protocol, textValue);
	appendChange(changes, "wire_color", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Couleur du fil"), before.m_wire_color, after.m_wire_color, textValue);
	appendChange(changes, "wire_section", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Section"), before.m_wire_section, after.m_wire_section, textValue);
	appendChange(changes, "cable", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Câble"), before.m_cable, after.m_cable, textValue);
	appendChange(changes, "bus", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Bus"), before.m_bus, after.m_bus, textValue);
	appendChange(changes, "show_text", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Afficher le repère"), before.m_show_text, after.m_show_text, boolValue);
	appendChange(changes, "one_text_per_folio", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Un repère par folio"), before.m_one_text_per_folio, after.m_one_text_per_folio, boolValue);
	appendChange(changes, "text_size", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Taille du texte"), before.text_size, after.text_size,
		[](int value) { return QString::number(value); });
	appendChange(changes, "text_color", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Couleur du texte"), before.text_color, after.text_color, colorValue);
	appendChange(changes, "vertical_alignment", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Alignement vertical"), before.m_vertical_alignment, after.m_vertical_alignment, alignmentValue);
	appendChange(changes, "horizontal_alignment", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Alignement horizontal"), before.m_horizontal_alignment, after.m_horizontal_alignment, alignmentValue);
	appendChange(changes, "vertical_rotation", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Rotation verticale"), before.verti_rotate_text, after.verti_rotate_text, numberValue);
	appendChange(changes, "horizontal_rotation", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Rotation horizontale"), before.horiz_rotate_text, after.horiz_rotate_text, numberValue);
	appendChange(changes, "color", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Couleur du conducteur"), before.color, after.color, colorValue);
	appendChange(changes, "style", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Style de trait"), before.style, after.style, penStyleValue);
	appendChange(changes, "size", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Épaisseur"), before.cond_size, after.cond_size, numberValue);
	appendChange(changes, "bicolor", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Bicolore"), before.m_bicolor, after.m_bicolor, boolValue);
	appendChange(changes, "second_color", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Deuxième couleur"), before.m_color_2, after.m_color_2, colorValue);
	appendChange(changes, "dash_size", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Longueur des tirets"), before.m_dash_size, after.m_dash_size,
		[](int value) { return QString::number(value); });
	appendChange(changes, "ground", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Terre"), before.singleLineProperties.hasGround, after.singleLineProperties.hasGround, boolValue);
	appendChange(changes, "neutral", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Neutre"), before.singleLineProperties.hasNeutral, after.singleLineProperties.hasNeutral, boolValue);
	appendChange(changes, "pen", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "PEN"), before.singleLineProperties.is_pen, after.singleLineProperties.is_pen, boolValue);

	SingleLineProperties before_single = before.singleLineProperties;
	SingleLineProperties after_single = after.singleLineProperties;
	appendChange(changes, "phases", QT_TRANSLATE_NOOP("ConductorPropertyDiff", "Nombre de phases"),
		static_cast<int>(before_single.phasesCount()),
		static_cast<int>(after_single.phasesCount()),
		[](int value) { return QString::number(value); });

	return changes;
}
