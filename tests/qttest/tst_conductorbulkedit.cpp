/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/SearchAndReplace/conductorbulkeditmodel.h"
#include "../../sources/SearchAndReplace/conductorbulkeditcsvexport.h"
#include "../../sources/SearchAndReplace/ui/conductorbulkeditdialog.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFont>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QTableView>
#include <QTemporaryDir>
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
	draft_row.cable = commonCell(QStringLiteral("C1"));
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
		void initTestCase();
		void init();
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
		void explicitColumnsNeverModifyHiddenFields();
		void hiddenColumnsAreExcludedFromFillSelection();
		void columnLayoutPersistsByStableKeys();
		void corruptedColumnLayoutFallsBackToDefault();
		void mandatoryAndLastEditableColumnsRemainVisible();
		void exactModeOnlyEditsCableAndColor();
		void exactDialogShowsContextColumns();
		void reorderedPasteFollowsVisibleColumns();
		void hiddenDraftChangesAreAnnounced();
		void reviewCsvFollowsVisibleVisualOrder();
		void reviewCsvCommitsActiveEditor();
		void reviewCsvUsesExactExcelSafeEncoding();
		void reviewCsvDistinguishesMixedAndExplicitEmpty();
		void reviewCsvWriteFailureIsAtomicAndReadOnly();
		void reviewCsvHandlesThousandRowsDeterministically();
};

void ConductorBulkEditTest::initTestCase()
{
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTechTests"));
	QCoreApplication::setApplicationName(QStringLiteral("ConductorBulkEditTest"));
}

void ConductorBulkEditTest::init()
{
	QSettings settings;
	settings.clear();
	settings.sync();
}

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
	QVERIFY(dialog.columnsButton()->isVisible());
	QVERIFY(dialog.columnsButton()->height() >= 32);
	QVERIFY(contained_in_dialog(dialog.columnsButton()));
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

void ConductorBulkEditTest::explicitColumnsNeverModifyHiddenFields()
{
	QVector<ConductorBulkEditModel::Row> draft_rows = rows(3);
	draft_rows[0].function = commonCell(QStringLiteral("Moteur"));
	draft_rows[0].tensionProtocol = commonCell(QStringLiteral("690 VAC"));
	draft_rows[0].wireColor = commonCell(QStringLiteral("BK"));
	ConductorBulkEditModel model(draft_rows);
	const QVector<int> visible_columns {
		ConductorBulkEditModel::FunctionColumn,
		ConductorBulkEditModel::WireColorColumn
	};
	QVERIFY(model.fillDown(0, 2, visible_columns));
	QCOMPARE(
		model.data(model.index(2, ConductorBulkEditModel::FunctionColumn), Qt::EditRole).toString(),
		QStringLiteral("Moteur"));
	QCOMPARE(
		model.data(model.index(2, ConductorBulkEditModel::WireColorColumn), Qt::EditRole).toString(),
		QStringLiteral("BK"));
	QCOMPARE(
		model.data(model.index(2, ConductorBulkEditModel::TensionProtocolColumn), Qt::EditRole).toString(),
		QStringLiteral("24 VDC"));
	QCOMPARE(
		model.data(model.index(2, ConductorBulkEditModel::WireSectionColumn), Qt::EditRole).toString(),
		QStringLiteral("1,5"));

	ConductorBulkEditModel pasted(rows(1));
	QVERIFY(pasted.pasteTsv(
		0,
		{ConductorBulkEditModel::WireSectionColumn,
		 ConductorBulkEditModel::FunctionColumn},
		QStringLiteral("6\tPuissance")));
	QCOMPARE(
		pasted.data(pasted.index(0, ConductorBulkEditModel::WireSectionColumn), Qt::EditRole).toString(),
		QStringLiteral("6"));
	QCOMPARE(
		pasted.data(pasted.index(0, ConductorBulkEditModel::FunctionColumn), Qt::EditRole).toString(),
		QStringLiteral("Puissance"));
	QCOMPARE(
		pasted.data(pasted.index(0, ConductorBulkEditModel::TensionProtocolColumn), Qt::EditRole).toString(),
		QStringLiteral("24 VDC"));
}

