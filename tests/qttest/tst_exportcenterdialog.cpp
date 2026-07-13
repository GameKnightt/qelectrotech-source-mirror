/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/ui/exportcenterdialog.h"

#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFont>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSignalSpy>
#include <QTreeWidget>
#include <QtTest>

namespace {

ExportCenterDialog::ProjectSummary projectSummary()
{
	return {
		QStringLiteral("Projet de test"),
		4,
		QStringLiteral("C:/projets/qet/exports")
	};
}

ExportCenterDialog::Entry entry(
	ExportCenterDialog::Section section,
	const QString &title,
	QAction *action)
{
	return {
		section,
		title,
		QStringLiteral("Description de %1").arg(title),
		QStringLiteral("Résultat de %1").arg(title),
		QStringLiteral("Raison de %1").arg(title),
		action
	};
}

QTreeWidget *operationTree(ExportCenterDialog &dialog)
{
	return dialog.findChild<QTreeWidget *>(
		QStringLiteral("exportCenterOperationTree"));
}

QPushButton *continueButton(ExportCenterDialog &dialog)
{
	return dialog.findChild<QPushButton *>(
		QStringLiteral("exportCenterContinueButton"));
}

QTreeWidgetItem *childWithTitle(QTreeWidget *tree, const QString &title)
{
	if (!tree) return nullptr;
	for (int section = 0; section < tree->topLevelItemCount(); ++section) {
		QTreeWidgetItem *section_item = tree->topLevelItem(section);
		for (int child = 0; child < section_item->childCount(); ++child) {
			QTreeWidgetItem *item = section_item->child(child);
			if (item->text(0) == title) return item;
		}
	}
	return nullptr;
}

}

class ExportCenterDialogTest : public QObject
{
	Q_OBJECT

	private slots:
		void groupsEntriesAndOmitsEmptySections();
		void unavailableEntriesRemainSelectableAndExplainState();
		void tracksLiveActionEnablementAndDestruction();
		void acceptsExactActionWithoutTriggeringIt();
		void triggersActionDeferredOnlyWhenStillAvailable();
		void defersLaunchUntilDialogIsDestroyed();
		void exposesAccessibilityFocusAndCompactGeometry();
};

void ExportCenterDialogTest::groupsEntriesAndOmitsEmptySections()
{
	QAction data_first(QStringLiteral("Données A"), this);
	QAction document_first(QStringLiteral("Document A"), this);
	QAction data_second(QStringLiteral("Données B"), this);
	QAction document_second(QStringLiteral("Document B"), this);
	QAction data_third(QStringLiteral("Données C"), this);
	QAction document_third(QStringLiteral("Document C"), this);
	const QList<ExportCenterDialog::Entry> entries {
		entry(ExportCenterDialog::Section::Data,
			QStringLiteral("Données A"), &data_first),
		entry(ExportCenterDialog::Section::Documents,
			QStringLiteral("Document A"), &document_first),
		entry(ExportCenterDialog::Section::Data,
			QStringLiteral("Données B"), &data_second),
		entry(ExportCenterDialog::Section::Documents,
			QStringLiteral("Document B"), &document_second),
		entry(ExportCenterDialog::Section::Data,
			QStringLiteral("Données C"), &data_third),
		entry(ExportCenterDialog::Section::Documents,
			QStringLiteral("Document C"), &document_third)
	};

	ExportCenterDialog dialog(projectSummary(), entries);
	QTreeWidget *tree = operationTree(dialog);
	QVERIFY(tree);
	QVERIFY(!(dialog.windowFlags() & Qt::WindowContextHelpButtonHint));
	QCOMPARE(dialog.entryCount(), 6);
	QCOMPARE(dialog.availableEntryCount(), 6);
	QWidget *summary_widget = dialog.findChild<QWidget *>(
		QStringLiteral("exportCenterProjectSummary"));
	QVERIFY(summary_widget);
	bool has_folio_summary = false;
	for (const QLabel *label : summary_widget->findChildren<QLabel *>()) {
		if (label->text() == QStringLiteral("4 folios dans le projet")) {
			has_folio_summary = true;
			break;
		}
	}
	QVERIFY(has_folio_summary);
	QCOMPARE(tree->topLevelItemCount(), 2);
	QCOMPARE(tree->topLevelItem(0)->text(0), QStringLiteral("Documents"));
	QCOMPARE(tree->topLevelItem(1)->text(0), QStringLiteral("Données"));
	QCOMPARE(tree->topLevelItem(0)->childCount(), 3);
	QCOMPARE(tree->topLevelItem(0)->child(0)->text(0),
		QStringLiteral("Document A"));
	QCOMPARE(tree->topLevelItem(0)->child(1)->text(0),
		QStringLiteral("Document B"));
	QCOMPARE(tree->topLevelItem(0)->child(2)->text(0),
		QStringLiteral("Document C"));
	QCOMPARE(tree->topLevelItem(1)->childCount(), 3);
	QCOMPARE(tree->topLevelItem(1)->child(0)->text(0),
		QStringLiteral("Données A"));
	QCOMPARE(tree->topLevelItem(1)->child(1)->text(0),
		QStringLiteral("Données B"));
	QCOMPARE(tree->topLevelItem(1)->child(2)->text(0),
		QStringLiteral("Données C"));

	ExportCenterDialog data_only(
		projectSummary(),
		{entry(ExportCenterDialog::Section::Data,
			QStringLiteral("Données seulement"), &data_first)});
	QTreeWidget *data_tree = operationTree(data_only);
	QVERIFY(data_tree);
	QCOMPARE(data_tree->topLevelItemCount(), 1);
	QCOMPARE(data_tree->topLevelItem(0)->text(0), QStringLiteral("Données"));

	ExportCenterDialog empty(projectSummary(), {});
	QTreeWidget *empty_tree = operationTree(empty);
	QVERIFY(empty_tree);
	QCOMPARE(empty.entryCount(), 0);
	QCOMPARE(empty.availableEntryCount(), 0);
	QCOMPARE(empty_tree->topLevelItemCount(), 0);
	QVERIFY(!continueButton(empty)->isEnabled());
}

