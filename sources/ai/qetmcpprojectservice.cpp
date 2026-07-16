/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "qetmcpprojectservice.h"

#include "diagram.h"
#include "qetmcpmetadata.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qet.h"
#include "qetproject.h"
#include "qetversion.h"
#include "titleblockproperties.h"

#include <QDate>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>
#include <QLocale>
#include <QSet>
#include <QTime>
#include <QUuid>
#include <QVersionNumber>
#include <QXmlStreamReader>

#ifdef Q_OS_UNIX
#	include <unistd.h>
#endif

namespace {

using QetMcp::ToolExecutionResult;

constexpr qint64 maxProjectBytes = 512LL * 1024LL * 1024LL;
constexpr int maxPathLength = 32767;
constexpr int maxProjectTitleLength = 1024;
constexpr int maxFolioTitleLength = 1024;
constexpr int maxTitleBlockKeyLength = 128;
constexpr int maxTitleBlockValueLength = 8192;

ToolExecutionResult executionError(const QString &message)
{
	ToolExecutionResult result;
	result.error = message;
	return result;
}

ToolExecutionResult executionSuccess(const QJsonObject &data)
{
	ToolExecutionResult result;
	result.ok = true;
	result.data = data;
	return result;
}

QString requiredString(
	const QJsonObject &arguments,
	const QString &key,
	QString *error)
{
	const QJsonValue value = arguments.value(key);
	if (!value.isString() || value.toString().trimmed().isEmpty()) {
		*error = QStringLiteral("Le paramètre « %1 » doit être une chaîne non vide.")
			.arg(key);
		return {};
	}
	const QString text = value.toString().trimmed();
	if (text.size() > maxPathLength) {
		*error = QStringLiteral(
			"Le paramètre « %1 » dépasse la longueur maximale autorisée.")
			.arg(key);
		return {};
	}
	return text;
}

bool rejectUnknownArguments(
	const QJsonObject &arguments,
	const QSet<QString> &allowed,
	QString *error)
{
	for (auto it = arguments.constBegin(); it != arguments.constEnd(); ++it) {
		if (!allowed.contains(it.key())) {
			*error = QStringLiteral("Paramètre non reconnu : %1").arg(it.key());
			return false;
		}
	}
	return true;
}

bool confirmed(const QJsonObject &arguments, QString *error)
{
	if (!arguments.value(QStringLiteral("confirm")).toBool(false)) {
		*error = QStringLiteral(
			"Cette écriture exige confirm=true après validation explicite "
			"de la destination par l’utilisateur.");
		return false;
	}
	return true;
}

int freeTerminals(Element *element)
{
	int count = 0;
	for (Terminal *terminal : element->terminals()) {
		if (terminal && terminal->conductorsCount() == 0)
			++count;
	}
	return count;
}

bool preflightProjectVersion(const QString &path, QString *error)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		*error = QStringLiteral("Impossible de lire « %1 ».").arg(path);
		return false;
	}

	QXmlStreamReader xml(&file);
	while (!xml.atEnd()) {
		xml.readNext();
		if (!xml.isStartElement())
			continue;
		if (xml.name() != QStringLiteral("project")) {
			*error = QStringLiteral("La racine XML attendue est <project>.");
			return false;
		}

		const QString version_text =
			xml.attributes().value(QStringLiteral("version")).toString();
		const QVersionNumber version =
			QVersionNumber::fromString(version_text);
		if (!version.isNull() && QetVersion::currentVersion() < version) {
			*error = QStringLiteral(
				"Le projet déclare QElectroTech %1, plus récent que ce serveur "
				"(%2). L’ouverture automatique est refusée pour éviter un "
				"dialogue bloquant et une perte de données.")
				.arg(version_text, QetVersion::currentVersion().toString());
			return false;
		}
		if (!version.isNull()
			&& version <= QetVersion::versionZeroDotSix()) {
			*error = QStringLiteral(
				"Les projets QElectroTech 0.6 ou antérieurs exigent une "
				"migration interactive avec une version intermédiaire.");
			return false;
		}
		return true;
	}

	*error = xml.hasError()
		? QStringLiteral("XML invalide : %1").arg(xml.errorString())
		: QStringLiteral("Le document ne contient aucun élément racine.");
	return false;
}

