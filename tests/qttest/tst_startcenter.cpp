/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/recentfiles.h"
#include "../../sources/ui/startcenterpagecontroller.h"
#include "../../sources/ui/startcenterwidget.h"

#include <QAction>
#include <QApplication>
#include <QCommandLinkButton>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QSettings>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QtTest>

#include <algorithm>
#include <memory>

namespace {

QString nativePath(const QString &path)
{
	return QDir::toNativeSeparators(path);
}

QByteArray insufficientNativeDesktopReason(const QSize &required_size)
{
	if (QGuiApplication::platformName().compare(
			QStringLiteral("windows"), Qt::CaseInsensitive) != 0) {
		return {};
	}

	QScreen *screen = QGuiApplication::primaryScreen();
	if (!screen) {
		return QByteArrayLiteral(
			"Native Windows DPI check skipped: no primary screen is exposed.");
	}

	const QSize available_size = screen->availableGeometry().size();
	if (available_size.width() >= required_size.width()
		&& available_size.height() >= required_size.height()) {
		return {};
	}

	return QStringLiteral(
		"Native Windows DPI check requires %1x%2 logical pixels; "
		"the runner exposes %3x%4. The same layout contract remains covered "
		"by the offscreen 150% matrix.")
		.arg(required_size.width())
		.arg(required_size.height())
		.arg(available_size.width())
		.arg(available_size.height())
		.toUtf8();
}

QCommandLinkButton *newButton(StartCenterWidget &widget)
{
	return widget.findChild<QCommandLinkButton *>(
		QStringLiteral("startCenterNewProject"));
}

QCommandLinkButton *openButton(StartCenterWidget &widget)
{
	return widget.findChild<QCommandLinkButton *>(
		QStringLiteral("startCenterOpenProject"));
}

QTreeWidget *recentTree(StartCenterWidget &widget)
{
	return widget.findChild<QTreeWidget *>(
		QStringLiteral("startCenterRecentProjects"));
}

class ApplicationFontGuard
{
	public:
		explicit ApplicationFontGuard(double scale) :
		m_original_font(QApplication::font())
		{
			QFont scaled_font = m_original_font;
			scaled_font.setPointSizeF(scaled_font.pointSizeF() * scale);
			QApplication::setFont(scaled_font);
		}

		~ApplicationFontGuard()
		{
			QApplication::setFont(m_original_font);
		}

	private:
		QFont m_original_font;
};

}

class StartCenterTest : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void init();
		void projectActionsTriggerExactActionsOnce();
		void recentProjectsPreserveMruAndEmitExactPath();
		void recentFilesPersistClearAndForget();
		void recentMenuRefreshIsDeferredDuringTrigger();
		void emptyAndMissingRecentStatesAreSafe();
		void keyboardContract();
		void accessibilityContract();
		void fitsScreenAt150Percent();
		void projectHomeCyclesAreIdempotent();
};

void StartCenterTest::initTestCase()
{
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTechTests"));
	QCoreApplication::setApplicationName(QStringLiteral("StartCenterTest"));
}

void StartCenterTest::init()
{
	QSettings settings;
	settings.clear();
	settings.sync();
}

void StartCenterTest::projectActionsTriggerExactActionsOnce()
{
	QAction new_action(QStringLiteral("New"), this);
	QAction open_action(QStringLiteral("Open"), this);
	RecentFiles recent(QStringLiteral("ui03-actions"));
	StartCenterWidget widget(&new_action, &open_action, &recent);
	widget.resize(900, 600);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));

	QCommandLinkButton *new_button = newButton(widget);
	QCommandLinkButton *open_button = openButton(widget);
	QVERIFY(new_button);
	QVERIFY(open_button);
	QSignalSpy new_spy(&new_action, &QAction::triggered);
	QSignalSpy open_spy(&open_action, &QAction::triggered);
	QTest::mouseClick(new_button, Qt::LeftButton);
	QTest::mouseClick(open_button, Qt::LeftButton);
	QCOMPARE(new_spy.count(), 1);
	QCOMPARE(open_spy.count(), 1);

	open_action.setEnabled(false);
	QCOMPARE(open_button->isEnabled(), false);
	open_action.setEnabled(true);
	QCOMPARE(open_button->isEnabled(), true);
}

