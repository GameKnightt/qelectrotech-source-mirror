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
#include "ui_terminalstripeditorwindow.h"

#include "../UndoCommand/addterminalstripcommand.h"
#include "freeterminaleditor.h"
#include "../../qetapp.h"
#include "../../qetdiagrameditor.h"
#include "../../qetproject.h"
#include "../../elementprovider.h"
#include "../../qetgraphicsitem/element.h"
#include "../../qetgraphicsitem/qetgraphicsitem.h"
#include "../realterminal.h"
#include "../terminalstrip.h"
#include "terminalstripcreatordialog.h"
#include "terminalstripeditor.h"
#include "terminalstripeditorwindow.h"
#include "cablecatalogwidget.h"
#include "terminalstripoverviewloader.h"
#include "terminalstripoverviewwidget.h"
#include "terminalstriptreedockwidget.h"
#include "../../cablecatalog/cablecatalogloader.h"
#include "../../diagram.h"
#include "../../qetgraphicsitem/conductor.h"

#include <QGraphicsView>
#include <QGuiApplication>
#include <QScreen>
#include <QTabWidget>

QPointer<TerminalStripEditorWindow> TerminalStripEditorWindow::window_;

static const int OVERVIEW_PAGE = 0;
static const int FREE_TERMINAL_PAGE = 1;
static const int TERMINAL_STRIP_PAGE = 2;
/**
 * @brief TerminalStripEditorWindow::TerminalStripEditorWindow
 * @param project
 * @param parent
 */
void TerminalStripEditorWindow::edit(TerminalStrip *strip)
{
	if (const auto project_ = strip->project())
	{
		auto editor_  = TerminalStripEditorWindow::instance(project_, QETApp::diagramEditor(project_));
		editor_->setCurrentStrip(strip);
		editor_->show();
	}
}

TerminalStripEditorWindow::TerminalStripEditorWindow(QETProject *project, QWidget *parent) :
	QMainWindow(parent),
    ui(new Ui::TerminalStripEditorWindow),
    m_project(project)
{
	ui->setupUi(this);
	setMinimumSize(640, 360);
	if (QScreen *screen = this->screen())
	{
		const QSize available = screen->availableGeometry().size();
		resize(qMin(1180, available.width()), qMin(760, available.height()));
	}
    if (auto diagram_editor = QETApp::diagramEditor(project)) {
        ui->m_tool_bar->addSeparator();
        ui->m_tool_bar->addAction(diagram_editor->undo);
        ui->m_tool_bar->addAction(diagram_editor->redo);
    }
	ui->m_remove_terminal->setDisabled(true);
	addTreeDockWidget();

	m_free_terminal_editor = new FreeTerminalEditor(m_project, this);
	m_terminal_strip_editor = new TerminalStripEditor{m_project, this};
	m_overview = new TerminalStripOverviewWidget(this);
	m_cable_catalog = new CableCatalogWidget(this);
	m_workspace_tabs = new QTabWidget(this);
	m_workspace_tabs->setObjectName(QStringLiteral("terminalCableWorkspaceTabs"));
	m_workspace_tabs->setAccessibleName(tr("Borniers et câbles"));
	m_workspace_tabs->addTab(m_overview, tr("&Bornes"));
	m_workspace_tabs->addTab(m_cable_catalog, tr("&Câbles"));

	connect(m_tree_dock, &TerminalStripTreeDockWidget::currentStripChanged, this, &TerminalStripEditorWindow::currentStripChanged);

	ui->m_stacked_widget->insertWidget(OVERVIEW_PAGE, m_workspace_tabs);
	ui->m_stacked_widget->insertWidget(FREE_TERMINAL_PAGE, m_free_terminal_editor);
	ui->m_stacked_widget->insertWidget(TERMINAL_STRIP_PAGE, m_terminal_strip_editor);
	connect(m_overview, &TerminalStripOverviewWidget::reloadRequested,
			this, &TerminalStripEditorWindow::refreshOverview);
	connect(m_overview, &TerminalStripOverviewWidget::showElementRequested,
			this, &TerminalStripEditorWindow::showElementInFolio);
	connect(m_cable_catalog, &CableCatalogWidget::reloadRequested,
			this, &TerminalStripEditorWindow::refreshOverview);
	connect(m_cable_catalog, &CableCatalogWidget::showConductorRequested,
			this, &TerminalStripEditorWindow::showConductorInFolio);
	setProject(project);
	ui->m_stacked_widget->setCurrentIndex(OVERVIEW_PAGE);
}

/**
 * @brief TerminalStripEditorWindow::~TerminalStripEditorWindow
 */
