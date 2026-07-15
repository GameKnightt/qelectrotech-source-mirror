/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/SearchAndReplace/conductorpropertydiff.h"
#include "../../sources/SearchAndReplace/conductorpropertytransform.h"
#include "../../sources/SearchAndReplace/conductorscopeutils.h"
#include "../../sources/SearchAndReplace/conductorchangeplan.h"
#include "../../sources/SearchAndReplace/ui/conductorchangepreviewdialog.h"
#include "../../sources/conductorpropertiesresolver.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QSet>
#include <QTableWidget>
#include <QUndoCommand>
#include <QUndoStack>
#include <QtTest>

#include <type_traits>

namespace {

using ExactTransformBuild = ConductorChangePlan (*)(
	QETProject *,
	const QList<Conductor *> &,
	const ConductorChangePlan::Transform &,
	ConductorChangePlan::ExpansionScope);

using ExactTargetTransformBuild = ConductorChangePlan (*)(
	QETProject *,
	const QList<Conductor *> &,
	const ConductorChangePlan::TargetTransform &,
	ConductorChangePlan::ExpansionScope);

static_assert(std::is_same_v<
	decltype(static_cast<ExactTransformBuild>(&ConductorChangePlan::build)),
	ExactTransformBuild>);
static_assert(std::is_same_v<
	decltype(static_cast<ExactTargetTransformBuild>(&ConductorChangePlan::build)),
	ExactTargetTransformBuild>);

ConductorChangePreviewData previewData(int row_count = 3, bool changes = true)
{
	ConductorChangePreviewData data;
	data.requestedCount = 2;
	data.consideredCount = qMax(row_count, 2);
	data.affectedCount = changes ? 2 : 0;
	data.unchangedCount = data.consideredCount - data.affectedCount;
	data.potentialCount = 1;
	data.folioCount = 2;
	for (int index = 0; index < row_count; ++index) {
		const bool row_changes = changes && index < 2;
		data.rows.append({
			static_cast<quintptr>(index + 1),
			QStringLiteral("%1 — Folio de test avec un titre long").arg(index % 2 + 1),
			QStringLiteral("W%1 · X1 ↔ X2").arg(index + 1),
			row_changes ? QStringLiteral("Section") : QStringLiteral("Aucun champ"),
			row_changes ? QStringLiteral("1,5 mm²") : QStringLiteral("—"),
			row_changes ? QStringLiteral("2,5 mm²") : QStringLiteral("—"),
			row_changes ? QStringLiteral("À modifier") : QStringLiteral("Inchangé"),
			row_changes});
	}
	return data;
}

QPushButton *cancelButton(ConductorChangePreviewDialog &dialog)
{
	auto box = dialog.findChild<QDialogButtonBox *>(
		QStringLiteral("conductorChangeButtons"));
	return box ? box->button(QDialogButtonBox::Cancel) : nullptr;
}

class ValuesCommand final : public QUndoCommand
{
	public:
		ValuesCommand(
				QVector<int> *values,
				QVector<int> before,
				QVector<int> after) :
			m_values(values),
			m_before(std::move(before)),
			m_after(std::move(after))
		{}

		void undo() override { *m_values = m_before; }
		void redo() override { *m_values = m_after; }

	private:
		QVector<int> *m_values;
		QVector<int> m_before;
		QVector<int> m_after;
};

class ApplicationFontGuard final
{
	public:
		ApplicationFontGuard() : m_original(QApplication::font()) {}
		~ApplicationFontGuard() { QApplication::setFont(m_original); }

	private:
		QFont m_original;
};

}

class ConductorChangePreviewTest : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void propertyDiffDescribesChangedFields();
		void formulaResolutionMatchesApplicationContract();
		void combinesPropertyPatchAndAdvancedReplacement();
		void deduplicatesOverlappingPotentialScopes();
		void executionGateRejectsStaleStateWithoutMutation();
		void executionGateCreatesSingleUndoTransaction();
		void executionGateHandlesNoOpAndApplicationFailure();
		void showsScopeAndAccessibleContract();
		void exactScopePreviewNeverClaimsPotentialExpansion();
		void cancelLeavesDialogRejected();
		void escapeLeavesDialogRejected();
		void applyIsEnabledOnlyForChanges();
		void enterInPreviewActivatesTargetWithoutApplying();
		void keyboardContract();
		void fitsScreenAt150Percent();
		void handlesThousandRowsDeterministically();
};