bool writeProjectToNewPath(
	QETProject *project,
	const QString &output_path,
	QString *error)
{
	const QString temporary_path = QDir(QFileInfo(output_path).absolutePath())
		.filePath(QStringLiteral(".qet-mcp-%1.qet").arg(
			QUuid::createUuid().toString(QUuid::WithoutBraces)));
	if (QFileInfo::exists(temporary_path)) {
		*error = QStringLiteral(
			"Impossible de réserver un nom de fichier temporaire unique.");
		return false;
	}

	DiagramContext properties = project->projectProperties();
	const QDate current_date = QDate::currentDate();
	properties.addValue(
		QStringLiteral("saveddate"),
		QLocale::system().toString(current_date, QLocale::ShortFormat));
	properties.addValue(
		QStringLiteral("saveddate-us"),
		current_date.toString(QStringLiteral("yyyy-MM-dd")));
	properties.addValue(
		QStringLiteral("saveddate-eu"),
		current_date.toString(QStringLiteral("dd-MM-yyyy")));
	properties.addValue(
		QStringLiteral("savedtime"),
		QTime::currentTime().toString(QStringLiteral("HH:mm")));
	properties.addValue(
		QStringLiteral("savedfilename"),
		QFileInfo(output_path).baseName());
	properties.addValue(QStringLiteral("savedfilepath"), output_path);
	project->setProjectProperties(properties);

	QDomDocument document = project->toXml();
	QString write_error;
	if (!QET::writeXmlFile(document, temporary_path, &write_error)) {
		QFile::remove(temporary_path);
		*error = write_error;
		return false;
	}
	bool published = false;
#ifdef Q_OS_WIN
	// QFile::rename maps to a no-replace move on Windows. Both paths are in the
	// same directory so publication remains atomic.
	published = QFile::rename(temporary_path, output_path);
#elif defined(Q_OS_UNIX)
	// POSIX link() atomically creates the destination only when it is absent.
	// Both paths share a directory and therefore a filesystem.
	const QByteArray temporary_native = QFile::encodeName(temporary_path);
	const QByteArray output_native = QFile::encodeName(output_path);
	published = ::link(
		temporary_native.constData(),
		output_native.constData()) == 0;
	if (published && ::unlink(temporary_native.constData()) != 0)
		QFile::remove(temporary_path);
#else
	published = QFile::rename(temporary_path, output_path);
#endif

	if (!published) {
		QFile::remove(temporary_path);
		*error = QFileInfo::exists(output_path)
			? QStringLiteral(
				"La destination a été créée pendant l’opération. Aucun fichier "
				"existant n’a été remplacé : %1").arg(output_path)
			: QStringLiteral(
				"Impossible de finaliser la nouvelle destination : %1")
				.arg(output_path);
		return false;
	}
	return true;
}

QJsonObject projectSummary(QETProject &project, const QString &path)
{
	QJsonArray folios;
	int total_elements = 0;
	int total_conductors = 0;
	int total_free_terminals = 0;
	int index = 0;

	for (Diagram *diagram : project.diagrams()) {
		++index;
		int elements = 0;
		int conductors = 0;
		int free_terminals = 0;
		for (QGraphicsItem *item : diagram->items()) {
			if (Element *element = qgraphicsitem_cast<Element *>(item)) {
				++elements;
				free_terminals += freeTerminals(element);
			} else if (qgraphicsitem_cast<Conductor *>(item)) {
				++conductors;
			}
		}

		QJsonObject folio;
		folio.insert(QStringLiteral("index"), index);
		folio.insert(
			QStringLiteral("uuid"),
			diagram->uuid().toString(QUuid::WithoutBraces));
		folio.insert(QStringLiteral("title"), diagram->title());
		folio.insert(QStringLiteral("elements"), elements);
		folio.insert(QStringLiteral("conductors"), conductors);
		folio.insert(QStringLiteral("freeTerminals"), free_terminals);
		folios.append(folio);

		total_elements += elements;
		total_conductors += conductors;
		total_free_terminals += free_terminals;
	}

	QJsonObject summary;
	summary.insert(QStringLiteral("path"), path);
	summary.insert(QStringLiteral("title"), project.title());
	summary.insert(
		QStringLiteral("uuid"),
		project.uuid().toString(QUuid::WithoutBraces));
	summary.insert(
		QStringLiteral("declaredQElectroTechVersion"),
		project.declaredQElectroTechVersion().toString());
	summary.insert(QStringLiteral("folios"), folios.size());
	summary.insert(QStringLiteral("elements"), total_elements);
	summary.insert(QStringLiteral("conductors"), total_conductors);
	summary.insert(QStringLiteral("freeTerminals"), total_free_terminals);
	summary.insert(QStringLiteral("folioList"), folios);
	return summary;
}

