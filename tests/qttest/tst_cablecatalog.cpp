/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "../../sources/cablecatalog/cablecatalogbuilder.h"
#include "../../sources/cablecatalog/cablecatalogcsvexporter.h"
#include "../../sources/TerminalStrip/ui/cablecatalogmodel.h"
#include "../../sources/TerminalStrip/ui/cablecatalogwidget.h"

#include <QAbstractItemModelTester>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QSignalSpy>
#include <QTreeView>
#include <QtTest>

namespace {

QUuid uuidFor(int value)
{
	return QUuid(QStringLiteral("{%1-0000-0000-0000-000000000001}")
			.arg(value, 8, 16, QLatin1Char('0')));
}

CableConductorSnapshot conductor(
		const QString &key,
		const QString &potential,
		const QString &cable,
		const QString &wire_color,
		const QString &folio,
		int identity)
{
	CableConductorSnapshot result;
	result.stable_key = key;
	result.potential_key = potential;
	result.cable = cable;
	result.wire_color = wire_color;
	result.conductor = QStringLiteral("W%1").arg(identity);
	result.section = QStringLiteral("1,5");
	result.function = QStringLiteral("Commande");
	result.tension_protocol = QStringLiteral("24 VDC");
	result.from.diagram_uuid = uuidFor(100);
	result.from.element_uuid = uuidFor(identity * 2 + 1);
	result.from.terminal_uuid = uuidFor(identity * 2 + 2);
	result.from.folio = folio;
	result.from.element_label = identity == 1
			? QStringLiteral("ÉTÉ-1") : QStringLiteral("X%1").arg(identity);
	result.from.terminal_name = QStringLiteral("1");
	result.from.family_key = result.from.element_uuid.toString();
	result.to = result.from;
	result.to.element_uuid = uuidFor(identity * 2 + 101);
	result.to.terminal_uuid = uuidFor(identity * 2 + 102);
	result.to.element_label = QStringLiteral("K%1").arg(identity);
	result.navigation_target.token = quint64(identity + 1);
	result.navigation_target.project_uuid = uuidFor(500);
	result.navigation_target.diagram_uuid = result.from.diagram_uuid;
	result.navigation_target.element_1_uuid = result.from.element_uuid;
	result.navigation_target.element_2_uuid = result.to.element_uuid;
	result.navigation_target.terminal_1_uuid = result.from.terminal_uuid;
	result.navigation_target.terminal_2_uuid = result.to.terminal_uuid;
	return result;
}

QVector<CableConductorSnapshot> fixtureSources()
{
	return {
		conductor("c2-1", "p1", "C2", "1", "F1", 1),
		conductor("c2-2", "p2", "C2", "2", "F1", 2),
		conductor("c2-10", "p3", "C2", "10", "F2", 3),
		conductor("c10-1", "p4", "C10", "", "F2", 4),
		conductor("c20-1", "p5", "C20", "BK", "F3", 5),
		conductor("c20-2", "p6", "C20", "BK", "F3", 6),
		conductor("c30-1", "p7", "C30", "BK", "F4", 7),
		conductor("c30-2", "p7", "C30", "BK", "F4", 8),
		conductor("c30-3", "p7", "C30", "BK", "F4", 9),
		conductor("c1-1", "p8", "C1", "1", "F5", 10),
		conductor("c1-lower", "p9", "c1", "2", "F5", 11),
		conductor("wire-pe", "p10", "", "PE", "F6", 12),
		conductor("wire-empty", "p11", "", "", "F6", 13)
	};
}

const CableCatalogEntry *entry(const CableCatalogSnapshot &snapshot,
							   const QString &reference)
{
	for (const CableCatalogEntry &candidate : snapshot.entries)
	{
		if (candidate.assigned && candidate.reference == reference) return &candidate;
	}
	return nullptr;
}

}

class CableCatalogTest : public QObject
{
	Q_OBJECT

