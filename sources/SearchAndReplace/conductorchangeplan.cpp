/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorchangeplan.h"

#include "conductorpropertydiff.h"
#include "conductorscopeutils.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetproject.h"
#include "../undocommand/changeconductorspropertiescommand.h"

#include <QCoreApplication>
#include <QSet>
#include <QThread>
#include <QUndoStack>

#include <algorithm>

namespace {

quintptr conductorKey(const Conductor *conductor)
{
	return reinterpret_cast<quintptr>(conductor);
}

QString folioLabel(Diagram *diagram)
{
	QString folio = diagram->border_and_titleblock.finalfolio().trimmed();
	if (folio.isEmpty()) folio = QString::number(diagram->folioIndex() + 1);

	const QString title = diagram->title().trimmed();
	return title.isEmpty()
		? folio
		: ConductorChangePlan::tr("%1 — %2").arg(folio, title);
}

QString conductorLabel(Conductor *conductor, const ConductorProperties &properties)
{
	QString label = properties.text.trimmed();
	if (label.isEmpty() || label == QStringLiteral("_")) {
		label = properties.m_formula.trimmed();
	}
	if (label.isEmpty()) label = ConductorChangePlan::tr("Conducteur sans repère");

	QStringList terminals;
	if (conductor->terminal1 && !conductor->terminal1->name().trimmed().isEmpty()) {
		terminals.append(conductor->terminal1->name().trimmed());
	}
	if (conductor->terminal2 && !conductor->terminal2->name().trimmed().isEmpty()) {
		terminals.append(conductor->terminal2->name().trimmed());
	}
	if (!terminals.isEmpty()) {
		label.append(ConductorChangePlan::tr(" · %1").arg(
			terminals.join(ConductorChangePlan::tr(" ↔ "))));
	}
	return label;
}

QSet<Conductor *> potentialMembers(Conductor *representative)
{
	QSet<Conductor *> members = representative->relatedPotentialConductors(true);
	members.insert(representative);
	return members;
}

QString terminalSortKey(const Terminal *terminal)
{
	if (!terminal) return QStringLiteral("~");
	const QString uuid = terminal->uuid().toString(QUuid::WithoutBraces);
	if (!uuid.isEmpty()) return uuid;
	const QPointF position = terminal->scenePos();
	return QStringLiteral("%1|%2|%3|%4")
		.arg(terminal->name())
		.arg(position.x(), 0, 'g', 17)
		.arg(position.y(), 0, 'g', 17)
		.arg(static_cast<int>(terminal->orientation()));
}

QString conductorSortKey(Conductor *conductor)
{
	QStringList terminals = {
		terminalSortKey(conductor->terminal1),
		terminalSortKey(conductor->terminal2)};
	std::sort(terminals.begin(), terminals.end());
	return QStringLiteral("%1|%2|%3")
		.arg(conductor->diagram()->uuid().toString(QUuid::WithoutBraces))
		.arg(terminals.at(0), terminals.at(1));
}

bool pointerLess(Conductor *left, Conductor *right)
{
	if (left->diagram()->folioIndex() != right->diagram()->folioIndex()) {
		return left->diagram()->folioIndex() < right->diagram()->folioIndex();
	}
	return conductorSortKey(left) < conductorSortKey(right);
}

}

ConductorChangePlan::ConductorChangePlan() = default;

