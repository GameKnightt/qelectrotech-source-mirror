/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef QET_MCP_PROJECT_SERVICE_H
#define QET_MCP_PROJECT_SERVICE_H

#include <QJsonObject>
#include <QStringList>

namespace QetMcp {

struct ToolExecutionResult
{
	bool ok = false;
	QJsonObject data;
	QString error;
};

/**
	Executes the QElectroTech-specific MCP tools.

	Every path is restricted to one of the explicitly configured roots. Write
	operations are disabled unless the server was launched with --mcp-write and
	always require a separate, non-existing output file plus confirm=true.
*/
class ProjectService
{
	public:
		ProjectService(const QStringList &roots, bool write_enabled);

		bool isValid() const;
		QString errorString() const;
		QStringList roots() const;
		bool writeEnabled() const;

		ToolExecutionResult execute(
			const QString &tool_name,
			const QJsonObject &arguments) const;

	private:
		ToolExecutionResult inspectProject(const QJsonObject &arguments) const;
		ToolExecutionResult validateProject(const QJsonObject &arguments) const;
		ToolExecutionResult createProject(const QJsonObject &arguments) const;
		ToolExecutionResult setTitleBlock(const QJsonObject &arguments) const;

		bool resolveInputProject(
			const QString &candidate,
			QString *resolved,
			QString *error) const;
		bool resolveOutputProject(
			const QString &candidate,
			QString *resolved,
			QString *error) const;
		bool isInsideRoots(const QString &canonical_path) const;

		QStringList m_roots;
		bool m_write_enabled = false;
		QString m_error;
};

} // namespace QetMcp

#endif // QET_MCP_PROJECT_SERVICE_H
