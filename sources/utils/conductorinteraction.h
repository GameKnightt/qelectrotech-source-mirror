/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORINTERACTION_H
#define CONDUCTORINTERACTION_H

#include <QPainterPath>

namespace ConductorInteraction
{
	constexpr qreal MinimumHitWidthPixels = 10.0;

	qreal selectionStrokeWidth(
			qreal view_scale,
			qreal conductor_width,
			qreal minimum_hit_width_pixels = MinimumHitWidthPixels);

	QPainterPath selectionShape(
			const QPainterPath &path,
			qreal view_scale,
			qreal conductor_width,
			qreal minimum_hit_width_pixels = MinimumHitWidthPixels);

	bool containsViewportPoint(
			const QPainterPath &item_path,
			const QTransform &item_to_viewport,
			const QPointF &viewport_point,
			qreal conductor_width,
			qreal minimum_hit_width_pixels = MinimumHitWidthPixels);
}

#endif // CONDUCTORINTERACTION_H
