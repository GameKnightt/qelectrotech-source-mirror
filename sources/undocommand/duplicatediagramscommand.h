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
#ifndef DUPLICATE_DIAGRAMS_COMMAND_H
#define DUPLICATE_DIAGRAMS_COMMAND_H

#include "../borderproperties.h"
#include "../titleblockproperties.h"

#include <QDomDocument>
#include <QPointer>
#include <QString>
#include <QUndoCommand>
#include <QVector>

class Diagram;
class QETProject;

/**
 * Makes an already-applied group of folio duplications undoable.
 *
 * The first redo performed by QUndoStack::push() is intentionally a no-op:
 * callers validate and import the complete group before registering the
 * command. Later redo operations recreate every folio from the same remapped
 * XML documents, so element identities and cross-folio references remain
 * stable across an undo/redo cycle.
 */
class DuplicateDiagramsCommand final : public QUndoCommand
{
public:
	struct DiagramSnapshot
	{
		QDomDocument document;
		QString titleBlockTemplateName;
		TitleBlockProperties titleBlockProperties;
		BorderProperties borderProperties;
		int projectPosition = -1;
	};

	DuplicateDiagramsCommand(
		QETProject *project,
		const QVector<DiagramSnapshot> &snapshots,
		const QVector<Diagram *> &appliedDiagrams,
		QUndoCommand *parent = nullptr);

	void undo() override;
	void redo() override;

private:
	bool restoreDiagrams();
	void refreshProject(bool allDiagrams);

	QPointer<QETProject> m_project;
	QVector<DiagramSnapshot> m_snapshots;
	QVector<QPointer<Diagram>> m_diagrams;
	bool m_skipFirstRedo = true;
};

#endif // DUPLICATE_DIAGRAMS_COMMAND_H
