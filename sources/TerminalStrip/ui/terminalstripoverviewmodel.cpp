/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "terminalstripoverviewmodel.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include <algorithm>

namespace {

QString boolText(bool value)
{
	return value
			? QCoreApplication::translate("TerminalStripOverviewModel", "Oui")
			: QCoreApplication::translate("TerminalStripOverviewModel", "Non");
}

int compareNatural(const QString &left, const QString &right)
{
	const QString a = TerminalStripOverviewModel::normalized(left);
	const QString b = TerminalStripOverviewModel::normalized(right);
	int a_index = 0;
	int b_index = 0;

	while (a_index < a.size() && b_index < b.size())
	{
		if (a.at(a_index).isDigit() && b.at(b_index).isDigit())
		{
			int a_end = a_index;
			int b_end = b_index;
			while (a_end < a.size() && a.at(a_end).isDigit()) ++a_end;
			while (b_end < b.size() && b.at(b_end).isDigit()) ++b_end;

			const QString a_number = a.mid(a_index, a_end - a_index);
			const QString b_number = b.mid(b_index, b_end - b_index);
			int a_first_significant = 0;
			int b_first_significant = 0;
			while (a_first_significant < a_number.size() - 1
					&& a_number.at(a_first_significant) == QLatin1Char('0'))
				++a_first_significant;
			while (b_first_significant < b_number.size() - 1
					&& b_number.at(b_first_significant) == QLatin1Char('0'))
				++b_first_significant;
			const QString a_trimmed = a_number.mid(a_first_significant);
			const QString b_trimmed = b_number.mid(b_first_significant);
			const QString a_significant = a_trimmed.isEmpty() ? QStringLiteral("0") : a_trimmed;
			const QString b_significant = b_trimmed.isEmpty() ? QStringLiteral("0") : b_trimmed;
			if (a_significant.size() != b_significant.size())
				return a_significant.size() < b_significant.size() ? -1 : 1;
			const int number_compare = QString::compare(
					a_significant, b_significant, Qt::CaseSensitive);
			if (number_compare != 0)
				return number_compare < 0 ? -1 : 1;
			if (a_number.size() != b_number.size())
				return a_number.size() < b_number.size() ? -1 : 1;
			a_index = a_end;
			b_index = b_end;
			continue;
		}

		if (a.at(a_index) != b.at(b_index))
			return a.at(a_index) < b.at(b_index) ? -1 : 1;
		++a_index;
		++b_index;
	}

	if (a.size() == b.size()) return 0;
	return a.size() < b.size() ? -1 : 1;
}

}

bool TerminalStripOverviewRow::cableIncomplete() const
{
	// A coloured wire without a cable reference is a valid standalone wire.
	// Only a referenced cable without an associated core/colour is incomplete.
	return !cable.trimmed().isEmpty() && cable_wire.trimmed().isEmpty();
}

bool TerminalStripOverviewRow::requiresAttention() const
{
	return !navigation_available || cableIncomplete();
}

QString TerminalStripOverviewRow::stableKey() const
{
	return QStringLiteral("%1|%2|%3|%4")
			.arg(strip_uuid.toString(QUuid::WithoutBraces),
				 element_uuid.toString(QUuid::WithoutBraces))
			.arg(position)
			.arg(level);
}

QString TerminalStripOverviewRow::searchableText() const
{
	return QStringList {
		assignment,
		installation,
		location,
		label,
		conductor,
		xref,
		cable,
		cable_wire,
		type,
		function,
		bridged ? QStringLiteral("pont") : QString(),
		led ? QStringLiteral("led") : QString()
	}.join(QLatin1Char(' '));
}

bool operator==(const TerminalStripOverviewRow &left,
				const TerminalStripOverviewRow &right)
{
	return left.element_uuid == right.element_uuid
			&& left.strip_uuid == right.strip_uuid
			&& left.assignment == right.assignment
			&& left.installation == right.installation
			&& left.location == right.location
			&& left.label == right.label
			&& left.conductor == right.conductor
			&& left.xref == right.xref
			&& left.cable == right.cable
			&& left.cable_wire == right.cable_wire
			&& left.type == right.type
			&& left.function == right.function
			&& left.position == right.position
			&& left.level == right.level
			&& left.assigned == right.assigned
			&& left.bridged == right.bridged
			&& left.led == right.led
			&& left.navigation_available == right.navigation_available;
}