	private slots:
		void builderIsDeterministicAndConservative();
		void modelContractIsHierarchicalAndReadOnly();
		void filtersComposeAndSearchIgnoresAccents();
		void widgetNavigationAndKeyboardContract();
		void emptyNoMatchAndUnavailableStatesAreDistinct();
		void csvExportIsUtf8AndStable();
		void largeFixtureStaysDeterministic();
		void fitsLogicalDesktopAt150Percent();
		void rendersEvidence();
};

void CableCatalogTest::builderIsDeterministicAndConservative()
{
	QVector<CableConductorSnapshot> sources = fixtureSources();
	const CableCatalogSnapshot snapshot = CableCatalogBuilder::build(sources);
	QCOMPARE(snapshot.conductor_count, 13);
	QCOMPARE(snapshot.assigned_conductor_count, 11);
	QCOMPARE(snapshot.unassigned_conductor_count, 2);
	QCOMPARE(snapshot.entries.size(), 7); // six cables + hidden unassigned bucket
	QVERIFY(entry(snapshot, "C2"));
	QVERIFY(entry(snapshot, "C10"));
	QVERIFY(entry(snapshot, "C20"));
	QVERIFY(entry(snapshot, "C30"));
	QVERIFY(entry(snapshot, "C1"));
	QVERIFY(entry(snapshot, "c1"));
	QVERIFY(!entry(snapshot, "C2")->requiresAttention());
	QVERIFY(entry(snapshot, "C10")->hasDiagnostic(
			CableDiagnostic::AssignedCableWithoutCoreOrColor));
	QVERIFY(entry(snapshot, "C20")->hasDiagnostic(
			CableDiagnostic::RepeatedCoreOrColor));
	QVERIFY(!entry(snapshot, "C30")->hasDiagnostic(
			CableDiagnostic::RepeatedCoreOrColor));
	QVERIFY(entry(snapshot, "C1")->hasDiagnostic(CableDiagnostic::NearReference));
	QVERIFY(entry(snapshot, "c1")->hasDiagnostic(CableDiagnostic::NearReference));

	std::reverse(sources.begin(), sources.end());
	const CableCatalogSnapshot reversed = CableCatalogBuilder::build(sources);
	QCOMPARE(reversed.entries.size(), snapshot.entries.size());
	for (int i = 0; i < snapshot.entries.size(); ++i)
	{
		QCOMPARE(reversed.entries.at(i).reference, snapshot.entries.at(i).reference);
		QCOMPARE(reversed.entries.at(i).members.size(), snapshot.entries.at(i).members.size());
	}
	QVERIFY(CableCatalogBuilder::naturalLessThan("C2", "C10"));
}