TerminalStripEditorWindow::~TerminalStripEditorWindow()
{
	delete ui;
}

/**
 * @brief TerminalStripEditorWindow::setProject
 * @param project
 */
void TerminalStripEditorWindow::setProject(QETProject *project)
{
	if (m_project_destroy_connection) disconnect(m_project_destroy_connection);
	if (m_project_read_only_connection) disconnect(m_project_read_only_connection);
    m_project = project;
    m_tree_dock->setProject(project);
    m_free_terminal_editor->setProject(project);
    m_terminal_strip_editor->setProject(project);
	if (m_project)
	{
		m_project_destroy_connection = connect(
				m_project, &QObject::destroyed, this, [this]() {
					m_project.clear();
					m_overview->setRows({});
					m_overview->setProjectAvailable(false);
					m_cable_targets.clear();
					m_cable_catalog->setSnapshot({});
					m_cable_catalog->setProjectAvailable(false);
					ui->m_stacked_widget->setCurrentIndex(OVERVIEW_PAGE);
					updateReadOnlyState();
				});
		m_project_read_only_connection = connect(
				m_project, &QETProject::readOnlyChanged,
				this, [this](QETProject *, bool) { updateReadOnlyState(); });
		setWindowTitle(tr("Borniers et câbles — %1").arg(
				m_project->title().isEmpty()
						? tr("Projet sans titre")
						: m_project->title()));
	}
	else
	{
		setWindowTitle(tr("Borniers et câbles"));
	}
	refreshOverview();
	updateReadOnlyState();
}

void TerminalStripEditorWindow::setCurrentStrip(TerminalStrip *strip) {
	m_tree_dock->setSelectedStrip(strip);
}

/**
 * @brief TerminalStripEditorWindow::addTreeDockWidget
 */
void TerminalStripEditorWindow::addTreeDockWidget()
{
	m_tree_dock = new TerminalStripTreeDockWidget(m_project, this);
	m_tree_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	addDockWidget(Qt::LeftDockWidgetArea, m_tree_dock);
}

/**
 * @brief TerminalStripEditorWindow::currentStripChanged
 * @param strip
 */
void TerminalStripEditorWindow::currentStripChanged(TerminalStrip *strip)
{
	Q_UNUSED(strip)
	updateUi();

}

void TerminalStripEditorWindow::updateUi()
{
	ui->m_remove_terminal->setEnabled(m_tree_dock->currentIsStrip());

	ui->m_stacked_widget->setCurrentIndex(OVERVIEW_PAGE);

	if (auto real_terminal = m_tree_dock->currentRealTerminal())
	{
		if (!real_terminal->parentStrip())
		{
			ui->m_stacked_widget->setCurrentIndex(FREE_TERMINAL_PAGE);
			m_free_terminal_editor->reload();
		}
	} else if (auto strip_ = m_tree_dock->currentStrip()) {
		ui->m_stacked_widget->setCurrentIndex(TERMINAL_STRIP_PAGE);
		m_terminal_strip_editor->setCurrentStrip(strip_);
	}
}

/**
 * @brief TerminalStripEditorWindow::on_m_add_terminal_strip_triggered
 * Action when user click on add terminal strip button
 */
void TerminalStripEditorWindow::on_m_add_terminal_strip_triggered()
{
	QScopedPointer<TerminalStripCreatorDialog> dialog(new TerminalStripCreatorDialog(m_project, this));

	dialog->setLocation(m_tree_dock->currentLocation());
	dialog->setInstallation(m_tree_dock->currentInstallation());

	if (dialog->exec() == QDialog::Accepted)
	{
		auto ts = dialog->generatedTerminalStrip();
		m_project->undoStack()->push(new AddTerminalStripCommand(ts, m_project));

		m_tree_dock->reload();
		m_tree_dock->setSelectedStrip(ts);
	}
}

/**
 * @brief TerminalStripEditorWindow::on_m_remove_terminal_triggered
 */
void TerminalStripEditorWindow::on_m_remove_terminal_triggered()
{
	if (m_tree_dock->currentIsStrip())
	{
		if (auto strip_ = m_tree_dock->currentStrip())
		{
			m_project->undoStack()->push(new RemoveTerminalStripCommand(strip_, m_project));
			m_tree_dock->reload();
		}

	}
}

/**
 * @brief TerminalStripEditorWindow::on_m_reload_triggered
 */
void TerminalStripEditorWindow::on_m_reload_triggered() {
	m_tree_dock->reload();
	m_terminal_strip_editor->reload();
	m_free_terminal_editor->reload();
	refreshOverview();
}

