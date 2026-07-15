/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "cablecatalogmodel.h"

#include "../../cablecatalog/cablecatalogbuilder.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <functional>

namespace {

QStringList uniqueMemberValues(
		const QVector<CableConductorSnapshot> &members,
		const std::function<QString(const CableConductorSnapshot &)> &getter)
{
	QStringList values;
	for (const CableConductorSnapshot &member : members)
	{
		const QString value = getter(member).trimmed();
		if (!value.isEmpty() && !values.contains(value)) values.append(value);
	}
	std::stable_sort(values.begin(), values.end(), CableCatalogBuilder::naturalLessThan);
	return values;
}

QString statusText(CableDiagnostic::Severity severity, bool has_diagnostics)
{
	if (severity == CableDiagnostic::Error)
		return QCoreApplication::translate("CableCatalogModel", "À corriger");
	if (severity == CableDiagnostic::Warning)
		return QCoreApplication::translate("CableCatalogModel", "À vérifier");
	if (has_diagnostics)
		return QCoreApplication::translate("CableCatalogModel", "Information");
	return QCoreApplication::translate("CableCatalogModel", "Sans alerte");
}

QVector<CableDiagnostic> diagnosticsForMember(
		const CableCatalogEntry &entry,
		const CableConductorSnapshot *member)
{
	if (!member) return entry.diagnostics;
	QVector<CableDiagnostic> diagnostics;
	for (const CableDiagnostic &diagnostic : entry.diagnostics)
	{
		if (diagnostic.member_keys.isEmpty()
				|| diagnostic.member_keys.contains(member->stable_key))
			diagnostics.append(diagnostic);
	}
	return diagnostics;
}

CableDiagnostic::Severity severityOf(const QVector<CableDiagnostic> &diagnostics)
{
	CableDiagnostic::Severity result = CableDiagnostic::Information;
	for (const CableDiagnostic &diagnostic : diagnostics)
		result = std::max(result, diagnostic.severity);
	return result;
}

QString diagnosticText(const QVector<CableDiagnostic> &diagnostics)
{
	QStringList messages;
	for (const CableDiagnostic &diagnostic : diagnostics)
	{
		if (!messages.contains(diagnostic.message)) messages.append(diagnostic.message);
	}
	return messages.join(QStringLiteral(" · "));
}

QStringList diagnosticCodes(const QVector<CableDiagnostic> &diagnostics)
{
	QStringList codes;
	for (const CableDiagnostic &diagnostic : diagnostics)
	{
		const QString code = CableDiagnostic::stableCode(diagnostic.code);
		if (!codes.contains(code)) codes.append(code);
	}
	return codes;
}

}

CableCatalogModel::CableCatalogModel(QObject *parent) :
	QAbstractItemModel(parent)
{}

quintptr CableCatalogModel::nodeId(int entry, int member)
{
	return (quintptr(entry + 1) << 32) | quintptr(member + 1);
}

int CableCatalogModel::entryFromId(quintptr id)
{
	return int(id >> 32) - 1;
}

int CableCatalogModel::memberFromId(quintptr id)
{
	return int(id & quintptr(0xffffffff)) - 1;
}

QModelIndex CableCatalogModel::index(
		int row, int column, const QModelIndex &parent_index) const
{
	if (row < 0 || column < 0 || column >= ColumnCount) return {};
	if (!parent_index.isValid())
	{
		if (row >= m_snapshot.entries.size()) return {};
		return createIndex(row, column, nodeId(row, -1));
	}
	if (parent_index.column() != 0 || memberFromId(parent_index.internalId()) >= 0)
		return {};
	const int entry = entryFromId(parent_index.internalId());
	if (entry < 0 || entry >= m_snapshot.entries.size()
			|| row >= m_snapshot.entries.at(entry).members.size()) return {};
	return createIndex(row, column, nodeId(entry, row));
}

QModelIndex CableCatalogModel::parent(const QModelIndex &child) const
{
	if (!child.isValid()) return {};
	const int member = memberFromId(child.internalId());
	if (member < 0) return {};
	const int entry = entryFromId(child.internalId());
	if (entry < 0 || entry >= m_snapshot.entries.size()) return {};
	return createIndex(entry, 0, nodeId(entry, -1));
}

int CableCatalogModel::rowCount(const QModelIndex &parent_index) const
{
	if (!parent_index.isValid()) return m_snapshot.entries.size();
	if (parent_index.column() != 0 || memberFromId(parent_index.internalId()) >= 0)
		return 0;
	const int entry = entryFromId(parent_index.internalId());
	return entry >= 0 && entry < m_snapshot.entries.size()
			? m_snapshot.entries.at(entry).members.size() : 0;
}

