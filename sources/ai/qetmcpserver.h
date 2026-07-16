/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef QET_MCP_SERVER_H
#define QET_MCP_SERVER_H

#include <QStringList>

namespace QetMcp {

bool isServerRequest(const QStringList &arguments);
int runStdioServer(const QStringList &arguments);

} // namespace QetMcp

#endif // QET_MCP_SERVER_H
