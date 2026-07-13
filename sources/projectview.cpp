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
#include "projectview.h"

#include "ElementsCollection/xmlelementcollection.h"
#include "autoNum/assignvariables.h"
#include "diagram.h"
#include "diagramview.h"
#include "editor/ui/qetelementeditor.h"
#include "exportdialog.h"
#include "qetapp.h"
#include "qeticons.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "titleblock/qettemplateeditor.h"
#include "ui/borderpropertieswidget.h"
#include "ui/conductorpropertieswidget.h"
#include "ui/dialogwaiting.h"
#include "ui/folionavigatordialog.h"
#include "ui/projectpropertiesdialog.h"
#include "ui/titleblockpropertieswidget.h"

#include <QTimer>

/**
	Constructeur
	@param project projet a visualiser
	@param parent Widget parent
*/
ProjectView::ProjectView(QETProject *project, QWidget *parent) :
	QWidget(parent),
	m_project(nullptr)
{
	initActions();
	initWidgets();
	initLayout();

	setProject(project);
}

/**
	Destructeur
	Supprime les DiagramView embarquees
*/
ProjectView::~ProjectView()
{
	for (auto dv_ : m_diagram_ids.values())
		dv_->deleteLater();
}

/**
	@return le projet actuellement visualise par le ProjectView
*/
QETProject *ProjectView::project()
{
	return(m_project);
}

/**
	@brief ProjectView::setProject
	Set the project display by the project view
	@param project
*/
void ProjectView::setProject(QETProject *project)
{
	if (!m_project)
	{
		m_project = project;
		connect(m_project, &QETProject::projectTitleChanged, this, &ProjectView::updateWindowTitle);
		connect(m_project, &QETProject::projectModified, this, &ProjectView::updateWindowTitle);
		connect(m_project, &QETProject::readOnlyChanged, this, &ProjectView::adjustReadOnlyState);
		connect(m_project, &QETProject::addAutoNumDiagram, [this](){this->project()->addNewDiagram();});

		connect(m_project, &QETProject::diagramAdded, [this](QETProject *project, Diagram *diagram) {
			Q_UNUSED(project)
			this->diagramAdded(diagram);
		});

		adjustReadOnlyState();
		loadDiagrams();
	}
}

/**
	@return la liste des schemas ouverts dans le projet
*/
QList<DiagramView *> ProjectView::diagram_views() const
{
	return(m_diagram_view_list);
}

/**
	@brief ProjectView::currentDiagram
	@return The current active diagram view or nullptr if there isn't diagramView in this project view.
*/
DiagramView *ProjectView::currentDiagram() const
{
	return qobject_cast<DiagramView *>(m_tab->currentWidget());
}

/**
	Gere la fermeture du schema.
	@param qce Le QCloseEvent decrivant l'evenement
*/
void ProjectView::closeEvent(QCloseEvent *qce) {
	bool can_close_project = tryClosing();
	if (can_close_project) {
		qce -> accept();
		emit(projectClosed(this));
	} else {
		qce -> ignore();
	}
}

/**
	@brief change current diagramview to next folio
*/
void ProjectView::changeTabDown()
{
	DiagramView *nextDiagramView = this->nextDiagram();
	if (nextDiagramView!=nullptr){
		m_tab -> setCurrentWidget(nextDiagramView);
	}
}

/**
	@return next folio of current diagramview
*/
DiagramView *ProjectView::nextDiagram()
{
	int current_tab_index = m_tab -> currentIndex();
	int next_tab_index = current_tab_index + 1;	//get next tab index
	if (next_tab_index < m_tab->count()) //if next tab index >= greatest tab the last tab is activated so no need to change tab.
		return qobject_cast<DiagramView *>(m_tab->widget(next_tab_index));
	else
		return nullptr;
}

/**
	@brief change current diagramview to previous tab
*/
void ProjectView::changeTabUp()
{
	DiagramView *previousDiagramView = this->previousDiagram();
	if (previousDiagramView!=nullptr){
		m_tab -> setCurrentWidget(previousDiagramView);
	}
}

/**
	@return previous folio of current diagramview
*/
DiagramView *ProjectView::previousDiagram()
{
	int current_tab_index = m_tab -> currentIndex();
	int previous_tab_index = current_tab_index - 1;	//get previous tab index
	if (previous_tab_index>=0) //if previous tab index = 0 then the first tab is activated so no need to change tab.
		return qobject_cast<DiagramView *>(m_tab->widget(previous_tab_index));
	else
		return nullptr;
}

/**
	@brief change current diagramview to last tab
*/
void ProjectView::changeLastTab()
{
	DiagramView *lastDiagramView = this->lastDiagram();
	if (lastDiagramView) {
		m_tab->setCurrentWidget(lastDiagramView);
	}
}

/**
	@return last folio of current project
*/
DiagramView *ProjectView::lastDiagram()
{
	return m_tab->count() == 0
			? nullptr
			: qobject_cast<DiagramView *>(m_tab->widget(m_tab->count() - 1));
}

/**
	@brief change current diagramview to first tab
*/
void ProjectView::changeFirstTab()
{
	DiagramView *firstDiagramView = this->firstDiagram();
	if (firstDiagramView) {
		m_tab->setCurrentWidget(firstDiagramView);
	}
}

/**
	@return first folio of current project
*/
DiagramView *ProjectView::firstDiagram()
{
	return m_tab->count() == 0
			? nullptr
			: qobject_cast<DiagramView *>(m_tab->widget(0));
}

void ProjectView::openFolioNavigator()
{
	if (!m_project) {
		return;
	}
	if (!m_folio_navigator)
	{
		m_folio_navigator = new FolioNavigatorDialog(this);
		connect(m_folio_navigator, &FolioNavigatorDialog::folioActivated,
				this, &ProjectView::activateFolio);
		connect(m_folio_navigator, &FolioNavigatorDialog::favoriteChanged,
				this, &ProjectView::setFolioFavorite);
		connect(m_folio_navigator, &FolioNavigatorDialog::navigateBackRequested,
				this, &ProjectView::navigateHistoryBack);
		connect(m_folio_navigator, &FolioNavigatorDialog::navigateForwardRequested,
				this, &ProjectView::navigateHistoryForward);
	}
	refreshFolioNavigator(false);
}