void ConductorBulkEditTest::hiddenColumnsAreExcludedFromFillSelection()
{
	QVector<ConductorBulkEditModel::Row> draft_rows = rows(3);
	draft_rows[0].function = commonCell(QStringLiteral("Moteur"));
	draft_rows[0].tensionProtocol = commonCell(QStringLiteral("690 VAC"));
	draft_rows[0].wireColor = commonCell(QStringLiteral("BK"));
	ConductorBulkEditDialog dialog(draft_rows);
	dialog.columnAction(
		ConductorBulkEditModel::TensionProtocolColumn)->setChecked(false);
	auto selection_model = dialog.draftTable()->selectionModel();
	selection_model->clearSelection();
	for (int row_index = 0; row_index < 3; ++row_index) {
		for (int column : {
			 ConductorBulkEditModel::FunctionColumn,
			 ConductorBulkEditModel::WireColorColumn}) {
			selection_model->select(
				dialog.draftModel()->index(row_index, column),
				QItemSelectionModel::Select);
		}
	}
	selection_model->setCurrentIndex(
		dialog.draftModel()->index(
			2, ConductorBulkEditModel::WireColorColumn),
		QItemSelectionModel::NoUpdate);
	QVERIFY(dialog.fillDownAction()->isEnabled());
	dialog.fillDownAction()->trigger();
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(2, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Moteur"));
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(2, ConductorBulkEditModel::WireColorColumn),
			Qt::EditRole).toString(),
		QStringLiteral("BK"));
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(
				2, ConductorBulkEditModel::TensionProtocolColumn),
			Qt::EditRole).toString(),
		QStringLiteral("24 VDC"));
}

void ConductorBulkEditTest::columnLayoutPersistsByStableKeys()
{
	QVector<int> stored_order;
	{
		ConductorBulkEditDialog dialog(rows(2));
		dialog.columnAction(ConductorBulkEditModel::FolioColumn)->setChecked(false);
		dialog.columnAction(
			ConductorBulkEditModel::TensionProtocolColumn)->setChecked(false);
		auto header = dialog.draftTable()->horizontalHeader();
		header->moveSection(
			header->visualIndex(ConductorBulkEditModel::WireSectionColumn),
			0);
		for (int visual = 0; visual < header->count(); ++visual) {
			stored_order.append(header->logicalIndex(visual));
		}
	}

	ConductorBulkEditDialog restored(rows(2));
	QVERIFY(restored.draftTable()->isColumnHidden(
		ConductorBulkEditModel::FolioColumn));
	QVERIFY(restored.draftTable()->isColumnHidden(
		ConductorBulkEditModel::TensionProtocolColumn));
	QVERIFY(!restored.draftTable()->isColumnHidden(
		ConductorBulkEditModel::PotentialColumn));
	auto restored_header = restored.draftTable()->horizontalHeader();
	for (int visual = 0; visual < restored_header->count(); ++visual) {
		QCOMPARE(restored_header->logicalIndex(visual), stored_order.at(visual));
	}

	QSettings settings;
	QCOMPARE(
		settings.value(QStringLiteral(
			"conductor-bulk-editor/column-layout/version")).toInt(),
		1);
	const QStringList order_keys = settings.value(QStringLiteral(
		"conductor-bulk-editor/column-layout/order")).toStringList();
	QCOMPARE(order_keys.first(), QStringLiteral("wire_section"));
	QVERIFY(!settings.value(QStringLiteral(
		"conductor-bulk-editor/column-layout/visible"))
		.toStringList().contains(QStringLiteral("folio")));
}

