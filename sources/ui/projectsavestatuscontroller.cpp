/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "projectsavestatuscontroller.h"

bool ProjectSaveStatusController::Snapshot::operator==(
	const ProjectSaveStatusController::Snapshot &other) const
{
	return state == other.state
		&& projectName == other.projectName
		&& filePath == other.filePath
		&& detail == other.detail
		&& recoveryState == other.recoveryState
		&& recoveryPath == other.recoveryPath
		&& recoveryError == other.recoveryError
		&& modified == other.modified;
}

ProjectSaveStatusController::ProjectSaveStatusController(QObject *parent) :
	QObject(parent)
{
	qRegisterMetaType<ProjectSaveStatusController::Snapshot>();
}

void ProjectSaveStatusController::registerProject(
	QObject *project,
	const QString &project_name,
	const QString &file_path,
	bool modified)
{
	if (!project) return;

	Record record;
	record.projectName = project_name;
	record.filePath = file_path;
	record.modified = modified || file_path.isEmpty();
	record.state = record.modified
		? ProjectSaveStatusWidget::State::Modified
		: ProjectSaveStatusWidget::State::Saved;
	m_records.insert(project, record);

	connect(project, &QObject::destroyed, this,
		[this](QObject *destroyed_project) {
			unregisterProject(destroyed_project);
		});
	notifyIfActive(project);
}

void ProjectSaveStatusController::unregisterProject(QObject *project)
{
	if (!project) return;
	const bool was_active = m_active_project == project;
	m_records.remove(project);
	if (was_active) {
		m_active_project.clear();
		notifyActive(true);
	}
}

void ProjectSaveStatusController::setActiveProject(QObject *project)
{
	if (project && !m_records.contains(project))
		project = nullptr;
	if (m_active_project == project && m_has_last_snapshot)
		return;
	m_active_project = project;
	notifyActive(true);
}

void ProjectSaveStatusController::updateProjectIdentity(
	QObject *project,
	const QString &project_name,
	const QString &file_path)
{
	auto iterator = m_records.find(project);
	if (iterator == m_records.end()) return;

	Record &record = iterator.value();
	record.projectName = project_name;
	if (record.filePath != file_path) {
		record.filePath = file_path;
		record.recoveryState = RecoveryState::None;
		record.recoveryPath.clear();
		record.recoveryError.clear();
		record.recoveryOperationId = 0;
	}

	if (record.state != ProjectSaveStatusWidget::State::Saving
		&& record.state != ProjectSaveStatusWidget::State::Error) {
		record.state = (record.modified || record.filePath.isEmpty())
			? ProjectSaveStatusWidget::State::Modified
			: ProjectSaveStatusWidget::State::Saved;
	}
	notifyIfActive(project);
}

void ProjectSaveStatusController::setModified(QObject *project, bool modified)
{
	auto iterator = m_records.find(project);
	if (iterator == m_records.end()) return;

	Record &record = iterator.value();
	if (modified && !record.modified)
		++record.revision;
	record.modified = modified || record.filePath.isEmpty();

	if (record.state != ProjectSaveStatusWidget::State::Saving
		&& record.state != ProjectSaveStatusWidget::State::Error) {
		record.state = record.modified
			? ProjectSaveStatusWidget::State::Modified
			: ProjectSaveStatusWidget::State::Saved;
	}
	notifyIfActive(project);
}

void ProjectSaveStatusController::beginCanonicalSave(
	QObject *project,
	quint64 operation_id)
{
	auto iterator = m_records.find(project);
	if (iterator == m_records.end() || operation_id == 0) return;

	Record &record = iterator.value();
	record.canonicalOperationId = operation_id;
	record.saveRevision = record.revision;
	record.detail.clear();
	record.state = ProjectSaveStatusWidget::State::Saving;
	notifyIfActive(project);
}

void ProjectSaveStatusController::finishCanonicalSave(
	QObject *project,
	quint64 operation_id,
	bool ok,
	const QString &detail,
	const QString &committed_path,
	bool has_unsaved_changes)
{
	auto iterator = m_records.find(project);
	if (iterator == m_records.end()) return;

	Record &record = iterator.value();
	if (operation_id == 0 || record.canonicalOperationId != operation_id)
		return;
	record.canonicalOperationId = 0;

	if (!ok) {
		record.modified = true;
		record.detail = detail;
		record.state = ProjectSaveStatusWidget::State::Error;
	} else {
		if (!committed_path.isEmpty())
			record.filePath = committed_path;
		record.modified = has_unsaved_changes || record.filePath.isEmpty();
		record.detail.clear();
		const bool changed_during_save = record.revision != record.saveRevision;
		record.state = (record.modified || changed_during_save)
			? ProjectSaveStatusWidget::State::Modified
			: ProjectSaveStatusWidget::State::Saved;
	}
	notifyIfActive(project);
}

void ProjectSaveStatusController::beginRecoveryBackup(
	QObject *project,
	quint64 operation_id)
{
	auto iterator = m_records.find(project);
	if (iterator == m_records.end() || operation_id == 0) return;
	Record &record = iterator.value();
	record.recoveryOperationId = operation_id;
	record.recoveryState = RecoveryState::Writing;
	record.recoveryPath.clear();
	record.recoveryError.clear();
	notifyIfActive(project);
}

void ProjectSaveStatusController::finishRecoveryBackup(
	QObject *project,
	quint64 operation_id,
	bool ok,
	const QString &detail,
	const QString &backup_path)
{
	auto iterator = m_records.find(project);
	if (iterator == m_records.end()) return;
	Record &record = iterator.value();
	if (operation_id == 0 || record.recoveryOperationId != operation_id)
		return;

	record.recoveryOperationId = 0;
	record.recoveryState = ok ? RecoveryState::Available : RecoveryState::Failed;
	record.recoveryPath = ok ? backup_path : QString();
	record.recoveryError = ok ? QString() : detail;
	notifyIfActive(project);
}

void ProjectSaveStatusController::invalidateRecoveryBackup(QObject *project)
{
	auto iterator = m_records.find(project);
	if (iterator == m_records.end()) return;
	Record &record = iterator.value();
	record.recoveryOperationId = 0;
	record.recoveryState = RecoveryState::None;
	record.recoveryPath.clear();
	record.recoveryError.clear();
	notifyIfActive(project);
}

ProjectSaveStatusController::Snapshot
ProjectSaveStatusController::activeSnapshot() const
{
	return snapshot(m_active_project.data());
}

ProjectSaveStatusController::Snapshot
ProjectSaveStatusController::snapshot(QObject *project) const
{
	const auto iterator = m_records.constFind(project);
	if (iterator == m_records.cend())
		return Snapshot();
	return snapshotFor(iterator.value());
}

ProjectSaveStatusController::Snapshot
ProjectSaveStatusController::snapshotFor(const Record &record) const
{
	Snapshot result;
	result.state = record.state;
	result.projectName = record.projectName;
	result.filePath = record.filePath;
	result.detail = record.detail;
	result.recoveryState = record.recoveryState;
	result.recoveryPath = record.recoveryPath;
	result.recoveryError = record.recoveryError;
	result.modified = record.modified;
	return result;
}

void ProjectSaveStatusController::notifyIfActive(QObject *project)
{
	if (m_active_project == project)
		notifyActive();
}

void ProjectSaveStatusController::notifyActive(bool force)
{
	const Snapshot current = activeSnapshot();
	if (!force && m_has_last_snapshot && current == m_last_snapshot)
		return;
	m_last_snapshot = current;
	m_has_last_snapshot = true;
	emit activeSnapshotChanged(current);
}