void ProjectView::refreshFolioNavigator(bool preserve_filters)
{
	if (!m_project || !m_folio_navigator) {
		return;
	}
	DiagramView *current = currentDiagram();
	const QUuid active_id = current && current->diagram()
			? current->diagram()->uuid() : QUuid();
	m_folio_navigator->openForProject(
			m_project->title(),
			folioNavigationEntries(),
			active_id,
			m_history_back->isEnabled(),
			m_history_forward->isEnabled(),
			preserve_filters);
}

void ProjectView::scheduleFolioNavigatorRefresh()
{
	if (!m_folio_navigator
			|| !m_folio_navigator->isVisible()
			|| m_folio_refresh_scheduled) {
		return;
	}
	m_folio_refresh_scheduled = true;
	QTimer::singleShot(0, this, [this]() {
		m_folio_refresh_scheduled = false;
		if (m_folio_navigator && m_folio_navigator->isVisible()) {
			refreshFolioNavigator(true);
		}
	});
}

void ProjectView::navigateHistoryBack()
{
	pruneNavigationHistory();
	for (int index = m_navigation_history_index - 1; index >= 0; --index)
	{
		DiagramView *diagram_view = m_navigation_history.at(index);
		if (diagram_view && m_diagram_indexes.contains(diagram_view))
		{
			m_navigation_history_index = index;
			m_navigating_history = true;
			m_tab->setCurrentWidget(diagram_view);
			updateNavigationHistoryActions();
			return;
		}
	}
}

void ProjectView::navigateHistoryForward()
{
	pruneNavigationHistory();
	for (int index = m_navigation_history_index + 1;
			 index < m_navigation_history.size(); ++index)
	{
		DiagramView *diagram_view = m_navigation_history.at(index);
		if (diagram_view && m_diagram_indexes.contains(diagram_view))
		{
			m_navigation_history_index = index;
			m_navigating_history = true;
			m_tab->setCurrentWidget(diagram_view);
			updateNavigationHistoryActions();
			return;
		}
	}
}

void ProjectView::recordNavigation(DiagramView *diagram_view)
{
	if (!diagram_view) {
		return;
	}
	pruneNavigationHistory();
	if (m_navigating_history)
	{
		m_navigating_history = false;
		updateNavigationHistoryActions();
		return;
	}
	if (m_navigation_history_index >= 0
			&& m_navigation_history_index < m_navigation_history.size()
			&& m_navigation_history.at(m_navigation_history_index) == diagram_view) {
		updateNavigationHistoryActions();
		return;
	}
	while (m_navigation_history.size() > m_navigation_history_index + 1) {
		m_navigation_history.removeLast();
	}
	m_navigation_history.append(QPointer<DiagramView>(diagram_view));
	while (m_navigation_history.size() > 50) {
		m_navigation_history.removeFirst();
	}
	m_navigation_history_index = m_navigation_history.size() - 1;
	updateNavigationHistoryActions();
}

void ProjectView::pruneNavigationHistory()
{
	QList<QPointer<DiagramView>> valid_history;
	int new_index = -1;
	for (int index = 0; index < m_navigation_history.size(); ++index)
	{
		DiagramView *diagram_view = m_navigation_history.at(index);
		if (!diagram_view || !m_diagram_indexes.contains(diagram_view)) {
			continue;
		}
		valid_history.append(diagram_view);
		if (index <= m_navigation_history_index) {
			new_index = valid_history.size() - 1;
		}
	}
	m_navigation_history = valid_history;
	m_navigation_history_index = m_navigation_history.isEmpty()
			? -1 : qBound(0, new_index, m_navigation_history.size() - 1);
}

void ProjectView::updateNavigationHistoryActions()
{
	pruneNavigationHistory();
	m_history_back->setEnabled(m_navigation_history_index > 0);
	m_history_forward->setEnabled(
			m_navigation_history_index >= 0
			&& m_navigation_history_index < m_navigation_history.size() - 1);
}

QVector<FolioNavigationEntry> ProjectView::folioNavigationEntries() const
{
	QVector<FolioNavigationEntry> entries;
	if (!m_project) {
		return entries;
	}

	QHash<QUuid, int> recent_ranks;
	int recent_rank = 0;
	for (int index = m_navigation_history.size() - 1;
			 index >= 0 && recent_rank < 12; --index)
	{
		DiagramView *diagram_view = m_navigation_history.at(index);
		if (!diagram_view || !diagram_view->diagram()) {
			continue;
		}
		const QUuid id = diagram_view->diagram()->uuid();
		if (!recent_ranks.contains(id)) {
			recent_ranks.insert(id, recent_rank++);
		}
	}

	DiagramView *active_view = currentDiagram();
	const QList<Diagram *> diagrams = m_project->diagrams();
	entries.reserve(diagrams.size());
	for (int position = 0; position < diagrams.size(); ++position)
	{
		Diagram *diagram = diagrams.at(position);
		if (!diagram) {
			continue;
		}
		FolioNavigationEntry entry;
		entry.diagram_id = diagram->uuid();
		entry.position = position;
		entry.folio_label = diagram->border_and_titleblock.finalfolio();
		entry.raw_folio_label = diagram->border_and_titleblock.folio();
		entry.title = diagram->title();
		entry.plant = diagram->border_and_titleblock.plant();
		entry.location = diagram->border_and_titleblock.locmach();
		entry.file_name = diagram->border_and_titleblock.fileName();
		const DiagramContext context =
				diagram->border_and_titleblock.additionalFields();
		for (const QString &key : context.keys()) {
			entry.additional_fields.append(
					key + QLatin1Char(' ') + context.value(key).toString());
		}
		QStringList group_parts;
		if (!entry.plant.trimmed().isEmpty()) {
			group_parts << entry.plant.trimmed();
		}
		if (!entry.location.trimmed().isEmpty()) {
			group_parts << entry.location.trimmed();
		}
		entry.group = group_parts.isEmpty()
				? tr("Sans installation ni localisation")
				: group_parts.join(QStringLiteral(" / "));
		entry.active = active_view && active_view->diagram() == diagram;
		entry.favorite = m_favorite_folios.contains(entry.diagram_id);
		entry.recent_rank = recent_ranks.value(entry.diagram_id, -1);
		entries.append(entry);
	}
	return entries;
}