bool applyTitleBlockFields(
	TitleBlockProperties *properties,
	const QJsonObject &fields,
	QString *error)
{
	for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
		if (!it.value().isString()) {
			*error = QStringLiteral(
				"Le champ de cartouche « %1 » doit être une chaîne.")
				.arg(it.key());
			return false;
		}

		const QString key = it.key().trimmed();
		const QString lower_key = key.toLower();
		const QString value = it.value().toString();
		if (key.isEmpty()) {
			*error = QStringLiteral("Un nom de champ de cartouche est vide.");
			return false;
		}
		if (key.size() > maxTitleBlockKeyLength) {
			*error = QStringLiteral(
				"Le nom de champ de cartouche « %1 » est trop long.").arg(key);
			return false;
		}
		if (value.size() > maxTitleBlockValueLength) {
			*error = QStringLiteral(
				"La valeur du champ de cartouche « %1 » dépasse 8192 "
				"caractères.").arg(key);
			return false;
		}

		if (lower_key == QStringLiteral("title"))
			properties->title = value;
		else if (lower_key == QStringLiteral("author"))
			properties->author = value;
		else if (lower_key == QStringLiteral("filename"))
			properties->filename = value;
		else if (lower_key == QStringLiteral("plant"))
			properties->plant = value;
		else if (lower_key == QStringLiteral("location"))
			properties->locmach = value;
		else if (lower_key == QStringLiteral("revision"))
			properties->indexrev = value;
		else if (lower_key == QStringLiteral("version"))
			properties->version = value;
		else if (lower_key == QStringLiteral("date")) {
			const QDate date = value.compare(
				QStringLiteral("today"),
				Qt::CaseInsensitive) == 0
				? QDate::currentDate()
				: QDate::fromString(value, Qt::ISODate);
			if (!date.isValid()) {
				*error = QStringLiteral(
					"La date « %1 » doit utiliser YYYY-MM-DD ou « today ».")
					.arg(value);
				return false;
			}
			properties->date = date;
			properties->useDate = TitleBlockProperties::UseDateValue;
		} else {
			properties->context.addValue(key, value);
		}
	}
	return true;
}

} // namespace

