/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/SearchAndReplace/conductorbulkeditmodel.h"
#include "../../sources/SearchAndReplace/ui/conductorbulkeditdialog.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFont>
#include <QHash>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollBar>
#include <QTableView>
#include <QtTest>

namespace {

ConductorBulkEditModel::Cell commonCell(const QString &value)
{
	return {value, value, false, false};
}

ConductorBulkEditModel::Cell mixedCell()
{
	return {QString(), QString(), true, false};
}

ConductorBulkEditModel::Row row(
	int group,
	QVector<quintptr> targets,
	const QString &function = QStringLiteral("Commande"),
	const QString &section = QStringLiteral("1,5"))
{
	ConductorBulkEditModel::Row draft_row;
	draft_row.groupIndex = group;
	draft_row.targetKeys = std::move(targets);
	draft_row.folios = QString::number(group + 1);
	draft_row.potential = QStringLiteral("W%1 · X1 ↔ X2").arg(group + 1);
	draft_row.function = commonCell(function);
	draft_row.tensionProtocol = commonCell(QStringLiteral("24 VDC"));
	draft_row.wireColor = commonCell(QStringLiteral("BU"));
	draft_row.wireSection = commonCell(section);
	return draft_row;
}

QVector<ConductorBulkEditModel::Row> rows(int count)
{
	QVector<ConductorBulkEditModel::Row> result;
	result.reserve(count);
	for (int index = 0; index < count; ++index) {
		result.append(row(
			index,
			{static_cast<quintptr>(index * 2 + 1),
			 static_cast<quintptr>(index * 2 + 2)}));
	}
	return result;
}

class ApplicationFontGuard final
{
	public:
		ApplicationFontGuard() : m_original(QApplication::font()) {}
		~ApplicationFontGuard() { QApplication::setFont(m_original); }

	private:
		QFont m_original;
};

QPushButton *cancelButton(ConductorBulkEditDialog &dialog)
{
	auto buttons = dialog.findChild<QDialogButtonBox *>(
		QStringLiteral("conductorBulkEditButtons"));
	return buttons ? buttons->button(QDialogButtonBox::Cancel) : nullptr;
}

QLabel *statusLabel(ConductorBulkEditDialog &dialog)
{
	return dialog.findChild<QLabel *>(QStringLiteral("conductorBulkEditStatus"));
}

void selectRange(
	ConductorBulkEditDialog &dialog,
	int top,
	int bottom,
	int left,
	int right)
{
	QItemSelection selection(
		dialog.draftModel()->index(top, left),
		dialog.draftModel()->index(bottom, right));
	dialog.draftTable()->selectionModel()->setCurrentIndex(
		dialog.draftModel()->index(bottom, right),
		QItemSelectionModel::NoUpdate);
	dialog.draftTable()->selectionModel()->select(
		selection,
		QItemSelectionModel::ClearAndSelect);
}

}

class ConductorBulkEditTest : public QObject
{
	Q_OBJECT

	private slots:
		void mixedValuesRemainUntouchedUntilEdited();
		void tsvPasteMapsFourEditableFields();
		void outOfBoundsPasteIsAtomic();
		void fillDownCopiesEachColumnAtomically();
		void fillDownPropagatesExplicitEmpty();
		void fillDownRejectsUntouchedMixedSourceAtomically();
		void fillDownAllowsEditedMixedSource();
		void fillDownRejectsInvalidRanges();
		void fillDownActionFollowsRectangularSelection();
		void fillDownContextAndAccessibilityContract();
		void activeEditorCommitsBeforeFillDown();
		void fillDownDraftRemainsDetachedUntilAccepted();
		void untouchedLegacyValuesRemainCompatible();
		void invalidCellsDisableVerification();
		void emptyEditedCellErasesProperty();
		void resetRestoresDraft();
		void escapeRejectsWithoutApplying();
		void keyboardContract();
		void fitsScreenAt150Percent();
		void handlesThousandPotentialsDeterministically();
		void fillDownHandlesThousandPotentialsDeterministically();
};