void StartCenterTest::recentProjectsPreserveMruAndEmitExactPath()
{
	QAction new_action(QStringLiteral("New"), this);
	QAction open_action(QStringLiteral("Open"), this);
	RecentFiles recent(QStringLiteral("ui03-mru"), 10);
	const QStringList paths {
		nativePath(QStringLiteral("C:/Projets/armoire A.qet")),
		nativePath(QStringLiteral("C:/Projets/énergie/ligne 2.qet")),
		nativePath(QStringLiteral("C:/Projets/pneumatique.qet")),
		nativePath(QStringLiteral("C:/Projets/hydraulique.qet")),
		nativePath(QStringLiteral("C:/Projets/process.qet")),
		nativePath(QStringLiteral("C:/Projets/automatisme.qet")),
		nativePath(QStringLiteral("C:/Projets/septième.qet"))
	};
	for (const QString &path : paths) recent.fileWasOpened(path);
	StartCenterWidget widget(&new_action, &open_action, &recent);
	widget.resize(1000, 650);
	widget.show();
	QVERIFY(QTest::qWaitForWindowExposed(&widget));

	QStringList expected = paths;
	std::reverse(expected.begin(), expected.end());
	expected = expected.mid(0, 6);
	QCOMPARE(widget.displayedRecentFiles(), expected);

	QTreeWidget *tree = recentTree(widget);
	QVERIFY(tree);
	QCOMPARE(tree->topLevelItemCount(), 6);
	QSignalSpy requested_spy(
		&widget,
		&StartCenterWidget::recentProjectRequested);
	tree->setCurrentItem(tree->topLevelItem(0));
	tree->setFocus();
	QTest::keyClick(tree, Qt::Key_Return);
	QTRY_COMPARE(requested_spy.count(), 1);
	QCOMPARE(requested_spy.takeFirst().at(0).toString(), expected.first());
	QCOMPARE(
		tree->topLevelItem(0)->data(0, Qt::AccessibleDescriptionRole).toString(),
		expected.first());
}

void StartCenterTest::recentFilesPersistClearAndForget()
{
	const QString identifier = QStringLiteral("ui03-persistence");
	const QString first = nativePath(QStringLiteral("C:/Projets/premier.qet"));
	const QString second = nativePath(QStringLiteral("C:/Projets/deuxième.qet"));
	{
		RecentFiles recent(identifier, 4);
		recent.fileWasOpened(first);
		recent.fileWasOpened(second);
		recent.fileWasOpened(first);
		QCOMPARE(recent.files(), QStringList({first, second}));
	}
	{
		RecentFiles loaded(identifier, 4);
		QCOMPARE(loaded.files(), QStringList({first, second}));
		QSignalSpy changed_spy(&loaded, &RecentFiles::filesChanged);
		loaded.forgetFile(first);
		QCOMPARE(changed_spy.count(), 1);
		QCOMPARE(loaded.files(), QStringList({second}));
	}
	{
		RecentFiles loaded(identifier, 4);
		QCOMPARE(loaded.files(), QStringList({second}));
		loaded.clear();
		QVERIFY(loaded.files().isEmpty());
	}
	RecentFiles empty(identifier, 4);
	QVERIFY(empty.files().isEmpty());
	QSettings settings;
	QVERIFY(!settings.contains(identifier + QStringLiteral("-recentfiles/file1")));
	QVERIFY(!settings.contains(identifier + QStringLiteral("-recentfiles/file2")));
}

void StartCenterTest::recentMenuRefreshIsDeferredDuringTrigger()
{
	RecentFiles recent(QStringLiteral("ui03-reentrant-menu"));
	const QString initial = nativePath(QStringLiteral("C:/Projets/initial.qet"));
	const QString replacement = nativePath(QStringLiteral("C:/Projets/nouveau.qet"));
	recent.fileWasOpened(initial);
	QVERIFY(recent.menu());
	QCOMPARE(recent.menu()->actions().count(), 1);
	QAction *triggered_action = recent.menu()->actions().first();
	QPointer<QAction> action_guard(triggered_action);
	QSignalSpy opening_spy(&recent, &RecentFiles::fileOpeningRequested);
	connect(
		&recent,
		&RecentFiles::fileOpeningRequested,
		&recent,
		[&recent, replacement](const QString &) {
			recent.fileWasOpened(replacement);
		});

	triggered_action->trigger();
	QVERIFY(!action_guard.isNull());
	QCOMPARE(opening_spy.count(), 1);
	QCOMPARE(opening_spy.first().at(0).toString(), initial);
	QCOMPARE(recent.files().first(), replacement);
	QCOMPARE(recent.menu()->actions().first()->text(), replacement);
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	QVERIFY(action_guard.isNull());
}

