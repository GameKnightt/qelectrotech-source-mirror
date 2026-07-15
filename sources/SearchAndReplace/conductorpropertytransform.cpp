/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorpropertytransform.h"

#include <QDebug>
#include <QRegularExpression>

namespace {

QString applyStringChange(const QString &original, const QString &change)
{
	if (change.isEmpty()) return original;
	if (change == conductorPropertyEraseText()) return QString();
	return change;
}

}

ConductorProperties applyConductorPropertyPatch(
	const ConductorProperties &original,
	const ConductorProperties &patch)
{
	ConductorProperties properties = original;
	if (patch.text_size > 2) properties.text_size = patch.text_size;
	properties.m_formula = applyStringChange(properties.m_formula, patch.m_formula);
	properties.text = applyStringChange(properties.text, patch.text);
	properties.m_show_text = patch.m_show_text;
	properties.m_function = applyStringChange(properties.m_function, patch.m_function);
	properties.m_tension_protocol = applyStringChange(
		properties.m_tension_protocol, patch.m_tension_protocol);
	properties.m_wire_color = applyStringChange(
		properties.m_wire_color, patch.m_wire_color);
	properties.m_wire_section = applyStringChange(
		properties.m_wire_section, patch.m_wire_section);
	if (patch.m_vertical_alignment == Qt::AlignLeft
		|| patch.m_vertical_alignment == Qt::AlignRight) {
		properties.m_vertical_alignment = patch.m_vertical_alignment;
	}
	if (patch.m_horizontal_alignment == Qt::AlignTop
		|| patch.m_horizontal_alignment == Qt::AlignBottom) {
		properties.m_horizontal_alignment = patch.m_horizontal_alignment;
	}
	if (patch.verti_rotate_text >= 0) {
		properties.verti_rotate_text = patch.verti_rotate_text;
	}
	if (patch.horiz_rotate_text >= 0) {
		properties.horiz_rotate_text = patch.horiz_rotate_text;
	}
	if (patch.color.isValid()) properties.color = patch.color;
	if (patch.style != Qt::NoPen) properties.style = patch.style;
	if (patch.cond_size >= 0.4) properties.cond_size = patch.cond_size;
	properties.m_bicolor = patch.m_bicolor;
	if (patch.m_color_2.isValid()) properties.m_color_2 = patch.m_color_2;
	if (patch.m_dash_size >= 2) properties.m_dash_size = patch.m_dash_size;
	properties.singleLineProperties = patch.singleLineProperties;
	return properties;
}

ConductorProperties applyAdvancedConductorReplacement(
	const ConductorProperties &original,
	const advancedReplaceStruct &replacement)
{
	ConductorProperties properties = original;
	if (replacement.who != 2) return properties;

	const QRegularExpression expression(replacement.search);
	if (!expression.isValid()) {
		qWarning() << QObject::tr("Expression régulière invalide")
			<< expression.errorString()
			<< expression.patternErrorOffset();
		return properties;
	}

	const QString &field = replacement.what;
	const QString &value = replacement.replace;
	if (field == QStringLiteral("formula")) {
		properties.m_formula.replace(expression, value);
	} else if (field == QStringLiteral("text")) {
		properties.text.replace(expression, value);
	} else if (field == QStringLiteral("function")) {
		properties.m_function.replace(expression, value);
	} else if (field == QStringLiteral("tension/protocol")) {
		properties.m_tension_protocol.replace(expression, value);
	} else if (field == QStringLiteral("conductor_color")) {
		properties.m_wire_color.replace(expression, value);
	} else if (field == QStringLiteral("conductor_section")) {
		properties.m_wire_section.replace(expression, value);
	}
	return properties;
}

ConductorProperties transformConductorProperties(
	const ConductorProperties &original,
	const ConductorProperties &patch,
	const advancedReplaceStruct &advanced,
	bool includePropertyPatch,
	bool includeAdvancedReplacement)
{
	ConductorProperties properties = includePropertyPatch
		? applyConductorPropertyPatch(original, patch)
		: original;
	if (includeAdvancedReplacement) {
		properties = applyAdvancedConductorReplacement(properties, advanced);
	}
	return properties;
}
