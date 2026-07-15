/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "../../sources/TerminalStrip/ui/terminalstripoverviewmodel.h"
#include "../../sources/TerminalStrip/ui/terminalstripoverviewwidget.h"

#include <QAbstractItemModelTester>
#include <QComboBox>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QPixmap>
#include <QSignalSpy>
#include <QTableView>
#include <QtTest>

#include <algorithm>

namespace {

TerminalStripOverviewRow row(
		const char *element_uuid,
		const char *strip_uuid,
		const QString &assignment,
		const QString &installation,
		const QString &location,
		int position,
		int level,
		const QString &label,
		const QString &conductor,
		const QString &cable = QString(),
		const QString &wire = QString())
{
	TerminalStripOverviewRow result;
	result.element_uuid = QUuid(QString::fromLatin1(element_uuid));
	result.strip_uuid = QUuid(QString::fromLatin1(strip_uuid));
	result.assignment = assignment;
	result.installation = installation;
	result.location = location;
	result.position = position;
	result.level = level;
	result.label = label;
	result.conductor = conductor;
	result.xref = QStringLiteral("F1-%1").arg(label);
	result.cable = cable;
	result.cable_wire = wire;
	result.type = QStringLiteral("Générique");
	result.function = QStringLiteral("Phase");
	result.assigned = !result.strip_uuid.isNull();
	result.navigation_available = !result.element_uuid.isNull();
	return result;
}

QVector<TerminalStripOverviewRow> fixtureRows()
{
	return {
		row("{00000000-0000-0000-0000-000000000001}",
			"{10000000-0000-0000-0000-000000000002}",
			QStringLiteral("X2"), QStringLiteral("Énergie"),
			QStringLiteral("Armoire A"), 1, 0, QStringLiteral("X2.1"),
			QStringLiteral("2 · 10"), QStringLiteral("C2"), QStringLiteral("1")),
		row("{00000000-0000-0000-0000-000000000002}",
			"{10000000-0000-0000-0000-000000000002}",
			QStringLiteral("X2"), QStringLiteral("Énergie"),
			QStringLiteral("Armoire A"), 1, 1, QStringLiteral("X2.2"),
			QStringLiteral("11"), QStringLiteral("C2"), QStringLiteral("2")),
		row("{00000000-0000-0000-0000-000000000003}",
			"{10000000-0000-0000-0000-000000000010}",
			QStringLiteral("X10"), QStringLiteral("Énergie"),
			QStringLiteral("Armoire B"), 9, 0, QStringLiteral("X10.1"),
			QStringLiteral("20"), QStringLiteral("C10"), QStringLiteral("1")),
		row("{00000000-0000-0000-0000-000000000004}",
			"{10000000-0000-0000-0000-000000000010}",
			QStringLiteral("X10"), QStringLiteral("Énergie"),
			QStringLiteral("Armoire B"), 10, 0, QStringLiteral("PE"),
			QStringLiteral("PE"), QStringLiteral("C10"), QString()),
		row("{00000000-0000-0000-0000-000000000005}",
			"{00000000-0000-0000-0000-000000000000}",
			QStringLiteral("Bornes indépendantes"), QString(), QString(),
			-1, -1, QStringLiteral("ÉTÉ-1"), QStringLiteral("30")),
		row("{00000000-0000-0000-0000-000000000006}",
			"{00000000-0000-0000-0000-000000000000}",
			QStringLiteral("Bornes indépendantes"), QString(), QString(),
			-1, -1, QStringLiteral("X2.1"), QStringLiteral("31"))
	};
}

}

class TerminalStripOverviewTest : public QObject
{
	Q_OBJECT

	private slots:
		void modelColumnsRolesAndFlagsAreReadOnly();
		void defaultOrderIsNaturalAndStable();
		void searchNormalizesAccentsAndCombinesTokens();
		void filtersComposeAndExposeCableIssues();
		void widgetKeepsSelectionAndActivatesOnce();
		void emptyAndNoMatchStatesAreDistinct();
		void keyboardContract();
		void readOnlyInteractionsDoNotMutateRows();
		void fitsLogicalDesktopAt150Percent();
		void rendersEvidence();
};

void TerminalStripOverviewTest::modelColumnsRolesAndFlagsAreReadOnly()
{
	TerminalStripOverviewModel model;
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
	model.setRows(fixtureRows());
	QCOMPARE(model.rowCount(), 6);
	QCOMPARE(model.columnCount(), int(TerminalStripOverviewModel::ColumnCount));
	QCOMPARE(model.headerData(TerminalStripOverviewModel::Cable,
			Qt::Horizontal).toString(), QStringLiteral("Câble"));
	for (int row_index = 0; row_index < model.rowCount(); ++row_index)
	{
		for (int column = 0; column < model.columnCount(); ++column)
		{
			const QModelIndex index = model.index(row_index, column);
			QVERIFY(index.flags().testFlag(Qt::ItemIsEnabled));
			QVERIFY(index.flags().testFlag(Qt::ItemIsSelectable));
			QVERIFY(!index.flags().testFlag(Qt::ItemIsEditable));
			QVERIFY(!model.setData(index, QStringLiteral("mutation")));
		}
	}
	QVERIFY(!model.index(0, 0).data(
			TerminalStripOverviewModel::ElementUuidRole).toUuid().isNull());
}