void StartCenterTest::emptyAndMissingRecentStatesAreSafe()
{
	QAction new_action(QStringLiteral("New"), this);
	QAction open_action(QStringLiteral("Open"), this);
	RecentFiles recent(QStringLiteral("ui03-empty"));
	StartCenterWidget widget(&new_action, &open_action, &recent);
	QTreeWidget *tree = recentTree(widget);
	QLabel *empty_label = widget.findChild<QLabel *>(
		QStringLiteral("startCenterEmptyRecent"));
	QVERIFY(tree);
	QVERIFY(empty_label);
	QVERIFY(tree->isHidden());
	QVERIFY(!empty_label->isHidden());

	const QString unavailable = nativePath(
		QStringLiteral("Z:/partage-indisponible/projet absent.qet"));
	recent.fileWasOpened(unavailable);
	QCOMPARE(widget.displayedRecentFiles(), QStringList({unavailable}));
	QVERIFY(!tree->isHidden());
	QSignalSpy requested_spy(
		&widget,
		&StartCenterWidget::recentProjectRequested);
	QMetaObject::invokeMethod(
		&widget,
		"activateRecentProject",
		Qt::DirectConnection,
		Q_ARG(QTreeWidgetItem *, tree->topLevelItem(0)),
		Q_ARG(int, 0));
	QCOMPARE(requested_spy.count(), 1);
	QCOMPARE(requested_spy.takeFirst().at(0).toString(), unavailable);
	QCOMPARE(recent.files(), QStringList({unavailable}));
}

void StartCenterTest::keyboardContract()
{
	QAction new_action(QStringLiteral("New"), this);
	QAction open_action(QStringLiteral("Open"), this);
	RecentFiles recent(QStringLiteral("ui03-keyboard"));
	recent.fileWasOpened(nativePath(QStringLiteral("C:/Projets/clavier.qet")));
	auto pages = std::make_unique<QStackedWidget>();
	auto start_center = new StartCenterWidget(
		&new_action,
		&open_action,
		&recent,
		QIcon(),
		pages.get());
	auto editor_page = new QWidget(pages.get());
	pages->addWidget(start_center);
	pages->addWidget(editor_page);
	StartCenterPageController controller(
		pages.get(), start_center, editor_page);
	pages->resize(1000, 650);
	pages->show();
	QVERIFY(QTest::qWaitForWindowExposed(pages.get()));
	pages->activateWindow();
	controller.setHasOpenProjects(false);
	QTRY_COMPARE(QApplication::focusWidget(), newButton(*start_center));

	QTest::keyClick(newButton(*start_center), Qt::Key_Tab);
	QTRY_COMPARE(QApplication::focusWidget(), openButton(*start_center));
	QTest::keyClick(openButton(*start_center), Qt::Key_Tab);
	QTRY_COMPARE(QApplication::focusWidget(), recentTree(*start_center));

	QSignalSpy requested_spy(
		start_center,
		&StartCenterWidget::recentProjectRequested);
	recentTree(*start_center)->setCurrentItem(
		recentTree(*start_center)->topLevelItem(0));
	QTest::keyClick(recentTree(*start_center), Qt::Key_Return);
	QTRY_COMPARE(requested_spy.count(), 1);
}