void CableCatalogTest::modelContractIsHierarchicalAndReadOnly()
{
	CableCatalogModel model;
	QAbstractItemModelTester tester(
			&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
	model.setSnapshot(CableCatalogBuilder::build(fixtureSources()));
	QCOMPARE(model.rowCount(), 7);
	QCOMPARE(model.columnCount(), int(CableCatalogModel::ColumnCount));
	QModelIndex c2;
	for (int row = 0; row < model.rowCount(); ++row)
	{
		const QModelIndex candidate = model.index(row, CableCatalogModel::Cable);
		if (candidate.data().toString() == QStringLiteral("C2"))
		{
			c2 = model.index(row, 0);
			break;
		}
	}
	QVERIFY(c2.isValid());
	QCOMPARE(model.rowCount(c2), 3);
	const QModelIndex child = model.index(0, CableCatalogModel::Cable, c2);
	QVERIFY(child.isValid());
	QCOMPARE(model.parent(child), c2);
	for (const QModelIndex &index : {c2, child})
	{
		QVERIFY(index.flags().testFlag(Qt::ItemIsEnabled));
		QVERIFY(index.flags().testFlag(Qt::ItemIsSelectable));
		QVERIFY(!index.flags().testFlag(Qt::ItemIsEditable));
		QVERIFY(!model.setData(index, QStringLiteral("mutation")));
	}
}

void CableCatalogTest::filtersComposeAndSearchIgnoresAccents()
{
	CableCatalogModel model;
	CableCatalogFilterProxyModel proxy;
	model.setSnapshot(CableCatalogBuilder::build(fixtureSources()));
	proxy.setSourceModel(&model);
	QCOMPARE(proxy.rowCount(), 6);
	proxy.setHealthScope(CableCatalogFilterProxyModel::HealthScope::Attention);
	QCOMPARE(proxy.rowCount(), 4);
	proxy.setDiagnosticScope(
			CableCatalogFilterProxyModel::DiagnosticScope::MissingCoreOrColor);
	QCOMPARE(proxy.rowCount(), 1);
	QCOMPARE(proxy.index(0, CableCatalogModel::Cable).data().toString(),
			QStringLiteral("C10"));
	proxy.setHealthScope(CableCatalogFilterProxyModel::HealthScope::All);
	proxy.setDiagnosticScope(CableCatalogFilterProxyModel::DiagnosticScope::All);
	proxy.setSearchText(QStringLiteral("ete-1 c2"));
	QCOMPARE(proxy.rowCount(), 1);
	QCOMPARE(proxy.index(0, CableCatalogModel::Cable).data().toString(),
			QStringLiteral("C2"));
	proxy.setSearchText(QString());
	proxy.setIncludeUnassigned(true);
	QCOMPARE(proxy.rowCount(), 7);
}

void CableCatalogTest::widgetNavigationAndKeyboardContract()
{
	CableCatalogWidget widget;
	widget.setSnapshot(CableCatalogBuilder::build(fixtureSources()));
	widget.resize(1100, 680);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));
	QSignalSpy activated(&widget, &CableCatalogWidget::showConductorRequested);
	QSignalSpy reloaded(&widget, &CableCatalogWidget::reloadRequested);
	QTest::keyClick(&widget, Qt::Key_F, Qt::ControlModifier);
	auto *search = widget.findChild<QLineEdit *>(QStringLiteral("cableCatalogSearch"));
	QVERIFY(search && search->hasFocus());
	search->setText(QStringLiteral("c20"));
	QTRY_COMPARE(widget.proxyModel()->rowCount(), 1);
	QTest::keyClick(search, Qt::Key_Down);
	QVERIFY(widget.treeView()->hasFocus());
	QTest::keyClick(widget.treeView(), Qt::Key_Return);
	QCOMPARE(activated.count(), 1);
	QVERIFY(activated.at(0).at(0).value<CableNavigationTarget>().isValid());
	QTest::keyClick(widget.treeView(), Qt::Key_F5);
	QCOMPARE(reloaded.count(), 1);
	QTest::keyClick(widget.treeView(), Qt::Key_Escape);
	QTRY_COMPARE(widget.proxyModel()->rowCount(), 6);
}

void CableCatalogTest::emptyNoMatchAndUnavailableStatesAreDistinct()
{
	CableCatalogWidget widget;
	widget.setSnapshot({});
	auto *title = widget.findChild<QLabel *>(QStringLiteral("cableCatalogMessageTitle"));
	QVERIFY(title);
	QCOMPARE(title->text(), QStringLiteral("Aucun câble n’a été trouvé dans ce projet."));
	widget.setSnapshot(CableCatalogBuilder::build(fixtureSources()));
	auto *search = widget.findChild<QLineEdit *>(QStringLiteral("cableCatalogSearch"));
	search->setText(QStringLiteral("introuvable-xyz"));
	QTRY_COMPARE(widget.proxyModel()->rowCount(), 0);
	QVERIFY(title->text().contains(QStringLiteral("Aucun câble ne correspond")));
	widget.setProjectAvailable(false);
	QCOMPARE(title->text(), QStringLiteral("Le projet n’est plus disponible."));
}