void ExportCenterDialogTest::unavailableEntriesRemainSelectableAndExplainState()
{
	QAction unavailable(QStringLiteral("PDF"), this);
	unavailable.setEnabled(false);
	QAction available(QStringLiteral("Images"), this);
	const QList<ExportCenterDialog::Entry> entries {
		entry(ExportCenterDialog::Section::Documents,
			QStringLiteral("PDF"), &unavailable),
		entry(ExportCenterDialog::Section::Documents,
			QStringLiteral("Sans commande"), nullptr),
		entry(ExportCenterDialog::Section::Documents,
			QStringLiteral("Images"), &available)
	};
	ExportCenterDialog dialog(projectSummary(), entries);
	QTreeWidget *tree = operationTree(dialog);
	QPushButton *continue_button = continueButton(dialog);
	QLabel *title = dialog.findChild<QLabel *>(
		QStringLiteral("exportCenterDetailTitle"));
	QLabel *description = dialog.findChild<QLabel *>(
		QStringLiteral("exportCenterDetailDescription"));
	QLabel *availability = dialog.findChild<QLabel *>(
		QStringLiteral("exportCenterDetailAvailability"));
	QVERIFY(tree);
	QVERIFY(continue_button);
	QVERIFY(title);
	QVERIFY(description);
	QVERIFY(availability);
	QCOMPARE(dialog.availableEntryCount(), 1);

	QTreeWidgetItem *unavailable_item = childWithTitle(tree, QStringLiteral("PDF"));
	QVERIFY(unavailable_item);
	QVERIFY(unavailable_item->flags() & Qt::ItemIsEnabled);
	QVERIFY(unavailable_item->flags() & Qt::ItemIsSelectable);
	QCOMPARE(unavailable_item->text(1), QStringLiteral("Indisponible"));
	tree->setCurrentItem(unavailable_item);
	QCOMPARE(title->text(), QStringLiteral("PDF"));
	QVERIFY(description->text().contains(QStringLiteral("PDF")));
	QVERIFY(availability->text().contains(QStringLiteral("Indisponible")));
	QVERIFY(availability->text().contains(QStringLiteral("Raison de PDF")));
	QVERIFY(!continue_button->isEnabled());

	QTreeWidgetItem *missing_item = childWithTitle(
		tree, QStringLiteral("Sans commande"));
	QVERIFY(missing_item);
	QVERIFY(missing_item->flags() & Qt::ItemIsSelectable);
	tree->setCurrentItem(missing_item);
	QCOMPARE(missing_item->text(1), QStringLiteral("Indisponible"));
	QVERIFY(!continue_button->isEnabled());
}