int CableCatalogModel::columnCount(const QModelIndex &) const
{
	return ColumnCount;
}

const CableCatalogEntry *CableCatalogModel::entryForIndex(
		const QModelIndex &index) const
{
	if (!index.isValid()) return nullptr;
	const int entry = entryFromId(index.internalId());
	return entry >= 0 && entry < m_snapshot.entries.size()
			? &m_snapshot.entries.at(entry) : nullptr;
}

const CableConductorSnapshot *CableCatalogModel::memberForIndex(
		const QModelIndex &index) const
{
	const CableCatalogEntry *entry = entryForIndex(index);
	if (!entry) return nullptr;
	const int member = memberFromId(index.internalId());
	return member >= 0 && member < entry->members.size()
			? &entry->members.at(member) : nullptr;
}

QVariant CableCatalogModel::data(const QModelIndex &index, int role) const
{
	const CableCatalogEntry *entry = entryForIndex(index);
	if (!entry) return {};
	const CableConductorSnapshot *member = memberForIndex(index);
	const QVector<CableDiagnostic> diagnostics = diagnosticsForMember(*entry, member);
	const CableDiagnostic::Severity severity = severityOf(diagnostics);
	const bool attention = std::any_of(diagnostics.cbegin(), diagnostics.cend(),
			[](const CableDiagnostic &diagnostic) {
				return diagnostic.severity >= CableDiagnostic::Warning;
			});

	if (role == NodeTypeRole) return member ? ConductorNode : CableNode;
	if (role == SeverityRole) return int(severity);
	if (role == AttentionRole) return attention;
	if (role == DiagnosticCodesRole) return diagnosticCodes(diagnostics);
	if (role == StableKeyRole)
		return member ? member->stable_key : entry->stable_key;
	if (role == NavigationTargetRole)
		return QVariant::fromValue(member
				? member->navigation_target : entry->preferredTarget());
	if (role == NavigationAvailableRole)
		return member ? member->navigation_target.isValid()
						  : entry->preferredTarget().isValid();
	if (role == AssignedRole) return entry->assigned;
	if (role == NumericSortRole)
	{
		if (index.column() == Conductor)
			return member ? 1 : entry->members.size();
		return {};
	}

	QString search_text;
	if (member)
	{
		search_text = QStringList {entry->reference, member->wire_color,
			member->conductor, member->folioText(), member->endpointText(),
			member->section, member->function, member->tension_protocol,
			diagnosticText(diagnostics)}.join(QLatin1Char(' '));
	}
	else
	{
		search_text = QStringList {entry->reference,
			entry->wireColors().join(QLatin1Char(' ')),
			entry->folios().join(QLatin1Char(' ')),
			entry->endpoints().join(QLatin1Char(' ')),
			entry->diagnosticText()}.join(QLatin1Char(' '));
	}
	if (role == SearchTextRole) return normalizedSearch(search_text);
	if (role == Qt::AccessibleTextRole)
	{
		if (member)
			return tr("Conducteur %1, câble %2, âme ou couleur %3, folio %4, %5")
					.arg(member->conductor, entry->reference, member->wire_color,
						 member->folioText(), statusText(severity, !diagnostics.isEmpty()));
		return tr("Câble %1, %2 conducteurs, %3, %4")
				.arg(entry->reference)
				.arg(entry->members.size())
				.arg(entry->wireColors().join(QStringLiteral(", ")),
					 statusText(severity, !diagnostics.isEmpty()));
	}

	if (role == Qt::ToolTipRole)
	{
		if (index.column() == Diagnostics)
			return diagnosticText(diagnostics).isEmpty()
					? tr("Aucune anomalie détectée par les règles actuelles.")
					: diagnosticText(diagnostics);
		if (!member && index.column() == WireColor)
			return entry->wireColors().join(QStringLiteral(" · "));
		if (!member && index.column() == Folio)
			return entry->folios().join(QStringLiteral(" · "));
		if (!member && index.column() == From)
			return uniqueMemberValues(entry->members,
					[](const CableConductorSnapshot &item) {
						return item.from.displayText();
					}).join(QLatin1Char('\n'));
		if (!member && index.column() == To)
			return uniqueMemberValues(entry->members,
					[](const CableConductorSnapshot &item) {
						return item.to.displayText();
					}).join(QLatin1Char('\n'));
	}

	if (role != Qt::DisplayRole && role != Qt::ToolTipRole) return {};
	if (!member)
	{
		switch (index.column())
		{
			case Status: return statusText(severity, !diagnostics.isEmpty());
			case Cable: return entry->reference;
			case WireColor: return entry->wireColors().join(QStringLiteral(" · "));
			case Conductor: return tr("%n conducteur(s)", nullptr, entry->members.size());
			case Folio: return entry->folios().join(QStringLiteral(" · "));
			case From: return uniqueMemberValues(entry->members,
					[](const CableConductorSnapshot &item) {
						return item.from.displayText();
					}).join(QStringLiteral(" · "));
			case To: return uniqueMemberValues(entry->members,
					[](const CableConductorSnapshot &item) {
						return item.to.displayText();
					}).join(QStringLiteral(" · "));
			case Section: return uniqueMemberValues(entry->members,
					[](const CableConductorSnapshot &item) { return item.section; })
					.join(QStringLiteral(" · "));
			case Function: return uniqueMemberValues(entry->members,
					[](const CableConductorSnapshot &item) { return item.function; })
					.join(QStringLiteral(" · "));
			case TensionProtocol: return uniqueMemberValues(entry->members,
					[](const CableConductorSnapshot &item) { return item.tension_protocol; })
					.join(QStringLiteral(" · "));
			case Diagnostics: return diagnosticText(diagnostics);
			default: return {};
		}
	}

	switch (index.column())
	{
		case Status: return statusText(severity, !diagnostics.isEmpty());
		case Cable: return entry->reference;
		case WireColor: return member->wire_color;
		case Conductor: return member->conductor;
		case Folio: return member->folioText();
		case From: return member->from.displayText();
		case To: return member->to.displayText();
		case Section: return member->section;
		case Function: return member->function;
		case TensionProtocol: return member->tension_protocol;
		case Diagnostics: return diagnosticText(diagnostics);
		default: return {};
	}
}