void CableCatalogTest::csvExportIsUtf8AndStable()
{
	const CableCatalogSnapshot snapshot = CableCatalogBuilder::build(fixtureSources());
	const QString csv = CableCatalogCsvExporter::toCsv(snapshot);
	QVERIFY(csv.startsWith(QStringLiteral("État;Codes diagnostic;Câble")));
	QVERIFY(csv.contains(QStringLiteral("missing-core-or-color")));
	QVERIFY(csv.contains(QStringLiteral("C20;BK")));
	QVERIFY(!csv.contains(QStringLiteral("Conducteurs sans référence de câble")));
}

void CableCatalogTest::largeFixtureStaysDeterministic()
{
	QVector<CableConductorSnapshot> sources;
	sources.reserve(10000);
	for (int cable = 0; cable < 1000; ++cable)
	{
		for (int member = 0; member < 10; ++member)
		{
			const int id = cable * 10 + member + 100;
			sources.append(conductor(
					QStringLiteral("large-%1").arg(id),
					QStringLiteral("p-%1-%2").arg(cable).arg(member),
					QStringLiteral("C%1").arg(cable),
					QString::number(member),
					QStringLiteral("F%1").arg(cable % 20), id));
		}
	}
	const CableCatalogSnapshot first = CableCatalogBuilder::build(sources);
	QCOMPARE(first.entries.size(), 1000);
	std::rotate(sources.begin(), sources.begin() + 317, sources.end());
	const CableCatalogSnapshot second = CableCatalogBuilder::build(sources);
	QCOMPARE(second.entries.size(), first.entries.size());
	for (int i = 0; i < first.entries.size(); ++i)
		QCOMPARE(second.entries.at(i).reference, first.entries.at(i).reference);
}

void CableCatalogTest::fitsLogicalDesktopAt150Percent()
{
	const QFont original = QGuiApplication::font();
	struct Restorer { QFont font; ~Restorer() { QGuiApplication::setFont(font); } }
		restorer {original};
	QFont large = original;
	large.setPointSizeF((large.pointSizeF() > 0 ? large.pointSizeF() : 9.0) * 1.5);
	QGuiApplication::setFont(large);
	CableCatalogWidget widget;
	widget.setSnapshot(CableCatalogBuilder::build(fixtureSources()));
	widget.resize(1280, 680);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));
	QVERIFY(widget.minimumSizeHint().width() <= 1280);
	QVERIFY(widget.minimumSizeHint().height() <= 680);
	for (const QString &name : {
		QStringLiteral("cableCatalogSearch"),
		QStringLiteral("cableCatalogHealthFilter"),
		QStringLiteral("cableCatalogDiagnosticFilter"),
		QStringLiteral("cableCatalogTree"),
		QStringLiteral("cableCatalogShowInFolio")})
	{
		QWidget *control = widget.findChild<QWidget *>(name);
		QVERIFY2(control && control->isVisibleTo(&widget), qPrintable(name));
		QVERIFY(!control->accessibleName().isEmpty());
	}
}

void CableCatalogTest::rendersEvidence()
{
	const QString output = QString::fromLocal8Bit(
			qgetenv("QET_CABLE_CATALOG_SCREENSHOT"));
	if (output.isEmpty()) QSKIP("No screenshot output requested");
	CableCatalogWidget widget;
	widget.setSnapshot(CableCatalogBuilder::build(fixtureSources()));
	widget.resize(1280, 760);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));
	if (widget.proxyModel()->rowCount() > 0)
		widget.treeView()->expand(widget.proxyModel()->index(0, 0));
	QPixmap snapshot(widget.size());
	snapshot.fill(widget.palette().color(QPalette::Window));
	widget.render(&snapshot);
	QVERIFY(snapshot.save(output, "PNG"));
	QVERIFY(QFileInfo(output).size() > 5000);
}

QTEST_MAIN(CableCatalogTest)
#include "tst_cablecatalog.moc"