void ExportCenterDialogTest::tracksLiveActionEnablementAndDestruction()
{
	auto action = new QAction(QStringLiteral("Export dynamique"), this);
	action->setEnabled(false);
	ExportCenterDialog dialog(
		projectSummary(),
		{entry(ExportCenterDialog::Section::Data,
			QStringLiteral("Export dynamique"), action)});
	QTreeWidget *tree = operationTree(dialog);
	QPushButton *continue_button = continueButton(dialog);
	QTreeWidgetItem *item = childWithTitle(tree, QStringLiteral("Export dynamique"));
	QVERIFY(tree);
	QVERIFY(continue_button);
	QVERIFY(item);
	QCOMPARE(dialog.availableEntryCount(), 0);
	QCOMPARE(item->text(1), QStringLiteral("Indisponible"));
	QVERIFY(!continue_button->isEnabled());

	action->setEnabled(true);
	QTRY_COMPARE(dialog.availableEntryCount(), 1);
	QTRY_COMPARE(item->text(1), QStringLiteral("Disponible"));
	QTRY_VERIFY(continue_button->isEnabled());

	action->setEnabled(false);
	QTRY_COMPARE(dialog.availableEntryCount(), 0);
	QTRY_COMPARE(item->text(1), QStringLiteral("Indisponible"));
	QTRY_VERIFY(!continue_button->isEnabled());

	action->setEnabled(true);
	QTRY_VERIFY(continue_button->isEnabled());
	delete action;
	QTRY_COMPARE(dialog.availableEntryCount(), 0);
	QTRY_COMPARE(item->text(1), QStringLiteral("Indisponible"));
	QTRY_VERIFY(!continue_button->isEnabled());
}

void ExportCenterDialogTest::acceptsExactActionWithoutTriggeringIt()
{
	QWidget owner;
	QMenu historical_menu;
	QAction action(QStringLiteral("Exporter les images"), &owner);
	action.setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X));
	action.setShortcutContext(Qt::ApplicationShortcut);
	action.setCheckable(true);
	action.setChecked(true);
	historical_menu.addAction(&action);
	QSignalSpy triggered(&action, &QAction::triggered);

	ExportCenterDialog dialog(
		projectSummary(),
		{entry(ExportCenterDialog::Section::Documents,
			QStringLiteral("Images"), &action)});
	QPushButton *continue_button = continueButton(dialog);
	QVERIFY(continue_button);
	QVERIFY(continue_button->isEnabled());
	continue_button->click();

	QCOMPARE(dialog.result(), int(QDialog::Accepted));
	QCOMPARE(dialog.selectedAction().data(), &action);
	QCOMPARE(triggered.count(), 0);
	QCOMPARE(action.parent(), &owner);
	QCOMPARE(action.shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X));
	QCOMPARE(action.shortcutContext(), Qt::ApplicationShortcut);
	QVERIFY(action.isCheckable());
	QVERIFY(action.isChecked());
	QVERIFY(historical_menu.actions().contains(&action));
}

void ExportCenterDialogTest::triggersActionDeferredOnlyWhenStillAvailable()
{
	QAction enabled_action(QStringLiteral("Disponible"), this);
	QSignalSpy enabled_triggered(&enabled_action, &QAction::triggered);
	ExportCenterDialog::triggerActionDeferred(
		QPointer<QAction>(&enabled_action));
	QCOMPARE(enabled_triggered.count(), 0);
	QTRY_COMPARE(enabled_triggered.count(), 1);

	QAction disabled_before_turn(QStringLiteral("Désactivée"), this);
	QSignalSpy disabled_triggered(&disabled_before_turn, &QAction::triggered);
	ExportCenterDialog::triggerActionDeferred(
		QPointer<QAction>(&disabled_before_turn));
	disabled_before_turn.setEnabled(false);
	QCoreApplication::processEvents();
	QCOMPARE(disabled_triggered.count(), 0);

	bool destroyed_action_triggered = false;
	auto destroyed_before_turn = new QAction(QStringLiteral("Détruite"));
	connect(destroyed_before_turn, &QAction::triggered, this,
		[&destroyed_action_triggered]() { destroyed_action_triggered = true; });
	ExportCenterDialog::triggerActionDeferred(
		QPointer<QAction>(destroyed_before_turn));
	delete destroyed_before_turn;
	QCoreApplication::processEvents();
	QVERIFY(!destroyed_action_triggered);

	ExportCenterDialog::triggerActionDeferred(QPointer<QAction>());
	QCoreApplication::processEvents();
}