void ConductorChangePreviewTest::initTestCase()
{
	qRegisterMetaType<quintptr>("quintptr");
}

void ConductorChangePreviewTest::propertyDiffDescribesChangedFields()
{
	ConductorProperties before;
	before.text = QStringLiteral("W1");
	before.m_function = QStringLiteral("Commande");
	before.m_wire_section = QStringLiteral("1,5");
	before.color = QColor(QStringLiteral("#123456"));
	before.m_show_text = true;

	ConductorProperties after = before;
	after.m_function = QStringLiteral("Puissance");
	after.m_wire_section = QStringLiteral("2,5");
	after.color = QColor(QStringLiteral("#654321"));
	after.m_show_text = false;
	after.singleLineProperties.setPhasesCount(3);

	const QVector<ConductorPropertyChange> changes =
		conductorPropertyChanges(before, after);
	QSet<QString> keys;
	QHash<QString, ConductorPropertyChange> changes_by_key;
	for (const ConductorPropertyChange &change : changes) {
		keys.insert(change.key);
		changes_by_key.insert(change.key, change);
		QVERIFY(!change.label.isEmpty());
		QVERIFY(!change.before.isEmpty());
		QVERIFY(!change.after.isEmpty());
	}
	QCOMPARE(keys, QSet<QString>({
		QStringLiteral("function"),
		QStringLiteral("wire_section"),
		QStringLiteral("show_text"),
		QStringLiteral("color"),
		QStringLiteral("phases")}));
	QCOMPARE(changes_by_key.value(QStringLiteral("function")).before,
		QStringLiteral("Commande"));
	QCOMPARE(changes_by_key.value(QStringLiteral("function")).after,
		QStringLiteral("Puissance"));
	QCOMPARE(changes_by_key.value(QStringLiteral("wire_section")).before,
		QStringLiteral("1,5"));
	QCOMPARE(changes_by_key.value(QStringLiteral("wire_section")).after,
		QStringLiteral("2,5"));
	QVERIFY(conductorPropertyChanges(before, before).isEmpty());
}

void ConductorChangePreviewTest::formulaResolutionMatchesApplicationContract()
{
	ConductorProperties requested;
	requested.m_formula = QStringLiteral("%F-W1");
	requested.text = QStringLiteral("texte saisi directement");
	QString evaluated_formula;
	const ConductorProperties resolved = resolveConductorProperties(
		requested,
		[&evaluated_formula](const QString &formula) {
			evaluated_formula = formula;
			return QStringLiteral("12-W1");
		});
	QCOMPARE(evaluated_formula, requested.m_formula);
	QCOMPARE(resolved.m_formula, requested.m_formula);
	QCOMPARE(resolved.text, QStringLiteral("12-W1"));

	ConductorProperties without_context = requested;
	without_context.text.clear();
	QCOMPARE(
		resolveConductorProperties(without_context, {}).text,
		requested.m_formula);

	ConductorProperties literal = requested;
	literal.m_formula.clear();
	literal.text = QStringLiteral("W42");
	QCOMPARE(resolveConductorProperties(literal, {}).text, literal.text);
}