namespace QetMcp {

ProjectService::ProjectService(const QStringList &roots, bool write_enabled) :
	m_write_enabled(write_enabled)
{
	if (roots.size() > 32) {
		m_error = QStringLiteral(
			"Le serveur MCP accepte au maximum 32 répertoires autorisés.");
		return;
	}
	for (const QString &root : roots) {
		const QFileInfo info(root);
		if (!info.exists() || !info.isDir()) {
			m_error = QStringLiteral(
				"Le répertoire MCP autorisé n’existe pas : %1").arg(root);
			return;
		}
		const QString canonical = QDir::cleanPath(info.canonicalFilePath());
		if (!m_roots.contains(canonical, Qt::CaseInsensitive)) {
			if (m_roots.size() >= 32) {
				m_error = QStringLiteral(
					"Le serveur MCP accepte au maximum 32 répertoires autorisés.");
				return;
			}
			m_roots.append(canonical);
		}
	}
	if (m_roots.isEmpty()) {
		m_error = QStringLiteral(
			"Au moins un répertoire autorisé doit être fourni avec --mcp-root.");
	}
}

bool ProjectService::isValid() const
{
	return m_error.isEmpty();
}

QString ProjectService::errorString() const
{
	return m_error;
}

QStringList ProjectService::roots() const
{
	return m_roots;
}

bool ProjectService::writeEnabled() const
{
	return m_write_enabled;
}

ToolExecutionResult ProjectService::execute(
	const QString &tool_name,
	const QJsonObject &arguments) const
{
	if (tool_name == inspectProjectToolName())
		return inspectProject(arguments);
	if (tool_name == validateProjectToolName())
		return validateProject(arguments);
	if (tool_name == createProjectToolName())
		return createProject(arguments);
	if (tool_name == setTitleBlockToolName())
		return setTitleBlock(arguments);
	return executionError(
		QStringLiteral("Outil MCP QElectroTech inconnu : %1").arg(tool_name));
}

ToolExecutionResult ProjectService::inspectProject(
	const QJsonObject &arguments) const
{
	QString error;
	if (!rejectUnknownArguments(
		arguments,
		{QStringLiteral("path")},
		&error)) {
		return executionError(error);
	}
	const QString candidate = requiredString(
		arguments,
		QStringLiteral("path"),
		&error);
	QString path;
	if (!error.isEmpty() || !resolveInputProject(candidate, &path, &error))
		return executionError(error);
	if (!preflightProjectVersion(path, &error))
		return executionError(error);

	QETProject project(path, QETProject::OpeningMode::NonInteractive);
	if (project.state() != QETProject::Ok) {
		return executionError(QStringLiteral(
			"QElectroTech n’a pas pu charger le projet « %1 » (état %2).")
			.arg(path)
			.arg(static_cast<int>(project.state())));
	}
	return executionSuccess(projectSummary(project, path));
}

ToolExecutionResult ProjectService::validateProject(
	const QJsonObject &arguments) const
{
	QString error;
	if (!rejectUnknownArguments(
		arguments,
		{QStringLiteral("path")},
		&error)) {
		return executionError(error);
	}
	const QString candidate = requiredString(
		arguments,
		QStringLiteral("path"),
		&error);
	QString path;
	if (!error.isEmpty() || !resolveInputProject(candidate, &path, &error))
		return executionError(error);

	QJsonArray issues;
	bool can_load_model = true;
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			return executionError(
				QStringLiteral("Impossible de lire « %1 ».").arg(path));
		}

		QDomDocument document;
		QString xml_error;
		int error_line = 0;
		int error_column = 0;
		if (!document.setContent(
				&file,
				&xml_error,
				&error_line,
				&error_column)) {
			QJsonObject issue;
			issue.insert(QStringLiteral("severity"), QStringLiteral("error"));
			issue.insert(QStringLiteral("code"), QStringLiteral("xml_parse_error"));
			issue.insert(
				QStringLiteral("message"),
				QStringLiteral("Erreur XML ligne %1, colonne %2 : %3")
					.arg(error_line)
					.arg(error_column)
					.arg(xml_error));
			issues.append(issue);

			QJsonObject result;
			result.insert(QStringLiteral("path"), path);
			result.insert(QStringLiteral("valid"), false);
			result.insert(QStringLiteral("errors"), 1);
			result.insert(QStringLiteral("warnings"), 0);
			result.insert(QStringLiteral("issues"), issues);
			return executionSuccess(result);
		}
		file.close();

