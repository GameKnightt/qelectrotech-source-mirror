/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef QET_MCP_METADATA_H
#define QET_MCP_METADATA_H

#include <QString>
#include <QStringList>

namespace QetMcp {

QString latestProtocolVersion();
QStringList supportedProtocolVersions();

QString inspectProjectToolName();
QString validateProjectToolName();
QString createProjectToolName();
QString setTitleBlockToolName();
QStringList toolNames();

} // namespace QetMcp

#endif // QET_MCP_METADATA_H
