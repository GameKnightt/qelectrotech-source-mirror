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

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFont>
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

}

class ConductorBulkEditTest : public QObject
{
	Q_OBJECT

	private slots:
		void mixedValuesRemainUntouchedUntilEdited();
		void tsvPasteMapsFourEditableFields();
		void outOfBoundsPasteIsAtomic();
		void untouchedLegacyValuesRemainCompatible();
		void invalidCellsDisableVerification();
		void emptyEditedCellErasesProperty();
		void resetRestoresDraft();
		void escapeRejectsWithoutApplying();
		void keyboardContract();
		void fitsScreenAt150Percent();
		void handlesThousandPotentialsDeterministically();
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
		minimum_hint.height() <= 720,
		qPrintable(QStringLiteral("minimum height: %1").arg(minimum_hint.height())));
	dialog.resize(900, 650);
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	QVERIFY(dialog.draftTable()->verticalScrollBar()->maximum() > 0);
	QVERIFY(dialog.verifyButton()->isVisible());
	QVERIFY(dialog.resetButton()->isVisible());
	QVERIFY(cancelButton(dialog)->isVisible());
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

QTEST_MAIN(ConductorBulkEditTest)
#include "tst_conductorbulkedit.moc"
