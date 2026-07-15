/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/

#include "../../sources/configdialog.h"
#include "../../sources/ui/configpage/configpage.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QtTest>

class OversizedConfigPage : public ConfigPage
{
public:
	explicit OversizedConfigPage(const QString &page_title) :
		ConfigPage(nullptr),
		m_title(page_title)
	{
		setMinimumSize(860, 960);
		QVBoxLayout *layout = new QVBoxLayout(this);
		m_first_control = new QLineEdit(this);
		m_first_control->setObjectName(QStringLiteral("firstControl"));
		m_last_control = new QLineEdit(this);
		m_last_control->setObjectName(QStringLiteral("lastControl"));
		layout->addWidget(m_first_control);
		layout->addStretch(1);
		layout->addWidget(m_last_control);
		QWidget::setTabOrder(m_first_control, m_last_control);
	}

	void applyConf() override {}
	QString title() const override { return m_title; }
	QIcon icon() const override { return QIcon(); }

	QLineEdit *firstControl() const { return m_first_control; }
	QLineEdit *lastControl() const { return m_last_control; }

private:
	QString m_title;
	QLineEdit *m_first_control = nullptr;
	QLineEdit *m_last_control = nullptr;
};

class ConfigDialogUxTest : public QObject
{
	Q_OBJECT

private slots:
	void wrapsEveryPageInAnIndependentScrollArea();
	void keepsNavigationHeaderAndActionsFixed();
	void synchronizesTheHeaderOnFirstDisplayAndPageChanges();
	void boundsDialogToLogicalAvailableGeometry_data();
	void boundsDialogToLogicalAvailableGeometry();
	void restoresDesiredSizeAcrossLogicalScreens();
	void refittingDoesNotRecenterTheDialog();
	void keyboardNavigationAndFocusRemainUsable();
	void standardDialogKeysAcceptAndReject();
};

void ConfigDialogUxTest::wrapsEveryPageInAnIndependentScrollArea()
{
	ConfigDialog dialog;
	auto *first_page = new OversizedConfigPage(QStringLiteral("First"));
	auto *second_page = new OversizedConfigPage(QStringLiteral("Second"));
	dialog.addPage(first_page);
	dialog.addPage(second_page);

	const QList<QScrollArea *> scroll_areas =
		dialog.findChildren<QScrollArea *>(QStringLiteral("configPageScrollArea"));
	QCOMPARE(scroll_areas.size(), 2);
	QVERIFY(scroll_areas.contains(qobject_cast<QScrollArea *>(first_page->parentWidget()->parentWidget())));
	QVERIFY(scroll_areas.contains(qobject_cast<QScrollArea *>(second_page->parentWidget()->parentWidget())));

	for (QScrollArea *scroll_area : scroll_areas) {
		QVERIFY(scroll_area->widgetResizable());
		QCOMPARE(scroll_area->frameShape(), QFrame::NoFrame);
	}
}

void ConfigDialogUxTest::keepsNavigationHeaderAndActionsFixed()
{
	ConfigDialog dialog;
	dialog.addPage(new OversizedConfigPage(QStringLiteral("Page")));

	auto *navigation =
		dialog.findChild<QListWidget *>(QStringLiteral("configPagesList"));
	auto *buttons =
		dialog.findChild<QDialogButtonBox *>(QStringLiteral("configDialogButtonBox"));
	auto *header =
		dialog.findChild<QLabel *>(QStringLiteral("configPageTitle"));
	const QList<QScrollArea *> scroll_areas =
		dialog.findChildren<QScrollArea *>(QStringLiteral("configPageScrollArea"));

	QVERIFY(navigation);
	QVERIFY(buttons);
	QVERIFY(header);
	QCOMPARE(header->text(), QStringLiteral("Page"));
	QCOMPARE(scroll_areas.size(), 1);
	QVERIFY(!scroll_areas.first()->isAncestorOf(navigation));
	QVERIFY(!scroll_areas.first()->isAncestorOf(header));
	QVERIFY(!scroll_areas.first()->isAncestorOf(buttons));
	QCOMPARE(navigation->parentWidget(), &dialog);
	QCOMPARE(header->parentWidget(), &dialog);
	QCOMPARE(buttons->parentWidget(), &dialog);
}

