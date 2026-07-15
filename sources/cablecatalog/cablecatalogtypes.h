/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef CABLECATALOGTYPES_H
#define CABLECATALOGTYPES_H

#include <QString>
#include <QStringList>
#include <QMetaType>
#include <QUuid>
#include <QVector>

struct CableNavigationTarget
{
	quint64 token = 0;
	QUuid project_uuid;
	QUuid diagram_uuid;
	QUuid element_1_uuid;
	QUuid element_2_uuid;
	QUuid terminal_1_uuid;
	QUuid terminal_2_uuid;

	bool isValid() const { return token != 0 || !diagram_uuid.isNull(); }
};

struct CableEndpointSnapshot
{
	QUuid diagram_uuid;
	QUuid element_uuid;
	QUuid terminal_uuid;
	QString folio;
	QString element_label;
	QString terminal_name;
	QString family_key;
	bool stable = true;

	QString displayText() const;
};

struct CableConductorSnapshot
{
	QString stable_key;
	QString potential_key;
	QString cable;
	QString wire_color;
	QString conductor;
	QString section;
	QString function;
	QString tension_protocol;
	CableEndpointSnapshot from;
	CableEndpointSnapshot to;
	CableNavigationTarget navigation_target;

	bool assignedToCable() const { return !cable.trimmed().isEmpty(); }
	QString folioText() const;
	QString endpointText() const;
};

struct CableDiagnostic
{
	enum Code {
		AssignedCableWithoutCoreOrColor,
		RepeatedCoreOrColor,
		NearReference,
		MultipleEndpointFamilies,
		MissingEndpoint,
		UnstableLegacyIdentity,
		MixedSections,
		MixedProtocols
	};

	enum Severity {
		Information = 0,
		Warning = 1,
		Error = 2
	};

	Code code = AssignedCableWithoutCoreOrColor;
	Severity severity = Information;
	QString message;
	QStringList member_keys;

	static QString stableCode(Code code);
};

struct CableCatalogEntry
{
	QString stable_key;
	QString reference;
	bool assigned = true;
	QVector<CableConductorSnapshot> members;
	QVector<CableDiagnostic> diagnostics;

	CableDiagnostic::Severity severity() const;
	bool requiresAttention() const;
	bool hasDiagnostic(CableDiagnostic::Code code) const;
	QString diagnosticText() const;
	QStringList wireColors() const;
	QStringList folios() const;
	QStringList endpoints() const;
	CableNavigationTarget preferredTarget() const;
};

struct CableCatalogSnapshot
{
	QVector<CableCatalogEntry> entries;
	int conductor_count = 0;
	int assigned_conductor_count = 0;
	int unassigned_conductor_count = 0;
};

Q_DECLARE_METATYPE(CableNavigationTarget)
Q_DECLARE_METATYPE(CableDiagnostic::Code)

#endif // CABLECATALOGTYPES_H