ConductorChangePlan ConductorChangePlan::build(
	QETProject *project,
	const QList<Conductor *> &roots,
	const Transform &transform)
{
	ConductorChangePlan plan;
	plan.m_project = project;
	if (!project) {
		plan.m_build_result.code = Code::ProjectUnavailable;
		return plan;
	}
	plan.m_project_uuid = project->uuid();

	if (QThread::currentThread() != project->thread()) {
		plan.m_build_result.code = Code::WrongThread;
		return plan;
	}
	if (project->isReadOnly()) {
		plan.m_build_result.code = Code::ReadOnlyProject;
		return plan;
	}
	if (roots.isEmpty()) {
		plan.m_build_result.code = Code::EmptyInput;
		return plan;
	}
	if (!transform) {
		plan.m_build_result.code = Code::InvalidTransform;
		return plan;
	}

	QSet<Conductor *> requested_set;
	for (Conductor *root : roots) {
		if (!root) {
			plan.m_build_result.code = Code::NullConductor;
			return plan;
		}
		if (!root->diagram()) {
			plan.m_build_result.code = Code::MissingDiagram;
			return plan;
		}
		if (root->diagram()->project() != project) {
			plan.m_build_result.code = Code::ProjectMismatch;
			return plan;
		}
		if (!requested_set.contains(root)) {
			requested_set.insert(root);
			plan.m_requested.append(root);
		}
	}

	QSet<Conductor *> visited;
	for (Conductor *root : roots)
	{
		if (visited.contains(root)) continue;

		QSet<Conductor *> member_set = potentialMembers(root);
		QVector<Conductor *> members;
		members.reserve(member_set.size());
		for (Conductor *member : member_set)
		{
			if (!member) {
				plan.m_build_result.code = Code::ConductorUnavailable;
				return plan;
			}
			if (!member->diagram()) {
				plan.m_build_result.code = Code::MissingDiagram;
				return plan;
			}
			if (member->diagram()->project() != project) {
				plan.m_build_result.code = Code::ProjectMismatch;
				return plan;
			}
			members.append(member);
		}
		std::sort(members.begin(), members.end(), pointerLess);

		PotentialSnapshot potential;
		potential.representative = root;
		const int group_index = plan.m_potentials.size();
		for (Conductor *member : members) potential.members.append(member);
		const QVector<Conductor *> unique_members =
			takeUnvisitedConductorScopeMembers(members, visited);
		for (Conductor *member : unique_members)
		{

			const ConductorProperties before = member->properties();
			const ConductorProperties requested_after = transform(before);
			const ConductorProperties after =
				member->resolvedProperties(requested_after);
			plan.m_entries.append({
				member,
				member->diagram(),
				before,
				requested_after,
				after,
				folioLabel(member->diagram()),
				conductorLabel(member, before),
				group_index,
				requested_set.contains(member)});
		}
		plan.m_potentials.append(potential);
	}

	plan.m_build_result.code = plan.changedConductorCount() == 0
		? Code::NoChanges
		: Code::Ok;
	return plan;
}

ConductorChangePlan::Result ConductorChangePlan::buildResult() const
{
	return m_build_result;
}

ConductorChangePlan::Result ConductorChangePlan::revalidate() const
{
	if (!m_build_result.canApply()) return m_build_result;
	if (!m_project || m_project->uuid() != m_project_uuid) {
		return {Code::ProjectUnavailable, -1, -1};
	}
	if (QThread::currentThread() != m_project->thread()) {
		return {Code::WrongThread, -1, -1};
	}
	if (m_project->isReadOnly()) {
		return {Code::ReadOnlyProject, -1, -1};
	}

	for (int index = 0; index < m_entries.size(); ++index)
	{
		const PreviewEntry &entry = m_entries.at(index);
		if (!entry.conductor) {
			return {Code::ConductorUnavailable, entry.groupIndex, index};
		}
		if (!entry.diagram
			|| entry.conductor->diagram() != entry.diagram
			|| entry.diagram->project() != m_project) {
			return {Code::ConductorMoved, entry.groupIndex, index};
		}
		if (entry.conductor->properties() != entry.before) {
			return {Code::PropertiesChanged, entry.groupIndex, index};
		}
		if (entry.conductor->resolvedProperties(entry.requestedAfter)
			!= entry.after) {
			return {Code::FormulaContextChanged, entry.groupIndex, index};
		}
	}

	for (int group_index = 0; group_index < m_potentials.size(); ++group_index)
	{
		const PotentialSnapshot &snapshot = m_potentials.at(group_index);
		if (!snapshot.representative) {
			return {Code::ConductorUnavailable, group_index, -1};
		}

		const QSet<Conductor *> current = potentialMembers(snapshot.representative.data());
		QSet<Conductor *> expected;
		for (const QPointer<Conductor> &member : snapshot.members) {
			if (!member) return {Code::ConductorUnavailable, group_index, -1};
			expected.insert(member.data());
		}
		if (current != expected) {
			return {Code::PotentialChanged, group_index, -1};
		}
	}

	return {};
}

ConductorChangePlan::Result ConductorChangePlan::applyAtomically(
	const QString &undoText) const
{
	return executeAtomically(
		changedConductorCount() > 0,
		[this]() { return revalidate(); },
		[this, undoText]() {
			if (!m_project || !m_project->undoStack()) return false;
			QVector<ChangeConductorsPropertiesCommand::Change> changes;
			changes.reserve(changedConductorCount());
			for (const PreviewEntry &entry : m_entries) {
				if (entry.before != entry.after) {
					changes.append({entry.conductor, entry.before, entry.after});
				}
			}

			auto command = new ChangeConductorsPropertiesCommand(changes);
			if (command->isEmpty()) {
				delete command;
				return false;
			}
			command->setText(undoText);
			m_project->undoStack()->push(command);
			return true;
		});
}

