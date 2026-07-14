/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/PropertiesEditor/propertieseditordockwidget.h"
#include "../../sources/PropertiesEditor/propertieseditorwidget.h"
#include "../../sources/factory/propertieseditorfactory.h"
#include "../../sources/ui/diagrampropertieseditordockwidget.h"

#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtTest>

namespace {

constexpr int ProbeFactoryTypeA = QGraphicsItem::UserType + 301;
constexpr int ProbeFactoryTypeB = QGraphicsItem::UserType + 302;

class ProbeGraphicsItem final : public QGraphicsRectItem
{
	public:
		explicit ProbeGraphicsItem(int item_type) :
			QGraphicsRectItem(QRectF(0, 0, 20, 20)),
			m_item_type(item_type)
		{
			setFlag(QGraphicsItem::ItemIsSelectable);
		}

		int type() const override { return m_item_type; }

	private:
		int m_item_type;
};

class ProbeEditor final : public PropertiesEditorWidget
{
	public:
		explicit ProbeEditor(
			bool tall = false,
			QWidget *parent = nullptr,
			int factory_type = -1) :
			PropertiesEditorWidget(parent),
			m_factory_type(factory_type)
		{
			setObjectName(QStringLiteral("probePropertiesEditor"));
			auto layout = new QVBoxLayout(this);
			m_first = new QLineEdit(this);
			m_first->setObjectName(QStringLiteral("probeFirstField"));
			m_last = new QLineEdit(this);
			m_last->setObjectName(QStringLiteral("probeLastField"));
			layout->addWidget(m_first);
			if (tall) layout->addSpacing(900);
			layout->addWidget(m_last);
			QWidget::setTabOrder(m_first, m_last);
		}

		void apply() override { ++apply_count; }
		void reset() override { ++reset_count; }
		QString title() const override { return QStringLiteral("Éditeur de test"); }
		bool setLiveEdit(bool live_edit) override
		{
			m_live_edit = live_edit;
			return true;
		}

		QLineEdit *firstField() const { return m_first; }
		QLineEdit *lastField() const { return m_last; }
		int factoryType() const { return m_factory_type; }

		int apply_count = 0;
		int reset_count = 0;
		int reuse_count = 0;

	private:
		QLineEdit *m_first = nullptr;
		QLineEdit *m_last = nullptr;
		int m_factory_type = -1;
};

QLabel *label(PropertiesEditorDockWidget &dock, const char *name)
{
	return dock.findChild<QLabel *>(QString::fromLatin1(name));
}

QScrollArea *editorScroll(PropertiesEditorDockWidget &dock)
{
	return dock.findChild<QScrollArea *>(QStringLiteral("m_editor_scroll"));
}

}

PropertiesEditorWidget *PropertiesEditorFactory::propertiesEditor(
	QList<QGraphicsItem *> items,
	PropertiesEditorWidget *editor,
	QWidget *parent)
{
	if (items.isEmpty()) return nullptr;
	const int item_type = items.first()->type();
	for (QGraphicsItem *item : items) {
		if (item->type() != item_type) return nullptr;
	}
	if (item_type != ProbeFactoryTypeA && item_type != ProbeFactoryTypeB) {
		return nullptr;
	}
	if (auto probe = dynamic_cast<ProbeEditor *>(editor)) {
		if (probe->factoryType() == item_type) {
			++probe->reuse_count;
			return probe;
		}
	}
	return new ProbeEditor(false, parent, item_type);
}

class PropertiesEditorDockWidgetTest : public QObject
{
	Q_OBJECT

	private slots:
		void startsInAccessibleMessageState();
		void switchesFromMessageToScrollableEditor();
		void removalAndClearAreIdempotent();
		void doesNotDuplicateEditorsAndDelegatesApplyReset();
		void changingContextDoesNotStealFocus();
		void remainsUsableNarrowWithLargeText();
		void diagramStatesAndAllowedAreas();
		void switchesDiagramSourceWithoutStaleStateOrFocusLoss();
		void reportsUnsupportedAndMixedSelections();
		void deferredSceneDestructionClearsContext();
		void reusesAndReplacesLiveEditor();
		void restoresDockAreaThroughMainWindowState();
};