void ConductorBulkEditTest::mixedValuesRemainUntouchedUntilEdited()
{
	ConductorBulkEditModel::Row draft_row = row(0, {101, 102});
	draft_row.function = mixedCell();
	ConductorBulkEditModel model({draft_row});

	ConductorProperties first;
	first.m_function = QStringLiteral("Commande");
	ConductorProperties second;
	second.m_function = QStringLiteral("Puissance");
	QCOMPARE(model.propertiesForTarget(101, first).m_function, first.m_function);
	QCOMPARE(model.propertiesForTarget(102, second).m_function, second.m_function);
	QCOMPARE(
		model.data(model.index(0, ConductorBulkEditModel::FunctionColumn), Qt::DisplayRole).toString(),
		QStringLiteral("Valeurs multiples"));
	QVERIFY(!model.hasChanges());

	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Sécurité")));
	QCOMPARE(model.propertiesForTarget(101, first).m_function, QStringLiteral("Sécurité"));
	QCOMPARE(model.propertiesForTarget(102, second).m_function, QStringLiteral("Sécurité"));
	QCOMPARE(model.changedPotentialCount(), 1);
	QCOMPARE(model.changedSegmentCount(), 2);
}

void ConductorBulkEditTest::tsvPasteMapsFourEditableFields()
{
	ConductorBulkEditModel model(rows(2));
	QString error;
	QVERIFY(model.pasteTsv(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Moteur\t400 VAC\tBK\t2,5\nCapteur\t24 VDC\tBU\t0,75\n"),
		&error));
	QVERIFY2(error.isEmpty(), qPrintable(error));
	QVERIFY(model.isValid());
	QCOMPARE(model.changedPotentialCount(), 2);
	QCOMPARE(model.changedSegmentCount(), 4);

	ConductorProperties before;
	ConductorProperties first = model.propertiesForTarget(1, before);
	QCOMPARE(first.m_function, QStringLiteral("Moteur"));
	QCOMPARE(first.m_tension_protocol, QStringLiteral("400 VAC"));
	QCOMPARE(first.m_wire_color, QStringLiteral("BK"));
	QCOMPARE(first.m_wire_section, QStringLiteral("2,5"));
	ConductorProperties second = model.propertiesForTarget(3, before);
	QCOMPARE(second.m_function, QStringLiteral("Capteur"));
	QCOMPARE(second.m_wire_section, QStringLiteral("0,75"));
}

void ConductorBulkEditTest::outOfBoundsPasteIsAtomic()
{
	ConductorBulkEditModel model(rows(1));
	QString error;
	QVERIFY(!model.pasteTsv(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("A\tB\nC\tD"),
		&error));
	QVERIFY(!error.isEmpty());
	QVERIFY(!model.hasChanges());
	QCOMPARE(
		model.data(model.index(0, ConductorBulkEditModel::FunctionColumn), Qt::EditRole).toString(),
		QStringLiteral("Commande"));
}