		const QDomElement root = document.documentElement();
		if (root.tagName() != QStringLiteral("project")) {
			QJsonObject issue;
			issue.insert(QStringLiteral("severity"), QStringLiteral("error"));
			issue.insert(QStringLiteral("code"), QStringLiteral("invalid_root"));
			issue.insert(
				QStringLiteral("message"),
				QStringLiteral("La racine XML attendue est <project>."));
			issues.append(issue);
			can_load_model = false;
		} else {
			const QVersionNumber version = QetVersion::fromXmlAttribute(root);
			if (!version.isNull() && QetVersion::currentVersion() < version) {
				QJsonObject issue;
				issue.insert(QStringLiteral("severity"), QStringLiteral("error"));
				issue.insert(
					QStringLiteral("code"),
					QStringLiteral("unsupported_newer_version"));
				issue.insert(
					QStringLiteral("message"),
					QStringLiteral(
						"Le projet déclare QElectroTech %1, plus récent que ce "
						"serveur (%2).")
						.arg(
							root.attribute(QStringLiteral("version")),
							QetVersion::currentVersion().toString()));
				issues.append(issue);
				can_load_model = false;
			} else if (!version.isNull()
				&& version <= QetVersion::versionZeroDotSix()) {
				QJsonObject issue;
				issue.insert(QStringLiteral("severity"), QStringLiteral("error"));
				issue.insert(
					QStringLiteral("code"),
					QStringLiteral("legacy_migration_required"));
				issue.insert(
					QStringLiteral("message"),
					QStringLiteral(
						"Les projets QElectroTech 0.6 ou antérieurs exigent une "
						"migration interactive avec une version intermédiaire."));
				issues.append(issue);
				can_load_model = false;
			}
		}
	}

	QJsonObject summary;
	if (can_load_model) {
		QETProject project(path, QETProject::OpeningMode::NonInteractive);
		if (project.state() != QETProject::Ok) {
			QJsonObject issue;
			issue.insert(QStringLiteral("severity"), QStringLiteral("error"));
			issue.insert(
				QStringLiteral("code"),
				QStringLiteral("model_load_failed"));
			issue.insert(
				QStringLiteral("message"),
				QStringLiteral(
					"Le modèle QElectroTech refuse le projet (état %1).")
					.arg(static_cast<int>(project.state())));
			issues.append(issue);
		} else {
			summary = projectSummary(project, path);
			QSet<QUuid> folio_uuids;
			int folio_index = 0;
			for (Diagram *diagram : project.diagrams()) {
				++folio_index;
				const QUuid uuid = diagram->uuid();
				if (folio_uuids.contains(uuid)) {
					QJsonObject issue;
					issue.insert(
						QStringLiteral("severity"),
						QStringLiteral("error"));
					issue.insert(
						QStringLiteral("code"),
						QStringLiteral("duplicate_folio_uuid"));
					issue.insert(
						QStringLiteral("message"),
						QStringLiteral(
							"UUID de folio dupliqué au folio %1 : %2")
							.arg(folio_index)
							.arg(uuid.toString(QUuid::WithoutBraces)));
					issues.append(issue);
				}
				folio_uuids.insert(uuid);

				if (diagram->title().trimmed().isEmpty()) {
					QJsonObject issue;
					issue.insert(
						QStringLiteral("severity"),
						QStringLiteral("warning"));
					issue.insert(
						QStringLiteral("code"),
						QStringLiteral("untitled_folio"));
					issue.insert(
						QStringLiteral("message"),
						QStringLiteral(
							"Le folio %1 ne possède pas de titre.")
							.arg(folio_index));
					issues.append(issue);
				}
			}

			const int free_terminals =
				summary.value(QStringLiteral("freeTerminals")).toInt();
			if (free_terminals > 0) {
				QJsonObject issue;
				issue.insert(
					QStringLiteral("severity"),
					QStringLiteral("info"));
				issue.insert(
					QStringLiteral("code"),
					QStringLiteral("unconnected_terminals"));
				issue.insert(
					QStringLiteral("message"),
					QStringLiteral(
						"%1 borne(s) ne sont reliées à aucun conducteur.")
						.arg(free_terminals));
				issues.append(issue);
			}
		}
	}

	int errors = 0;
	int warnings = 0;
	for (const QJsonValue &value : issues) {
		const QString severity =
			value.toObject().value(QStringLiteral("severity")).toString();
		if (severity == QStringLiteral("error"))
			++errors;
		else if (severity == QStringLiteral("warning"))
			++warnings;
	}

	QJsonObject result;
	result.insert(QStringLiteral("path"), path);
	result.insert(QStringLiteral("valid"), errors == 0);
	result.insert(QStringLiteral("errors"), errors);
	result.insert(QStringLiteral("warnings"), warnings);
	result.insert(QStringLiteral("issues"), issues);
	if (!summary.isEmpty())
		result.insert(QStringLiteral("summary"), summary);
	return executionSuccess(result);
}