void ConfigDialogUxTest::synchronizesTheHeaderOnFirstDisplayAndPageChanges()
{
	QWidget parent;
	ConfigDialog dialog;
	dialog.addPage(new OversizedConfigPage(QStringLiteral("First")));
	dialog.addPage(new OversizedConfigPage(QStringLiteral("Second")));
	// QET assigns the active application window after constructing the pages.
	dialog.setParent(&parent, dialog.windowFlags());

	auto *header =
		dialog.findChild<QLabel *>(QStringLiteral("configPageTitle"));
	QVERIFY(header);
	dialog.show();
	QTRY_COMPARE(header->text(), QStringLiteral("First"));

	dialog.setCurrentPage(1);
	QTRY_COMPARE(header->text(), QStringLiteral("Second"));
}

void ConfigDialogUxTest::boundsDialogToLogicalAvailableGeometry_data()
{
	QTest::addColumn<QRect>("available_geometry");
	QTest::newRow("1366x768 at 100 percent") << QRect(0, 0, 1366, 728);
	QTest::newRow("1280x680 logical viewport for 1920x1080 at 150 percent")
			<< QRect(0, 0, 1280, 680);
}

void ConfigDialogUxTest::boundsDialogToLogicalAvailableGeometry()
{
	QFETCH(QRect, available_geometry);

	ConfigDialog dialog;
	dialog.addPage(new OversizedConfigPage(QStringLiteral("Page")));
	dialog.show();
	dialog.fitToAvailableGeometry(available_geometry);
	QApplication::processEvents();

	const QSize expected_maximum {
		static_cast<int>(available_geometry.width() * 0.94),
		static_cast<int>(available_geometry.height() * 0.94)
	};
	QCOMPARE(dialog.maximumSize(), expected_maximum);
	QVERIFY(dialog.width() <= expected_maximum.width());
	QVERIFY(dialog.height() <= expected_maximum.height());
	QVERIFY(available_geometry.contains(dialog.frameGeometry()));
}

void ConfigDialogUxTest::restoresDesiredSizeAcrossLogicalScreens()
{
	ConfigDialog dialog;
	dialog.addPage(new OversizedConfigPage(QStringLiteral("Page")));
	dialog.show();
	dialog.fitToAvailableGeometry(QRect(0, 0, 1920, 1040));
	dialog.resize(1000, 700);
	QApplication::processEvents();

	dialog.fitToAvailableGeometry(QRect(0, 0, 1280, 680));
	QVERIFY(dialog.height() < 700);
	dialog.fitToAvailableGeometry(QRect(0, 0, 1920, 1040));
	QCOMPARE(dialog.size(), QSize(1000, 700));
}

void ConfigDialogUxTest::refittingDoesNotRecenterTheDialog()
{
	const QRect available_geometry(0, 0, 1920, 1040);
	ConfigDialog dialog;
	dialog.addPage(new OversizedConfigPage(QStringLiteral("Page")));
	dialog.show();
	dialog.fitToAvailableGeometry(available_geometry);
	dialog.move(40, 30);
	QApplication::processEvents();

	const QPoint position_before_refit = dialog.pos();
	dialog.fitToAvailableGeometry(available_geometry);
	QCOMPARE(dialog.pos(), position_before_refit);

	dialog.move(1800, 900);
	dialog.fitToAvailableGeometry(available_geometry);
	QVERIFY(available_geometry.contains(dialog.frameGeometry()));
}