void PropertiesEditorDockWidgetTest::startsInAccessibleMessageState()
{
	PropertiesEditorDockWidget dock;
	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Message);
	QVERIFY(dock.editors().isEmpty());
	QVERIFY(!dock.accessibleName().isEmpty());
	QVERIFY(!dock.accessibleDescription().isEmpty());

	QLabel *context_title = label(dock, "m_context_title");
	QLabel *context_summary = label(dock, "m_context_summary");
	QLabel *message_title = label(dock, "m_message_title");
	QLabel *message_description = label(dock, "m_message_description");
	QVERIFY(context_title);
	QVERIFY(context_summary);
	QVERIFY(message_title);
	QVERIFY(message_description);
	QVERIFY(!context_title->text().isEmpty());
	QVERIFY(!context_summary->text().isEmpty());
	QVERIFY(!message_title->text().isEmpty());
	QVERIFY(!message_description->text().isEmpty());
	QCOMPARE(context_title->accessibleName(), context_title->text());
	QCOMPARE(context_summary->accessibleName(), context_summary->text());
	QCOMPARE(message_title->accessibleName(), message_title->text());
	QCOMPARE(message_description->accessibleName(), message_description->text());

	QScrollArea *scroll = editorScroll(dock);
	QVERIFY(scroll);
	QVERIFY(scroll->widgetResizable());
	QVERIFY(!scroll->accessibleName().isEmpty());
}

void PropertiesEditorDockWidgetTest::switchesFromMessageToScrollableEditor()
{
	PropertiesEditorDockWidget dock;
	dock.showMessage(
		QStringLiteral("Contexte"),
		QStringLiteral("Aucune sélection"),
		QStringLiteral("Choisissez un objet"),
		QStringLiteral("Un éditeur apparaîtra ici."));
	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Message);

	auto editor = new ProbeEditor(true);
	QVERIFY(dock.addEditor(editor));
	dock.showEditorContext(
		QStringLiteral("Conducteur"),
		QStringLiteral("1 objet sélectionné"));
	dock.resize(320, 480);
	dock.show();
	QApplication::processEvents();

	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Editor);
	QCOMPARE(dock.editors(), QList<PropertiesEditorWidget *>({editor}));
	QCOMPARE(label(dock, "m_context_title")->text(), QStringLiteral("Conducteur"));
	QCOMPARE(label(dock, "m_context_summary")->text(),
		QStringLiteral("1 objet sélectionné"));
	QScrollArea *scroll = editorScroll(dock);
	QVERIFY(scroll);
	QVERIFY(scroll->widget()->isAncestorOf(editor));
	QVERIFY(scroll->verticalScrollBar()->maximum() > 0);
	dock.close();
}

void PropertiesEditorDockWidgetTest::removalAndClearAreIdempotent()
{
	PropertiesEditorDockWidget dock;
	auto removed_editor = new ProbeEditor;
	QVERIFY(dock.addEditor(removed_editor));
	dock.showEditorContext(QStringLiteral("Texte"), QStringLiteral("Sélection"));
	QVERIFY(dock.removeEditor(removed_editor));
	QVERIFY(dock.editors().isEmpty());
	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Message);
	QVERIFY(!dock.removeEditor(removed_editor));
	QVERIFY(removed_editor->parentWidget());
	delete removed_editor;

	auto deleted_by_clear = new ProbeEditor;
	QPointer<ProbeEditor> guarded_editor(deleted_by_clear);
	QVERIFY(dock.addEditor(deleted_by_clear));
	dock.showEditorContext(QStringLiteral("Forme"), QStringLiteral("Sélection"));
	dock.clear();
	QVERIFY(guarded_editor.isNull());
	QVERIFY(dock.editors().isEmpty());
	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Message);
	dock.clear();
	QVERIFY(dock.editors().isEmpty());
	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Message);
}