void ConductorBulkEditTest::fillDownCopiesEachColumnAtomically()
{
	QVector<ConductorBulkEditModel::Row> draft_rows = rows(3);
	draft_rows[0].function = commonCell(QStringLiteral("Moteur"));
	draft_rows[0].tensionProtocol = commonCell(QStringLiteral("400 VAC"));
	draft_rows[0].wireColor = commonCell(QStringLiteral("BK"));
	draft_rows[1].function = commonCell(QStringLiteral("Capteur"));
	draft_rows[1].tensionProtocol = commonCell(QStringLiteral("24 VDC"));
	draft_rows[1].wireColor = commonCell(QStringLiteral("BU"));
	draft_rows[2].function = commonCell(QStringLiteral("Sécurité"));
	draft_rows[2].tensionProtocol = commonCell(QStringLiteral("48 VDC"));
	draft_rows[2].wireColor = commonCell(QStringLiteral("RD"));
	ConductorBulkEditModel model(draft_rows);
	QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
	QString error;

	QVERIFY(model.fillDown(
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::WireColorColumn,
		&error));
	QVERIFY2(error.isEmpty(), qPrintable(error));
	QCOMPARE(changed.count(), 1);
	QCOMPARE(
		changed.first().at(0).value<QModelIndex>(),
		model.index(1, ConductorBulkEditModel::FunctionColumn));
	QCOMPARE(
		changed.first().at(1).value<QModelIndex>(),
		model.index(2, ConductorBulkEditModel::WireColorColumn));

	for (quintptr target : {static_cast<quintptr>(3), static_cast<quintptr>(5)}) {
		const ConductorProperties after = model.propertiesForTarget(
			target, ConductorProperties());
		QCOMPARE(after.m_function, QStringLiteral("Moteur"));
		QCOMPARE(after.m_tension_protocol, QStringLiteral("400 VAC"));
		QCOMPARE(after.m_wire_color, QStringLiteral("BK"));
	}
	QCOMPARE(
		model.data(model.index(2, ConductorBulkEditModel::WireSectionColumn), Qt::EditRole).toString(),
		QStringLiteral("1,5"));
	QCOMPARE(
		model.data(model.index(0, ConductorBulkEditModel::FunctionColumn), Qt::EditRole).toString(),
		QStringLiteral("Moteur"));
}

void ConductorBulkEditTest::fillDownPropagatesExplicitEmpty()
{
	ConductorBulkEditModel model(rows(3));
	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::FunctionColumn), QString()));
	QVERIFY(model.fillDown(
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn));

	for (quintptr target : {static_cast<quintptr>(3), static_cast<quintptr>(5)}) {
		ConductorProperties before;
		before.m_function = QStringLiteral("Avant");
		QCOMPARE(model.propertiesForTarget(target, before).m_function, QString());
	}
	QCOMPARE(model.changedPotentialCount(), 3);
}

void ConductorBulkEditTest::fillDownRejectsUntouchedMixedSourceAtomically()
{
	QVector<ConductorBulkEditModel::Row> draft_rows = rows(3);
	draft_rows[0].function = mixedCell();
	ConductorBulkEditModel model(draft_rows);
	QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
	QString error;

	QVERIFY(!model.fillDown(
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn,
		&error));
	QVERIFY(error.contains(QStringLiteral("Valeurs multiples")));
	QCOMPARE(changed.count(), 0);
	QVERIFY(!model.hasChanges());
	QCOMPARE(
		model.data(model.index(1, ConductorBulkEditModel::FunctionColumn), Qt::EditRole).toString(),
		QStringLiteral("Commande"));
}

void ConductorBulkEditTest::fillDownAllowsEditedMixedSource()
{
	QVector<ConductorBulkEditModel::Row> draft_rows = rows(3);
	draft_rows[0].function = mixedCell();
	ConductorBulkEditModel model(draft_rows);
	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Sécurité")));
	QVERIFY(model.fillDown(
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn));
	QCOMPARE(
		model.data(model.index(2, ConductorBulkEditModel::FunctionColumn), Qt::EditRole).toString(),
		QStringLiteral("Sécurité"));
}

void ConductorBulkEditTest::fillDownRejectsInvalidRanges()
{
	ConductorBulkEditModel model(rows(3));
	QString error;
	QVERIFY(!model.fillDown(
		0,
		0,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn,
		&error));
	QVERIFY(!error.isEmpty());
	QVERIFY(!model.fillDown(
		0,
		2,
		ConductorBulkEditModel::SegmentCountColumn,
		ConductorBulkEditModel::FunctionColumn,
		&error));
	QVERIFY(!error.isEmpty());
	QVERIFY(!model.fillDown(
		0,
		3,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn,
		&error));
	QVERIFY(!error.isEmpty());
	QVERIFY(!model.hasChanges());
}

