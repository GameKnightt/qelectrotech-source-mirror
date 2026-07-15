/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORCHANGEPLAN_H
#define CONDUCTORCHANGEPLAN_H

#include "../conductorproperties.h"
#include "conductorchangepreviewdata.h"

#include <functional>
#include <QCoreApplication>
#include <QList>
#include <QPointer>
#include <QString>
#include <QUuid>
#include <QVector>

class Conductor;
class Diagram;
class QETProject;

class ConductorChangePlan final
{
	public:
		Q_DECLARE_TR_FUNCTIONS(ConductorChangePlan)

	public:
		enum class Code {
			Ok,
			NoChanges,
			EmptyInput,
			NullConductor,
			MissingDiagram,
			ProjectUnavailable,
			ProjectMismatch,
			ReadOnlyProject,
			WrongThread,
			InvalidTransform,
			ConductorUnavailable,
			ConductorMoved,
			PropertiesChanged,
			FormulaContextChanged,
			PotentialChanged,
			ApplyFailed
		};

		struct Result {
			Code code = Code::Ok;
			int groupIndex = -1;
			int entryIndex = -1;

			bool canApply() const { return code == Code::Ok; }
			bool isNoOp() const { return code == Code::NoChanges; }
		};

		struct PreviewEntry {
			QPointer<Conductor> conductor;
			QPointer<Diagram> diagram;
			ConductorProperties before;
			ConductorProperties requestedAfter;
			ConductorProperties after;
			QString folioLabel;
			QString conductorLabel;
			int groupIndex = -1;
			bool requested = false;
		};

		using Transform = std::function<
			ConductorProperties(const ConductorProperties &)>;
		using TargetTransform = std::function<
			ConductorProperties(Conductor *, const ConductorProperties &)>;
		using Validation = std::function<Result()>;
		using Application = std::function<bool()>;

		ConductorChangePlan();

		static ConductorChangePlan build(
			QETProject *project,
			const QList<Conductor *> &roots,
			const Transform &transform);
		static ConductorChangePlan build(
			QETProject *project,
			const QList<Conductor *> &roots,
			const TargetTransform &transform);

		Result buildResult() const;
		Result revalidate() const;
		Result applyAtomically(const QString &undoText) const;
		static Result executeAtomically(
				bool hasChanges,
				const Validation &validation,
				const Application &application);
		static QString resultMessage(const Result &result);

		bool isEmpty() const;
		int requestedCount() const;
		int changedConductorCount() const;
		int potentialCount() const;
		const QVector<PreviewEntry> &previewEntries() const;
		ConductorChangePreviewData previewData() const;
		Conductor *conductorForKey(quintptr key) const;

	private:
		struct PotentialSnapshot {
			QPointer<Conductor> representative;
			QVector<QPointer<Conductor>> members;
		};

		QPointer<QETProject> m_project;
		QUuid m_project_uuid;
		QVector<QPointer<Conductor>> m_requested;
		QVector<PreviewEntry> m_entries;
		QVector<PotentialSnapshot> m_potentials;
		Result m_build_result;
};

#endif // CONDUCTORCHANGEPLAN_H