void ConductorChangePreviewTest::combinesPropertyPatchAndAdvancedReplacement()
{
	ConductorProperties original;
	original.m_formula = QStringLiteral("%wf-%ws");
	original.text = QStringLiteral("Puissance-1,5");
	original.m_function = QStringLiteral("Puissance");
	original.m_wire_section = QStringLiteral("1,5");
	original.m_wire_color = QStringLiteral("BK");

	ConductorProperties patch;
	patch.m_function = QStringLiteral("Commande");
	advancedReplaceStruct advanced;
	advanced.who = 2;
	advanced.what = QStringLiteral("conductor_section");
	advanced.search = QStringLiteral("1,5");
	advanced.replace = QStringLiteral("2,5");

	const ConductorProperties transformed = transformConductorProperties(
		original,
		patch,
		advanced,
		true,
		true);
	QCOMPARE(transformed.m_function, QStringLiteral("Commande"));
	QCOMPARE(transformed.m_wire_section, QStringLiteral("2,5"));
	QCOMPARE(transformed.m_wire_color, original.m_wire_color);
	QCOMPARE(transformed.m_formula, original.m_formula);

	const ConductorProperties advanced_only = transformConductorProperties(
		original,
		patch,
		advanced,
		false,
		true);
	QCOMPARE(advanced_only.m_function, original.m_function);
	QCOMPARE(advanced_only.m_wire_section, QStringLiteral("2,5"));
}

void ConductorChangePreviewTest::deduplicatesOverlappingPotentialScopes()
{
	QSet<int> visited;
	QCOMPARE(
		takeUnvisitedConductorScopeMembers(QVector<int>{1, 2, 2}, visited),
		QVector<int>({1, 2}));
	QCOMPARE(
		takeUnvisitedConductorScopeMembers(QVector<int>{2, 3, 1}, visited),
		QVector<int>({3}));
	QCOMPARE(
		takeUnvisitedConductorScopeMembers(QVector<int>{3, 2}, visited),
		QVector<int>());
	QCOMPARE(visited, QSet<int>({1, 2, 3}));
}

void ConductorChangePreviewTest::executionGateRejectsStaleStateWithoutMutation()
{
	int validation_calls = 0;
	int application_calls = 0;
	const auto result = ConductorChangePlan::executeAtomically(
		true,
		[&validation_calls]() {
			++validation_calls;
			return ConductorChangePlan::Result{
				ConductorChangePlan::Code::PropertiesChanged, 2, 7};
		},
		[&application_calls]() {
			++application_calls;
			return true;
		});
	QCOMPARE(result.code, ConductorChangePlan::Code::PropertiesChanged);
	QCOMPARE(result.groupIndex, 2);
	QCOMPARE(result.entryIndex, 7);
	QCOMPARE(validation_calls, 1);
	QCOMPARE(application_calls, 0);
}

void ConductorChangePreviewTest::executionGateCreatesSingleUndoTransaction()
{
	QVector<int> values = {1, 2};
	const QVector<int> before = values;
	const QVector<int> after = {10, 200};
	QUndoStack stack;
	int validation_calls = 0;
	int application_calls = 0;

	const auto result = ConductorChangePlan::executeAtomically(
		true,
		[&validation_calls]() {
			++validation_calls;
			return ConductorChangePlan::Result{};
		},
		[&]() {
			++application_calls;
			stack.push(new ValuesCommand(&values, before, after));
			return true;
		});

	QVERIFY(result.canApply());
	QCOMPARE(validation_calls, 1);
	QCOMPARE(application_calls, 1);
	QCOMPARE(stack.count(), 1);
	QCOMPARE(values, after);
	stack.undo();
	QCOMPARE(values, before);
	stack.redo();
	QCOMPARE(values, after);
}

void ConductorChangePreviewTest::executionGateHandlesNoOpAndApplicationFailure()
{
	int validation_calls = 0;
	int application_calls = 0;
	const auto no_op = ConductorChangePlan::executeAtomically(
		false,
		[&validation_calls]() {
			++validation_calls;
			return ConductorChangePlan::Result{};
		},
		[&application_calls]() {
			++application_calls;
			return true;
		});
	QVERIFY(no_op.isNoOp());
	QCOMPARE(validation_calls, 0);
	QCOMPARE(application_calls, 0);

	const auto failed = ConductorChangePlan::executeAtomically(
		true,
		[]() { return ConductorChangePlan::Result{}; },
		[]() { return false; });
	QCOMPARE(failed.code, ConductorChangePlan::Code::ApplyFailed);

	const auto invalid = ConductorChangePlan::executeAtomically(
		true,
		ConductorChangePlan::Validation{},
		[]() { return true; });
	QCOMPARE(invalid.code, ConductorChangePlan::Code::InvalidTransform);
}