void ConductorBulkEditTest::fillDownActionFollowsRectangularSelection()
{
	ConductorBulkEditDialog dialog(rows(3));
	QVERIFY(!dialog.fillDownAction()->isEnabled());
	QVERIFY(!dialog.fillDownButton()->isEnabled());

	selectRange(
		dialog,
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::WireColorColumn);
	QVERIFY(dialog.fillDownAction()->isEnabled());
	QVERIFY(dialog.fillDownButton()->isEnabled());

	dialog.draftTable()->selectionModel()->clearSelection();
	dialog.draftTable()->selectionModel()->select(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		QItemSelectionModel::Select);
	dialog.draftTable()->selectionModel()->select(
		dialog.draftModel()->index(2, ConductorBulkEditModel::FunctionColumn),
		QItemSelectionModel::Select);
	QVERIFY(!dialog.fillDownAction()->isEnabled());
	QVERIFY(dialog.fillDownButton()->toolTip().contains(QStringLiteral("rectangle")));

	selectRange(
		dialog,
		0,
		2,
		ConductorBulkEditModel::SegmentCountColumn,
		ConductorBulkEditModel::FunctionColumn);
	QVERIFY(!dialog.fillDownAction()->isEnabled());
	QVERIFY(dialog.fillDownButton()->toolTip().contains(QStringLiteral("colonnes")));

	QVector<ConductorBulkEditModel::Row> mixed_rows = rows(3);
	mixed_rows[0].function = mixedCell();
	ConductorBulkEditDialog mixed_dialog(mixed_rows);
	selectRange(
		mixed_dialog,
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn);
	QVERIFY(mixed_dialog.fillDownAction()->isEnabled());
	QVERIFY(mixed_dialog.fillDownButton()->toolTip().contains(
		QStringLiteral("Valeurs multiples")));
	mixed_dialog.fillDownAction()->trigger();
	QCOMPARE(
		mixed_dialog.draftModel()->data(
			mixed_dialog.draftModel()->index(
				2, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Commande"));
	QVERIFY(statusLabel(mixed_dialog)->text().contains(
		QStringLiteral("Valeurs multiples")));

	ConductorBulkEditDialog invalid_dialog(rows(3));
	QVERIFY(invalid_dialog.draftModel()->setData(
		invalid_dialog.draftModel()->index(
			0, ConductorBulkEditModel::FunctionColumn),
		QString(1, QChar(0x0001))));
	selectRange(
		invalid_dialog,
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn);
	QVERIFY(invalid_dialog.fillDownAction()->isEnabled());
	QVERIFY(invalid_dialog.fillDownButton()->toolTip().contains(
		QStringLiteral("invalide")));
	invalid_dialog.fillDownAction()->trigger();
	QCOMPARE(
		invalid_dialog.draftModel()->data(
			invalid_dialog.draftModel()->index(
				2, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Commande"));
	QVERIFY(statusLabel(invalid_dialog)->text().contains(
		QStringLiteral("invalide")));
}

void ConductorBulkEditTest::fillDownContextAndAccessibilityContract()
{
	ConductorBulkEditDialog dialog(rows(3));
	QCOMPARE(dialog.draftTable()->contextMenuPolicy(), Qt::ActionsContextMenu);
	QCOMPARE(dialog.draftTable()->actions().count(dialog.fillDownAction()), 1);
	QCOMPARE(dialog.fillDownAction()->objectName(),
		QStringLiteral("fillDownConductorBulkCellsAction"));
	QCOMPARE(dialog.fillDownAction()->shortcut(), QKeySequence(QStringLiteral("Ctrl+D")));
	QCOMPARE(dialog.fillDownAction()->shortcutContext(), Qt::WidgetWithChildrenShortcut);
	QVERIFY(!dialog.fillDownAction()->text().isEmpty());
	QVERIFY(!dialog.fillDownAction()->toolTip().isEmpty());
	QVERIFY(dialog.draftTable()->accessibleDescription().contains(QStringLiteral("Ctrl+D")));
	QVERIFY(!dialog.fillDownButton()->accessibleName().isEmpty());

	selectRange(
		dialog,
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn);
	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Moteur")));
	dialog.fillDownAction()->trigger();
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(2, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Moteur"));
	QVERIFY(statusLabel(dialog));
	QCOMPARE(statusLabel(dialog)->text(), statusLabel(dialog)->accessibleName());
	QVERIFY(statusLabel(dialog)->text().contains(QStringLiteral("Recopie effectuée")));
	QTest::mouseClick(dialog.resetButton(), Qt::LeftButton);
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(2, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Commande"));
	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		QString()));
	dialog.fillDownAction()->trigger();
	QVERIFY(statusLabel(dialog)->text().contains(QStringLiteral("Effacement préparé")));
}

void ConductorBulkEditTest::activeEditorCommitsBeforeFillDown()
{
	ConductorBulkEditDialog dialog(rows(2));
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	selectRange(
		dialog,
		0,
		1,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn);
	dialog.draftTable()->selectionModel()->setCurrentIndex(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		QItemSelectionModel::NoUpdate);
	dialog.draftTable()->edit(dialog.draftTable()->currentIndex());
	QTRY_VERIFY(dialog.findChild<QLineEdit *>());
	QLineEdit *editor = dialog.findChild<QLineEdit *>();
	editor->setFocus();
	editor->selectAll();
	QTest::keyClicks(editor, QStringLiteral("Source saisie"));
	QTest::keyClick(editor, Qt::Key_D, Qt::ControlModifier);
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Source saisie"));
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(1, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Source saisie"));
	QVERIFY(statusLabel(dialog)->text().contains(QStringLiteral("Recopie effectuée")));
}

void ConductorBulkEditTest::fillDownDraftRemainsDetachedUntilAccepted()
{
	QHash<quintptr, ConductorProperties> project_properties;
	for (quintptr key = 1; key <= 6; ++key) {
		ConductorProperties properties;
		properties.m_function = key <= 2
			? QStringLiteral("Source")
			: QStringLiteral("Projet inchangé");
		project_properties.insert(key, properties);
	}
	const QHash<quintptr, ConductorProperties> before = project_properties;

	ConductorBulkEditDialog dialog(rows(3));
	QSignalSpy accepted(&dialog, &QDialog::accepted);
	QSignalSpy target_activated(
		&dialog, &ConductorBulkEditDialog::targetActivated);
	selectRange(
		dialog,
		0,
		2,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn);
	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Brouillon UX-05C")));
	dialog.fillDownAction()->trigger();

	QVERIFY(project_properties == before);
	QCOMPARE(accepted.count(), 0);
	QCOMPARE(target_activated.count(), 0);
	QCOMPARE(
		dialog.propertiesForTarget(3, project_properties.value(3)).m_function,
		QStringLiteral("Brouillon UX-05C"));

	dialog.verifyButton()->click();
	QCOMPARE(accepted.count(), 1);
	QCOMPARE(target_activated.count(), 0);
	QVERIFY(project_properties == before);
}

void ConductorBulkEditTest::untouchedLegacyValuesRemainCompatible()
{
	ConductorBulkEditModel::Row draft_row = row(0, {1, 2});
	draft_row.function = commonCell(QString(300, QLatin1Char('A')));
	ConductorBulkEditModel model({draft_row});
	QVERIFY(model.isValid());
	QVERIFY(!model.hasChanges());
	QVERIFY(!model.data(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		Qt::BackgroundRole).isValid());

	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::WireSectionColumn),
		QStringLiteral("2,5")));
	QVERIFY(model.isValid());
	QVERIFY(model.hasChanges());

	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		QString(300, QLatin1Char('B'))));
	QVERIFY(!model.isValid());
}

void ConductorBulkEditTest::invalidCellsDisableVerification()
{
	ConductorBulkEditDialog dialog(rows(1));
	QVERIFY(!dialog.verifyButton()->isEnabled());
	const QString control_character(1, QChar(0x0001));
	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		control_character));
	QVERIFY(!dialog.draftModel()->isValid());
	QCOMPARE(dialog.draftModel()->invalidCellCount(), 1);
	QVERIFY(!dialog.verifyButton()->isEnabled());
	QVERIFY(dialog.draftModel()->data(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		Qt::BackgroundRole).isValid());

	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Commande sûre")));
	QVERIFY(dialog.draftModel()->isValid());
	QVERIFY(dialog.verifyButton()->isEnabled());
}

