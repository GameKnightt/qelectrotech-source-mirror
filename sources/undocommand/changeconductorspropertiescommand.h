/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CHANGECONDUCTORSPROPERTIESCOMMAND_H
#define CHANGECONDUCTORSPROPERTIESCOMMAND_H

#include "../conductorproperties.h"

#include <QPointer>
#include <QUndoCommand>
#include <QVector>

class Conductor;

class ChangeConductorsPropertiesCommand : public QUndoCommand
{
	public:
		struct Change {
			QPointer<Conductor> conductor;
			ConductorProperties before;
			ConductorProperties after;
		};

		explicit ChangeConductorsPropertiesCommand(
				const QVector<Change> &changes,
				QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;
		bool isEmpty() const;

	private:
		QVector<Change> m_changes;
};

#endif // CHANGECONDUCTORSPROPERTIESCOMMAND_H