/**
 * @brief TerminalStripEditorWindow::on_m_button_box_clicked
 * Action when user click on the apply/reset button
 * @param button
 */
void TerminalStripEditorWindow::on_m_button_box_clicked(QAbstractButton *button)
{
	auto role_{ui->m_button_box->buttonRole(button)};

	if (role_ == QDialogButtonBox::ApplyRole)
	{
		switch (ui->m_stacked_widget->currentIndex()) {
			case FREE_TERMINAL_PAGE:
				m_free_terminal_editor->apply();
				break;
			case TERMINAL_STRIP_PAGE:
				m_terminal_strip_editor->apply();
				break;
			default:
				break;
		}
	}
	else if (role_ == QDialogButtonBox::ResetRole)
	{
		m_terminal_strip_editor->reload();
		m_free_terminal_editor->reload();
	}
}


void TerminalStripEditorWindow::on_m_stacked_widget_currentChanged(int arg1) {
	ui->m_button_box->setHidden(arg1 == OVERVIEW_PAGE);
	updateReadOnlyState();
}

void TerminalStripEditorWindow::refreshOverview()
{
	if (!m_overview) return;
	if (!m_project)
	{
		m_overview->setRows({});
		m_overview->setProjectAvailable(false);
		m_cable_targets.clear();
		if (m_cable_catalog)
		{
			m_cable_catalog->setSnapshot({});
			m_cable_catalog->setProjectAvailable(false);
		}
		return;
	}
	m_overview->setProjectAvailable(true);
	m_overview->setReadOnly(m_project->isReadOnly());
	m_overview->setRows(TerminalStripOverviewLoader::rowsForProject(m_project));
	if (m_cable_catalog)
	{
		const CableCatalogLoadResult result = CableCatalogLoader::load(m_project);
		m_cable_targets = result.live_targets;
		m_cable_catalog->setProjectAvailable(true);
		m_cable_catalog->setReadOnly(m_project->isReadOnly());
		m_cable_catalog->setLoadFailed(!result.ok);
		m_cable_catalog->setSnapshot(result.snapshot);
	}
}

void TerminalStripEditorWindow::showElementInFolio(const QUuid &element_uuid)
{
	if (!m_project || element_uuid.isNull()) return;
	const QList<Element *> elements = ElementProvider(m_project).fromUuids({element_uuid});
	if (elements.isEmpty() || !elements.first())
	{
		statusBar()->showMessage(
				tr("Cette borne n’est plus disponible. Rechargez la vue."), 5000);
		refreshOverview();
		return;
	}
	QetGraphicsItem::showItem(elements.first());
}

void TerminalStripEditorWindow::showConductorInFolio(
		const CableNavigationTarget &target)
{
	if (!m_project || !target.isValid()
			|| target.project_uuid != m_project->uuid())
	{
		statusBar()->showMessage(
				tr("Ce câble n’est plus disponible. Rechargez la vue."), 5000);
		return;
	}
	const QPointer<Conductor> conductor = m_cable_targets.value(target.token);
	if (!conductor || !conductor->diagram()
			|| conductor->diagram()->uuid() != target.diagram_uuid)
	{
		statusBar()->showMessage(
				tr("Ce câble n’est plus disponible. Rechargez la vue."), 5000);
		refreshOverview();
		return;
	}
	conductor->diagram()->showMe();
	conductor->setSelected(true);
	if (conductor->scene())
	{
		for (QGraphicsView *view : conductor->scene()->views())
		{
			QRectF area = conductor->sceneBoundingRect();
			area.adjust(-200, -200, 200, 200);
			view->fitInView(area, Qt::KeepAspectRatioByExpanding);
		}
	}
}

void TerminalStripEditorWindow::updateReadOnlyState()
{
	const bool has_project = !m_project.isNull();
	const bool read_only = has_project && m_project->isReadOnly();
	const bool editable = has_project && !read_only;
	ui->m_add_terminal_strip->setEnabled(editable);
	ui->m_remove_terminal->setEnabled(
			editable && m_tree_dock && m_tree_dock->currentIsStrip());
	ui->m_button_box->setEnabled(editable);
	if (m_terminal_strip_editor) m_terminal_strip_editor->setEnabled(editable);
	if (m_free_terminal_editor) m_free_terminal_editor->setEnabled(editable);
	if (m_overview)
	{
		m_overview->setReadOnly(read_only);
		m_overview->setProjectAvailable(has_project);
	}
	if (m_cable_catalog)
	{
		m_cable_catalog->setReadOnly(read_only);
		m_cable_catalog->setProjectAvailable(has_project);
	}
}