void ConductorBulkEditTest::emptyEditedCellErasesProperty()
{
	ConductorBulkEditModel model(rows(1));
	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::WireSectionColumn), QString()));
	ConductorProperties before;
	before.m_wire_section = QStringLiteral("1,5");
	QCOMPARE(model.propertiesForTarget(1, before).m_wire_section, QString());
	QVERIFY(model.hasChanges());
}

void ConductorBulkEditTest::resetRestoresDraft()
{
	ConductorBulkEditDialog dialog(rows(1));
	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Moteur")));
	QVERIFY(dialog.verifyButton()->isEnabled());
	QVERIFY(dialog.resetButton()->isEnabled());
	QTest::mouseClick(dialog.resetButton(), Qt::LeftButton);
	QVERIFY(!dialog.draftModel()->hasChanges());
	QVERIFY(!dialog.verifyButton()->isEnabled());
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Commande"));
}

void ConductorBulkEditTest::escapeRejectsWithoutApplying()
{
	ConductorBulkEditDialog dialog(rows(1));
	QSignalSpy rejected(&dialog, &QDialog::rejected);
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	QTest::keyClick(&dialog, Qt::Key_Escape);
	QCOMPARE(rejected.count(), 1);
	QCOMPARE(dialog.result(), static_cast<int>(QDialog::Rejected));
}