void ProjectView::activateFolio(const QUuid &diagram_id)
{
	if (diagram_id.isNull()) {
		return;
	}
	if (DiagramView *diagram_view = m_diagram_views_by_id.value(diagram_id)) {
		showDiagram(diagram_view);
	}
}

void ProjectView::setFolioFavorite(
		const QUuid &diagram_id,
		bool favorite)
{
	if (diagram_id.isNull()) {
		return;
	}
	if (favorite) {
		m_favorite_folios.insert(diagram_id);
	} else {
		m_favorite_folios.remove(diagram_id);
	}
}


/**
	Cette methode essaye de fermer successivement les editeurs d'element puis
	les schemas du projet. L'utilisateur peut refuser de fermer un schema ou un
	editeur.
	@return true si tout a pu etre ferme, false sinon
	@see tryClosingElementEditors()
	@see tryClosingDiagrams()
*/
bool ProjectView::tryClosing()
{
	if (!m_project) return(true);

	// First step: require external editors closing -- users may either cancel
	// the whole closing process or save (and therefore add) content into this
	// project. Of course, they may also discard them.
	if (!tryClosingElementEditors()) {
		return(false);
	}

	// Check how different the current situation is from a brand new, untouched project
	if (m_project -> filePath().isEmpty() && !m_project -> projectWasModified()) {
		return(true);
	}

	// Second step: users are presented with a dialog that enables them to
	// choose whether they want to:
	//   - cancel the closing process,
	//   - discard all modifications,
	//   - or specify what is to be saved, i.e. they choose whether they wants to
	// save/give up/remove diagrams considered as modified.
	int user_input = tryClosingDiagrams();
	if (user_input == QMessageBox::Cancel) {
		return(false); // the closing process was cancelled
	} else if (user_input == QMessageBox::Discard) {
		return(true); // all modifications were discarded
	}

	// Check how different the current situation is from a brand new, untouched project (yes , again)
	if (m_project -> filePath().isEmpty() && !m_project -> projectWasModified()) {
		return(true);
	}

	if (m_project -> filePath().isEmpty()) {
		QString filepath = askUserForFilePath(false);
		if (filepath.isEmpty()) return(false); // users may cancel the closing
		QETResult result = m_project -> write(filepath);
		updateWindowTitle();
		if (!result.isOk()) emit(errorEncountered(result.errorMessage()));
		return(result.isOk());
	}
	QETResult result = m_project -> write();
	updateWindowTitle();
	if (!result.isOk()) emit(errorEncountered(result.errorMessage()));
	return(result.isOk());
}

/**
	Un projet comporte des elements integres. Cette methode ferme les editeurs
	d'elements associes a ce projet. L'utilisateur peut refuser la fermeture
	d'un editeur d'element.
	@return true si tous les editeurs d'element ont pu etre fermes, false sinon
*/
bool ProjectView::tryClosingElementEditors()
{
	if (!m_project) return(true);
	/*
		La QETApp permet d'acceder rapidement aux editeurs d'element
		editant un element du projet.
	*/
	QList<QETElementEditor *> editors = QETApp::elementEditors(m_project);
	foreach(QETElementEditor *editor, editors) {
		if (!editor -> close()) return(false);
	}

	QList<QETTitleBlockTemplateEditor *> template_editors = QETApp::titleBlockTemplateEditors(m_project);
	foreach(QETTitleBlockTemplateEditor *template_editor, template_editors) {
		if (!template_editor -> close()) return(false);
	}
	return(true);
}

/**
	@brief ProjectView::tryClosingDiagrams
	try to close this project, if diagram or project option are changed
	a dialog ask if user want to save the modification.
	@return the answer of dialog or discard if no change.
*/
int ProjectView::tryClosingDiagrams()
{
	if (!m_project) return(QMessageBox::Discard);

	if (!project()->projectOptionsWereModified() &&
		project()->undoStack()->isClean() &&
		!project()->filePath().isEmpty()) {
		// nothing was modified, and we have a filepath, i.e. everything was already
		// saved, i.e we can close the project right now
		return(QMessageBox::Discard);
	}

	QString title = project()->title();
	if (title.isEmpty()) title = "QElectroTech ";

	int close_dialog = QMessageBox::question(this, title,
								   tr("Le projet à été modifié.\n"
									  "Voulez-vous enregistrer les modifications ?"),
								   QMessageBox::Save | QMessageBox::Discard
								   | QMessageBox::Cancel,
								   QMessageBox::Save);

	return(close_dialog);
}