TerminalStripOverviewModel::TerminalStripOverviewModel(QObject *parent) :
	QAbstractTableModel(parent)
{}

int TerminalStripOverviewModel::rowCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : m_rows.size();
}

int TerminalStripOverviewModel::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : ColumnCount;
}

QVariant TerminalStripOverviewModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
		return {};

	const TerminalStripOverviewRow &row = m_rows.at(index.row());
	if (role == ElementUuidRole) return row.element_uuid;
	if (role == StripUuidRole) return row.strip_uuid;
	if (role == AssignedRole) return row.assigned;
	if (role == PositionRole) return row.position;
	if (role == LevelRole) return row.level;
	if (role == NavigationAvailableRole) return row.navigation_available;
	if (role == AttentionRole) return row.requiresAttention();
	if (role == StableKeyRole) return row.stableKey();
	if (role == SearchTextRole) return normalized(row.searchableText());

	if (role == Qt::AccessibleTextRole)
	{
		return tr("%1, %2, repère %3, conducteur %4, câble %5, âme %6")
				.arg(row.requiresAttention() ? tr("À vérifier") : tr("Sans alerte"),
					 row.assignment,
					 row.label,
					 row.conductor,
					 row.cable,
					 row.cable_wire);
	}

	if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
		return {};

	if (role == Qt::ToolTipRole && index.column() == Status)
	{
		if (!row.navigation_available)
			return tr("La source de cette borne n’est plus disponible.");
		if (row.cableIncomplete())
			return tr("Un câble renseigné doit aussi indiquer une âme ou une couleur.");
		if (!row.assigned)
			return tr("Cette borne n’est affectée à aucun bornier.");
		return tr("Aucun point à vérifier détecté.");
	}

	switch (index.column())
	{
		case Status:
			if (!row.navigation_available) return tr("À vérifier");
			if (row.cableIncomplete()) return tr("Câble incomplet");
			if (!row.assigned) return tr("Indépendante");
			return tr("Sans alerte");
		case Assignment: return row.assignment;
		case Installation: return row.installation;
		case Location: return row.location;
		case Position: return row.position < 0 ? QVariant() : QVariant(row.position + 1);
		case Level: return row.level < 0 ? QVariant() : QVariant(row.level);
		case Label: return row.label;
		case Conductor: return row.conductor;
		case Xref: return row.xref;
		case Cable: return row.cable;
		case CableWire: return row.cable_wire;
		case Type: return row.type;
		case Function: return row.function;
		case Bridge: return boolText(row.bridged);
		default: return {};
	}
}

QVariant TerminalStripOverviewModel::headerData(
		int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return QAbstractTableModel::headerData(section, orientation, role);

	switch (section)
	{
		case Status: return tr("État");
		case Assignment: return tr("Bornier");
		case Installation: return tr("Installation");
		case Location: return tr("Localisation");
		case Position: return tr("Position");
		case Level: return tr("Étage");
		case Label: return tr("Repère");
		case Conductor: return tr("Conducteur");
		case Xref: return tr("Référence croisée");
		case Cable: return tr("Câble");
		case CableWire: return tr("Âme / couleur");
		case Type: return tr("Type");
		case Function: return tr("Fonction");
		case Bridge: return tr("Pont");
		default: return {};
	}
}