void TerminalStripOverviewTest::defaultOrderIsNaturalAndStable()
{
	QVector<TerminalStripOverviewRow> rows = fixtureRows();
	std::reverse(rows.begin(), rows.end());
	TerminalStripOverviewModel model;
	model.setRows(rows);
	QCOMPARE(model.rowAt(0).assignment, QStringLiteral("X2"));
	QCOMPARE(model.rowAt(0).level, 0);
	QCOMPARE(model.rowAt(1).assignment, QStringLiteral("X2"));
	QCOMPARE(model.rowAt(1).level, 1);
	QCOMPARE(model.rowAt(2).assignment, QStringLiteral("X10"));
	QVERIFY(model.rowAt(4).assigned == false);
	QVERIFY(TerminalStripOverviewModel::naturalLessThan(
			QStringLiteral("X2"), QStringLiteral("X10")));
}

void TerminalStripOverviewTest::searchNormalizesAccentsAndCombinesTokens()
{
	TerminalStripOverviewModel model;
	TerminalStripOverviewFilterProxyModel proxy;
	model.setRows(fixtureRows());
	proxy.setSourceModel(&model);
	proxy.setSearchText(QStringLiteral("energie"));
	QCOMPARE(proxy.rowCount(), 4);
	proxy.setSearchText(QStringLiteral("energie armoire b c10"));
	QCOMPARE(proxy.rowCount(), 2);
	proxy.setSearchText(QStringLiteral("independantes ete-1 30"));
	QCOMPARE(proxy.rowCount(), 1);
	QCOMPARE(proxy.index(0, TerminalStripOverviewModel::Label)
			.data().toString(), QStringLiteral("ÉTÉ-1"));
}

void TerminalStripOverviewTest::filtersComposeAndExposeCableIssues()
{
	TerminalStripOverviewModel model;
	TerminalStripOverviewFilterProxyModel proxy;
	model.setRows(fixtureRows());
	proxy.setSourceModel(&model);
	proxy.setAssignmentScope(
			TerminalStripOverviewFilterProxyModel::AssignmentScope::Assigned);
	QCOMPARE(proxy.rowCount(), 4);
	proxy.setAttentionScope(
			TerminalStripOverviewFilterProxyModel::AttentionScope::RequiresAttention);
	QCOMPARE(proxy.rowCount(), 1);
	QCOMPARE(proxy.index(0, TerminalStripOverviewModel::Label)
			.data().toString(), QStringLiteral("PE"));
	proxy.setAssignmentScope(
			TerminalStripOverviewFilterProxyModel::AssignmentScope::Independent);
	QCOMPARE(proxy.rowCount(), 0);
	proxy.setAttentionScope(
			TerminalStripOverviewFilterProxyModel::AttentionScope::All);
	QCOMPARE(proxy.rowCount(), 2);
}

void TerminalStripOverviewTest::widgetKeepsSelectionAndActivatesOnce()
{
	TerminalStripOverviewWidget widget;
	widget.setRows(fixtureRows());
	QCOMPARE(widget.proxyModel()->index(
			0, TerminalStripOverviewModel::Assignment).data().toString(),
			QStringLiteral("X2"));
	widget.resize(1100, 650);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));
	QSignalSpy activated(&widget,
			&TerminalStripOverviewWidget::showElementRequested);
	QTableView *table = widget.tableView();
	QVERIFY(table);
	const QModelIndex second = widget.proxyModel()->index(1, 0);
	table->setCurrentIndex(second);
	const QUuid selected = widget.selectedElementUuid();
	widget.proxyModel()->sort(TerminalStripOverviewModel::Label, Qt::DescendingOrder);
	QTRY_COMPARE(widget.selectedElementUuid(), selected);
	QTest::keyClick(table, Qt::Key_Return);
	QCOMPARE(activated.count(), 1);
	QCOMPARE(activated.at(0).at(0).toUuid(), selected);
}

void TerminalStripOverviewTest::emptyAndNoMatchStatesAreDistinct()
{
	TerminalStripOverviewWidget widget;
	widget.setRows({});
	auto *message = widget.findChild<QLabel *>(
			QStringLiteral("terminalStripOverviewMessageTitle"));
	QVERIFY(message);
	QCOMPARE(message->text(),
			QStringLiteral("Aucune borne n’a été trouvée dans ce projet."));
	widget.setRows(fixtureRows());
	auto *search = widget.findChild<QLineEdit *>(
			QStringLiteral("terminalStripOverviewSearch"));
	search->setText(QStringLiteral("absent-xyz"));
	QTRY_COMPARE(widget.proxyModel()->rowCount(), 0);
	QVERIFY(message->text().contains(QStringLiteral("Aucune borne ne correspond")));
	QVERIFY(widget.findChild<QPushButton *>(
			QStringLiteral("terminalStripOverviewClearFilters"))->isEnabled());
}