void ConfigDialogUxTest::keyboardNavigationAndFocusRemainUsable()
{
	ConfigDialog dialog;
	auto *first_page = new OversizedConfigPage(QStringLiteral("First"));
	auto *second_page = new OversizedConfigPage(QStringLiteral("Second"));
	dialog.addPage(first_page);
	dialog.addPage(second_page);
	dialog.show();
	dialog.fitToAvailableGeometry(QRect(0, 0, 1280, 680));
	QApplication::processEvents();

	auto *navigation =
		dialog.findChild<QListWidget *>(QStringLiteral("configPagesList"));
	auto *stack =
		dialog.findChild<QStackedWidget *>(QStringLiteral("configPagesStack"));
	auto *buttons =
		dialog.findChild<QDialogButtonBox *>(QStringLiteral("configDialogButtonBox"));
	QVERIFY(navigation);
	QVERIFY(stack);
	QVERIFY(buttons);
	QVERIFY(buttons->isVisible());

	const QRect button_geometry(
		buttons->mapTo(&dialog, QPoint(0, 0)), buttons->size());
	QVERIFY(dialog.contentsRect().contains(button_geometry));

	navigation->setFocus();
	QTest::keyClick(navigation, Qt::Key_Down);
	QCOMPARE(navigation->currentRow(), 1);
	QCOMPARE(stack->currentIndex(), 1);

	auto *current_scroll = qobject_cast<QScrollArea *>(stack->currentWidget());
	QVERIFY(current_scroll);
	QVERIFY(current_scroll->verticalScrollBar()->maximum() > 0);
	QCOMPARE(current_scroll->horizontalScrollBar()->maximum(), 0);

	current_scroll->verticalScrollBar()->setValue(0);
	second_page->firstControl()->setFocus();
	QTest::keyClick(second_page->firstControl(), Qt::Key_Tab);
	QApplication::processEvents();
	QCOMPARE(QApplication::focusWidget(), second_page->lastControl());
	QVERIFY(current_scroll->verticalScrollBar()->value() > 0);

	QPushButton *ok_button = buttons->button(QDialogButtonBox::Ok);
	QVERIFY(ok_button);
	bool reached_ok = QApplication::focusWidget() == ok_button;
	for (int index = 0; index < 16 && !reached_ok; ++index) {
		QWidget *focused_widget = QApplication::focusWidget();
		QVERIFY(focused_widget);
		QTest::keyClick(focused_widget, Qt::Key_Tab);
		reached_ok = QApplication::focusWidget() == ok_button;
	}
	QVERIFY(reached_ok);
	QTest::keyClick(ok_button, Qt::Key_Tab, Qt::ShiftModifier);
	QVERIFY(QApplication::focusWidget() != ok_button);
}

void ConfigDialogUxTest::standardDialogKeysAcceptAndReject()
{
	ConfigDialog accepted_dialog;
	accepted_dialog.addPage(new OversizedConfigPage(QStringLiteral("Page")));
	QSignalSpy accepted_spy(&accepted_dialog, &QDialog::accepted);
	accepted_dialog.show();
	auto *buttons = accepted_dialog.findChild<QDialogButtonBox *>(
			QStringLiteral("configDialogButtonBox"));
	QVERIFY(buttons);
	QPushButton *ok_button = buttons->button(QDialogButtonBox::Ok);
	QVERIFY(ok_button);
	ok_button->setFocus();
	QTest::keyClick(ok_button, Qt::Key_Return);
	QTRY_COMPARE(accepted_spy.count(), 1);

	ConfigDialog rejected_dialog;
	rejected_dialog.addPage(new OversizedConfigPage(QStringLiteral("Page")));
	QSignalSpy rejected_spy(&rejected_dialog, &QDialog::rejected);
	rejected_dialog.show();
	QTest::keyClick(&rejected_dialog, Qt::Key_Escape);
	QTRY_COMPARE(rejected_spy.count(), 1);
}

QTEST_MAIN(ConfigDialogUxTest)

#include "tst_configdialogux.moc"