ToolExecutionResult ProjectService::createProject(
	const QJsonObject &arguments) const
{
	if (!m_write_enabled) {
		return executionError(QStringLiteral(
			"Les écritures MCP sont désactivées. Relancez le serveur avec "
			"--mcp-write après accord explicite de l’utilisateur."));
	}

	QString error;
	if (!rejectUnknownArguments(
		arguments,
		{
			QStringLiteral("outputPath"),
			QStringLiteral("title"),
			QStringLiteral("folios"),
			QStringLiteral("confirm")
		},
		&error)) {
		return executionError(error);
	}
	if (!confirmed(arguments, &error))
		return executionError(error);

	const QString candidate = requiredString(
		arguments,
		QStringLiteral("outputPath"),
		&error);
	QString output_path;
	if (!error.isEmpty()
		|| !resolveOutputProject(candidate, &output_path, &error)) {
		return executionError(error);
	}

	const QJsonValue title_value = arguments.value(QStringLiteral("title"));
	if (!title_value.isUndefined() && !title_value.isString()) {
		return executionError(QStringLiteral(
			"Le paramètre « title » doit être une chaîne."));
	}
	QString title = title_value.toString().trimmed();
	if (title.size() > maxProjectTitleLength) {
		return executionError(QStringLiteral(
			"Le titre du projet dépasse 1024 caractères."));
	}
	if (title.isEmpty())
		title = QFileInfo(output_path).completeBaseName();
	if (title.size() > maxProjectTitleLength) {
		return executionError(QStringLiteral(
			"Le titre du projet dérivé du nom de fichier dépasse 1024 "
			"caractères."));
	}

	QStringList folio_titles;
	const QJsonValue folios_value = arguments.value(QStringLiteral("folios"));
	if (!folios_value.isUndefined()) {
		if (!folios_value.isArray()) {
			return executionError(QStringLiteral(
				"Le paramètre « folios » doit être un tableau de titres."));
		}
		const QJsonArray folios = folios_value.toArray();
		if (folios.size() > 200) {
			return executionError(QStringLiteral(
				"La création est limitée à 200 folios par appel."));
		}
		for (const QJsonValue &value : folios) {
			if (!value.isString() || value.toString().trimmed().isEmpty()) {
				return executionError(QStringLiteral(
					"Chaque entrée de « folios » doit être un titre non vide."));
			}
			const QString folio_title = value.toString().trimmed();
			if (folio_title.size() > maxFolioTitleLength) {
				return executionError(QStringLiteral(
					"Un titre de folio dépasse 1024 caractères."));
			}
			folio_titles.append(folio_title);
		}
	}
	if (folio_titles.isEmpty())
		folio_titles.append(QStringLiteral("Folio 1"));

	QETProject project;
	project.setTitle(title);
	for (const QString &folio_title : folio_titles) {
		Diagram *diagram = project.addNewDiagram();
		if (!diagram)
			return executionError(QStringLiteral("Impossible de créer un folio."));
		TitleBlockProperties properties =
			diagram->border_and_titleblock.exportTitleBlock();
		properties.title = folio_title;
		diagram->border_and_titleblock.importTitleBlock(properties);
	}

	if (!writeProjectToNewPath(&project, output_path, &error))
		return executionError(error);

	QJsonObject result = projectSummary(project, output_path);
	result.insert(QStringLiteral("created"), true);
	return executionSuccess(result);
}

ToolExecutionResult ProjectService::setTitleBlock(
	const QJsonObject &arguments) const
{
	if (!m_write_enabled) {
		return executionError(QStringLiteral(
			"Les écritures MCP sont désactivées. Relancez le serveur avec "
			"--mcp-write après accord explicite de l’utilisateur."));
	}

	QString error;
	if (!rejectUnknownArguments(
		arguments,
		{
			QStringLiteral("inputPath"),
			QStringLiteral("outputPath"),
			QStringLiteral("fields"),
			QStringLiteral("confirm")
		},
		&error)) {
		return executionError(error);
	}
	if (!confirmed(arguments, &error))
		return executionError(error);

	QString input_path;
	const QString input_candidate = requiredString(
		arguments,
		QStringLiteral("inputPath"),
		&error);
	if (!error.isEmpty()
		|| !resolveInputProject(input_candidate, &input_path, &error)) {
		return executionError(error);
	}

	QString output_path;
	const QString output_candidate = requiredString(
		arguments,
		QStringLiteral("outputPath"),
		&error);
	if (!error.isEmpty()
		|| !resolveOutputProject(output_candidate, &output_path, &error)) {
		return executionError(error);
	}
	if (QDir::cleanPath(input_path).compare(
		QDir::cleanPath(output_path),
		Qt::CaseInsensitive) == 0) {
		return executionError(QStringLiteral(
			"La destination doit être différente du projet source."));
	}

	const QJsonValue fields_value = arguments.value(QStringLiteral("fields"));
	if (!fields_value.isObject() || fields_value.toObject().isEmpty()) {
		return executionError(QStringLiteral(
			"Le paramètre « fields » doit être un objet non vide."));
	}
	const QJsonObject fields = fields_value.toObject();
	if (fields.size() > 64)
		return executionError(QStringLiteral("Trop de champs de cartouche."));

	if (!preflightProjectVersion(input_path, &error))
		return executionError(error);

	QETProject project(
		input_path,
		QETProject::OpeningMode::NonInteractive);
	if (project.state() != QETProject::Ok) {
		return executionError(QStringLiteral(
			"QElectroTech n’a pas pu charger le projet source (état %1).")
			.arg(static_cast<int>(project.state())));
	}

	TitleBlockProperties defaults = project.defaultTitleBlockProperties();
	if (!applyTitleBlockFields(&defaults, fields, &error))
		return executionError(error);
	project.setDefaultTitleBlockProperties(defaults);

	int folios = 0;
	for (Diagram *diagram : project.diagrams()) {
		TitleBlockProperties properties =
			diagram->border_and_titleblock.exportTitleBlock();
		if (!applyTitleBlockFields(&properties, fields, &error))
			return executionError(error);
		diagram->border_and_titleblock.importTitleBlock(properties);
		++folios;
	}

	if (!writeProjectToNewPath(&project, output_path, &error))
		return executionError(error);

	QJsonObject result;
	result.insert(QStringLiteral("created"), true);
	result.insert(QStringLiteral("sourcePath"), input_path);
	result.insert(QStringLiteral("outputPath"), output_path);
	result.insert(QStringLiteral("foliosUpdated"), folios);
	result.insert(QStringLiteral("fieldsUpdated"), fields.size());
	return executionSuccess(result);
}