void ConductorBulkEditTest::corruptedColumnLayoutFallsBackToDefault()
{
	QSettings settings;
	settings.setValue(
		QStringLiteral("conductor-bulk-editor/column-layout/version"), 1);
	settings.setValue(
		QStringLiteral("conductor-bulk-editor/column-layout/order"),
		QStringList({QStringLiteral("function"), QStringLiteral("function")}));
	settings.setValue(
		QStringLiteral("conductor-bulk-editor/column-layout/visible"),
		QStringList({QStringLiteral("unknown")}));
	settings.sync();

	ConductorBulkEditDialog dialog(rows(1));
	auto header = dialog.draftTable()->horizontalHeader();
	const auto descriptors = ConductorBulkEditModel::columnDescriptors();
	for (int logical = 0;
		 logical < ConductorBulkEditModel::ColumnCount;
		 ++logical) {
		QCOMPARE(header->logicalIndex(logical), logical);
		const auto &descriptor = descriptors.at(logical);
		QCOMPARE(
			dialog.draftTable()->isColumnHidden(logical),
			!(descriptor.mandatory || descriptor.defaultVisible));
	}
}

void ConductorBulkEditTest::mandatoryAndLastEditableColumnsRemainVisible()
{
	ConductorBulkEditDialog dialog(rows(1));
	QAction *potential = dialog.columnAction(
		ConductorBulkEditModel::PotentialColumn);
	QVERIFY(potential);
	QVERIFY(potential->isChecked());
	QVERIFY(!potential->isEnabled());

	for (int column : {
		 ConductorBulkEditModel::FunctionColumn,
		 ConductorBulkEditModel::TensionProtocolColumn,
		 ConductorBulkEditModel::WireColorColumn}) {
		dialog.columnAction(column)->setChecked(false);
		QVERIFY(dialog.draftTable()->isColumnHidden(column));
	}
	QAction *last = dialog.columnAction(
		ConductorBulkEditModel::WireSectionColumn);
	last->setChecked(false);
	QVERIFY(last->isChecked());
	QVERIFY(!dialog.draftTable()->isColumnHidden(
		ConductorBulkEditModel::WireSectionColumn));
	QVERIFY(statusLabel(dialog)->text().contains(
		QStringLiteral("au moins une colonne")));
}

void ConductorBulkEditTest::exactModeOnlyEditsCableAndColor()
{
	auto draft_row = row(0, {101});
	ConductorBulkEditModel model(
		{draft_row},
		ConductorBulkEditModel::Mode::ExactConductors);
	QCOMPARE(
		model.mode(),
		ConductorBulkEditModel::Mode::ExactConductors);
	QVERIFY(!(model.flags(model.index(
		0, ConductorBulkEditModel::FunctionColumn)) & Qt::ItemIsEditable));
	QVERIFY(model.flags(model.index(
		0, ConductorBulkEditModel::WireColorColumn)) & Qt::ItemIsEditable);
	QVERIFY(model.flags(model.index(
		0, ConductorBulkEditModel::CableColumn)) & Qt::ItemIsEditable);
	QVERIFY(!model.setData(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		QStringLiteral("Ne doit pas changer")));
	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::WireColorColumn),
		QStringLiteral("GY")));
	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::CableColumn),
		QStringLiteral("C20")));

	ConductorProperties before;
	before.m_function = QStringLiteral("Commande");
	before.m_tension_protocol = QStringLiteral("24 VDC");
	before.m_wire_color = QStringLiteral("BU");
	before.m_wire_section = QStringLiteral("1,5");
	before.m_cable = QStringLiteral("C1");
	const ConductorProperties after = model.propertiesForTarget(101, before);
	QCOMPARE(after.m_function, before.m_function);
	QCOMPARE(after.m_tension_protocol, before.m_tension_protocol);
	QCOMPARE(after.m_wire_section, before.m_wire_section);
	QCOMPARE(after.m_wire_color, QStringLiteral("GY"));
	QCOMPARE(after.m_cable, QStringLiteral("C20"));
}

