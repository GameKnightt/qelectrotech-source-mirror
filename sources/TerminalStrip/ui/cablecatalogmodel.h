/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef CABLECATALOGMODEL_H
#define CABLECATALOGMODEL_H

#include "../../cablecatalog/cablecatalogtypes.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

class CableCatalogModel final : public QAbstractItemModel
{
	Q_OBJECT

	public:
		enum Column {
			Status = 0,
			Cable,
			WireColor,
			Conductor,
			Folio,
			From,
			To,
			Section,
			Function,
			TensionProtocol,
			Diagnostics,
			ColumnCount
		};

		enum NodeType {
			CableNode,
			ConductorNode
		};

		enum Role {
			NodeTypeRole = Qt::UserRole + 1,
			SeverityRole,
			AttentionRole,
			DiagnosticCodesRole,
			StableKeyRole,
			SearchTextRole,
			NavigationTargetRole,
			NavigationAvailableRole,
			AssignedRole,
			NumericSortRole
		};

		explicit CableCatalogModel(QObject *parent = nullptr);

		QModelIndex index(int row, int column,
						  const QModelIndex &parent = QModelIndex()) const override;
		QModelIndex parent(const QModelIndex &child) const override;
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

		void setSnapshot(CableCatalogSnapshot snapshot);
		CableCatalogSnapshot snapshot() const;
		CableCatalogEntry entryAt(int row) const;
		QVector<CableNavigationTarget> navigationTargetsForIndex(
				const QModelIndex &index) const;

		static QString normalizedSearch(const QString &text);

	private:
		static quintptr nodeId(int entry, int member);
		static int entryFromId(quintptr id);
		static int memberFromId(quintptr id);
		const CableCatalogEntry *entryForIndex(const QModelIndex &index) const;
		const CableConductorSnapshot *memberForIndex(const QModelIndex &index) const;

		CableCatalogSnapshot m_snapshot;
};

class CableCatalogFilterProxyModel final : public QSortFilterProxyModel
{
	Q_OBJECT

	public:
		enum class HealthScope {
			All,
			Attention,
			WithoutAttention
		};

		enum class DiagnosticScope {
			All,
			MissingCoreOrColor,
			RepeatedCoreOrColor,
			NearReference,
			TopologyOrIdentity,
			Information
		};

		explicit CableCatalogFilterProxyModel(QObject *parent = nullptr);

		void setSearchText(const QString &text);
		QString searchText() const;
		void setHealthScope(HealthScope scope);
		HealthScope healthScope() const;
		void setDiagnosticScope(DiagnosticScope scope);
		DiagnosticScope diagnosticScope() const;
		void setIncludeUnassigned(bool include);
		bool includeUnassigned() const;

	protected:
		bool filterAcceptsRow(int source_row,
						  const QModelIndex &source_parent) const override;
		bool lessThan(const QModelIndex &source_left,
					  const QModelIndex &source_right) const override;

	private:
		bool rowMatchesSearch(const QModelIndex &source_index) const;
		bool cableMatchesDiagnostic(const QModelIndex &source_index) const;

		QString m_search_text;
		QStringList m_search_tokens;
		HealthScope m_health_scope = HealthScope::All;
		DiagnosticScope m_diagnostic_scope = DiagnosticScope::All;
		bool m_include_unassigned = false;
};

#endif // CABLECATALOGMODEL_H
