/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "diagrampropertieseditordockwidget.h"

#include "../PropertiesEditor/propertieseditorwidget.h"
#include "../diagram.h"
#include "../factory/propertieseditorfactory.h"

#include <QGraphicsItem>
#include <QGraphicsScene>

/**
	@brief DiagramPropertiesEditorDockWidget::DiagramPropertiesEditorDockWidget
	Constructor
	@param parent : parent widget
*/
DiagramPropertiesEditorDockWidget::DiagramPropertiesEditorDockWidget(QWidget *parent) :
	PropertiesEditorDockWidget(parent)
{
	setFeatures(
		QDockWidget::DockWidgetClosable
		| QDockWidget::DockWidgetMovable
		| QDockWidget::DockWidgetFloatable);
	showNoDiagramState();
}

/**
	@brief DiagramPropertiesEditorDockWidget::setDiagram
	Set the diagram to edit the selection.
	Connect the diagram signal selectionChanged() to this slot selectionChanged();
	If diagram = nullptr, we just disconnect all signal and remove editor.
	@param diagram
*/
void DiagramPropertiesEditorDockWidget::setDiagram(Diagram *diagram)
{
	setGraphicsScene(diagram);
}

void DiagramPropertiesEditorDockWidget::setGraphicsScene(QGraphicsScene *scene)
{
	if (m_scene == scene) return;

	if (m_scene)
	{
		disconnect(
			m_scene.data(),
			&QGraphicsScene::selectionChanged,
			this,
			&DiagramPropertiesEditorDockWidget::selectionChanged);
		disconnect(
			m_scene.data(),
			&QObject::destroyed,
			this,
			&DiagramPropertiesEditorDockWidget::diagramWasDeleted);
	}

	clear();
	m_scene = scene;
	if (scene)
	{
		connect(
			m_scene.data(),
			&QGraphicsScene::selectionChanged,
			this,
			&DiagramPropertiesEditorDockWidget::selectionChanged,
			Qt::QueuedConnection);
		connect(
			m_scene.data(),
			&QObject::destroyed,
			this,
			&DiagramPropertiesEditorDockWidget::diagramWasDeleted);
		selectionChanged();
	}
	else
	{
		showNoDiagramState();
	}
}

DiagramPropertiesEditorDockWidget::State
DiagramPropertiesEditorDockWidget::state() const
{
	return m_state;
}

/**
	@brief DiagramPropertiesEditorDockWidget::selectionChanged
	The current selection of diagram was changed.
	We fill the dock with the appropriate ElementPropertiesWidget of the current selection.
*/
void DiagramPropertiesEditorDockWidget::selectionChanged()
{
	if (!m_scene) {
		clear();
		showNoDiagramState();
		return;
	}

	const QList<QGraphicsItem *> selected_items = m_scene->selectedItems();
	if (selected_items.isEmpty()) {
		clear();
		showNoSelectionState();
		return;
	}

	auto editor_ = PropertiesEditorFactory::propertiesEditor(
				selected_items,
				editors().count() ? editors().first() : nullptr,
				this);
	if (!editor_) {
		bool mixed_types = false;
		const int first_type = selected_items.first()->type();
		for (QGraphicsItem *item : selected_items) {
			if (item->type() != first_type) {
				mixed_types = true;
				break;
			}
		}
		clear();
		showUnsupportedSelectionState(selected_items.count(), mixed_types);
		return;
	}
	if (editors().count() &&
		editors().first() != editor_) {
		clear();
	}

	addEditor(editor_);
	for (PropertiesEditorWidget *pew : editors()) {
		pew->setLiveEdit(true);
	}
	const QString requested_title = editor_->title().trimmed();
	const QString editor_title = requested_title.isEmpty()
		? tr("Propriétés de la sélection")
		: requested_title;
	showEditorContext(editor_title, selectionSummary(selected_items.count()));
	m_state = State::Editor;
}

/**
	@brief DiagramPropertiesEditorDockWidget::diagramWasDeleted
	Remove current editor and set m_diagram to nullptr.
*/
void DiagramPropertiesEditorDockWidget::diagramWasDeleted()
{
	m_scene = nullptr;
	clear();
	showNoDiagramState();
}

void DiagramPropertiesEditorDockWidget::showNoDiagramState()
{
	showMessage(
		tr("Propriétés"),
		tr("Aucun folio actif"),
		tr("Ouvrez un folio"),
		tr("Ouvrez ou créez un folio pour afficher ses propriétés de sélection."));
	m_state = State::NoDiagram;
}

void DiagramPropertiesEditorDockWidget::showNoSelectionState()
{
	showMessage(
		tr("Propriétés"),
		tr("Aucune sélection"),
		tr("Sélectionnez un objet"),
		tr("Cliquez sur un élément, un conducteur, un texte ou une forme dans le folio."));
	m_state = State::NoSelection;
}

void DiagramPropertiesEditorDockWidget::showUnsupportedSelectionState(
	int count,
	bool mixed_types)
{
	showMessage(
		tr("Propriétés"),
		selectionSummary(count),
		mixed_types
			? tr("Sélection mixte")
			: tr("Propriétés non disponibles"),
		mixed_types
			? tr("Sélectionnez des objets du même type pour les modifier ensemble.")
			: tr("Cet objet ne peut pas encore être modifié dans ce panneau."));
	m_state = State::UnsupportedSelection;
}

QString DiagramPropertiesEditorDockWidget::selectionSummary(int count) const
{
	return count == 1
		? tr("1 objet sélectionné")
		: tr("%1 objets sélectionnés").arg(count);
}
