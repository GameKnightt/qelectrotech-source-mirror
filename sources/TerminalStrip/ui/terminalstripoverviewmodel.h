/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef TERMINALSTRIPOVERVIEWMODEL_H
#define TERMINALSTRIPOVERVIEWMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QUuid>
#include <QVector>

struct TerminalStripOverviewRow
{
	QUuid element_uuid;
	QUuid strip_uuid;
	QString assignment;
	QString installation;
	QString location;
	QString label;
	QString conductor;
	QString xref;
	QString cable;
	QString cable_wire;
	QString type;
	QString function;
	int position = -1;
	int level = -1;
	bool assigned = false;
	bool bridged = false;
	bool led = false;
	bool navigation_available = false;

	bool cableIncomplete() const;
	bool requiresAttention() const;
	QString stableKey() const;
	QString searchableText() const;
};

bool operator==(const TerminalStripOverviewRow &left,
				const TerminalStripOverviewRow &right);

class TerminalStripOverviewModel : public QAbstractTableModel
{
	Q_OBJECT

	public:
		enum Column {
			Status = 0,
			Assignment,
			Installation,
			Location,
			Position,
			Level,
			Label,
			Conductor,
			Xref,
			Cable,
			CableWire,
			Type,
			Function,
			Bridge,
			ColumnCount
		};

		enum Role {
			ElementUuidRole = Qt::UserRole + 1,
			StripUuidRole,
			AssignedRole,
			PositionRole,
			LevelRole,
			NavigationAvailableRole,
			AttentionRole,
			StableKeyRole,
			SearchTextRole
		};

		explicit TerminalStripOverviewModel(QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		int columnCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index,
					  int role = Qt::DisplayRole) const override;
		QVariant headerData(int section,
						Qt::Orientation orientation,
						int role = Qt::DisplayRole) const override;
		Qt::ItemFlags flags(const QModelIndex &index) const override;
		bool setData(const QModelIndex &index,
					 const QVariant &value,
					 int role = Qt::EditRole) override;

		void setRows(QVector<TerminalStripOverviewRow> rows);
		QVector<TerminalStripOverviewRow> rows() const;
		TerminalStripOverviewRow rowAt(int row) const;

		static QString normalized(const QString &text);
		static bool naturalLessThan(const QString &left, const QString &right);
		static bool defaultLessThan(const TerminalStripOverviewRow &left,
								const TerminalStripOverviewRow &right);

	private:
		QVector<TerminalStripOverviewRow> m_rows;
};

class TerminalStripOverviewFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	public:
		enum class AssignmentScope {
			All,
			Assigned,
			Independent
		};

		enum class AttentionScope {
			All,
			RequiresAttention,
			WithoutAttention
		};

		explicit TerminalStripOverviewFilterProxyModel(QObject *parent = nullptr);

		void setSearchText(const QString &text);
		QString searchText() const;
		void setAssignmentScope(AssignmentScope scope);
		AssignmentScope assignmentScope() const;
		void setAttentionScope(AttentionScope scope);
		AttentionScope attentionScope() const;

	protected:
		bool filterAcceptsRow(int source_row,
						  const QModelIndex &source_parent) const override;
		bool lessThan(const QModelIndex &source_left,
					  const QModelIndex &source_right) const override;

	private:
		QStringList m_search_tokens;
		QString m_search_text;
		AssignmentScope m_assignment_scope = AssignmentScope::All;
		AttentionScope m_attention_scope = AttentionScope::All;
};

Q_DECLARE_METATYPE(TerminalStripOverviewRow)

#endif // TERMINALSTRIPOVERVIEWMODEL_H
