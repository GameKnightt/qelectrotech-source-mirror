/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "qetresult.h"
#include "ui/projectsavestatuscontroller.h"
#include "ui/projectsavestatuswidget.h"

#include <QAccessible>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QSignalSpy>
#include <QStatusBar>
#include <QTest>
#include <QVBoxLayout>

class ProjectSaveStatusTest : public QObject
{
	Q_OBJECT

	private slots:
		void cancellationIsNotSuccess();
		void unsavedProjectCannotAppearSaved();
		void exposesEveryStateAsText();
		void errorRemainsExplicitAndAccessible();
		void stateMachineRejectsFalseAndStaleSuccess();
		void keepsProjectStatesIndependent();
		void recoveryEvidenceIsHonest();
		void staysOutsideKeyboardFocus();
		void fitsStatusBarWithLargeText();
		void exposesAccessibleStateUpdates();
};

void ProjectSaveStatusTest::cancellationIsNotSuccess()
{
	const QETResult result = QETResult::cancelled();
	QVERIFY(result.isCancelled());
	QVERIFY(!result.isOk());
	QVERIFY(result.errorMessage().isEmpty());
}

void ProjectSaveStatusTest::unsavedProjectCannotAppearSaved()
{
	ProjectSaveStatusWidget widget;
	widget.setState(
		ProjectSaveStatusWidget::State::Saved,
		QStringLiteral("Projet sans titre"));

	QCOMPARE(widget.state(), ProjectSaveStatusWidget::State::Modified);
	QCOMPARE(widget.statusText(), QStringLiteral("Modifié"));
}

void ProjectSaveStatusTest::exposesEveryStateAsText()
{
	ProjectSaveStatusWidget widget;
	widget.setState(ProjectSaveStatusWidget::State::Modified, QStringLiteral("Projet A"));
	QCOMPARE(widget.state(), ProjectSaveStatusWidget::State::Modified);
	QCOMPARE(widget.statusText(), QStringLiteral("Modifié"));
	QVERIFY(widget.accessibleName().contains(QStringLiteral("Modifié")));

	widget.setState(ProjectSaveStatusWidget::State::Saving, QStringLiteral("Projet A"));
	QCOMPARE(widget.statusText(), QStringLiteral("Enregistrement…"));

	widget.setState(
		ProjectSaveStatusWidget::State::Saved,
		QStringLiteral("Projet A"),
		QStringLiteral("C:/projets/a.qet"));
	QCOMPARE(widget.statusText(), QStringLiteral("Sauvegardé"));
	QVERIFY(widget.toolTip().contains(QStringLiteral("C:/projets/a.qet")));
}

void ProjectSaveStatusTest::errorRemainsExplicitAndAccessible()
{
	ProjectSaveStatusWidget widget;
	widget.setState(
		ProjectSaveStatusWidget::State::Error,
		QStringLiteral("Projet A"),
		QString(),
		QStringLiteral("Accès refusé"));

	QCOMPARE(widget.state(), ProjectSaveStatusWidget::State::Error);
	QCOMPARE(widget.statusText(), QStringLiteral("Erreur d’enregistrement"));
	QVERIFY(widget.accessibleDescription().contains(QStringLiteral("non enregistrées")));
	QVERIFY(widget.accessibleDescription().contains(QStringLiteral("Accès refusé")));
}

void ProjectSaveStatusTest::stateMachineRejectsFalseAndStaleSuccess()
{
	QObject project;
	ProjectSaveStatusController controller;
	controller.registerProject(
		&project,
		QStringLiteral("Projet A"),
		QStringLiteral("C:/projets/a.qet"),
		true);
	controller.setActiveProject(&project);

	controller.beginCanonicalSave(&project, 1);
	controller.beginCanonicalSave(&project, 2);
	controller.finishCanonicalSave(
		&project, 1, true, QString(), QStringLiteral("C:/projets/a.qet"), false);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Saving);

	controller.finishCanonicalSave(
		&project, 2, false, QStringLiteral("Accès refusé"), QString(), true);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Error);
	QCOMPARE(controller.activeSnapshot().detail, QStringLiteral("Accès refusé"));

	controller.beginCanonicalSave(&project, 3);
	controller.setModified(&project, false);
	controller.finishCanonicalSave(
		&project, 3, true, QString(), QStringLiteral("C:/projets/a.qet"), false);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Saved);

	controller.setModified(&project, true);
	controller.beginCanonicalSave(&project, 4);
	controller.setModified(&project, false);
	controller.setModified(&project, true);
	controller.finishCanonicalSave(
		&project, 4, true, QString(), QStringLiteral("C:/projets/a.qet"), true);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Modified);

	QObject unsaved_project;
	controller.registerProject(
		&unsaved_project, QStringLiteral("Sans titre"), QString(), false);
	controller.setActiveProject(&unsaved_project);
	controller.beginCanonicalSave(&unsaved_project, 5);
	controller.finishCanonicalSave(
		&unsaved_project, 5, true, QString(), QString(), false);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Modified);
}

void ProjectSaveStatusTest::keepsProjectStatesIndependent()
{
	QObject project_a;
	QObject project_b;
	ProjectSaveStatusController controller;
	controller.registerProject(
		&project_a, QStringLiteral("Projet A"), QStringLiteral("A.qet"), true);
	controller.registerProject(
		&project_b, QStringLiteral("Projet B"), QStringLiteral("B.qet"), false);
	controller.setActiveProject(&project_a);
	QSignalSpy spy(&controller, &ProjectSaveStatusController::activeSnapshotChanged);

	controller.beginCanonicalSave(&project_b, 10);
	controller.finishCanonicalSave(
		&project_b, 10, false, QStringLiteral("Disque plein"), QString(), true);
	QCOMPARE(spy.count(), 0);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Modified);

	controller.setActiveProject(&project_b);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Error);
	QCOMPARE(controller.activeSnapshot().detail, QStringLiteral("Disque plein"));

	controller.setActiveProject(&project_a);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Modified);
	controller.unregisterProject(&project_a);
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::NoProject);
}