bool ProjectService::resolveInputProject(
	const QString &candidate,
	QString *resolved,
	QString *error) const
{
	const QFileInfo info(candidate);
	if (!info.exists() || !info.isFile()) {
		*error = QStringLiteral("Projet introuvable : %1").arg(candidate);
		return false;
	}
	if (info.suffix().compare(QStringLiteral("qet"), Qt::CaseInsensitive) != 0) {
		*error = QStringLiteral("Le fichier doit avoir l’extension .qet.");
		return false;
	}
	if (info.size() > maxProjectBytes) {
		*error = QStringLiteral(
			"Le projet dépasse la limite MCP de 512 Mio : %1").arg(candidate);
		return false;
	}
	const QString canonical = QDir::cleanPath(info.canonicalFilePath());
	if (!isInsideRoots(canonical)) {
		*error = QStringLiteral(
			"Accès refusé hors des répertoires MCP autorisés : %1").arg(candidate);
		return false;
	}
	*resolved = canonical;
	return true;
}

bool ProjectService::resolveOutputProject(
	const QString &candidate,
	QString *resolved,
	QString *error) const
{
	const QFileInfo candidate_info(candidate);
	if (candidate_info.suffix().compare(
		QStringLiteral("qet"),
		Qt::CaseInsensitive) != 0) {
		*error = QStringLiteral("La destination doit avoir l’extension .qet.");
		return false;
	}
	if (candidate_info.exists()) {
		*error = QStringLiteral(
			"La destination existe déjà. Le MCP ne remplace jamais un fichier : %1")
			.arg(candidate);
		return false;
	}

	const QFileInfo parent_info(candidate_info.absolutePath());
	if (!parent_info.exists() || !parent_info.isDir()) {
		*error = QStringLiteral(
			"Le dossier de destination doit déjà exister : %1")
			.arg(candidate_info.absolutePath());
		return false;
	}
	const QString canonical_parent =
		QDir::cleanPath(parent_info.canonicalFilePath());
	const QString canonical_output = QDir(canonical_parent).filePath(
		candidate_info.fileName());
	if (!isInsideRoots(canonical_output)) {
		*error = QStringLiteral(
			"Écriture refusée hors des répertoires MCP autorisés : %1")
			.arg(candidate);
		return false;
	}
	*resolved = QDir::cleanPath(canonical_output);
	return true;
}

bool ProjectService::isInsideRoots(const QString &canonical_path) const
{
	for (const QString &root : m_roots) {
		const QString relative = QDir(root).relativeFilePath(canonical_path);
		if (!QDir::isAbsolutePath(relative)
			&& (relative == QStringLiteral(".")
			|| (relative != QStringLiteral("..")
				&& !relative.startsWith(QStringLiteral("../"))
				&& !relative.startsWith(QStringLiteral("..\\"))))) {
			return true;
		}
	}
	return false;
}

} // namespace QetMcp
