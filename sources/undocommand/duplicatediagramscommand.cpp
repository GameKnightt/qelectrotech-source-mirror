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
#include "duplicatediagramscommand.h"

#include "../dataBase/projectdatabase.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"

#include <QDebug>
#include <QGraphicsItem>
#include <QObject>

DuplicateDiagramsCommand::DuplicateDiagramsCommand(
	QETProject *project,
	const QVector<DiagramSnapshot> &snapshots,
	const QVector<Diagram *> &appliedDiagrams,
	QUndoCommand *parent) :
	QUndoCommand(parent),
	m_project(project),
	m_snapshots(snapshots)
{
	setText(QObject::tr(
		"dupliquer %n folio(s)",
		"undo caption",
		m_snapshots.size()));
	m_diagrams.reserve(appliedDiagrams.size());
	for (Diagram *diagram : appliedDiagrams) {
		m_diagrams.append(QPointer<Diagram>(diagram));
	}
}

void DuplicateDiagramsCommand::undo()
{
	if (!m_project || m_project->isReadOnly()) {
		qWarning() << "DuplicateDiagramsCommand::undo: project unavailable or read-only";
		return;
	}

	for (const QPointer<Diagram> &diagram : m_diagrams) {
		if (diagram) {
			m_project->removeDiagram(diagram.data());
		}
	}
	m_diagrams.clear();
	refreshProject(true);
}

void DuplicateDiagramsCommand::redo()
{
	if (m_skipFirstRedo) {
		m_skipFirstRedo = false;
		return;
	}

	if (!restoreDiagrams()) {
		qWarning() << "DuplicateDiagramsCommand::redo: the duplicated folios could not be restored";
		setObsolete(true);
	}
}

bool DuplicateDiagramsCommand::restoreDiagrams()
{
	if (!m_project || m_project->isReadOnly()) {
		return false;
	}

	QVector<QPointer<Diagram>> restored;
	restored.reserve(m_snapshots.size());

	for (const DiagramSnapshot &snapshot : m_snapshots) {
		const int diagramCount = m_project->diagrams().size();
		const int position = snapshot.projectPosition < 0
			? -1
			: qMin(snapshot.projectPosition, diagramCount);
		Diagram *diagram = m_project->addNewDiagram(position);
		if (!diagram) {
			for (const QPointer<Diagram> &created : restored) {
				if (created) {
					m_project->removeDiagram(created.data());
				}
			}
			refreshProject(true);
			return false;
		}

		diagram->setTitleBlockTemplate(snapshot.titleBlockTemplateName);
		diagram->border_and_titleblock.importTitleBlock(
			snapshot.titleBlockProperties);
		diagram->border_and_titleblock.importBorder(snapshot.borderProperties);

		// Keep the command snapshot immutable even if a future importer starts
		// normalizing or otherwise mutating the DOM it receives.
		QDomDocument document = snapshot.document.cloneNode(true).toDocument();
		QDomElement diagramElement = document.documentElement();
		if (!diagram->fromXml(
				diagramElement, QPointF(0, 0), false, nullptr)) {
			m_project->removeDiagram(diagram);
			for (const QPointer<Diagram> &created : restored) {
				if (created) {
					m_project->removeDiagram(created.data());
				}
			}
			refreshProject(true);
			return false;
		}
		restored.append(QPointer<Diagram>(diagram));
	}

	m_diagrams = restored;
	refreshProject(false);
	return true;
}

void DuplicateDiagramsCommand::refreshProject(bool allDiagrams)
{
	if (!m_project) {
		return;
	}

	QList<Diagram *> diagrams;
	if (allDiagrams) {
		diagrams = m_project->diagrams();
	} else {
		for (const QPointer<Diagram> &diagram : m_diagrams) {
			if (diagram) {
				diagrams.append(diagram.data());
			}
		}
	}

	for (Diagram *diagram : diagrams) {
		for (QGraphicsItem *item : diagram->items()) {
			if (Element *element = dynamic_cast<Element *>(item)) {
				diagram->restoreText(element);
			}
		}
		diagram->refreshContents();
	}

	const auto result = m_project->dataBase()->updateDB();
	if (!result.isOk()) {
		qWarning() << "DuplicateDiagramsCommand: database refresh failed:"
				   << result.diagnostic();
	}
}