void TerminalStripOverviewTest::keyboardContract()
{
	TerminalStripOverviewWidget widget;
	widget.setRows(fixtureRows());
	widget.resize(1000, 620);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));
	QSignalSpy reload(&widget, &TerminalStripOverviewWidget::reloadRequested);
	QSignalSpy activated(&widget,
			&TerminalStripOverviewWidget::showElementRequested);
	QTest::keyClick(&widget, Qt::Key_F, Qt::ControlModifier);
	auto *search = widget.findChild<QLineEdit *>(
			QStringLiteral("terminalStripOverviewSearch"));
	QVERIFY(search->hasFocus());
	search->setText(QStringLiteral("x10"));
	QTRY_COMPARE(widget.proxyModel()->rowCount(), 2);
	QTest::keyClick(search, Qt::Key_Down);
	QVERIFY(widget.tableView()->hasFocus());
	QTest::keyClick(widget.tableView(), Qt::Key_Return);
	QCOMPARE(activated.count(), 1);
	QTest::keyClick(widget.tableView(), Qt::Key_F5);
	QCOMPARE(reload.count(), 1);
	QTest::keyClick(widget.tableView(), Qt::Key_Escape);
	QTRY_COMPARE(widget.proxyModel()->rowCount(), 6);
}

void TerminalStripOverviewTest::readOnlyInteractionsDoNotMutateRows()
{
	TerminalStripOverviewWidget widget;
	const QVector<TerminalStripOverviewRow> original = fixtureRows();
	widget.setRows(original);
	widget.setReadOnly(true);
	QSignalSpy changed(widget.model(), &QAbstractItemModel::dataChanged);
	QSignalSpy inserted(widget.model(), &QAbstractItemModel::rowsInserted);
	QSignalSpy removed(widget.model(), &QAbstractItemModel::rowsRemoved);
	widget.proxyModel()->setSearchText(QStringLiteral("energie"));
	widget.proxyModel()->setAssignmentScope(
			TerminalStripOverviewFilterProxyModel::AssignmentScope::Assigned);
	widget.proxyModel()->sort(TerminalStripOverviewModel::Cable);
	QCOMPARE(changed.count(), 0);
	QCOMPARE(inserted.count(), 0);
	QCOMPARE(removed.count(), 0);
	QCOMPARE(widget.model()->rows(), original);
	QVERIFY(widget.findChild<QLabel *>(
			QStringLiteral("terminalStripReadOnlyBanner"))->isVisibleTo(&widget));
}

void TerminalStripOverviewTest::fitsLogicalDesktopAt150Percent()
{
	const QFont original = QGuiApplication::font();
	struct Restorer { QFont font; ~Restorer() { QGuiApplication::setFont(font); } }
		restorer {original};
	QFont large = original;
	large.setPointSizeF((large.pointSizeF() > 0 ? large.pointSizeF() : 9.0) * 1.5);
	QGuiApplication::setFont(large);
	TerminalStripOverviewWidget widget;
	widget.setRows(fixtureRows());
	widget.resize(1280, 680);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));
	QVERIFY(widget.minimumSizeHint().width() <= 1280);
	QVERIFY(widget.minimumSizeHint().height() <= 680);
	for (const QString &name : {
			QStringLiteral("terminalStripOverviewSearch"),
			QStringLiteral("terminalStripOverviewAssignmentFilter"),
			QStringLiteral("terminalStripOverviewAttentionFilter"),
			QStringLiteral("terminalStripOverviewShowInFolio")})
	{
		QWidget *control = widget.findChild<QWidget *>(name);
		QVERIFY2(control && control->isVisibleTo(&widget), qPrintable(name));
		QVERIFY(!control->accessibleName().isEmpty());
	}
}

void TerminalStripOverviewTest::rendersEvidence()
{
	const QString output = QString::fromLocal8Bit(
			qgetenv("QET_TERMINAL_OVERVIEW_SCREENSHOT"));
	if (output.isEmpty()) QSKIP("No screenshot output requested");
	TerminalStripOverviewWidget widget;
	widget.setRows(fixtureRows());
	widget.resize(1280, 760);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));
	QVERIFY2(QFileInfo(output).absoluteDir().exists(), qPrintable(output));
	QCoreApplication::processEvents();
	QPixmap snapshot(widget.size());
	snapshot.fill(widget.palette().color(QPalette::Window));
	widget.render(&snapshot);
	QVERIFY(snapshot.save(output, "PNG"));
	QVERIFY(QFileInfo(output).size() > 5000);
}

QTEST_MAIN(TerminalStripOverviewTest)
#include "tst_terminalstripoverview.moc"