void PropertiesEditorDockWidgetTest::doesNotDuplicateEditorsAndDelegatesApplyReset()
{
	PropertiesEditorDockWidget dock;
	auto first = new ProbeEditor;
	auto second = new ProbeEditor;
	QVERIFY(dock.addEditor(first));
	QVERIFY(dock.addEditor(first));
	QCOMPARE(dock.editors().count(first), 1);
	QVERIFY(dock.addEditor(second));
	QCOMPARE(dock.editors().size(), 2);

	dock.apply();
	dock.reset();
	QCOMPARE(first->apply_count, 1);
	QCOMPARE(first->reset_count, 1);
	QCOMPARE(second->apply_count, 1);
	QCOMPARE(second->reset_count, 1);

	QVERIFY(dock.removeEditor(first));
	dock.apply();
	dock.reset();
	QCOMPARE(first->apply_count, 1);
	QCOMPARE(first->reset_count, 1);
	QCOMPARE(second->apply_count, 2);
	QCOMPARE(second->reset_count, 2);
	delete first;
}

void PropertiesEditorDockWidgetTest::changingContextDoesNotStealFocus()
{
	QMainWindow window;
	auto canvas_focus = new QLineEdit(&window);
	canvas_focus->setObjectName(QStringLiteral("canvasFocus"));
	window.setCentralWidget(canvas_focus);
	auto dock = new PropertiesEditorDockWidget(&window);
	dock->setObjectName(QStringLiteral("propertiesDockUnderTest"));
	window.addDockWidget(Qt::RightDockWidgetArea, dock);
	window.resize(900, 600);
	window.show();
	window.activateWindow();
	canvas_focus->setFocus(Qt::OtherFocusReason);
	QTRY_VERIFY(canvas_focus->hasFocus());

	dock->showMessage(
		QStringLiteral("Propriétés"),
		QStringLiteral("Aucune sélection"),
		QStringLiteral("Sélectionnez un objet"),
		QStringLiteral("Les propriétés apparaîtront ici."));
	QApplication::processEvents();
	QCOMPARE(QApplication::focusWidget(), canvas_focus);

	auto editor = new ProbeEditor;
	QVERIFY(dock->addEditor(editor));
	dock->showEditorContext(
		QStringLiteral("Élément"),
		QStringLiteral("1 objet sélectionné"));
	QApplication::processEvents();
	QCOMPARE(QApplication::focusWidget(), canvas_focus);
	window.close();
}

void PropertiesEditorDockWidgetTest::remainsUsableNarrowWithLargeText()
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

	PropertiesEditorDockWidget dock;
	auto editor = new ProbeEditor(true);
	QVERIFY(dock.addEditor(editor));
	dock.showEditorContext(
		QStringLiteral("Propriétés d'une sélection industrielle"),
		QStringLiteral("Plusieurs valeurs détaillées restent accessibles au clavier"));
	dock.resize(320, 600);
	dock.show();
	QApplication::processEvents();

	const QSize logical_budget(360, 680);
	QVERIFY2(dock.minimumSizeHint().width() <= logical_budget.width(),
		"The properties dock must remain usable at 150% scaling");
	QVERIFY2(dock.minimumSizeHint().height() <= logical_budget.height(),
		"The properties dock must remain usable at 150% scaling");
	QVERIFY(dock.width() <= logical_budget.width());
	QVERIFY(dock.height() <= logical_budget.height());
	QScrollArea *scroll = editorScroll(dock);
	QVERIFY(scroll);
	QVERIFY(scroll->verticalScrollBar()->maximum() > 0);
	QCOMPARE(scroll->horizontalScrollBar()->maximum(), 0);

	editor->firstField()->setFocus(Qt::OtherFocusReason);
	QTRY_VERIFY(editor->firstField()->hasFocus());
	QTest::keyClick(editor->firstField(), Qt::Key_Tab);
	QTRY_COMPARE(QApplication::focusWidget(), editor->lastField());
	QVERIFY(scroll->verticalScrollBar()->value() > 0);
	dock.close();
}