QVariant CableCatalogModel::headerData(
		int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return QAbstractItemModel::headerData(section, orientation, role);
	switch (section)
	{
		case Status: return tr("État");
		case Cable: return tr("Câble");
		case WireColor: return tr("Âme / couleur");
		case Conductor: return tr("Conducteur");
		case Folio: return tr("Folio");
		case From: return tr("De");
		case To: return tr("Vers");
		case Section: return tr("Section");
		case Function: return tr("Fonction");
		case TensionProtocol: return tr("Tension / protocole");
		case Diagnostics: return tr("Diagnostics");
		default: return {};
	}
}

Qt::ItemFlags CableCatalogModel::flags(const QModelIndex &index) const
{
	return index.isValid()
			? Qt::ItemIsEnabled | Qt::ItemIsSelectable
			: Qt::NoItemFlags;
}

bool CableCatalogModel::setData(const QModelIndex &, const QVariant &, int)
{
	return false;
}

void CableCatalogModel::setSnapshot(CableCatalogSnapshot snapshot)
{
	beginResetModel();
	m_snapshot = std::move(snapshot);
	endResetModel();
}

CableCatalogSnapshot CableCatalogModel::snapshot() const
{
	return m_snapshot;
}

CableCatalogEntry CableCatalogModel::entryAt(int row) const
{
	return row >= 0 && row < m_snapshot.entries.size()
			? m_snapshot.entries.at(row) : CableCatalogEntry();
}

QVector<CableNavigationTarget> CableCatalogModel::navigationTargetsForIndex(
		const QModelIndex &index) const
{
	QVector<CableNavigationTarget> targets;
	const CableCatalogEntry *entry = entryForIndex(index);
	if (!entry) return targets;

	if (const CableConductorSnapshot *member = memberForIndex(index))
	{
		if (member->navigation_target.isValid())
			targets.append(member->navigation_target);
		return targets;
	}

	targets.reserve(entry->members.size());
	for (const CableConductorSnapshot &member : entry->members)
	{
		if (member.navigation_target.isValid())
			targets.append(member.navigation_target);
	}
	return targets;
}

QString CableCatalogModel::normalizedSearch(const QString &text)
{
	QString value = text.normalized(QString::NormalizationForm_D).toCaseFolded();
	value.remove(QRegularExpression(QStringLiteral("[\\p{Mn}]")));
	value.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
	return value.trimmed();
}