void ConductorChangePreviewTest::showsScopeAndAccessibleContract()
{
	ConductorChangePreviewDialog dialog(previewData());
	QVERIFY(!dialog.accessibleName().isEmpty());
	QVERIFY(!dialog.accessibleDescription().isEmpty());
	QVERIFY(dialog.previewTable());
	QVERIFY(!dialog.previewTable()->accessibleName().isEmpty());
	QVERIFY(!dialog.previewTable()->accessibleDescription().isEmpty());
	QCOMPARE(dialog.previewTable()->columnCount(), 6);
	QCOMPARE(dialog.previewTable()->rowCount(), 3);
	QVERIFY(dialog.applyButton());
	QVERIFY(!dialog.applyButton()->accessibleName().isEmpty());
	QVERIFY(dialog.applyButton()->isEnabled());

	auto summary = dialog.findChild<QLabel *>(
		QStringLiteral("conductorChangeSummary"));
	auto notice = dialog.findChild<QLabel *>(
		QStringLiteral("conductorChangeScopeNotice"));
	QVERIFY(summary);
	QVERIFY(notice);
	QVERIFY(summary->text().contains(QStringLiteral("2")));
	QVERIFY(!summary->accessibleName().isEmpty());
	QVERIFY(!notice->accessibleName().isEmpty());
}

void ConductorChangePreviewTest::exactScopePreviewNeverClaimsPotentialExpansion()
{
	ConductorChangePreviewData data = previewData(3);
	data.requestedCount = 3;
	data.consideredCount = 3;
	data.affectedCount = 2;
	data.unchangedCount = 1;
	data.potentialCount = 0;
	data.groupCount = 3;
	data.scope = ConductorChangePreviewData::Scope::ExactConductors;

	ConductorChangePreviewDialog dialog(data);
	auto notice = dialog.findChild<QLabel *>(
		QStringLiteral("conductorChangeScopeNotice"));
	QVERIFY(notice);
	QVERIFY(notice->text().contains(QStringLiteral("Portée exacte")));
	QVERIFY(notice->text().contains(QStringLiteral("aucune branche")));
	QVERIFY(!notice->text().contains(QStringLiteral("Portée étendue")));
	QVERIFY(!notice->accessibleName().isEmpty());

	bool exact_count_exposed = false;
	bool exact_description_exposed = false;
	for (QLabel *label : dialog.findChildren<QLabel *>())
	{
		if (label->accessibleName().contains(
				QStringLiteral("conducteurs concernés"))
			&& label->accessibleName().contains(QStringLiteral("3"))) {
			exact_count_exposed = true;
		}
		if (label->text().contains(
				QStringLiteral("explicitement sélectionnés"))) {
			exact_description_exposed = true;
		}
	}
	QVERIFY(exact_count_exposed);
	QVERIFY(exact_description_exposed);
	QVERIFY(dialog.applyButton()->isEnabled());
}

void ConductorChangePreviewTest::cancelLeavesDialogRejected()
{
	ConductorChangePreviewDialog dialog(previewData());
	QSignalSpy accepted(&dialog, &QDialog::accepted);
	QSignalSpy rejected(&dialog, &QDialog::rejected);
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	QPushButton *cancel = cancelButton(dialog);
	QVERIFY(cancel);
	QTest::mouseClick(cancel, Qt::LeftButton);
	QCOMPARE(accepted.count(), 0);
	QCOMPARE(rejected.count(), 1);
	QCOMPARE(dialog.result(), static_cast<int>(QDialog::Rejected));
}

void ConductorChangePreviewTest::escapeLeavesDialogRejected()
{
	ConductorChangePreviewDialog dialog(previewData());
	QSignalSpy accepted(&dialog, &QDialog::accepted);
	QSignalSpy rejected(&dialog, &QDialog::rejected);
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	QTest::keyClick(&dialog, Qt::Key_Escape);
	QCOMPARE(accepted.count(), 0);
	QCOMPARE(rejected.count(), 1);
}