void ConductorBulkEditTest::exactDialogShowsContextColumns()
{
	ConductorBulkEditDialog dialog(
		rows(2),
		ConductorBulkEditModel::Mode::ExactConductors);
	QVERIFY(dialog.windowTitle().contains(QStringLiteral("sélectionnés")));
	QVERIFY(!dialog.draftTable()->isColumnHidden(
		ConductorBulkEditModel::WireColorColumn));
	QVERIFY(!dialog.draftTable()->isColumnHidden(
		ConductorBulkEditModel::CableColumn));
	for (int column : {
		 ConductorBulkEditModel::FunctionColumn,
		 ConductorBulkEditModel::TensionProtocolColumn,
		 ConductorBulkEditModel::WireSectionColumn}) {
		QVERIFY(dialog.draftTable()->isColumnHidden(column));
		QVERIFY(!dialog.columnAction(column)->isEnabled());
	}
	QVERIFY(dialog.columnAction(
		ConductorBulkEditModel::CableColumn)->isEnabled());
	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(
			0, ConductorBulkEditModel::CableColumn),
		QStringLiteral("C20")));
	QVERIFY(statusLabel(dialog)->text().contains(
		QStringLiteral("conducteur")));
	QVERIFY(dialog.verifyButton()->isEnabled());
}

void ConductorBulkEditTest::reorderedPasteFollowsVisibleColumns()
{
	ConductorBulkEditDialog dialog(rows(2));
	dialog.columnAction(
		ConductorBulkEditModel::TensionProtocolColumn)->setChecked(false);
	auto header = dialog.draftTable()->horizontalHeader();
	header->moveSection(
		header->visualIndex(ConductorBulkEditModel::WireSectionColumn),
		header->visualIndex(ConductorBulkEditModel::FunctionColumn));
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	dialog.draftTable()->setCurrentIndex(dialog.draftModel()->index(
		0, ConductorBulkEditModel::WireSectionColumn));
	dialog.draftTable()->setFocus();
	QApplication::clipboard()->setText(
		QStringLiteral("6\tMoteur\tBK\n2,5\tCapteur\tBU"));
	QTest::keyClick(dialog.draftTable(), Qt::Key_V, Qt::ControlModifier);
	QTRY_COMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(1, ConductorBulkEditModel::FunctionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("Capteur"));
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(1, ConductorBulkEditModel::WireSectionColumn),
			Qt::EditRole).toString(),
		QStringLiteral("2,5"));
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(1, ConductorBulkEditModel::WireColorColumn),
			Qt::EditRole).toString(),
		QStringLiteral("BU"));
	QCOMPARE(
		dialog.draftModel()->data(
			dialog.draftModel()->index(1, ConductorBulkEditModel::TensionProtocolColumn),
			Qt::EditRole).toString(),
		QStringLiteral("24 VDC"));
}

void ConductorBulkEditTest::hiddenDraftChangesAreAnnounced()
{
	ConductorBulkEditDialog dialog(rows(2));
	QVERIFY(dialog.draftModel()->setData(
		dialog.draftModel()->index(
			0, ConductorBulkEditModel::TensionProtocolColumn),
		QStringLiteral("PROFINET")));
	dialog.columnAction(
		ConductorBulkEditModel::TensionProtocolColumn)->setChecked(false);
	QVERIFY(statusLabel(dialog)->text().contains(
		QStringLiteral("colonnes masquées")));
	QVERIFY(!dialog.columnsButton()->accessibleName().isEmpty());
	QVERIFY(dialog.columnsButton()->menu());
	QVERIFY(!dialog.columnsButton()->menu()->accessibleName().isEmpty());
	QVERIFY(dialog.resetColumnLayoutAction());
	dialog.resetColumnLayoutAction()->trigger();
	QVERIFY(!dialog.draftTable()->isColumnHidden(
		ConductorBulkEditModel::TensionProtocolColumn));
}