void ExportCenterDialogTest::defersLaunchUntilDialogIsDestroyed()
{
	QAction action(QStringLiteral("Export après fermeture"), this);
	QSignalSpy triggered(&action, &QAction::triggered);
	bool triggered_without_modal_dialog = false;
	connect(&action, &QAction::triggered, this,
		[&triggered_without_modal_dialog]() {
			triggered_without_modal_dialog =
				QApplication::activeModalWidget() == nullptr;
		});

	QPointer<QAction> selected_action;
	QPointer<ExportCenterDialog> dialog_guard;
	{
		ExportCenterDialog dialog(
			projectSummary(),
			{entry(ExportCenterDialog::Section::Documents,
				QStringLiteral("Images"), &action)});
		dialog_guard = &dialog;
		dialog.show();
		QTRY_COMPARE(QApplication::activeModalWidget(), &dialog);
		continueButton(dialog)->click();
		QCOMPARE(dialog.result(), int(QDialog::Accepted));
		selected_action = dialog.selectedAction();
		QCOMPARE(triggered.count(), 0);
	}

	QVERIFY(dialog_guard.isNull());
	QTRY_VERIFY(QApplication::activeModalWidget() == nullptr);
	ExportCenterDialog::triggerActionDeferred(selected_action);
	QCOMPARE(triggered.count(), 0);
	QTRY_COMPARE(triggered.count(), 1);
	QVERIFY(triggered_without_modal_dialog);
}

void ExportCenterDialogTest::exposesAccessibilityFocusAndCompactGeometry()
{
	const QFont original_font = QGuiApplication::font();
	struct FontRestorer {
		QFont font;
		~FontRestorer() { QGuiApplication::setFont(font); }
	} restorer {original_font};
	QFont large_font = original_font;
	const qreal base_size = original_font.pointSizeF() > 0
		? original_font.pointSizeF() : 9.0;
	large_font.setPointSizeF(base_size * 1.5);
	QGuiApplication::setFont(large_font);

	QAction action(QStringLiteral("Export accessible"), this);
	ExportCenterDialog::ProjectSummary summary {
		QStringLiteral("Projet industriel avec un titre volontairement long"),
		42,
		QStringLiteral("C:/projets/clients/atelier/ligne-de-production/exports")
	};
	ExportCenterDialog dialog(
		summary,
		{entry(ExportCenterDialog::Section::Documents,
			QStringLiteral("Exporter un document accessible"), &action)});
	dialog.show();
	QApplication::processEvents();

	QTreeWidget *tree = operationTree(dialog);
	QPushButton *continue_button = continueButton(dialog);
	QPushButton *close_button = dialog.findChild<QPushButton *>(
		QStringLiteral("exportCenterCloseButton"));
	QDialogButtonBox *buttons = dialog.findChild<QDialogButtonBox *>(
		QStringLiteral("exportCenterButtonBox"));
	QVERIFY(tree);
	QVERIFY(continue_button);
	QVERIFY(close_button);
	QVERIFY(buttons);
	QVERIFY(!dialog.accessibleName().isEmpty());
	QVERIFY(!dialog.accessibleDescription().isEmpty());
	QVERIFY(!tree->accessibleName().isEmpty());
	QVERIFY(!tree->accessibleDescription().isEmpty());
	QVERIFY(!continue_button->accessibleName().isEmpty());
	QCOMPARE(close_button->text(), QStringLiteral("&Fermer"));
	QVERIFY(!close_button->accessibleName().isEmpty());
	QCOMPARE(tree->currentItem()->text(1), QStringLiteral("Disponible"));
	QTRY_VERIFY(tree->hasFocus());

	const QSize target_logical_size(1280, 720);
	QVERIFY2(dialog.minimumSizeHint().width() <= target_logical_size.width(),
		"The export center must fit a 1920x1080 display at 150% scaling");
	QVERIFY2(dialog.minimumSizeHint().height() <= target_logical_size.height(),
		"The export center must fit a 1920x1080 display at 150% scaling");
	QVERIFY(dialog.width() <= target_logical_size.width());
	QVERIFY(dialog.height() <= target_logical_size.height());
	const QRect button_geometry(
		buttons->mapTo(&dialog, QPoint(0, 0)), buttons->size());
	QVERIFY(dialog.contentsRect().contains(button_geometry));

	bool reached_continue = QApplication::focusWidget() == continue_button;
	for (int index = 0; index < 8 && !reached_continue; ++index) {
		QWidget *focused = QApplication::focusWidget();
		QVERIFY(focused);
		QTest::keyClick(focused, Qt::Key_Tab);
		reached_continue = QApplication::focusWidget() == continue_button;
	}
	QVERIFY(reached_continue);
	dialog.close();
}

QTEST_MAIN(ExportCenterDialogTest)

#include "tst_exportcenterdialog.moc"