void ConductorChangePreviewTest::applyIsEnabledOnlyForChanges()
{
	ConductorChangePreviewDialog no_op(previewData(2, false));
	QVERIFY(!no_op.applyButton()->isEnabled());

	ConductorChangePreviewDialog dialog(previewData());
	QSignalSpy accepted(&dialog, &QDialog::accepted);
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	QTest::mouseClick(dialog.applyButton(), Qt::LeftButton);
	QCOMPARE(accepted.count(), 1);
	QCOMPARE(dialog.result(), static_cast<int>(QDialog::Accepted));
}

void ConductorChangePreviewTest::enterInPreviewActivatesTargetWithoutApplying()
{
	ConductorChangePreviewDialog dialog(previewData());
	QSignalSpy accepted(&dialog, &QDialog::accepted);
	QSignalSpy activated(&dialog, &ConductorChangePreviewDialog::targetActivated);
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	dialog.previewTable()->setFocus();
	dialog.previewTable()->selectRow(0);
	QTest::keyClick(dialog.previewTable(), Qt::Key_Return);
	QCOMPARE(activated.count(), 1);
	QCOMPARE(accepted.count(), 0);
	QVERIFY(dialog.isVisible());
}

void ConductorChangePreviewTest::keyboardContract()
{
	ConductorChangePreviewDialog dialog(previewData());
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));
	dialog.previewTable()->setFocus();
	QVERIFY(dialog.previewTable()->hasFocus());
	QTest::keyClick(dialog.previewTable(), Qt::Key_Tab);
	QTRY_COMPARE(QApplication::focusWidget(), dialog.applyButton());
	dialog.previewTable()->setFocus();
	QTest::keyClick(dialog.previewTable(), Qt::Key_Tab, Qt::ShiftModifier);
	QTRY_COMPARE(QApplication::focusWidget(), cancelButton(dialog));
	QVERIFY(dialog.applyButton()->focusPolicy() != Qt::NoFocus);
	QVERIFY(cancelButton(dialog)->focusPolicy() != Qt::NoFocus);
}

void ConductorChangePreviewTest::fitsScreenAt150Percent()
{
	ApplicationFontGuard font_guard;
	QFont large_font = QApplication::font();
	large_font.setPointSize(qMax(large_font.pointSize() + 3, 12));
	QApplication::setFont(large_font);

	ConductorChangePreviewDialog dialog(previewData(30));
	QVERIFY(dialog.minimumSizeHint().width() <= 1280);
	QVERIFY(dialog.minimumSizeHint().height() <= 720);
	dialog.resize(800, 620);
	dialog.show();
	QVERIFY(QTest::qWaitForWindowExposed(&dialog));

	auto buttons = dialog.findChild<QDialogButtonBox *>(
		QStringLiteral("conductorChangeButtons"));
	QVERIFY(buttons);
	QVERIFY(dialog.contentsRect().contains(buttons->geometry().center()));
	QVERIFY(dialog.previewTable()->verticalScrollBar()->maximum() > 0);
	QVERIFY(dialog.applyButton()->isVisible());
	QVERIFY(cancelButton(dialog)->isVisible());

}

void ConductorChangePreviewTest::handlesThousandRowsDeterministically()
{
	ConductorChangePreviewData data = previewData(1000);
	data.consideredCount = 1000;
	data.affectedCount = 1000;
	data.unchangedCount = 0;
	for (ConductorChangePreviewRow &row : data.rows) {
		row.field = QStringLiteral("Section");
		row.before = QStringLiteral("1,5 mm²");
		row.after = QStringLiteral("2,5 mm²");
		row.state = QStringLiteral("À modifier");
		row.changes = true;
	}
	ConductorChangePreviewDialog dialog(data);
	QCOMPARE(dialog.previewTable()->rowCount(), 1000);
	QCOMPARE(dialog.previewTable()->item(999, 0)->text(),
		QStringLiteral("2 — Folio de test avec un titre long"));
	QCOMPARE(dialog.previewTable()->item(999, 5)->text(),
		QStringLiteral("À modifier"));
	QVERIFY(dialog.applyButton()->isEnabled());
	QVERIFY(dialog.minimumSizeHint().height() <= 720);
}

QTEST_MAIN(ConductorChangePreviewTest)
#include "tst_conductorchangepreview.moc"
