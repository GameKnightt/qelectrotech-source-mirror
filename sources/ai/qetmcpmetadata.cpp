/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "qetmcpmetadata.h"

namespace QetMcp {

QString latestProtocolVersion()
{
	return QStringLiteral("2025-11-25");
}

QStringList supportedProtocolVersions()
{
	return {
		latestProtocolVersion(),
		QStringLiteral("2025-06-18"),
		QStringLiteral("2025-03-26"),
		QStringLiteral("2024-11-05")
	};
}

QString inspectProjectToolName()
{
	return QStringLiteral("qet.project.inspect");
}

QString validateProjectToolName()
{
	return QStringLiteral("qet.project.validate");
}

QString createProjectToolName()
{
	return QStringLiteral("qet.project.create");
}

QString setTitleBlockToolName()
{
	return QStringLiteral("qet.project.set_titleblock");
}

QStringList toolNames()
{
	return {
		inspectProjectToolName(),
		validateProjectToolName(),
		createProjectToolName(),
		setTitleBlockToolName()
	};
}

} // namespace QetMcp