void PropertiesEditorDockWidgetTest::diagramStatesAndAllowedAreas()
{
	DiagramPropertiesEditorDockWidget dock;
	QCOMPARE(dock.allowedAreas(), Qt::AllDockWidgetAreas);
	QCOMPARE(dock.state(), DiagramPropertiesEditorDockWidget::State::NoDiagram);
	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Message);
	QCOMPARE(label(dock, "m_context_summary")->text(),
		QStringLiteral("Aucun folio actif"));

	QGraphicsScene scene;
	dock.setGraphicsScene(&scene);
	QCOMPARE(dock.state(), DiagramPropertiesEditorDockWidget::State::NoSelection);
	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Message);
	QCOMPARE(label(dock, "m_context_summary")->text(),
		QStringLiteral("Aucune sélection"));
	dock.setGraphicsScene(nullptr);
}

void PropertiesEditorDockWidgetTest::switchesDiagramSourceWithoutStaleStateOrFocusLoss()
{
	QGraphicsScene scene_a;
	QGraphicsScene scene_b;
	auto item_a = new ProbeGraphicsItem(QGraphicsItem::UserType + 101);
	scene_a.addItem(item_a);
	item_a->setSelected(true);

	QMainWindow window;
	auto canvas_focus = new QLineEdit(&window);
	window.setCentralWidget(canvas_focus);
	auto dock = new DiagramPropertiesEditorDockWidget(&window);
	window.addDockWidget(Qt::RightDockWidgetArea, dock);
	window.resize(900, 600);
	window.show();
	window.activateWindow();
	canvas_focus->setFocus(Qt::OtherFocusReason);
	QTRY_VERIFY(canvas_focus->hasFocus());

	dock->setGraphicsScene(&scene_a);
	QCOMPARE(dock->state(),
		DiagramPropertiesEditorDockWidget::State::UnsupportedSelection);
	QCOMPARE(QApplication::focusWidget(), canvas_focus);

	dock->setGraphicsScene(&scene_b);
	QCOMPARE(dock->state(), DiagramPropertiesEditorDockWidget::State::NoSelection);
	QCOMPARE(QApplication::focusWidget(), canvas_focus);

	item_a->setSelected(false);
	QApplication::processEvents();
	QCOMPARE(dock->state(), DiagramPropertiesEditorDockWidget::State::NoSelection);
	QCOMPARE(QApplication::focusWidget(), canvas_focus);
	dock->setGraphicsScene(nullptr);
	window.close();
}

void PropertiesEditorDockWidgetTest::reportsUnsupportedAndMixedSelections()
{
	QGraphicsScene scene;
	auto first = new ProbeGraphicsItem(QGraphicsItem::UserType + 201);
	scene.addItem(first);
	first->setSelected(true);

	DiagramPropertiesEditorDockWidget dock;
	dock.setGraphicsScene(&scene);
	QCOMPARE(dock.state(),
		DiagramPropertiesEditorDockWidget::State::UnsupportedSelection);
	QCOMPARE(label(dock, "m_message_title")->text(),
		QStringLiteral("Propriétés non disponibles"));
	QCOMPARE(label(dock, "m_context_summary")->text(),
		QStringLiteral("1 objet sélectionné"));

	auto second = new ProbeGraphicsItem(QGraphicsItem::UserType + 202);
	scene.addItem(second);
	second->setSelected(true);
	QTRY_COMPARE(label(dock, "m_message_title")->text(),
		QStringLiteral("Sélection mixte"));
	QCOMPARE(dock.state(),
		DiagramPropertiesEditorDockWidget::State::UnsupportedSelection);
	QCOMPARE(label(dock, "m_context_summary")->text(),
		QStringLiteral("2 objets sélectionnés"));
	QVERIFY(dock.accessibleDescription().contains(
		QStringLiteral("2 objets sélectionnés")));
	dock.setGraphicsScene(nullptr);
}

