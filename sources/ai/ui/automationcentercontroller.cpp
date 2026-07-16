/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "automationcentercontroller.h"

#include "ai/qetmcpmetadata.h"

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

AutomationCenterController::AutomationCenterController(QObject *parent) :
	QObject(parent)
{
	m_scope_directory =
		QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	if (m_scope_directory.isEmpty())
		m_scope_directory = QDir::homePath();
}

QString AutomationCenterController::projectTitle() const
{
	return m_project_title;
}

QString AutomationCenterController::projectPath() const
{
	return m_project_path;
}

QString AutomationCenterController::scopeDirectory() const
{
	return m_scope_directory;
}

int AutomationCenterController::folioCount() const
{
	return m_folio_count;
}

int AutomationCenterController::elementCount() const
{
	return m_element_count;
}

int AutomationCenterController::conductorCount() const
{
	return m_conductor_count;
}

bool AutomationCenterController::writeAccess() const
{
	return m_write_access;
}

QString AutomationCenterController::serverCommand() const
{
	QString command = quoted(QDir::toNativeSeparators(
		QCoreApplication::applicationFilePath()));
	command += QStringLiteral(" --mcp-stdio --mcp-root ");
	command += quoted(QDir::toNativeSeparators(m_scope_directory));
	if (m_write_access)
		command += QStringLiteral(" --mcp-write");
	return command;
}

QString AutomationCenterController::configurationSnippet() const
{
	QJsonArray arguments {
		QStringLiteral("--mcp-stdio"),
		QStringLiteral("--mcp-root"),
		QDir::toNativeSeparators(m_scope_directory)
	};
	if (m_write_access)
		arguments.append(QStringLiteral("--mcp-write"));

	const QJsonObject server {
		{QStringLiteral("command"), QDir::toNativeSeparators(
			QCoreApplication::applicationFilePath())},
		{QStringLiteral("args"), arguments}
	};
	const QJsonObject root {
		{QStringLiteral("mcpServers"), QJsonObject {
			{QStringLiteral("qelectrotech"), server}
		}}
	};
	return QString::fromUtf8(
		QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QString AutomationCenterController::statusMessage() const
{
	return m_status_message;
}

QStringList AutomationCenterController::availableTools() const
{
	return QetMcp::toolNames();
}

QString AutomationCenterController::protocolVersion() const
{
	return QStringLiteral("MCP %1 · stdio")
		.arg(QetMcp::latestProtocolVersion());
}

void AutomationCenterController::setProjectSnapshot(
	const QString &title,
	const QString &path,
	int folios,
	int elements,
	int conductors)
{
	const QString scope = path.isEmpty()
		? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
		: QFileInfo(path).absolutePath();
	const QString effective_scope =
		scope.isEmpty() ? QDir::homePath() : scope;
	const QString effective_title =
		title.isEmpty() && !path.isEmpty()
		? QFileInfo(path).completeBaseName()
		: title;
	if (m_project_title == effective_title
		&& m_project_path == path
		&& m_scope_directory == effective_scope
		&& m_folio_count == folios
		&& m_element_count == elements
		&& m_conductor_count == conductors) {
		return;
	}

	m_project_title = effective_title;
	m_project_path = path;
	m_scope_directory = effective_scope;
	m_folio_count = folios;
	m_element_count = elements;
	m_conductor_count = conductors;
	emit projectChanged();
	emit commandChanged();
}

void AutomationCenterController::setWriteAccess(bool enabled)
{
	if (m_write_access == enabled)
		return;
	m_write_access = enabled;
	setStatusMessage(enabled
		? tr("Mode écriture préparé. Chaque écriture exige encore confirm=true "
			 "et une nouvelle destination.")
		: tr("Mode lecture seule actif."));
	emit writeAccessChanged();
	emit commandChanged();
}

void AutomationCenterController::copyCommand()
{
	QApplication::clipboard()->setText(serverCommand());
	setStatusMessage(tr("Commande MCP copiée dans le presse-papiers."));
}

void AutomationCenterController::copyConfiguration()
{
	QApplication::clipboard()->setText(configurationSnippet());
	setStatusMessage(tr("Configuration MCP copiée dans le presse-papiers."));
}

QString AutomationCenterController::quoted(const QString &value) const
{
	QString escaped = value;
	escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
	return QLatin1Char('"') + escaped + QLatin1Char('"');
}

void AutomationCenterController::setStatusMessage(const QString &message)
{
	if (m_status_message == message)
		return;
	m_status_message = message;
	emit statusMessageChanged();
}
