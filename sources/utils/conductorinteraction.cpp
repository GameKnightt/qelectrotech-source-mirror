/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorinteraction.h"

#include <QPainterPathStroker>
#include <QtGlobal>
#include <QtMath>

qreal ConductorInteraction::selectionStrokeWidth(
		qreal view_scale,
		qreal conductor_width,
		qreal minimum_hit_width_pixels)
{
	const qreal safe_scale = qMax(qAbs(view_scale), qreal(0.0001));
	return qMax(qMax(conductor_width, qreal(1.0)),
				minimum_hit_width_pixels / safe_scale);
}

QPainterPath ConductorInteraction::selectionShape(
		const QPainterPath &path,
		qreal view_scale,
		qreal conductor_width,
		qreal minimum_hit_width_pixels)
{
	QPainterPathStroker stroker;
	stroker.setWidth(selectionStrokeWidth(
			view_scale, conductor_width, minimum_hit_width_pixels));
	stroker.setJoinStyle(Qt::MiterJoin);
	stroker.setCapStyle(Qt::SquareCap);
	return stroker.createStroke(path);
}

bool ConductorInteraction::containsViewportPoint(
		const QPainterPath &item_path,
		const QTransform &item_to_viewport,
		const QPointF &viewport_point,
		qreal conductor_width,
		qreal minimum_hit_width_pixels)
{
	const QPainterPath viewport_path = item_to_viewport.map(item_path);
	const qreal view_scale = qSqrt(qAbs(item_to_viewport.determinant()));
	return selectionShape(
			viewport_path,
			1.0,
			conductor_width * view_scale,
			minimum_hit_width_pixels).contains(viewport_point);
}
