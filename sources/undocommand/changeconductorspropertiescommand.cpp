/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "changeconductorspropertiescommand.h"

#include "../qetgraphicsitem/conductor.h"

ChangeConductorsPropertiesCommand::ChangeConductorsPropertiesCommand(
		const QVector<Change> &changes,
		QUndoCommand *parent) :
	QUndoCommand(parent)
{
	for (const Change &change : changes)
	{
		if (change.conductor && change.before != change.after) {
			m_changes.append(change);
		}
	}
	setText(m_changes.size() == 1
			? QObject::tr("Modifier les propriétés d’un conducteur")
			: QObject::tr("Modifier les propriétés de %1 conducteurs")
					.arg(m_changes.size()));
}

void ChangeConductorsPropertiesCommand::undo()
{
	for (const Change &change : m_changes)
	{
		if (change.conductor) {
			change.conductor->setProperties(change.before);
		}
	}
	QUndoCommand::undo();
}

void ChangeConductorsPropertiesCommand::redo()
{
	for (const Change &change : m_changes)
	{
		if (change.conductor) {
			change.conductor->setProperties(change.after);
		}
	}
	QUndoCommand::redo();
}

bool ChangeConductorsPropertiesCommand::isEmpty() const
{
	return m_changes.isEmpty();
}