CableCatalogFilterProxyModel::CableCatalogFilterProxyModel(QObject *parent) :
	QSortFilterProxyModel(parent)
{
	setDynamicSortFilter(true);
	setRecursiveFilteringEnabled(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	setAutoAcceptChildRows(true);
#endif
}

void CableCatalogFilterProxyModel::setSearchText(const QString &text)
{
	if (m_search_text == text) return;
	m_search_text = text;
	m_search_tokens = CableCatalogModel::normalizedSearch(text)
			.split(QLatin1Char(' '), Qt::SkipEmptyParts);
	invalidateFilter();
}

QString CableCatalogFilterProxyModel::searchText() const { return m_search_text; }

void CableCatalogFilterProxyModel::setHealthScope(HealthScope scope)
{
	if (m_health_scope == scope) return;
	m_health_scope = scope;
	invalidateFilter();
}

CableCatalogFilterProxyModel::HealthScope
CableCatalogFilterProxyModel::healthScope() const { return m_health_scope; }

void CableCatalogFilterProxyModel::setDiagnosticScope(DiagnosticScope scope)
{
	if (m_diagnostic_scope == scope) return;
	m_diagnostic_scope = scope;
	invalidateFilter();
}

CableCatalogFilterProxyModel::DiagnosticScope
CableCatalogFilterProxyModel::diagnosticScope() const { return m_diagnostic_scope; }

void CableCatalogFilterProxyModel::setIncludeUnassigned(bool include)
{
	if (m_include_unassigned == include) return;
	m_include_unassigned = include;
	invalidateFilter();
}

bool CableCatalogFilterProxyModel::includeUnassigned() const
{
	return m_include_unassigned;
}

bool CableCatalogFilterProxyModel::rowMatchesSearch(
		const QModelIndex &source_index) const
{
	const QString searchable = source_index.data(
			CableCatalogModel::SearchTextRole).toString();
	for (const QString &token : m_search_tokens)
	{
		if (!searchable.contains(token)) return false;
	}
	return true;
}

bool CableCatalogFilterProxyModel::cableMatchesDiagnostic(
		const QModelIndex &source_index) const
{
	if (m_diagnostic_scope == DiagnosticScope::All) return true;
	const QStringList codes = source_index.data(
			CableCatalogModel::DiagnosticCodesRole).toStringList();
	auto has = [&codes](CableDiagnostic::Code code) {
		return codes.contains(CableDiagnostic::stableCode(code));
	};
	switch (m_diagnostic_scope)
	{
		case DiagnosticScope::MissingCoreOrColor:
			return has(CableDiagnostic::AssignedCableWithoutCoreOrColor);
		case DiagnosticScope::RepeatedCoreOrColor:
			return has(CableDiagnostic::RepeatedCoreOrColor);
		case DiagnosticScope::NearReference:
			return has(CableDiagnostic::NearReference);
		case DiagnosticScope::TopologyOrIdentity:
			return has(CableDiagnostic::MissingEndpoint)
					|| has(CableDiagnostic::UnstableLegacyIdentity)
					|| has(CableDiagnostic::MultipleEndpointFamilies);
		case DiagnosticScope::Information:
			return has(CableDiagnostic::MixedSections)
					|| has(CableDiagnostic::MixedProtocols);
		case DiagnosticScope::All:
			return true;
	}
	return true;
}

bool CableCatalogFilterProxyModel::filterAcceptsRow(
		int source_row, const QModelIndex &source_parent) const
{
	const QModelIndex source_index = sourceModel()->index(
			source_row, CableCatalogModel::Cable, source_parent);
	if (!source_index.isValid()) return false;
	if (!m_include_unassigned
			&& !source_index.data(CableCatalogModel::AssignedRole).toBool())
		return false;
	const bool attention = source_index.data(
			CableCatalogModel::AttentionRole).toBool();
	if (m_health_scope == HealthScope::Attention && !attention) return false;
	if (m_health_scope == HealthScope::WithoutAttention && attention) return false;
	if (!cableMatchesDiagnostic(source_index)) return false;
	return rowMatchesSearch(source_index);
}

bool CableCatalogFilterProxyModel::lessThan(
		const QModelIndex &source_left,
		const QModelIndex &source_right) const
{
	if (source_left.column() == CableCatalogModel::Conductor)
	{
		const QVariant left_number = source_left.data(CableCatalogModel::NumericSortRole);
		const QVariant right_number = source_right.data(CableCatalogModel::NumericSortRole);
		if (left_number.isValid() && right_number.isValid()
				&& left_number.toInt() != right_number.toInt())
			return left_number.toInt() < right_number.toInt();
	}
	const QString left = source_left.data(Qt::DisplayRole).toString();
	const QString right = source_right.data(Qt::DisplayRole).toString();
	if (left != right) return CableCatalogBuilder::naturalLessThan(left, right);
	return source_left.data(CableCatalogModel::StableKeyRole).toString()
			< source_right.data(CableCatalogModel::StableKeyRole).toString();
}
