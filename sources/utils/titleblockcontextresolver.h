/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef TITLEBLOCKCONTEXTRESOLVER_H
#define TITLEBLOCKCONTEXTRESOLVER_H

#include "../diagramcontext.h"

namespace QET {

/**
 * Build the custom-property context used by title blocks and formulas.
 * A non-empty folio value overrides the project value. An empty folio value
 * falls back to the project value when one exists; otherwise it remains empty.
 */
inline DiagramContext effectiveTitleBlockContext(
		const DiagramContext &project_context,
		const DiagramContext &folio_context)
{
	DiagramContext context = project_context;

	for (const QString &key : folio_context.keys()) {
		const QVariant folio_value = folio_context.value(key);
		if (!folio_value.toString().isEmpty()
				|| !project_context.contains(key)) {
			context.addValue(
					key,
					folio_value,
					folio_context.keyMustShow(key));
		}
	}

	return context;
}

}

#endif // TITLEBLOCKCONTEXTRESOLVER_H