Qt::ItemFlags TerminalStripOverviewModel::flags(const QModelIndex &index) const
{
	if (!index.isValid()) return Qt::NoItemFlags;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool TerminalStripOverviewModel::setData(
		const QModelIndex &, const QVariant &, int)
{
	return false;
}

void TerminalStripOverviewModel::setRows(QVector<TerminalStripOverviewRow> rows)
{
	std::stable_sort(rows.begin(), rows.end(), defaultLessThan);
	beginResetModel();
	m_rows = std::move(rows);
	endResetModel();
}

QVector<TerminalStripOverviewRow> TerminalStripOverviewModel::rows() const
{
	return m_rows;
}

TerminalStripOverviewRow TerminalStripOverviewModel::rowAt(int row) const
{
	if (row < 0 || row >= m_rows.size()) return {};
	return m_rows.at(row);
}

QString TerminalStripOverviewModel::normalized(const QString &text)
{
	QString value = text.normalized(QString::NormalizationForm_D).toCaseFolded();
	value.remove(QRegularExpression(QStringLiteral("[\\p{Mn}]")));
	value.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
	return value.trimmed();
}

bool TerminalStripOverviewModel::naturalLessThan(
		const QString &left, const QString &right)
{
	return compareNatural(left, right) < 0;
}

bool TerminalStripOverviewModel::defaultLessThan(
		const TerminalStripOverviewRow &left,
		const TerminalStripOverviewRow &right)
{
	if (left.assigned != right.assigned) return left.assigned;
	for (const auto &pair : {
			qMakePair(left.installation, right.installation),
			qMakePair(left.location, right.location),
			qMakePair(left.assignment, right.assignment)})
	{
		const int compare = compareNatural(pair.first, pair.second);
		if (compare != 0) return compare < 0;
	}
	if (left.position != right.position) return left.position < right.position;
	if (left.level != right.level) return left.level < right.level;
	const int label_compare = compareNatural(left.label, right.label);
	if (label_compare != 0) return label_compare < 0;
	return left.stableKey() < right.stableKey();
}

TerminalStripOverviewFilterProxyModel::TerminalStripOverviewFilterProxyModel(
		QObject *parent) :
	QSortFilterProxyModel(parent)
{
	setDynamicSortFilter(true);
	setSortCaseSensitivity(Qt::CaseInsensitive);
}

void TerminalStripOverviewFilterProxyModel::setSearchText(const QString &text)
{
	if (m_search_text == text) return;
	m_search_text = text;
	m_search_tokens = TerminalStripOverviewModel::normalized(text)
			.split(QLatin1Char(' '), Qt::SkipEmptyParts);
	invalidateFilter();
}

QString TerminalStripOverviewFilterProxyModel::searchText() const
{
	return m_search_text;
}

void TerminalStripOverviewFilterProxyModel::setAssignmentScope(
		AssignmentScope scope)
{
	if (m_assignment_scope == scope) return;
	m_assignment_scope = scope;
	invalidateFilter();
}

TerminalStripOverviewFilterProxyModel::AssignmentScope
TerminalStripOverviewFilterProxyModel::assignmentScope() const
{
	return m_assignment_scope;
}

void TerminalStripOverviewFilterProxyModel::setAttentionScope(
		AttentionScope scope)
{
	if (m_attention_scope == scope) return;
	m_attention_scope = scope;
	invalidateFilter();
}

TerminalStripOverviewFilterProxyModel::AttentionScope
TerminalStripOverviewFilterProxyModel::attentionScope() const
{
	return m_attention_scope;
}

bool TerminalStripOverviewFilterProxyModel::filterAcceptsRow(
		int source_row, const QModelIndex &source_parent) const
{
	const QModelIndex index = sourceModel()->index(
			source_row, TerminalStripOverviewModel::Label, source_parent);
	const bool assigned = index.data(
			TerminalStripOverviewModel::AssignedRole).toBool();
	const bool attention = index.data(
			TerminalStripOverviewModel::AttentionRole).toBool();

	if (m_assignment_scope == AssignmentScope::Assigned && !assigned) return false;
	if (m_assignment_scope == AssignmentScope::Independent && assigned) return false;
	if (m_attention_scope == AttentionScope::RequiresAttention && !attention) return false;
	if (m_attention_scope == AttentionScope::WithoutAttention && attention) return false;

	const QString searchable = index.data(
			TerminalStripOverviewModel::SearchTextRole).toString();
	for (const QString &token : m_search_tokens)
	{
		if (!searchable.contains(token)) return false;
	}
	return true;
}

bool TerminalStripOverviewFilterProxyModel::lessThan(
		const QModelIndex &source_left,
		const QModelIndex &source_right) const
{
	if (!source_left.isValid() || !source_right.isValid())
		return QSortFilterProxyModel::lessThan(source_left, source_right);

	const int column = source_left.column();
	if (column == TerminalStripOverviewModel::Position
			|| column == TerminalStripOverviewModel::Level)
	{
		const int role = column == TerminalStripOverviewModel::Position
				? TerminalStripOverviewModel::PositionRole
				: TerminalStripOverviewModel::LevelRole;
		const int left_number = source_left.data(role).toInt();
		const int right_number = source_right.data(role).toInt();
		if (left_number != right_number) return left_number < right_number;
	}
	else
	{
		const int compare = compareNatural(
				source_left.data(Qt::DisplayRole).toString(),
				source_right.data(Qt::DisplayRole).toString());
		if (compare != 0) return compare < 0;
	}

	return source_left.data(TerminalStripOverviewModel::StableKeyRole).toString()
			< source_right.data(TerminalStripOverviewModel::StableKeyRole).toString();
}
