/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "qetresult.h"
#include "ui/projectsavestatuswidget.h"

#include <QTest>

class ProjectSaveStatusTest : public QObject
{
	Q_OBJECT

	private slots:
		void cancellationIsNotSuccess();
		void unsavedProjectCannotAppearSaved();
		void exposesEveryStateAsText();
		void errorRemainsExplicitAndAccessible();
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

QTEST_MAIN(ProjectSaveStatusTest)
#include "tst_projectsavestatus.moc"
