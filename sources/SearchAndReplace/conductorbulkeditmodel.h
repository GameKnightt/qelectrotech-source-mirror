/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORBULKEDITMODEL_H
#define CONDUCTORBULKEDITMODEL_H

#include "../conductorproperties.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

class ConductorBulkEditModel final : public QAbstractTableModel
{
	Q_OBJECT

	public:
		enum Column {
			FolioColumn,
			PotentialColumn,
			SegmentCountColumn,
			FunctionColumn,
			TensionProtocolColumn,
			WireColorColumn,
			WireSectionColumn,
			ColumnCount
		};

		struct Cell {
			QString initialValue;
			QString value;
			bool mixed = false;
			bool edited = false;

			bool changes() const
			{
				return edited && (mixed || value != initialValue);
			}
		};

		struct ColumnDescriptor {
			Column column;
			QString key;
			bool editable = false;
			bool defaultVisible = true;
			bool mandatory = false;
		};

		struct Row {
			int groupIndex = -1;
			QVector<quintptr> targetKeys;
			QString folios;
			QString potential;
			Cell function;
			Cell tensionProtocol;
			Cell wireColor;
			Cell wireSection;
		};

		explicit ConductorBulkEditModel(
			QVector<Row> rows,
			QObject *parent = nullptr);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		int columnCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role) const override;
		QVariant headerData(
			int section,
			Qt::Orientation orientation,
			int role) const override;
		Qt::ItemFlags flags(const QModelIndex &index) const override;
		bool setData(
			const QModelIndex &index,
			const QVariant &value,
			int role = Qt::EditRole) override;

		bool pasteTsv(
			const QModelIndex &start,
			const QString &text,
			QString *errorMessage = nullptr);
		bool pasteTsv(
			int startRow,
			const QVector<int> &logicalColumns,
			const QString &text,
			QString *errorMessage = nullptr);
		bool canFillDown(
			int topRow,
			int bottomRow,
			int leftColumn,
			int rightColumn,
			QString *errorMessage = nullptr) const;
		bool canFillDown(
			int topRow,
			int bottomRow,
			const QVector<int> &logicalColumns,
			QString *errorMessage = nullptr) const;
		bool fillDown(
			int topRow,
			int bottomRow,
			int leftColumn,
			int rightColumn,
			QString *errorMessage = nullptr);
		bool fillDown(
			int topRow,
			int bottomRow,
			const QVector<int> &logicalColumns,
			QString *errorMessage = nullptr);
		void resetDraft();

		bool hasChanges() const;
		bool isValid() const;
		int invalidCellCount() const;
		int changedCellCount(const QVector<int> &logicalColumns) const;
		int changedPotentialCount() const;
		int changedSegmentCount() const;
		QString firstValidationError() const;
		quintptr targetKeyForRow(int row) const;
		bool cellState(int row, int column, Cell *state) const;
		ConductorProperties propertiesForTarget(
			quintptr targetKey,
			const ConductorProperties &before) const;

		static bool isEditableColumn(int column);
		static QVector<ColumnDescriptor> columnDescriptors();
		static QString columnKey(int column);
		static int columnForKey(const QString &key);
		static QVector<int> defaultColumnOrder();
		static QString validationError(const QString &value);

	private:
		Cell *cell(int row, int column);
		const Cell *cell(int row, int column) const;
		QString fieldLabel(int column) const;
		void rebuildTargetIndex();

		QVector<Row> m_rows;
		QHash<quintptr, int> m_target_rows;
};

#endif // CONDUCTORBULKEDITMODEL_H