void ProjectSaveStatusTest::recoveryEvidenceIsHonest()
{
	QObject project;
	ProjectSaveStatusController controller;
	ProjectSaveStatusWidget widget;
	connect(
		&controller,
		&ProjectSaveStatusController::activeSnapshotChanged,
		&widget,
		[&widget](const ProjectSaveStatusController::Snapshot &snapshot) {
			widget.setState(
				snapshot.state,
				snapshot.projectName,
				snapshot.filePath,
				snapshot.detail,
				snapshot.recoveryPath,
				snapshot.recoveryError);
		});
	controller.registerProject(
		&project, QStringLiteral("Projet A"), QStringLiteral("A.qet"), true);
	controller.setActiveProject(&project);

	QVERIFY(!widget.toolTip().contains(QStringLiteral("protège")));
	QVERIFY(!widget.toolTip().contains(QStringLiteral("disponible")));
	controller.beginRecoveryBackup(&project, 20);
	controller.finishRecoveryBackup(
		&project, 20, false, QStringLiteral("Accès refusé"), QString());
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Modified);
	QVERIFY(widget.toolTip().contains(QStringLiteral("n’a pas pu")));
	QVERIFY(widget.toolTip().contains(QStringLiteral("Accès refusé")));

	controller.beginRecoveryBackup(&project, 21);
	controller.finishRecoveryBackup(
		&project,
		21,
		true,
		QString(),
		QStringLiteral("C:/autosave/a.qet.autosave"));
	QCOMPARE(
		controller.activeSnapshot().state,
		ProjectSaveStatusWidget::State::Modified);
	QVERIFY(widget.toolTip().contains(QStringLiteral("Copie de récupération disponible")));
	QVERIFY(widget.toolTip().contains(QStringLiteral("a.qet.autosave")));

	controller.invalidateRecoveryBackup(&project);
	QVERIFY(!widget.toolTip().contains(QStringLiteral("a.qet.autosave")));
}

void ProjectSaveStatusTest::staysOutsideKeyboardFocus()
{
	QWidget page;
	auto *layout = new QVBoxLayout(&page);
	QLineEdit before;
	QLineEdit after;
	ProjectSaveStatusWidget widget;
	layout->addWidget(&before);
	layout->addWidget(&widget);
	layout->addWidget(&after);
	page.show();
	QTest::qWait(1);

	after.setFocus();
	QVERIFY(after.hasFocus());
	widget.setState(
		ProjectSaveStatusWidget::State::Saving,
		QStringLiteral("Projet A"),
		QStringLiteral("C:/projets/a.qet"));
	QVERIFY(after.hasFocus());
	QCOMPARE(widget.focusPolicy(), Qt::NoFocus);
	const auto labels = widget.findChildren<QLabel *>();
	QVERIFY(!labels.isEmpty());
	for (QLabel *label : labels)
		QCOMPARE(label->focusPolicy(), Qt::NoFocus);
}

void ProjectSaveStatusTest::fitsStatusBarWithLargeText()
{
	QMainWindow window;
	window.resize(853, 453); // 1280 x 680 logical desktop at 150 % scaling.
	ProjectSaveStatusWidget widget;
	QFont font = window.font();
	font.setPointSizeF(font.pointSizeF() * 1.5);
	window.setFont(font);
	widget.setFont(font);
	window.statusBar()->addPermanentWidget(&widget);
	widget.setState(
		ProjectSaveStatusWidget::State::Error,
		QStringLiteral("Projet industriel avec un nom très long"),
		QStringLiteral("C:/projets/industriel.qet"),
		QStringLiteral("Accès refusé"));
	window.show();
	QTest::qWait(1);

	QVERIFY(widget.isVisible());
	QVERIFY(window.statusBar()->contentsRect().contains(widget.geometry()));
	QVERIFY(widget.height() <= window.statusBar()->height());
	QVERIFY(widget.sizeHint().width() < window.statusBar()->width());
}

void ProjectSaveStatusTest::exposesAccessibleStateUpdates()
{
	ProjectSaveStatusWidget widget;
	widget.show();
	widget.setState(
		ProjectSaveStatusWidget::State::Modified,
		QStringLiteral("Projet A"),
		QStringLiteral("C:/projets/a.qet"));
	widget.setState(
		ProjectSaveStatusWidget::State::Error,
		QStringLiteral("Projet A"),
		QStringLiteral("C:/projets/a.qet"),
		QStringLiteral("Disque plein"));
	auto *accessible = QAccessible::queryAccessibleInterface(&widget);
	QVERIFY(accessible);
	QVERIFY(widget.accessibleName().contains(QStringLiteral("Projet A")));
	QVERIFY(widget.accessibleDescription().contains(QStringLiteral("Disque plein")));
	QCOMPARE(
		accessible->text(QAccessible::Name),
		widget.accessibleName());
	QCOMPARE(
		accessible->text(QAccessible::Description),
		widget.accessibleDescription());
}

QTEST_MAIN(ProjectSaveStatusTest)
#include "tst_projectsavestatus.moc"