/**
	Ask the user to provide a file path in which the currently edited project will
	be saved.
	@param assign When true, assign the provided filepath to the project through
	setFilePath(). Defaults to true.
	@return the file path, or an empty string if none were provided
*/
QString ProjectView::askUserForFilePath(bool assign) {
	// ask the user for a filepath in order to save the project
	QString filepath = QFileDialog::getSaveFileName(
		this,
		tr("Enregistrer sous", "dialog title"),
		m_project -> currentDir() + "/" + tr("sansnom") + ".qet",
		tr("Projet QElectroTech (*.qet)", "filetypes allowed when saving a project file")
	);

	// if no filepath is provided, return an empty string
	if (filepath.isEmpty()) return(filepath);

	// if the name does not end with the .qet extension and we're _not_ using xdg-desktop-portal, append it
	bool usesPortal = 
		qEnvironmentVariableIsSet("FLATPAK_ID") || 
		qEnvironmentVariableIsSet("SNAP_NAME");
	if (!filepath.endsWith(".qet", Qt::CaseInsensitive) && !usesPortal) filepath += ".qet";

	if (assign) {
		// assign the provided filepath to the currently edited project
		m_project -> setFilePath(filepath);
	}

	return(filepath);
}

/**
	@return the QETResult object to be returned when it appears this project
	view is not associated to any project.
*/
QETResult ProjectView::noProjectResult() const
{
	QETResult no_project(tr("aucun projet affiché", "error message"), false);
	return(no_project);
}

/**
 * @brief ProjectView::removeDiagram
 * Remove a diagram (folio) of the project
 * @param diagram_view : diagram view to remove
 * @param silent : if true, bypasses the confirmation message box
 */
void ProjectView::removeDiagram(DiagramView *diagram_view, bool silent)
{
	if (!diagram_view)
		return;
	if (m_project -> isReadOnly())
		return;
	if (!m_diagram_indexes.contains(diagram_view))
		return;

	if (!silent) {
		//Ask confirmation to user.
		int answer = QET::QetMessageBox::question(
			this,
			tr("Supprimer le folio ?", "message box title"),
												  tr("Êtes-vous sûr de vouloir supprimer ce folio du projet ? Ce changement est irréversible.", "message box content"),
												  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
											QMessageBox::No
		);
		if (answer != QMessageBox::Yes) {
			return;
		}
	}

	const QUuid removed_diagram_id = diagram_view->diagram()->uuid();

	//Remove the diagram view of the tabs widget
	int index_to_remove = diagramIndex(diagram_view);
	m_tab->removeTab(index_to_remove);
	m_diagram_view_list.removeAll(diagram_view);
	rebuildDiagramsMap();

	m_project -> removeDiagram(diagram_view -> diagram());
	delete diagram_view;
	m_favorite_folios.remove(removed_diagram_id);
	pruneNavigationHistory();
	updateNavigationHistoryActions();

	emit(diagramRemoved(diagram_view));
	updateAllTabsTitle();
	m_project -> setModified(true);
}

/**
 * Enleve un schema du ProjectView
 * @param diagram Schema a enlever
 * @param silent  Si vrai, supprime sans demander confirmation
 */
void ProjectView::removeDiagram(Diagram *diagram, bool silent) {
	if (!diagram) return;

	if (DiagramView *diagram_view = findDiagram(diagram)) {
		removeDiagram(diagram_view, silent);
	}
}

/**
	Active l'onglet adequat pour afficher le schema passe en parametre
	@param diagram Schema a afficher
*/
void ProjectView::showDiagram(DiagramView *diagram) {
	if (!diagram) return;
	m_tab -> setCurrentWidget(diagram);
}

/**
	Active l'onglet adequat pour afficher le schema passe en parametre
	@param diagram Schema a afficher
*/
void ProjectView::showDiagram(Diagram *diagram) {
	if (!diagram) return;
	if (DiagramView *diagram_view = findDiagram(diagram)) {
		m_tab -> setCurrentWidget(diagram_view);
	}
}

/**
	Enable the user to edit properties of the current project through a
	configuration dialog.
*/
void ProjectView::editProjectProperties()
{
	if (!m_project) return;
	ProjectPropertiesDialog dialog(m_project, parentWidget());
	dialog.exec();
}

/**
	Edite les proprietes du schema courant
*/
void ProjectView::editCurrentDiagramProperties()
{
	editDiagramProperties(currentDiagram());
}

/**
	Edite les proprietes du schema diagram_view
*/
void ProjectView::editDiagramProperties(DiagramView *diagram_view) {
	if (!diagram_view) return;
	showDiagram(diagram_view);
	diagram_view -> editDiagramProperties();
}

/**
	Edite les proprietes du schema diagram
*/
void ProjectView::editDiagramProperties(Diagram *diagram) {
	editDiagramProperties(findDiagram(diagram));
}