void PropertiesEditorDockWidgetTest::deferredSceneDestructionClearsContext()
{
	DiagramPropertiesEditorDockWidget dock;
	auto scene = new QGraphicsScene;
	QPointer<QGraphicsScene> guarded_scene(scene);
	dock.setGraphicsScene(scene);
	QCOMPARE(dock.state(), DiagramPropertiesEditorDockWidget::State::NoSelection);

	scene->deleteLater();
	QTRY_VERIFY(guarded_scene.isNull());
	QCOMPARE(dock.state(), DiagramPropertiesEditorDockWidget::State::NoDiagram);
	QCOMPARE(dock.view(), PropertiesEditorDockWidget::View::Message);
	QCOMPARE(label(dock, "m_context_summary")->text(),
		QStringLiteral("Aucun folio actif"));
	QVERIFY(dock.editors().isEmpty());
}

void PropertiesEditorDockWidgetTest::reusesAndReplacesLiveEditor()
{
	QGraphicsScene scene;
	auto first_item = new ProbeGraphicsItem(ProbeFactoryTypeA);
	scene.addItem(first_item);
	first_item->setSelected(true);

	DiagramPropertiesEditorDockWidget dock;
	dock.setGraphicsScene(&scene);
	QCOMPARE(dock.state(), DiagramPropertiesEditorDockWidget::State::Editor);
	QCOMPARE(dock.editors().size(), 1);
	auto first_editor = dynamic_cast<ProbeEditor *>(dock.editors().first());
	QVERIFY(first_editor);
	QVERIFY(first_editor->isLiveEdit());
	QCOMPARE(first_editor->factoryType(), ProbeFactoryTypeA);
	QPointer<ProbeEditor> guarded_first_editor(first_editor);

	auto same_type_item = new ProbeGraphicsItem(ProbeFactoryTypeA);
	scene.addItem(same_type_item);
	first_item->setSelected(false);
	same_type_item->setSelected(true);
	dock.selectionChanged();
	QVERIFY(first_editor->reuse_count > 0);
	QCOMPARE(dock.state(), DiagramPropertiesEditorDockWidget::State::Editor);
	QCOMPARE(dock.editors().first(), first_editor);
	QVERIFY(first_editor->isLiveEdit());

	auto other_type_item = new ProbeGraphicsItem(ProbeFactoryTypeB);
	scene.addItem(other_type_item);
	same_type_item->setSelected(false);
	other_type_item->setSelected(true);
	dock.selectionChanged();
	QVERIFY(guarded_first_editor.isNull());
	QCOMPARE(dock.state(), DiagramPropertiesEditorDockWidget::State::Editor);
	QCOMPARE(dock.editors().size(), 1);
	auto replacement = dynamic_cast<ProbeEditor *>(dock.editors().first());
	QVERIFY(replacement);
	QCOMPARE(replacement->factoryType(), ProbeFactoryTypeB);
	QVERIFY(replacement->isLiveEdit());
	dock.setGraphicsScene(nullptr);
}

void PropertiesEditorDockWidgetTest::restoresDockAreaThroughMainWindowState()
{
	QMainWindow window;
	auto dock = new DiagramPropertiesEditorDockWidget(&window);
	dock->setObjectName(QStringLiteral("diagram_properties_editor_dock_widget"));
	window.addDockWidget(Qt::TopDockWidgetArea, dock);
	QCOMPARE(window.dockWidgetArea(dock), Qt::TopDockWidgetArea);
	const QByteArray top_state = window.saveState();
	QVERIFY(!top_state.isEmpty());

	window.addDockWidget(Qt::BottomDockWidgetArea, dock);
	QCOMPARE(window.dockWidgetArea(dock), Qt::BottomDockWidgetArea);
	const QByteArray bottom_state = window.saveState();
	QVERIFY(!bottom_state.isEmpty());

	QVERIFY(window.restoreState(top_state));
	QCOMPARE(window.dockWidgetArea(dock), Qt::TopDockWidgetArea);
	QVERIFY(window.restoreState(bottom_state));
	QCOMPARE(window.dockWidgetArea(dock), Qt::BottomDockWidgetArea);
	QVERIFY(window.restoreState(top_state));
	QCOMPARE(window.dockWidgetArea(dock), Qt::TopDockWidgetArea);
}

QTEST_MAIN(PropertiesEditorDockWidgetTest)

#include "tst_propertieseditordockwidget.moc"