QString ConductorChangePlan::resultMessage(const Result &result)
{
	switch (result.code) {
		case Code::Ok: return QString();
		case Code::NoChanges: return tr("Aucune propriété de conducteur ne serait modifiée.");
		case Code::EmptyInput: return tr("Aucun conducteur n’est sélectionné.");
		case Code::NullConductor: return tr("La sélection contient un conducteur indisponible.");
		case Code::MissingDiagram: return tr("Un conducteur n’appartient plus à un folio.");
		case Code::ProjectUnavailable: return tr("Le projet n’est plus disponible.");
		case Code::ProjectMismatch: return tr("Les conducteurs sélectionnés n’appartiennent pas tous au même projet.");
		case Code::ReadOnlyProject: return tr("Le projet est en lecture seule.");
		case Code::WrongThread: return tr("L’opération ne peut pas être appliquée depuis ce contexte d’exécution.");
		case Code::InvalidTransform: return tr("La transformation demandée n’est pas valide.");
		case Code::ConductorUnavailable: return tr("Un conducteur a été supprimé depuis l’ouverture de l’aperçu.");
		case Code::ConductorMoved: return tr("Un conducteur a changé de projet ou de folio.");
		case Code::PropertiesChanged: return tr("Des propriétés ont changé depuis l’ouverture de l’aperçu. Vérifiez de nouveau les modifications.");
		case Code::FormulaContextChanged: return tr("Le contexte d’une formule a changé depuis l’ouverture de l’aperçu. Vérifiez de nouveau les modifications.");
		case Code::PotentialChanged: return tr("La composition d’un potentiel a changé depuis l’ouverture de l’aperçu. Vérifiez de nouveau les modifications.");
		case Code::ApplyFailed: return tr("La transaction n’a pas pu être ajoutée à l’historique d’annulation.");
	}
	return tr("L’opération ne peut pas être appliquée.");
}

bool ConductorChangePlan::isEmpty() const
{
	return changedConductorCount() == 0;
}

int ConductorChangePlan::requestedCount() const
{
	return m_requested.size();
}

int ConductorChangePlan::changedConductorCount() const
{
	return static_cast<int>(std::count_if(
		m_entries.cbegin(),
		m_entries.cend(),
		[](const PreviewEntry &entry) { return entry.before != entry.after; }));
}

int ConductorChangePlan::potentialCount() const
{
	QSet<int> groups;
	for (const PreviewEntry &entry : m_entries) {
		if (entry.before != entry.after) groups.insert(entry.groupIndex);
	}
	return groups.size();
}

const QVector<ConductorChangePlan::PreviewEntry> &
ConductorChangePlan::previewEntries() const
{
	return m_entries;
}

ConductorChangePreviewData ConductorChangePlan::previewData() const
{
	ConductorChangePreviewData data;
	data.requestedCount = requestedCount();
	data.consideredCount = m_entries.size();
	data.affectedCount = changedConductorCount();
	data.unchangedCount = data.consideredCount - data.affectedCount;
	data.potentialCount = potentialCount();

	QSet<Diagram *> folios;
	for (const PreviewEntry &entry : m_entries)
	{
		if (entry.before != entry.after && entry.diagram) {
			folios.insert(entry.diagram.data());
		}
		const quintptr key = conductorKey(entry.conductor.data());
		const QVector<ConductorPropertyChange> changes =
			conductorPropertyChanges(entry.before, entry.after);
		if (changes.isEmpty()) {
			data.rows.append({
				key,
				entry.folioLabel,
				entry.conductorLabel,
				tr("Aucun champ"),
				tr("—"),
				tr("—"),
				tr("Inchangé"),
				false});
			continue;
		}

		for (const ConductorPropertyChange &change : changes) {
			data.rows.append({
				key,
				entry.folioLabel,
				entry.conductorLabel,
				change.label,
				change.before,
				change.after,
				tr("À modifier"),
				true});
		}
	}
	data.folioCount = folios.size();
	return data;
}

Conductor *ConductorChangePlan::conductorForKey(quintptr key) const
{
	for (const PreviewEntry &entry : m_entries) {
		if (entry.conductor && conductorKey(entry.conductor.data()) == key) {
			return entry.conductor.data();
		}
	}
	return nullptr;
}