/**
	Deplace le schema diagram_view vers le haut / la gauche
*/
void ProjectView::moveDiagramUp(DiagramView *diagram_view) {
	if (!diagram_view) return;

	int diagram_view_position = diagramIndex(diagram_view);
	if (diagram_view_position <= 0) {
		// le schema est le premier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, diagram_view_position - 1);
}

/**
	Deplace le schema diagram vers le haut / la gauche
*/
void ProjectView::moveDiagramUp(Diagram *diagram) {
	moveDiagramUp(findDiagram(diagram));
}

/**
	Deplace le schema diagram_view vers le bas / la droite
*/
void ProjectView::moveDiagramDown(DiagramView *diagram_view) {
	if (!diagram_view) return;

	int diagram_view_position = diagramIndex(diagram_view);
	if (diagram_view_position < 0
			|| diagram_view_position + 1 == m_diagram_ids.count()) {
		// le schema est le dernier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, diagram_view_position + 1);
}

/**
	Deplace le schema diagram vers le bas / la droite
*/
void ProjectView::moveDiagramDown(Diagram *diagram) {
	moveDiagramDown(findDiagram(diagram));
}

/*
	Deplace le schema diagram_view vers le haut / la gauche en position 0
*/
void ProjectView::moveDiagramUpTop(DiagramView *diagram_view)
{
	if (!diagram_view) return;

	int diagram_view_position = diagramIndex(diagram_view);
	if (diagram_view_position <= 0) {
		// le schema est le premier du projet
		return;
	}
	m_tab->tabBar()->moveTab(diagram_view_position, 0);
}

/*
	Deplace le schema diagram vers le haut / la gauche en position 0
*/
void ProjectView::moveDiagramUpTop(Diagram *diagram)
{
	moveDiagramUpTop(findDiagram(diagram));
}

/**
	Deplace le schema diagram_view vers le haut / la gauche x10
*/
void ProjectView::moveDiagramUpx10(DiagramView *diagram_view) {
	if (!diagram_view) return;

	int diagram_view_position = diagramIndex(diagram_view);
	if (diagram_view_position <= 0) {
		// le schema est le premier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, diagram_view_position - 10);
}

/**
	Deplace le schema diagram vers le haut / la gauche x10
*/
void ProjectView::moveDiagramUpx10(Diagram *diagram) {
	moveDiagramUpx10(findDiagram(diagram));
}

/**
 * @brief ProjectView::moveDiagramUpx100
 * Moves the diagram_view up / left x100
 * @param diagram_view View to move
 */
void ProjectView::moveDiagramUpx100(DiagramView *diagram_view) {
	if (!diagram_view) return;

	int diagram_view_position = diagramIndex(diagram_view);
	if (diagram_view_position <= 0) {
		// The diagram is the first of the project
		return;
	}
	m_tab->tabBar()->moveTab(diagram_view_position, diagram_view_position - 100);
}

/**
 * @brief ProjectView::moveDiagramUpx100
 * Moves the diagram up / left x100
 * @param diagram Diagram to move
 */
void ProjectView::moveDiagramUpx100(Diagram *diagram) {
	moveDiagramUpx100(findDiagram(diagram));
}

/**
 * @brief ProjectView::moveDiagramDownx100
 * Moves the diagram_view down / right x100
 * @param diagram_view View to move
 */
void ProjectView::moveDiagramDownx100(DiagramView *diagram_view) {
	if (!diagram_view) return;

	int diagram_view_position = diagramIndex(diagram_view);
	if (diagram_view_position < 0
			|| diagram_view_position + 1 == m_diagram_ids.count()) {
		// The diagram is the last of the project
		return;
	}
	m_tab->tabBar()->moveTab(diagram_view_position, diagram_view_position + 100);
}

/**
 * @brief ProjectView::moveDiagramDownx100
 * Moves the diagram down / right x100
 * @param diagram Diagram to move
 */
void ProjectView::moveDiagramDownx100(Diagram *diagram) {
	moveDiagramDownx100(findDiagram(diagram));
}

/**
	Deplace le schema diagram_view vers le bas / la droite x10
*/
void ProjectView::moveDiagramDownx10(DiagramView *diagram_view) {
	if (!diagram_view) return;

	int diagram_view_position = diagramIndex(diagram_view);
	if (diagram_view_position < 0
			|| diagram_view_position + 1 == m_diagram_ids.count()) {
		// le schema est le dernier du projet
		return;
	}
	m_tab -> tabBar() -> moveTab(diagram_view_position, diagram_view_position + 10);
}

/**
	Deplace le schema diagram vers le bas / la droite x10
*/
void ProjectView::moveDiagramDownx10(Diagram *diagram) {
	moveDiagramDownx10(findDiagram(diagram));
}

/**
	Exporte le schema.
*/
void ProjectView::exportProject()
{
	if (!m_project) return;

	ExportDialog ed(m_project, parentWidget());
#ifdef Q_OS_MACOS
	ed.setWindowFlags(Qt::Sheet);
#endif
	ed.exec();
}

/**
	Save project properties along with all modified diagrams.
	@see filePath()
	@see setFilePath()
	@return a QETResult object reflecting the situation
*/
QETResult ProjectView::save()
{
	return(doSave());
}

/**
	Ask users for a filepath in order to save the project.
	@param options May be used to specify what should be saved; defaults to
	all modified diagrams.
	@return a QETResult object reflecting the situation, including cancellation.
*/
QETResult ProjectView::saveAs()
{
	if (!m_project) return(noProjectResult());

	QString filepath = askUserForFilePath(false);
	if (filepath.isEmpty()) return(QETResult::cancelled());
	return(doSave(filepath));
}

/**
	Save project content, then write the project file. May
	call saveAs if no filepath was provided before.
	@return a QETResult object reflecting the situation, including cancellation.
*/
QETResult ProjectView::doSave(const QString &file_path)
{
	if (!m_project) return(noProjectResult());

	if (m_project -> filePath().isEmpty() && file_path.isEmpty()) {
		// The project has not been saved to a file yet,
		// so save() actually means saveAs().
		return(saveAs());
	}

	// write to file
	emit saveStarted(m_project);
	QETResult result = file_path.isEmpty()
			? m_project -> write()
			: m_project -> write(file_path);
	updateWindowTitle();
	if (result.isOk()) {
		project()->undoStack()->clear();
	}
	emit saveFinished(m_project, result.isOk(), result.errorMessage());
	return(result);
}

/**
	Allow the user to clean the project, which includes:
	  * deleting unused title block templates
	  * deleting unused elements
	  * deleting empty categories
	@return an integer value above zero if elements and/or categories were
	cleaned.
*/
int ProjectView::cleanProject()
{
	if (!m_project) return(0);

	// s'assure que le schema n'est pas en lecture seule
	if (m_project -> isReadOnly()) {
		QET::QetMessageBox::critical(
			this,
			tr("Projet en lecture seule", "message box title"),
			tr("Ce projet est en lecture seule. Il n'est donc pas possible de le nettoyer.", "message box content")
		);
		return(0);
	}

	// construit un petit dialogue pour parametrer le nettoyage
	QCheckBox *clean_tbt		= new QCheckBox(tr("Supprimer les modèles de cartouche inutilisés dans le projet"));
	QCheckBox *clean_elements   = new QCheckBox(tr("Supprimer les éléments inutilisés dans le projet"));
	QCheckBox *clean_categories = new QCheckBox(tr("Supprimer les catégories vides"));
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	clean_tbt		-> setChecked(true);
	clean_elements   -> setChecked(true);
	clean_categories -> setChecked(true);

	QDialog clean_dialog(parentWidget());
#ifdef Q_OS_MACOS
	clean_dialog.setWindowFlags(Qt::Sheet);
#endif

	clean_dialog.setWindowTitle(tr("Nettoyer le projet", "window title"));
	QVBoxLayout *clean_dialog_layout = new QVBoxLayout();
	clean_dialog_layout -> addWidget(clean_tbt);
	clean_dialog_layout -> addWidget(clean_elements);
	clean_dialog_layout -> addWidget(clean_categories);
	clean_dialog_layout -> addWidget(buttons);
	clean_dialog.setLayout(clean_dialog_layout);

	connect(buttons, SIGNAL(accepted()), &clean_dialog, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), &clean_dialog, SLOT(reject()));

	int clean_count = 0;
	if (clean_dialog.exec() == QDialog::Accepted)
	{
		if (clean_tbt -> isChecked()) {
			m_project->embeddedTitleBlockTemplatesCollection()->deleteUnusedTitleBlocKTemplates();
		}
		if (clean_elements->isChecked()) {
			m_project->embeddedElementCollection()->cleanUnusedElement();
		}
		if (clean_categories->isChecked()) {
			m_project->embeddedElementCollection()->cleanUnusedDirectory();
		}
	}
	
	m_project -> setModified(true);
	return(clean_count);
}

/**
	Initialize actions for this widget.
*/
void ProjectView::initActions()
{
	m_add_new_diagram = new QAction(QET::Icons::AddFolio, tr("Ajouter un folio"), this);
	connect(m_add_new_diagram, &QAction::triggered, [this](){this->m_project->addNewDiagram();});

	m_history_back = new QAction(
			QET::Icons::ArrowLeft,
			tr("Folio précédent dans l’historique"),
			this);
	m_history_back->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
	m_history_back->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	m_history_back->setToolTip(tr(
			"Revenir au folio précédemment consulté (Alt+Gauche)"));
	m_history_back->setEnabled(false);
	addAction(m_history_back);
	connect(m_history_back, &QAction::triggered,
			this, &ProjectView::navigateHistoryBack);

	m_history_forward = new QAction(
			QET::Icons::ArrowRight,
			tr("Folio suivant dans l’historique"),
			this);
	m_history_forward->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Right));
	m_history_forward->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	m_history_forward->setToolTip(tr(
			"Avancer dans l’historique des folios (Alt+Droite)"));
	m_history_forward->setEnabled(false);
	addAction(m_history_forward);
	connect(m_history_forward, &QAction::triggered,
			this, &ProjectView::navigateHistoryForward);
	
	m_first_view = new QAction(QET::Icons::ArrowLeftDouble, tr("Revenir au debut du projet"),this);
	connect(m_first_view, &QAction::triggered, [this](){this->m_tab->setCurrentWidget(firstDiagram());});
	
	m_end_view = new QAction(QET::Icons::ArrowRightDouble, tr("Aller à la fin du projet"),this);
	connect(m_end_view, &QAction::triggered, [this](){this->m_tab->setCurrentWidget(lastDiagram());});

		// button to scroll one page left
	m_next_view_left = new QAction(QET::Icons::ArrowLeft, tr("go one page left"),this);
	connect(m_next_view_left, &QAction::triggered, [this](){this->m_tab->setCurrentWidget(previousDiagram());});

		// button to scroll one page right
	m_next_view_right = new QAction(QET::Icons::ArrowRight, tr("go one page right"),this);
	connect(m_next_view_right, &QAction::triggered, [this](){this->m_tab->setCurrentWidget(nextDiagram());});
}