void ConductorBulkEditTest::reviewCsvFollowsVisibleVisualOrder()
{
	ConductorBulkEditDialog dialog(rows(1));
	for (int column : {
		 ConductorBulkEditModel::FolioColumn,
		 ConductorBulkEditModel::SegmentCountColumn,
		 ConductorBulkEditModel::TensionProtocolColumn,
		 ConductorBulkEditModel::WireColorColumn}) {
		dialog.columnAction(column)->setChecked(false);
	}
	auto header = dialog.draftTable()->horizontalHeader();
	header->moveSection(
		header->visualIndex(ConductorBulkEditModel::WireSectionColumn), 0);
	header->moveSection(
		header->visualIndex(ConductorBulkEditModel::PotentialColumn), 1);
	header->moveSection(
		header->visualIndex(ConductorBulkEditModel::FunctionColumn), 2);

	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("review.csv"));
	QVERIFY(dialog.exportReviewToFile(path));
	QFile file(path);
	QVERIFY(file.open(QIODevice::ReadOnly));
	const QByteArray bytes = file.readAll();
	QCOMPARE(bytes.left(3), QByteArray::fromHex("efbbbf"));
	const QString decoded = QString::fromUtf8(bytes.mid(3));
	QVERIFY(decoded.startsWith(QStringLiteral(
		"Section;Potentiel / conducteur;Fonction\r\n")));
	QVERIFY(!decoded.contains(QStringLiteral("Folio(s)")));
	QVERIFY(!decoded.contains(QStringLiteral("Tension / protocole")));
	QVERIFY(dialog.reviewExportButton()->isEnabled());
	QCOMPARE(
		dialog.reviewExportButton()->accessibleName(),
		QStringLiteral("Exporter le brouillon pour revue"));
	QVERIFY(statusLabel(dialog)->text().contains(QStringLiteral("Export terminé")));
	QVERIFY(statusLabel(dialog)->text().contains(QStringLiteral("projet reste inchangé")));
}

void ConductorBulkEditTest::reviewCsvCommitsActiveEditor()
{
	ConductorBulkEditDialog dialog(rows(1));
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	const QModelIndex function = dialog.draftModel()->index(
		0, ConductorBulkEditModel::FunctionColumn);
	dialog.draftTable()->setCurrentIndex(function);
	dialog.draftTable()->edit(function);
	QTRY_VERIFY(dialog.findChild<QLineEdit *>());
	QLineEdit *editor = dialog.findChild<QLineEdit *>();
	editor->setFocus();
	editor->selectAll();
	QTest::keyClicks(editor, QStringLiteral("Source saisie"));

	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath(QStringLiteral("active-editor.csv"));
	QVERIFY(dialog.exportReviewToFile(path));
	QCOMPARE(
		dialog.draftModel()->data(function, Qt::EditRole).toString(),
		QStringLiteral("Source saisie"));
	QFile file(path);
	QVERIFY(file.open(QIODevice::ReadOnly));
	QVERIFY(QString::fromUtf8(file.readAll()).contains(
		QStringLiteral("Source saisie")));
}

void ConductorBulkEditTest::reviewCsvUsesExactExcelSafeEncoding()
{
	QVector<ConductorBulkEditModel::Row> draft_rows;
	auto first = row(0, {1});
	first.folios = QStringLiteral("Étage été");
	first.potential = QStringLiteral("+Départ éclairage");
	first.function = commonCell(QStringLiteral("=HYPERLINK(\"x\")"));
	first.tensionProtocol = commonCell(QStringLiteral("-10+20"));
	first.wireColor = commonCell(QStringLiteral("@SUM(A1:A2)"));
	first.wireSection = commonCell(QStringLiteral("1;5 mm²"));
	draft_rows.append(first);
	auto second = row(1, {2});
	second.function = commonCell(QStringLiteral("Ligne A\nLigne B"));
	draft_rows.append(second);
	ConductorBulkEditModel model(draft_rows);

	const auto result = ConductorBulkEditCsvExporter::toCsv(
		model, ConductorBulkEditModel::defaultColumnOrder());
	QVERIFY2(result.success, qPrintable(result.error));
	QCOMPARE(result.formulaNeutralizedCellCount, 4);
	const QByteArray bytes = result.contents.toUtf8();
	QCOMPARE(bytes.left(3), QByteArray::fromHex("efbbbf"));
	QCOMPARE(bytes.indexOf(QByteArray::fromHex("efbbbf"), 3), -1);
	const QString decoded = QString::fromUtf8(bytes.mid(3));
	QVERIFY(decoded.contains(QStringLiteral("Étage été")));
	QVERIFY(decoded.contains(QStringLiteral("__QET_LITERAL__+Départ éclairage")));
	QVERIFY(decoded.contains(QStringLiteral(
		"\"__QET_LITERAL__=HYPERLINK(\"\"x\"\")\"")));
	QVERIFY(decoded.contains(QStringLiteral("\"1;5 mm²\"")));
	QVERIFY(decoded.contains(QStringLiteral("\"Ligne A\r\nLigne B\"")));
	QVERIFY(decoded.endsWith(QStringLiteral("\r\n")));
	for (int index = 0; index < decoded.size(); ++index) {
		if (decoded.at(index) == QLatin1Char('\n')) {
			QVERIFY(index > 0 && decoded.at(index - 1) == QLatin1Char('\r'));
		}
		if (decoded.at(index) == QLatin1Char('\r')) {
			QVERIFY(index + 1 < decoded.size()
				&& decoded.at(index + 1) == QLatin1Char('\n'));
		}
	}
}

