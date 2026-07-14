/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef PROJECT_SAVE_STATUS_CONTROLLER_H
#define PROJECT_SAVE_STATUS_CONTROLLER_H

#include "projectsavestatuswidget.h"

#include <QHash>
#include <QObject>
#include <QPointer>

class ProjectSaveStatusController final : public QObject
{
	Q_OBJECT

	public:
		enum class RecoveryState { None, Writing, Available, Failed };
		Q_ENUM(RecoveryState)

		struct Snapshot {
			ProjectSaveStatusWidget::State state =
				ProjectSaveStatusWidget::State::NoProject;
			QString projectName;
			QString filePath;
			QString detail;
			RecoveryState recoveryState = RecoveryState::None;
			QString recoveryPath;
			QString recoveryError;
			bool modified = false;

			bool operator==(const Snapshot &other) const;
			bool operator!=(const Snapshot &other) const { return !(*this == other); }
		};

		explicit ProjectSaveStatusController(QObject *parent = nullptr);

		void registerProject(
			QObject *project,
			const QString &project_name,
			const QString &file_path,
			bool modified);
		void unregisterProject(QObject *project);
		void setActiveProject(QObject *project);
		void updateProjectIdentity(
			QObject *project,
			const QString &project_name,
			const QString &file_path);
		void setModified(QObject *project, bool modified);

		void beginCanonicalSave(QObject *project, quint64 operation_id);
		void finishCanonicalSave(
			QObject *project,
			quint64 operation_id,
			bool ok,
			const QString &detail,
			const QString &committed_path,
			bool has_unsaved_changes);

		void beginRecoveryBackup(QObject *project, quint64 operation_id);
		void finishRecoveryBackup(
			QObject *project,
			quint64 operation_id,
			bool ok,
			const QString &detail,
			const QString &backup_path);
		void invalidateRecoveryBackup(QObject *project);

		Snapshot activeSnapshot() const;
		Snapshot snapshot(QObject *project) const;

	signals:
		void activeSnapshotChanged(
			const ProjectSaveStatusController::Snapshot &snapshot);

	private:
		struct Record {
			QString projectName;
			QString filePath;
			QString detail;
			ProjectSaveStatusWidget::State state =
				ProjectSaveStatusWidget::State::Modified;
			RecoveryState recoveryState = RecoveryState::None;
			QString recoveryPath;
			QString recoveryError;
			bool modified = false;
			quint64 revision = 0;
			quint64 saveRevision = 0;
			quint64 canonicalOperationId = 0;
			quint64 recoveryOperationId = 0;
		};

		Snapshot snapshotFor(const Record &record) const;
		void notifyIfActive(QObject *project);
		void notifyActive(bool force = false);

		QHash<QObject *, Record> m_records;
		QPointer<QObject> m_active_project;
		Snapshot m_last_snapshot;
		bool m_has_last_snapshot = false;
};

Q_DECLARE_METATYPE(ProjectSaveStatusController::Snapshot)

#endif // PROJECT_SAVE_STATUS_CONTROLLER_H