/**
	Initialize child widgets for this widget.
*/
void ProjectView::initWidgets()
{
	setObjectName("ProjectView");
	setWindowIcon(QET::Icons::ProjectFileGP);

	// initialize the "fallback" widget
	fallback_widget_ = new QWidget();
	fallback_label_ = new QLabel(
		tr(
			"Ce projet ne contient aucun folio",
			"label displayed when a project contains no diagram"
		)
	);
	fallback_label_ -> setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

	// initialize tabs
#ifdef Q_OS_MACOS
	m_tab = new WheelEnabledTabBar(this);
#else
	m_tab = new QTabWidget(this);
#endif
	m_tab -> setMovable(true);
		// setting UsesScrollButton ensures that when the tab bar is full, the tabs are scrolled.
	m_tab -> setUsesScrollButtons(true);
		// disable the internal scroll buttons of the TabWidget, we will use our own buttons.
	m_tab->setStyleSheet("QTabBar QToolButton {border-image: ;border-width: 0px}");
	m_tab->setStyleSheet("QTabBar::scroller {width: 0px;}");

		   // add layouts
	QHBoxLayout *TopRightCorner_Layout = new QHBoxLayout();
	TopRightCorner_Layout->setContentsMargins(0,0,0,0);
	// some place left to the 'next_right_view_button' button
	TopRightCorner_Layout->insertSpacing(1,10);

	QHBoxLayout *TopLeftCorner_Layout = new QHBoxLayout();
	TopLeftCorner_Layout->setContentsMargins(0,0,0,0);

		// add buttons
	QToolButton *m_next_right_view_button =new QToolButton;
	m_next_right_view_button->setDefaultAction(m_next_view_right);
	m_next_right_view_button->setAutoRaise(true);
	TopRightCorner_Layout->addWidget(m_next_right_view_button);

	QToolButton *m_end_view_button =new QToolButton;
	m_end_view_button->setDefaultAction(m_end_view);
	m_end_view_button->setAutoRaise(true);
	TopRightCorner_Layout->addWidget(m_end_view_button);

	QToolButton *add_new_diagram_button = new QToolButton;
	add_new_diagram_button -> setDefaultAction(m_add_new_diagram);
	add_new_diagram_button -> setAutoRaise(true);
	TopRightCorner_Layout->addWidget(add_new_diagram_button);
		// some place right to the 'add_new_diagram_button' button
	TopRightCorner_Layout->addSpacing(5);

	QToolButton *m_first_view_button =new QToolButton;
	m_first_view_button->setDefaultAction(m_first_view);
	m_first_view_button->setAutoRaise(true);
	TopLeftCorner_Layout->addWidget(m_first_view_button);

	QToolButton *m_next_left_view_button =new QToolButton;
	m_next_left_view_button->setDefaultAction(m_next_view_left);
	m_next_left_view_button->setAutoRaise(true);
	TopLeftCorner_Layout->addWidget(m_next_left_view_button);

		// some place right to the 'first_view_button' button
	TopLeftCorner_Layout->addSpacing(10);

		// add widgets to tabbar
	QWidget *tabwidgetRight=new QWidget(this);
	tabwidgetRight->setLayout(TopRightCorner_Layout);
	m_tab -> setCornerWidget(tabwidgetRight, Qt::TopRightCorner);

	QWidget *tabwidgetLeft=new QWidget(this);
	tabwidgetLeft->setLayout(TopLeftCorner_Layout);
	m_tab -> setCornerWidget(tabwidgetLeft, Qt::TopLeftCorner);

		// manage signals
	connect(m_tab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
	connect(m_tab, SIGNAL(tabBarDoubleClicked(int)), this, SLOT(tabDoubleClicked(int)));
	connect(m_tab->tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(tabMoved(int,int)), Qt::QueuedConnection);

	fallback_widget_ -> setVisible(false);
	m_tab -> setVisible(false);
}

/**
	Initialize layout for this widget.
*/
void ProjectView::initLayout()
{
	QVBoxLayout *fallback_widget_layout_ = new QVBoxLayout(fallback_widget_);
	fallback_widget_layout_ -> addWidget(fallback_label_);

	layout_ = new QVBoxLayout(this);
#ifdef Q_OS_MACOS
	layout_ -> setContentsMargins(0, 8, 0, 0);
#else
	layout_ -> setContentsMargins(0, 0, 0, 0);
#endif
	layout_ -> setSpacing(0);
	layout_ -> addWidget(fallback_widget_);
	layout_ -> addWidget(m_tab);
}


/**
	@brief ProjectView::loadDiagrams
	Load diagrams of project.
	We create a diagram view for each diagram,
	and add it to the project view.
*/
void ProjectView::loadDiagrams()
{
	if (!m_project) return;

	setDisplayFallbackWidget(m_project -> diagrams().isEmpty());

	auto dialog = DialogWaiting::instance();
	if(dialog)
	{
		dialog->setTitle( tr("<p align=\"center\">"
												"<b>Ouverture du projet en cours...</b><br/>"
												"Création des onglets de folio :"
												"</p>"));
	}

	const QList<Diagram *> diagrams = m_project->diagrams();
	for (int index = 0; index < diagrams.size(); ++index)
	{
		Diagram *diagram = diagrams.at(index);
		if(dialog)
		{
			dialog->setDetail(diagram->title());
			dialog->setProgressBar(dialog->progressBarValue()+1);
		}

		auto dv = new DiagramView(diagram);
		dv->setFrameStyle(QFrame::Plain | QFrame::NoFrame);

		m_tab->insertTab(index, dv, QET::Icons::Diagram, dv->title());

		connect(dv, &DiagramView::showDiagram,         this, QOverload<Diagram*>::of(&ProjectView::showDiagram));
		connect(dv, &DiagramView::titleChanged,        this, &ProjectView::updateTabTitle);
		connect(dv, &DiagramView::findElementRequired, this, &ProjectView::findElementRequired);
		connect(&dv->diagram()->border_and_titleblock , &BorderTitleBlock::titleBlockFolioChanged, [this, dv]() {this->updateTabTitle(dv);});
	}

	rebuildDiagramsMap();
	updateAllTabsTitle();

	if (DiagramView *first = firstDiagram()) {
		m_tab->setCurrentWidget(first);
		recordNavigation(first);
	}
}

/**
	@brief ProjectView::updateWindowTitle
	Update the project view title
*/
void ProjectView::updateWindowTitle()
{
	QString title;
	if (m_project) {
		title = m_project -> pathNameTitle();
	} else {
		title = tr("Projet", "window title for a project-less ProjectView");
	}
	setWindowTitle(title);
}

/**
	Effectue les actions necessaires lorsque le projet visualise entre ou sort
	du mode lecture seule.
*/
void ProjectView::adjustReadOnlyState()
{
	bool editable = !(m_project -> isReadOnly());

	// prevent users from moving existing diagrams
	m_tab -> setMovable(editable);
	// prevent users from adding new diagrams
	m_add_new_diagram -> setEnabled(editable);

	// on met a jour le titre du widget, qui reflete l'etat de lecture seule
	updateWindowTitle();
}

/**
	@brief ProjectView::diagramAdded
	Slot called when qetproject emit diagramAdded
	@param diagram
*/
void ProjectView::diagramAdded(Diagram *diagram)
{
	auto dv = new DiagramView(diagram);
	auto index = m_project->folioIndex(diagram);
	m_tab->insertTab(index, dv, QET::Icons::Diagram, dv->title());
	dv->setFrameStyle(QFrame::Plain | QFrame::NoFrame);

	rebuildDiagramsMap();
	updateAllTabsTitle();

	connect(dv, &DiagramView::showDiagram,         this, QOverload<Diagram*>::of(&ProjectView::showDiagram));
	connect(dv, &DiagramView::titleChanged,        this, &ProjectView::updateTabTitle);
	connect(dv, &DiagramView::findElementRequired, this, &ProjectView::findElementRequired);
	connect(&dv->diagram()->border_and_titleblock , &BorderTitleBlock::titleBlockFolioChanged, [this, dv]() {this->updateTabTitle(dv);});

		// signal diagram view was added
	emit(diagramAdded(dv));
	m_project->setModified(true);
	showDiagram(dv);
}

/**
	@brief ProjectView::updateTabTitle
	Update the title of the tab which display the diagram view.
	@param diagram_view : The diagram view.
*/
void ProjectView::updateTabTitle(DiagramView *diagram_view)
{
	const int diagram_tab_id = diagramIndex(diagram_view);
	if (diagram_tab_id != -1) {
		const bool use_folio_label = QSettings().value(
				"genericpanel/folio", false).toBool();
		updateTabTitleAtIndex(
				diagram_view, diagram_tab_id, use_folio_label);
		scheduleFolioNavigatorRefresh();
	}
}

void ProjectView::updateTabTitleAtIndex(
		DiagramView *diagram_view,
		int diagram_tab_id,
		bool use_folio_label)
{
	if (!diagram_view || diagram_tab_id < 0) {
		return;
	}

	QString title;
	Diagram *diagram = diagram_view->diagram();
	if (use_folio_label)
	{
		QString formula = diagram->border_and_titleblock.folio();
		autonum::sequentialNumbers seq;
		title = autonum::AssignVariables::formulaToLabel(
				formula,
				seq,
				diagram);
	}
	else {
		title = QString::number(diagram_tab_id + 1);
	}

	title += " - ";
	title += diagram->title();
	m_tab->setTabText(diagram_tab_id, title);
}

/**
	@brief ProjectView::updateAllTabsTitle
	Update all tabs title
*/
void ProjectView::updateAllTabsTitle()
{
	const bool use_folio_label = QSettings().value(
			"genericpanel/folio", false).toBool();
	for (auto iterator = m_diagram_ids.constBegin();
		 iterator != m_diagram_ids.constEnd(); ++iterator) {
		updateTabTitleAtIndex(
				iterator.value(), iterator.key(), use_folio_label);
	}
	scheduleFolioNavigatorRefresh();
}

/**
	@param from Index de l'onglet avant le deplacement
	@param to   Index de l'onglet apres le deplacement
*/
void ProjectView::tabMoved(int from, int to)
{
	if (!m_project)
		return;
	
	m_project->diagramOrderChanged(from, to);
	rebuildDiagramsMap();
	const bool use_folio_label = QSettings().value(
			"genericpanel/folio", false).toBool();
	
		//Rebuild the title of each diagram in range from - to
	for (int i= qMin(from,to) ; i< qMax(from,to)+1 ; ++i)
	{
		DiagramView *dv = m_diagram_ids.value(i);
		updateTabTitleAtIndex(dv, i, use_folio_label);
	}
	scheduleFolioNavigatorRefresh();
}

/**
	@param diagram Schema a trouver
	@return le DiagramView correspondant au schema passe en parametre, ou 0 si
	le schema n'est pas trouve
*/
DiagramView *ProjectView::findDiagram(Diagram *diagram) {
	return m_diagram_views.value(diagram, nullptr);
}

int ProjectView::diagramIndex(DiagramView *diagram_view) const
{
	return diagram_view
			? m_diagram_indexes.value(diagram_view, -1)
			: -1;
}

/**
	Reconstruit la map associant les index des onglets avec les DiagramView
*/
void ProjectView::rebuildDiagramsMap()
{
	m_diagram_ids.clear();
	m_diagram_indexes.clear();
	m_diagram_views.clear();
	m_diagram_views_by_id.clear();
	m_diagram_view_list.clear();

	for (int index = 0; index < m_tab->count(); ++index)
	{
		DiagramView *diagram_view =
				qobject_cast<DiagramView *>(m_tab->widget(index));
		if (!diagram_view || !diagram_view->diagram()) {
			continue;
		}
		m_diagram_ids.insert(index, diagram_view);
		m_diagram_indexes.insert(diagram_view, index);
		m_diagram_views.insert(diagram_view->diagram(), diagram_view);
		m_diagram_views_by_id.insert(
				diagram_view->diagram()->uuid(), diagram_view);
		m_diagram_view_list.append(diagram_view);
	}
}

/**
	@brief ProjectView::tabChanged
	Manage the tab change.
	If tab_id == -1 (there is no diagram opened),
	we display the fallback widget.
	@param tab_id
*/
void ProjectView::tabChanged(int tab_id)
{
	if (tab_id < 0)
	{
		if (m_tab->count() == 0) {
			setDisplayFallbackWidget(true);
		}
		updateNavigationHistoryActions();
		return;
	}

	DiagramView *activated_view =
			qobject_cast<DiagramView *>(m_tab->widget(tab_id));
	if (!activated_view) {
		updateNavigationHistoryActions();
		return;
	}
	setDisplayFallbackWidget(false);

	emit(diagramActivated(activated_view));
	
	if (activated_view)
		activated_view->diagram()->diagramActivated();

		//Clear the event interface of the previous diagram
	if (DiagramView *dv = m_diagram_ids.value(m_previous_tab_index))
		dv->diagram()->clearEventInterface();
	m_previous_tab_index = tab_id;
	recordNavigation(activated_view);
	scheduleFolioNavigatorRefresh();
}

/**
	Gere le double-clic sur un onglet : edite les proprietes du schema
	@param tab_id Index de l'onglet concerne
*/
void ProjectView::tabDoubleClicked(int tab_id) {
	// repere le schema concerne
	DiagramView *diagram_view = m_diagram_ids[tab_id];
	if (!diagram_view) return;

	diagram_view -> editDiagramProperties();
}

/**
	@param fallback true pour afficher le widget de fallback, false pour
	afficher les onglets.
	Le widget de Fallback est le widget affiche lorsque le projet ne comporte
	aucun schema.
*/
void ProjectView::setDisplayFallbackWidget(bool fallback) {
	fallback_widget_ -> setVisible(fallback);
	m_tab -> setVisible(!fallback);
}