void ConductorBulkEditTest::keyboardContract()
{
	ConductorBulkEditDialog dialog(rows(2));
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	dialog.draftTable()->setFocus();
	dialog.draftTable()->setCurrentIndex(dialog.draftModel()->index(
		0, ConductorBulkEditModel::FunctionColumn));
	QApplication::clipboard()->setText(
		QStringLiteral("Moteur\t400 VAC\tBK\t2,5\nCapteur\t24 VDC\tBU\t0,75"));
	QTest::keyClick(dialog.draftTable(), Qt::Key_V, Qt::ControlModifier);
	QTRY_COMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(1, ConductorBulkEditModel::WireSectionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("0,75"));
	selectRange(
		dialog,
		0,
		1,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::FunctionColumn);
	dialog.draftTable()->setFocus();
	const QModelIndex current_before_fill = dialog.draftTable()->currentIndex();
	QSignalSpy fill_changed(
		dialog.draftModel(), &QAbstractItemModel::dataChanged);
	QTest::keyClick(dialog.draftTable(), Qt::Key_D, Qt::ControlModifier);
	QTRY_COMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(1, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Moteur"));
	QCOMPARE(fill_changed.count(), 1);
	QCOMPARE(dialog.draftTable()->selectionModel()->selectedIndexes().size(), 2);
	QCOMPARE(dialog.draftTable()->currentIndex(), current_before_fill);
	QVERIFY(dialog.draftTable()->hasFocus());
	QVERIFY(dialog.verifyButton()->isEnabled());
	QVERIFY(dialog.resetButton()->focusPolicy() != Qt::NoFocus);
	QVERIFY(dialog.verifyButton()->focusPolicy() != Qt::NoFocus);
	QVERIFY(cancelButton(dialog));
	QVERIFY(cancelButton(dialog)->focusPolicy() != Qt::NoFocus);
}

void ConductorBulkEditTest::fitsScreenAt150Percent()
{
	ApplicationFontGuard font_guard;
	QFont large_font = QApplication::font();
	large_font.setPointSize(qMax(large_font.pointSize() + 3, 12));
	QApplication::setFont(large_font);

	ConductorBulkEditDialog dialog(rows(30));
	const QSize minimum_hint = dialog.minimumSizeHint();
	QVERIFY2(
		minimum_hint.width() <= 1280,
		qPrintable(QStringLiteral("minimum width: %1").arg(minimum_hint.width())));
	QVERIFY2(
		minimum_hint.height() <= 680,
		qPrintable(QStringLiteral("minimum height: %1").arg(minimum_hint.height())));
	dialog.resize(1000, 650);
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	QVERIFY(dialog.size().width() <= 1280);
	QVERIFY(dialog.size().height() <= 680);
	QVERIFY(dialog.draftTable()->verticalScrollBar()->maximum() > 0);
	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("DPI-150")));
	selectRange(
		dialog,
		0,
		29,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::WireSectionColumn);
	QVERIFY(dialog.fillDownAction()->isEnabled());
	dialog.draftTable()->setFocus();
	QTest::keyClick(dialog.draftTable(), Qt::Key_D, Qt::ControlModifier);
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(29, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("DPI-150"));
	QMenu context_menu;
	context_menu.addActions(dialog.draftTable()->actions());
	const QSize menu_hint = context_menu.sizeHint();
	QVERIFY(menu_hint.width() <= 1280);
	QVERIFY(menu_hint.height() <= 680);
	auto contained_in_dialog = [&dialog](QWidget *widget) {
		return dialog.rect().contains(QRect(
			widget->mapTo(&dialog, QPoint(0, 0)), widget->size()));
	};
	QVERIFY(contained_in_dialog(dialog.draftTable()));
	QVERIFY(dialog.fillDownButton()->isVisible());
	QVERIFY(dialog.fillDownButton()->height() >= 32);
	QVERIFY(contained_in_dialog(dialog.fillDownButton()));
	QVERIFY(dialog.verifyButton()->isVisible());
	QVERIFY(contained_in_dialog(dialog.verifyButton()));
	QVERIFY(dialog.resetButton()->isVisible());
	QVERIFY(contained_in_dialog(dialog.resetButton()));
	QVERIFY(cancelButton(dialog)->isVisible());
	QVERIFY(contained_in_dialog(cancelButton(dialog)));
}

