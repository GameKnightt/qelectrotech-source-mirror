/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "qetmcpserver.h"

#include "qetmcpmetadata.h"
#include "qetmcpprojectservice.h"
#include "qetversion.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QTextStream>

#include <cstddef>
#include <iostream>
#include <string>

namespace {

constexpr std::size_t maxMessageBytes = 1024 * 1024;

bool readBoundedLine(
	std::istream &input,
	std::string *line,
	bool *limit_exceeded)
{
	line->clear();
	*limit_exceeded = false;
	char character = '\0';
	while (input.get(character)) {
		if (character == '\n')
			return true;
		if (line->size() < maxMessageBytes)
			line->push_back(character);
		else
			*limit_exceeded = true;
	}
	return !line->empty() || *limit_exceeded;
}

QJsonObject objectSchema(
	const QJsonObject &properties,
	const QJsonArray &required = {})
{
	QJsonObject schema;
	schema.insert(QStringLiteral("type"), QStringLiteral("object"));
	schema.insert(QStringLiteral("properties"), properties);
	schema.insert(QStringLiteral("additionalProperties"), false);
	if (!required.isEmpty())
		schema.insert(QStringLiteral("required"), required);
	return schema;
}

QJsonObject stringProperty(const QString &description, int max_length = 0)
{
	QJsonObject property {
		{QStringLiteral("type"), QStringLiteral("string")},
		{QStringLiteral("description"), description}
	};
	if (max_length > 0)
		property.insert(QStringLiteral("maxLength"), max_length);
	return property;
}

QJsonObject booleanProperty(const QString &description)
{
	return {
		{QStringLiteral("type"), QStringLiteral("boolean")},
		{QStringLiteral("description"), description}
	};
}

QJsonObject annotations(
	bool read_only,
	bool destructive,
	bool idempotent)
{
	return {
		{QStringLiteral("readOnlyHint"), read_only},
		{QStringLiteral("destructiveHint"), destructive},
		{QStringLiteral("idempotentHint"), idempotent},
		{QStringLiteral("openWorldHint"), false}
	};
}

QJsonArray toolDefinitions()
{
	const QJsonObject path_property = stringProperty(
		QStringLiteral(
			"Chemin absolu d’un projet .qet situé dans un répertoire autorisé."),
		32767);
	const QJsonObject output_property = stringProperty(
		QStringLiteral(
			"Nouvelle destination .qet. Le fichier ne doit pas déjà exister."),
		32767);
	const QJsonObject confirm_property = booleanProperty(QStringLiteral(
		"Doit valoir true après confirmation explicite de l’utilisateur."));

	QJsonArray tools;
	tools.append(QJsonObject {
		{QStringLiteral("name"), QetMcp::inspectProjectToolName()},
		{QStringLiteral("title"), QStringLiteral("Inspecter un projet QElectroTech")},
		{QStringLiteral("description"), QStringLiteral(
			"Charge un projet avec le vrai modèle QElectroTech et retourne ses "
			"folios, éléments, conducteurs et bornes non connectées.")},
		{QStringLiteral("inputSchema"), objectSchema(
			{{QStringLiteral("path"), path_property}},
			{QStringLiteral("path")})},
		{QStringLiteral("annotations"), annotations(true, false, true)}
	});
	tools.append(QJsonObject {
		{QStringLiteral("name"), QetMcp::validateProjectToolName()},
		{QStringLiteral("title"), QStringLiteral("Valider un projet QElectroTech")},
		{QStringLiteral("description"), QStringLiteral(
			"Vérifie le XML, le chargement par QElectroTech, les UUID de folio "
			"et signale les folios sans titre sans modifier le fichier.")},
		{QStringLiteral("inputSchema"), objectSchema(
			{{QStringLiteral("path"), path_property}},
			{QStringLiteral("path")})},
		{QStringLiteral("annotations"), annotations(true, false, true)}
	});

	QJsonObject create_properties {
		{QStringLiteral("outputPath"), output_property},
		{QStringLiteral("title"), stringProperty(
			QStringLiteral("Titre du nouveau projet."),
			1024)},
		{QStringLiteral("folios"), QJsonObject {
			{QStringLiteral("type"), QStringLiteral("array")},
			{QStringLiteral("description"), QStringLiteral(
				"Titres des folios à créer, dans l’ordre.")},
			{QStringLiteral("maxItems"), 200},
			{QStringLiteral("items"), QJsonObject {
				{QStringLiteral("type"), QStringLiteral("string")},
				{QStringLiteral("minLength"), 1},
				{QStringLiteral("maxLength"), 1024}
			}}
		}},
		{QStringLiteral("confirm"), confirm_property}
	};
	tools.append(QJsonObject {
		{QStringLiteral("name"), QetMcp::createProjectToolName()},
		{QStringLiteral("title"), QStringLiteral("Créer un projet QElectroTech")},
		{QStringLiteral("description"), QStringLiteral(
			"Crée un nouveau projet XML .qet avec des folios vides. Requiert "
			"--mcp-write, confirm=true et une destination inexistante.")},
		{QStringLiteral("inputSchema"), objectSchema(
			create_properties,
			{QStringLiteral("outputPath"), QStringLiteral("confirm")})},
		{QStringLiteral("annotations"), annotations(false, false, false)}
	});

	QJsonObject titleblock_properties {
		{QStringLiteral("inputPath"), path_property},
		{QStringLiteral("outputPath"), output_property},
		{QStringLiteral("fields"), QJsonObject {
			{QStringLiteral("type"), QStringLiteral("object")},
			{QStringLiteral("description"), QStringLiteral(
				"Champs de cartouche. Clés standard : title, author, date, "
				"plant, location, revision, version, filename. Les autres "
				"clés deviennent des champs personnalisés.")},
			{QStringLiteral("additionalProperties"), QJsonObject {
				{QStringLiteral("type"), QStringLiteral("string")},
				{QStringLiteral("maxLength"), 8192}
			}},
			{QStringLiteral("propertyNames"), QJsonObject {
				{QStringLiteral("maxLength"), 128}
			}},
			{QStringLiteral("minProperties"), 1},
			{QStringLiteral("maxProperties"), 64}
		}},
		{QStringLiteral("confirm"), confirm_property}
	};
	tools.append(QJsonObject {
		{QStringLiteral("name"), QetMcp::setTitleBlockToolName()},
		{QStringLiteral("title"), QStringLiteral("Mettre à jour les cartouches")},
		{QStringLiteral("description"), QStringLiteral(
			"Applique des champs de cartouche au projet et à tous ses folios, "
			"puis écrit une nouvelle copie sans remplacer la source.")},
		{QStringLiteral("inputSchema"), objectSchema(
			titleblock_properties,
			{
				QStringLiteral("inputPath"),
				QStringLiteral("outputPath"),
				QStringLiteral("fields"),
				QStringLiteral("confirm")
			})},
		{QStringLiteral("annotations"), annotations(false, false, false)}
	});
	return tools;
}

QJsonObject response(const QJsonValue &id, const QJsonObject &result)
{
	return {
		{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
		{QStringLiteral("id"), id},
		{QStringLiteral("result"), result}
	};
}

QJsonObject errorResponse(
	const QJsonValue &id,
	int code,
	const QString &message,
	const QJsonValue &data = {})
{
	QJsonObject error {
		{QStringLiteral("code"), code},
		{QStringLiteral("message"), message}
	};
	if (!data.isUndefined())
		error.insert(QStringLiteral("data"), data);
	return {
		{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
		{QStringLiteral("id"), id},
		{QStringLiteral("error"), error}
	};
}

QJsonObject toolResult(const QetMcp::ToolExecutionResult &execution)
{
	const QString text = execution.ok
		? QString::fromUtf8(
			QJsonDocument(execution.data).toJson(QJsonDocument::Compact))
		: execution.error;
	QJsonObject result {
		{QStringLiteral("content"), QJsonArray {
			QJsonObject {
				{QStringLiteral("type"), QStringLiteral("text")},
				{QStringLiteral("text"), text}
			}
		}},
		{QStringLiteral("isError"), !execution.ok}
	};
	if (execution.ok)
		result.insert(QStringLiteral("structuredContent"), execution.data);
	return result;
}

class ProtocolSession
{
	public:
		explicit ProtocolSession(const QetMcp::ProjectService &service) :
			m_service(service)
		{
		}

		bool handle(const QJsonObject &request, QJsonObject *reply)
		{
			const bool has_id = request.contains(QStringLiteral("id"));
			const QJsonValue supplied_id =
				request.value(QStringLiteral("id"));
			const bool valid_id = !has_id
				|| supplied_id.isString()
				|| supplied_id.isDouble();
			const QJsonValue id = valid_id && has_id
				? supplied_id
				: QJsonValue::Null;
			const QJsonValue method_value =
				request.value(QStringLiteral("method"));
			const QString method = method_value.toString();

			if (request.value(QStringLiteral("jsonrpc")).toString()
					!= QStringLiteral("2.0")
				|| !method_value.isString()
				|| method.isEmpty()
				|| !valid_id) {
				*reply = errorResponse(
					QJsonValue::Null,
					-32600,
					QStringLiteral("Invalid Request"));
				return true;
			}

			if (!has_id) {
				if (method == QStringLiteral("notifications/initialized")
					&& m_initialize_seen) {
					m_initialized = true;
				}
				return false;
			}

			if (method == QStringLiteral("initialize")) {
				if (m_initialize_seen) {
					*reply = errorResponse(
						id,
						-32600,
						QStringLiteral("Server already initialized"));
					return true;
				}
				const QJsonValue params_value =
					request.value(QStringLiteral("params"));
				if (!params_value.isObject()) {
					*reply = errorResponse(
						id,
						-32602,
						QStringLiteral("Initialize params must be an object"));
					return true;
				}
				const QJsonObject params = params_value.toObject();
				const QString requested =
					params.value(QStringLiteral("protocolVersion")).toString();
				const QJsonValue client_capabilities =
					params.value(QStringLiteral("capabilities"));
				const QJsonValue client_info_value =
					params.value(QStringLiteral("clientInfo"));
				const QJsonObject client_info =
					client_info_value.toObject();
				if (requested.isEmpty()
					|| !client_capabilities.isObject()
					|| !client_info_value.isObject()
					|| client_info
						.value(QStringLiteral("name")).toString().isEmpty()
					|| client_info
						.value(QStringLiteral("version")).toString().isEmpty()) {
					*reply = errorResponse(
						id,
						-32602,
						QStringLiteral(
							"Initialize requires protocolVersion, capabilities "
							"and clientInfo.name/version"));
					return true;
				}
				m_initialize_seen = true;
				const QString negotiated =
					QetMcp::supportedProtocolVersions().contains(requested)
					? requested
					: QetMcp::latestProtocolVersion();
				QJsonObject capabilities;
				capabilities.insert(
					QStringLiteral("tools"),
					QJsonObject {{QStringLiteral("listChanged"), false}});

				QJsonObject result {
					{QStringLiteral("protocolVersion"), negotiated},
					{QStringLiteral("capabilities"), capabilities},
					{QStringLiteral("serverInfo"), QJsonObject {
						{QStringLiteral("name"), QStringLiteral("qelectrotech-mcp")},
						{QStringLiteral("title"), QStringLiteral("QElectroTech MCP")},
						{QStringLiteral("version"), QetVersion::displayedVersion()},
						{QStringLiteral("description"), QStringLiteral(
							"Serveur local pour inspecter et produire des projets "
							"QElectroTech avec le modèle C++/Qt natif.")}
					}},
					{QStringLiteral("instructions"), QStringLiteral(
						"Travaillez uniquement dans les répertoires annoncés. "
						"Validez avant toute écriture, utilisez une destination "
						"distincte et demandez l’accord humain avant confirm=true.")}
				};
				*reply = response(id, result);
				return true;
			}

			if (method == QStringLiteral("ping")) {
				*reply = response(id, {});
				return true;
			}

			if (!m_initialize_seen || !m_initialized) {
				*reply = errorResponse(
					id,
					-32002,
					QStringLiteral("Server not initialized"));
				return true;
			}

			if (method == QStringLiteral("tools/list")) {
				*reply = response(
					id,
					QJsonObject {{QStringLiteral("tools"), toolDefinitions()}});
				return true;
			}

			if (method == QStringLiteral("tools/call")) {
				const QJsonValue params_value =
					request.value(QStringLiteral("params"));
				if (!params_value.isObject()) {
					*reply = errorResponse(
						id,
						-32602,
						QStringLiteral("Tool call params must be an object"));
					return true;
				}
				const QJsonObject params = params_value.toObject();
				const QString name =
					params.value(QStringLiteral("name")).toString();
				if (name.isEmpty()) {
					*reply = errorResponse(
						id,
						-32602,
						QStringLiteral("Missing tool name"));
					return true;
				}
				const bool known = QetMcp::toolNames().contains(name);
				if (!known) {
					*reply = errorResponse(
						id,
						-32602,
						QStringLiteral("Unknown tool: %1").arg(name));
					return true;
				}
				const QJsonValue arguments_value =
					params.value(QStringLiteral("arguments"));
				if (!arguments_value.isUndefined()
					&& !arguments_value.isObject()) {
					*reply = errorResponse(
						id,
						-32602,
						QStringLiteral("Tool arguments must be an object"));
					return true;
				}
				const auto execution = m_service.execute(
					name,
					arguments_value.toObject());
				*reply = response(id, toolResult(execution));
				return true;
			}

			*reply = errorResponse(
				id,
				-32601,
				QStringLiteral("Method not found: %1").arg(method));
			return true;
		}

	private:
		const QetMcp::ProjectService &m_service;
		bool m_initialize_seen = false;
		bool m_initialized = false;
};

} // namespace

namespace QetMcp {

bool isServerRequest(const QStringList &arguments)
{
	return arguments.contains(QStringLiteral("--mcp-stdio"));
}

int runStdioServer(const QStringList &arguments)
{
	QStringList roots;
	bool write_enabled = false;
	for (int i = 0; i < arguments.size(); ++i) {
		const QString argument = arguments.at(i);
		if (argument == QStringLiteral("--mcp-root")) {
			if (i + 1 >= arguments.size()) {
				QTextStream(stderr)
					<< "Missing path after --mcp-root." << Qt::endl;
				return 2;
			}
			roots.append(arguments.at(++i));
		} else if (argument == QStringLiteral("--mcp-write")) {
			write_enabled = true;
		}
	}
	if (roots.isEmpty())
		roots.append(QDir::currentPath());

	ProjectService service(roots, write_enabled);
	if (!service.isValid()) {
		QTextStream(stderr) << service.errorString() << Qt::endl;
		return 2;
	}

	ProtocolSession session(service);

	std::string std_line;
	bool limit_exceeded = false;
	while (readBoundedLine(std::cin, &std_line, &limit_exceeded)) {
		if (limit_exceeded) {
			const QByteArray serialized = QJsonDocument(errorResponse(
				QJsonValue::Null,
				-32600,
				QStringLiteral("Request exceeds the 1 MiB stdio limit")))
				.toJson(QJsonDocument::Compact);
			std::cout.write(serialized.constData(), serialized.size());
			std::cout.put('\n');
			std::cout.flush();
			continue;
		}
		const QByteArray line = QByteArray::fromStdString(std_line);
		if (line.trimmed().isEmpty())
			continue;

		QJsonParseError parse_error;
		const QJsonDocument document =
			QJsonDocument::fromJson(line, &parse_error);
		QJsonObject reply;
		bool has_reply = false;
		if (parse_error.error != QJsonParseError::NoError) {
			reply = errorResponse(
				QJsonValue::Null,
				-32700,
				QStringLiteral("Parse error"),
				parse_error.errorString());
			has_reply = true;
		} else if (!document.isObject()) {
			reply = errorResponse(
				QJsonValue::Null,
				-32600,
				QStringLiteral("Invalid Request"));
			has_reply = true;
		} else {
			has_reply = session.handle(document.object(), &reply);
		}
		if (has_reply) {
			const QByteArray serialized =
				QJsonDocument(reply).toJson(QJsonDocument::Compact);
			std::cout.write(serialized.constData(), serialized.size());
			std::cout.put('\n');
			std::cout.flush();
		}
	}
	return 0;
}

} // namespace QetMcp
