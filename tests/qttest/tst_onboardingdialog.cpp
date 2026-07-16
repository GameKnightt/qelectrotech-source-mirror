/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/ui/onboardingdialog.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QtTest>

class OnboardingDialogTest : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void init();
		void firstRunEligibilitySurvivesLaterSettingsWrites();
		void existingProfilesAreNotAutomaticallyEligible();
		void firstRunStateIsPersistedOnlyAfterExplicitCompletion();
		void closingTheDialogKeepsFirstRunAvailable();
		void skipCompletesWithoutVisitingEveryPage();
		void exposesFourReadableKeyboardNavigableSteps();
		void fitsA150PercentLogicalDesktop();
};

void OnboardingDialogTest::initTestCase()
{
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTechTests"));
	QCoreApplication::setApplicationName(QStringLiteral("OnboardingDialogTest"));
}

void OnboardingDialogTest::init()
{
	QSettings settings;
	settings.clear();
	settings.sync();
	QVERIFY(OnboardingDialog::shouldShowOnStartup());
}

void OnboardingDialogTest::firstRunEligibilitySurvivesLaterSettingsWrites()
{
	QVERIFY(OnboardingDialog::initializeFirstRunEligibility());
	QSettings settings;
	settings.setValue(QStringLiteral("window/geometry"), QByteArray("test"));
	settings.sync();

	QVERIFY(OnboardingDialog::initializeFirstRunEligibility());
	QVERIFY(OnboardingDialog::shouldShowOnStartup());
}

void OnboardingDialogTest::existingProfilesAreNotAutomaticallyEligible()
{
	QSettings settings;
	settings.setValue(QStringLiteral("window/geometry"), QByteArray("existing"));
	settings.sync();

	QVERIFY(!OnboardingDialog::initializeFirstRunEligibility());
	QVERIFY(!OnboardingDialog::shouldShowOnStartup());
}

void OnboardingDialogTest::firstRunStateIsPersistedOnlyAfterExplicitCompletion()
{
	OnboardingDialog dialog;
	auto pages = dialog.findChild<QStackedWidget *>(
		QStringLiteral("onboardingPages"));
	auto next = dialog.findChild<QPushButton *>(
		QStringLiteral("onboardingNextButton"));
	QVERIFY(pages);
	QVERIFY(next);
	QCOMPARE(pages->count(), 4);

	QSignalSpy accepted_spy(&dialog, &QDialog::accepted);
	dialog.show();
	for (int page = 0; page < 3; ++page) {
		QTest::mouseClick(next, Qt::LeftButton);
		QCOMPARE(pages->currentIndex(), page + 1);
		QVERIFY(OnboardingDialog::shouldShowOnStartup());
	}
	QCOMPARE(next->text(), QStringLiteral("Commencer"));
	QTest::mouseClick(next, Qt::LeftButton);
	QTRY_COMPARE(accepted_spy.count(), 1);
	QVERIFY(!OnboardingDialog::shouldShowOnStartup());
	QVERIFY(QSettings().value(
		OnboardingDialog::completionSettingsKey()).toBool());
}

void OnboardingDialogTest::closingTheDialogKeepsFirstRunAvailable()
{
	OnboardingDialog dialog;
	QSignalSpy rejected_spy(&dialog, &QDialog::rejected);
	dialog.show();
	QTest::keyClick(&dialog, Qt::Key_Escape);
	QTRY_COMPARE(rejected_spy.count(), 1);
	QVERIFY(OnboardingDialog::shouldShowOnStartup());
}

void OnboardingDialogTest::skipCompletesWithoutVisitingEveryPage()
{
	OnboardingDialog dialog;
	auto pages = dialog.findChild<QStackedWidget *>(
		QStringLiteral("onboardingPages"));
	auto skip = dialog.findChild<QPushButton *>(
		QStringLiteral("onboardingSkipButton"));
	QVERIFY(pages);
	QVERIFY(skip);
	QCOMPARE(pages->currentIndex(), 0);

	QSignalSpy accepted_spy(&dialog, &QDialog::accepted);
	dialog.show();
	QTest::mouseClick(skip, Qt::LeftButton);
	QTRY_COMPARE(accepted_spy.count(), 1);
	QVERIFY(!OnboardingDialog::shouldShowOnStartup());
}

void OnboardingDialogTest::exposesFourReadableKeyboardNavigableSteps()
{
	OnboardingDialog dialog;
	auto pages = dialog.findChild<QStackedWidget *>(
		QStringLiteral("onboardingPages"));
	auto progress = dialog.findChild<QProgressBar *>(
		QStringLiteral("onboardingProgress"));
	auto step_label = dialog.findChild<QLabel *>(
		QStringLiteral("onboardingStepLabel"));
	auto back = dialog.findChild<QPushButton *>(
		QStringLiteral("onboardingBackButton"));
	auto next = dialog.findChild<QPushButton *>(
		QStringLiteral("onboardingNextButton"));
	QVERIFY(pages);
	QVERIFY(progress);
	QVERIFY(step_label);
	QVERIFY(back);
	QVERIFY(next);
	QCOMPARE(
		dialog.findChildren<QScrollArea *>(
			QStringLiteral("onboardingPageScrollArea")).size(),
		4);

	dialog.show();
	QTRY_VERIFY(next->hasFocus());
	QCOMPARE(progress->value(), 1);
	QVERIFY(step_label->text().contains(QStringLiteral("1")));
	QVERIFY(!back->isEnabled());

	QTest::keyClick(next, Qt::Key_Return);
	QCOMPARE(pages->currentIndex(), 1);
	QCOMPARE(progress->value(), 2);
	QVERIFY(back->isEnabled());

	back->setFocus();
	QTest::keyClick(back, Qt::Key_Return);
	QCOMPARE(pages->currentIndex(), 0);
	QVERIFY(dialog.accessibleName().contains(QStringLiteral("QElectroTech")));
	QVERIFY(!dialog.accessibleDescription().isEmpty());
}

void OnboardingDialogTest::fitsA150PercentLogicalDesktop()
{
	const QFont original_font = QApplication::font();
	QFont large_font = original_font;
	large_font.setPointSizeF(original_font.pointSizeF() * 1.5);
	QApplication::setFont(large_font);

	OnboardingDialog dialog;
	dialog.show();
	dialog.fitToAvailableGeometry(QRect(0, 0, 1280, 680));
	QApplication::processEvents();
	QVERIFY(dialog.width() <= 1177);
	QVERIFY(dialog.height() <= 626);

	for (QScrollArea *scroll_area :
		 dialog.findChildren<QScrollArea *>(
			QStringLiteral("onboardingPageScrollArea"))) {
		QCOMPARE(
			scroll_area->horizontalScrollBarPolicy(),
			Qt::ScrollBarAlwaysOff);
	}

	dialog.fitToAvailableGeometry(QRect(0, 0, 640, 360));
	QApplication::processEvents();
	QVERIFY(dialog.width() <= 588);
	QVERIFY(dialog.height() <= 331);

	QApplication::setFont(original_font);
}

QTEST_MAIN(OnboardingDialogTest)

#include "tst_onboardingdialog.moc"