void ConductorBulkEditTest::handlesThousandPotentialsDeterministically()
{
	ConductorBulkEditModel model(rows(1000));
	QCOMPARE(model.rowCount(), 1000);
	QCOMPARE(model.targetKeyForRow(999), static_cast<quintptr>(1999));
	QString error;
	QVERIFY(model.pasteTsv(
		model.index(999, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Dernier\t48 VDC\tRD\t6"),
		&error));
	QVERIFY(error.isEmpty());
	QCOMPARE(model.changedPotentialCount(), 1);
	QCOMPARE(model.changedSegmentCount(), 2);
	ConductorProperties after = model.propertiesForTarget(2000, ConductorProperties());
	QCOMPARE(after.m_function, QStringLiteral("Dernier"));
	QCOMPARE(after.m_wire_section, QStringLiteral("6"));
}

void ConductorBulkEditTest::fillDownHandlesThousandPotentialsDeterministically()
{
	QVector<ConductorBulkEditModel::Row> draft_rows = rows(1000);
	draft_rows[0].function = commonCell(QStringLiteral("COMMUN-1000"));
	draft_rows[0].tensionProtocol = commonCell(QStringLiteral("690 VAC"));
	draft_rows[0].wireColor = commonCell(QStringLiteral("OG"));
	draft_rows[0].wireSection = commonCell(QStringLiteral("16"));
	ConductorBulkEditModel model(draft_rows);
	QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
	QVERIFY(model.fillDown(
		0,
		999,
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::WireSectionColumn));
	QCOMPARE(changed.count(), 1);
	QCOMPARE(
		model.data(model.index(999, ConductorBulkEditModel::FunctionColumn), Qt::EditRole).toString(),
		QStringLiteral("COMMUN-1000"));
	QCOMPARE(
		model.data(model.index(999, ConductorBulkEditModel::TensionProtocolColumn), Qt::EditRole).toString(),
		QStringLiteral("690 VAC"));
	QCOMPARE(
		model.data(model.index(999, ConductorBulkEditModel::WireColorColumn), Qt::EditRole).toString(),
		QStringLiteral("OG"));
	QCOMPARE(
		model.data(model.index(999, ConductorBulkEditModel::WireSectionColumn), Qt::EditRole).toString(),
		QStringLiteral("16"));
	QCOMPARE(model.changedPotentialCount(), 999);
}

QTEST_MAIN(ConductorBulkEditTest)
#include "tst_conductorbulkedit.moc"