void ConductorBulkEditTest::reviewCsvDistinguishesMixedAndExplicitEmpty()
{
	auto first = row(0, {1});
	first.function = mixedCell();
	auto second = row(1, {2});
	second.function = mixedCell();
	ConductorBulkEditModel model({first, second});
	QVERIFY(model.setData(
		model.index(0, ConductorBulkEditModel::WireSectionColumn), QString()));
	QVERIFY(model.setData(
		model.index(1, ConductorBulkEditModel::FunctionColumn), QString()));

	const auto result = ConductorBulkEditCsvExporter::toCsv(
		model,
		{ConductorBulkEditModel::FunctionColumn,
		 ConductorBulkEditModel::WireSectionColumn});
	QVERIFY2(result.success, qPrintable(result.error));
	QCOMPARE(result.mixedCellCount, 1);
	QCOMPARE(result.explicitEmptyCellCount, 2);
	QCOMPARE(
		result.contents.mid(1),
		QStringLiteral(
			"Fonction;Section\r\n"
			"__QET_MIXED_VALUE__;__QET_EXPLICIT_EMPTY__\r\n"
			"__QET_EXPLICIT_EMPTY__;1,5\r\n"));
}

void ConductorBulkEditTest::reviewCsvWriteFailureIsAtomicAndReadOnly()
{
	ConductorBulkEditModel model(rows(2));
	QSignalSpy data_changed(&model, &QAbstractItemModel::dataChanged);
	QSignalSpy model_reset(&model, &QAbstractItemModel::modelReset);
	const QString before = model.data(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		Qt::EditRole).toString();
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = QDir(directory.path()).filePath(
		QStringLiteral("missing/review.csv"));
	const auto result = ConductorBulkEditCsvExporter::writeCsv(
		model, ConductorBulkEditModel::defaultColumnOrder(), path);
	QVERIFY(!result.success);
	QVERIFY(!result.error.isEmpty());
	QVERIFY(!QFileInfo::exists(path));
	QCOMPARE(model.data(
		model.index(0, ConductorBulkEditModel::FunctionColumn),
		Qt::EditRole).toString(), before);
	QCOMPARE(data_changed.count(), 0);
	QCOMPARE(model_reset.count(), 0);
	QVERIFY(!model.hasChanges());
}

void ConductorBulkEditTest::reviewCsvHandlesThousandRowsDeterministically()
{
	ConductorBulkEditModel model(rows(1000));
	QSignalSpy data_changed(&model, &QAbstractItemModel::dataChanged);
	const auto first = ConductorBulkEditCsvExporter::toCsv(
		model, ConductorBulkEditModel::defaultColumnOrder());
	const auto second = ConductorBulkEditCsvExporter::toCsv(
		model, ConductorBulkEditModel::defaultColumnOrder());
	QVERIFY(first.success);
	QVERIFY(second.success);
	QCOMPARE(first.rowCount, 1000);
	QCOMPARE(first.contents, second.contents);
	QCOMPARE(first.contents.count(QStringLiteral("\r\n")), 1001);
	QCOMPARE(data_changed.count(), 0);
}

QTEST_MAIN(ConductorBulkEditTest)
#include "tst_conductorbulkedit.moc"