void StartCenterTest::accessibilityContract()
{
	QAction new_action(QStringLiteral("New"), this);
	QAction open_action(QStringLiteral("Open"), this);
	RecentFiles recent(QStringLiteral("ui03-accessibility"));
	recent.fileWasOpened(nativePath(QStringLiteral("C:/Projets/accessibilité.qet")));
	StartCenterWidget widget(&new_action, &open_action, &recent);
	QVERIFY(!widget.accessibleName().isEmpty());
	QVERIFY(!newButton(widget)->accessibleName().isEmpty());
	QVERIFY(!newButton(widget)->text().isEmpty());
	QVERIFY(!openButton(widget)->accessibleName().isEmpty());
	QVERIFY(!openButton(widget)->text().isEmpty());
	QTreeWidget *tree = recentTree(widget);
	QVERIFY(tree);
	QVERIFY(!tree->accessibleName().isEmpty());
	QVERIFY(!tree->accessibleDescription().isEmpty());
	QVERIFY(!tree->topLevelItem(0)
		->data(0, Qt::AccessibleDescriptionRole).toString().isEmpty());
}

void StartCenterTest::fitsScreenAt150Percent()
{
	const QSize target_size(1280, 680);
	const QByteArray skip_reason = insufficientNativeDesktopReason(target_size);
	if (!skip_reason.isEmpty()) QSKIP(skip_reason.constData());

	ApplicationFontGuard font_guard(1.5);
	QAction new_action(QStringLiteral("New"), this);
	QAction open_action(QStringLiteral("Open"), this);
	RecentFiles recent(QStringLiteral("ui03-scale"));
	for (int index = 0; index < 6; ++index) {
		recent.fileWasOpened(nativePath(QStringLiteral(
			"C:/Projets industriels/une arborescence très longue/projet %1.qet")
			.arg(index)));
	}
	StartCenterWidget widget(&new_action, &open_action, &recent);
	widget.resize(target_size);
	widget.show();
	// A headless Windows runner can keep a native window unexposed even though
	// Qt has made the widget visible and completed its layout. The assertions
	// below validate the actual reflow contract, so do not make them depend on
	// the CI desktop compositor acknowledging exposure.
	QTRY_VERIFY(widget.isVisible());
	QCoreApplication::processEvents();

	QScrollArea *scroll = widget.findChild<QScrollArea *>(
		QStringLiteral("startCenterScrollArea"));
	QVERIFY(scroll);
	QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);
	QCOMPARE(scroll->verticalScrollBar()->maximum(), 0);
	QVERIFY(newButton(widget)->isVisible());
	QVERIFY(openButton(widget)->isVisible());
	QVERIFY(recentTree(widget)->isVisible());
	const QRect viewport_rect = scroll->viewport()->rect();
	for (QWidget *control : QList<QWidget *>({
		newButton(widget),
		openButton(widget),
		recentTree(widget)})) {
		const QRect control_rect(
			control->mapTo(scroll->viewport(), QPoint(0, 0)),
			control->size());
		QVERIFY(viewport_rect.contains(control_rect));
	}
	QVERIFY(widget.minimumSizeHint().width() <= target_size.width());
	QVERIFY(widget.minimumSizeHint().height() <= target_size.height());
}

void StartCenterTest::projectHomeCyclesAreIdempotent()
{
	QAction new_action(QStringLiteral("New"), this);
	QAction open_action(QStringLiteral("Open"), this);
	RecentFiles recent(QStringLiteral("ui03-cycles"));
	QStackedWidget pages;
	auto start_center = new StartCenterWidget(
		&new_action,
		&open_action,
		&recent,
		QIcon(),
		&pages);
	auto editor_page = new QWidget(&pages);
	pages.addWidget(start_center);
	pages.addWidget(editor_page);
	StartCenterPageController controller(&pages, start_center, editor_page);

	for (int cycle = 0; cycle < 100; ++cycle) {
		controller.setHasOpenProjects(false);
		QVERIFY(controller.isStartCenterVisible());
		QCOMPARE(pages.currentWidget(), static_cast<QWidget *>(start_center));
		controller.setHasOpenProjects(true);
		QVERIFY(!controller.isStartCenterVisible());
		QCOMPARE(pages.currentWidget(), editor_page);
	}
	controller.setHasOpenProjects(false);
	QVERIFY(controller.isStartCenterVisible());
	QCOMPARE(pages.count(), 2);
}

QTEST_MAIN(StartCenterTest)
#include "tst_startcenter.moc"
