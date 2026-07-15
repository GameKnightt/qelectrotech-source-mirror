/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "startcenterwidget.h"

#include "../recentfiles.h"

#include <QAction>
#include <QBoxLayout>
#include <QCommandLinkButton>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyle>
#include <QTreeWidget>

StartCenterWidget::StartCenterWidget(
	QAction *new_project_action,
	QAction *open_project_action,
	RecentFiles *recent_files,
	const QIcon &application_icon,
	QWidget *parent) :
	QWidget(parent),
	m_recent_files(recent_files)
{
	setObjectName(QStringLiteral("startCenter"));
	setAccessibleName(tr("Centre de démarrage"));

	auto scroll_area = new QScrollArea(this);
	scroll_area->setObjectName(QStringLiteral("startCenterScrollArea"));
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	auto scroll_content = new QWidget(scroll_area);
	auto scroll_layout = new QVBoxLayout(scroll_content);
	scroll_layout->setContentsMargins(24, 24, 24, 24);
	scroll_layout->addStretch();

	auto content = new QWidget(scroll_content);
	content->setObjectName(QStringLiteral("startCenterContent"));
	content->setMaximumWidth(1040);
	auto content_layout = new QVBoxLayout(content);
	content_layout->setContentsMargins(0, 0, 0, 0);
	content_layout->setSpacing(20);

	auto header_layout = new QHBoxLayout;
	header_layout->setContentsMargins(0, 0, 0, 0);
	header_layout->setSpacing(12);
	if (!application_icon.isNull()) {
		auto logo = new QLabel(content);
		logo->setObjectName(QStringLiteral("startCenterLogo"));
		const int icon_size = style()->pixelMetric(QStyle::PM_LargeIconSize);
		logo->setPixmap(application_icon.pixmap(icon_size, icon_size));
		logo->setAccessibleName(tr("QElectroTech"));
		header_layout->addWidget(logo, 0, Qt::AlignTop);
	}

	auto title_layout = new QVBoxLayout;
	title_layout->setContentsMargins(0, 0, 0, 0);
	title_layout->setSpacing(4);
	auto title = new QLabel(tr("Bienvenue dans QElectroTech"), content);
	title->setObjectName(QStringLiteral("startCenterTitle"));
	QFont title_font = title->font();
	title_font.setWeight(QFont::DemiBold);
	if (title_font.pointSizeF() > 0) {
		title_font.setPointSizeF(title_font.pointSizeF() + 5.0);
	}
	title->setFont(title_font);
	title->setWordWrap(true);
	auto subtitle = new QLabel(
		tr("Commencez un nouveau schéma ou reprenez un projet récent."),
		content);
	subtitle->setObjectName(QStringLiteral("startCenterSubtitle"));
	subtitle->setWordWrap(true);
	subtitle->setForegroundRole(QPalette::PlaceholderText);
	title_layout->addWidget(title);
	title_layout->addWidget(subtitle);
	header_layout->addLayout(title_layout, 1);
	content_layout->addLayout(header_layout);

	auto start_group = new QGroupBox(tr("Commencer"), content);
	start_group->setObjectName(QStringLiteral("startCenterActionsGroup"));
	auto start_layout = new QVBoxLayout(start_group);
	start_layout->setSpacing(8);
	m_new_project_button = new QCommandLinkButton(
		tr("Nouveau projet"),
		tr("Créer un projet vide avec un premier folio."),
		start_group);
	m_new_project_button->setObjectName(QStringLiteral("startCenterNewProject"));
	m_new_project_button->setDefault(true);
	configureActionButton(
		m_new_project_button,
		new_project_action,
		tr("Créer un nouveau projet"));
	m_open_project_button = new QCommandLinkButton(
		tr("Ouvrir un projet…"),
		tr("Choisir un fichier QElectroTech existant."),
		start_group);
	m_open_project_button->setObjectName(QStringLiteral("startCenterOpenProject"));
	configureActionButton(
		m_open_project_button,
		open_project_action,
		tr("Ouvrir un projet existant"));
	start_layout->addWidget(m_new_project_button);
	start_layout->addWidget(m_open_project_button);
	auto drop_hint = new QLabel(
		tr("Vous pouvez aussi déposer un fichier .qet dans cette fenêtre."),
		start_group);
	drop_hint->setObjectName(QStringLiteral("startCenterDropHint"));
	drop_hint->setWordWrap(true);
	drop_hint->setForegroundRole(QPalette::PlaceholderText);
	start_layout->addWidget(drop_hint);

	auto recent_group = new QGroupBox(tr("Projets récents"), content);
	recent_group->setObjectName(QStringLiteral("startCenterRecentGroup"));
	auto recent_layout = new QVBoxLayout(recent_group);
	m_recent_projects = new QTreeWidget(recent_group);
	m_recent_projects->setObjectName(QStringLiteral("startCenterRecentProjects"));
	m_recent_projects->setAccessibleName(tr("Liste des projets récents"));
	m_recent_projects->setAccessibleDescription(tr(
		"Activez un projet avec Entrée ou un double-clic."));
	m_recent_projects->setHeaderLabels({tr("Projet"), tr("Emplacement")});
	m_recent_projects->setRootIsDecorated(false);
	m_recent_projects->setItemsExpandable(false);
	m_recent_projects->setUniformRowHeights(true);
	m_recent_projects->setSelectionMode(QAbstractItemView::SingleSelection);
	m_recent_projects->setTextElideMode(Qt::ElideMiddle);
	m_recent_projects->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_recent_projects->setContextMenuPolicy(Qt::CustomContextMenu);
	m_recent_projects->header()->setSectionResizeMode(0, QHeaderView::Interactive);
	m_recent_projects->header()->setStretchLastSection(true);
	m_recent_projects->setColumnWidth(0, 220);
	m_recent_projects->setMinimumHeight(150);
	recent_layout->addWidget(m_recent_projects, 1);

	m_empty_recent_label = new QLabel(
		tr("Aucun projet récent. Les projets que vous ouvrez apparaîtront ici."),
		recent_group);
	m_empty_recent_label->setObjectName(QStringLiteral("startCenterEmptyRecent"));
	m_empty_recent_label->setAlignment(Qt::AlignCenter);
	m_empty_recent_label->setWordWrap(true);
	m_empty_recent_label->setForegroundRole(QPalette::PlaceholderText);
	recent_layout->addWidget(m_empty_recent_label, 1);

	auto recent_actions = new QHBoxLayout;
	recent_actions->addStretch();
	m_forget_recent_button = new QPushButton(
		tr("Oublier de la liste"),
		recent_group);
	m_forget_recent_button->setObjectName(QStringLiteral("startCenterForgetRecent"));
	m_forget_recent_button->setAccessibleDescription(tr(
		"Retire le projet de l'historique sans supprimer son fichier."));
	m_forget_recent_button->setEnabled(false);
	recent_actions->addWidget(m_forget_recent_button);
	recent_layout->addLayout(recent_actions);

	m_sections_layout = new QBoxLayout(QBoxLayout::LeftToRight);
	m_sections_layout->setContentsMargins(0, 0, 0, 0);
	m_sections_layout->setSpacing(32);
	m_sections_layout->addWidget(start_group, 0);
	m_sections_layout->addWidget(recent_group, 1);
	content_layout->addLayout(m_sections_layout, 1);

	auto centered_content = new QHBoxLayout;
	centered_content->setContentsMargins(0, 0, 0, 0);
	centered_content->addStretch();
	centered_content->addWidget(content, 1);
	centered_content->addStretch();
	scroll_layout->addLayout(centered_content, 1);
	scroll_layout->addStretch();
	scroll_area->setWidget(scroll_content);

	auto root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(0, 0, 0, 0);
	root_layout->addWidget(scroll_area);

	connect(
		m_recent_projects,
		&QTreeWidget::itemActivated,
		this,
		&StartCenterWidget::activateRecentProject);
	connect(
		m_recent_projects,
		&QTreeWidget::itemSelectionChanged,
		this,
		&StartCenterWidget::updateRecentSelectionActions);
	connect(
		m_recent_projects,
		&QTreeWidget::customContextMenuRequested,
		this,
		&StartCenterWidget::showRecentProjectContextMenu);
	connect(
		m_forget_recent_button,
		&QPushButton::clicked,
		this,
		&StartCenterWidget::forgetSelectedRecentProject);
	if (m_recent_files) {
		connect(
			m_recent_files,
			&RecentFiles::filesChanged,
			this,
			&StartCenterWidget::rebuildRecentProjects);
	}

	setTabOrder(m_new_project_button, m_open_project_button);
	setTabOrder(m_open_project_button, m_recent_projects);
	setTabOrder(m_recent_projects, m_forget_recent_button);
	rebuildRecentProjects();
	updateResponsiveLayout();
}

void StartCenterWidget::focusPrimaryAction()
{
	if (m_new_project_button && m_new_project_button->isEnabled()) {
		m_new_project_button->setFocus(Qt::OtherFocusReason);
	}
}

QStringList StartCenterWidget::displayedRecentFiles() const
{
	QStringList result;
	if (!m_recent_projects) return result;
	for (int row = 0; row < m_recent_projects->topLevelItemCount(); ++row) {
		result.append(pathForItem(m_recent_projects->topLevelItem(row)));
	}
	return result;
}

void StartCenterWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	updateResponsiveLayout();
}

void StartCenterWidget::rebuildRecentProjects()
{
	if (!m_recent_projects) return;
	m_recent_projects->clear();
	const QStringList recent_paths = m_recent_files
		? m_recent_files->files().mid(0, MaximumRecentProjects)
		: QStringList();
	for (const QString &path : recent_paths) {
		const QFileInfo file_info(path);
		QString file_name = file_info.fileName();
		if (file_name.isEmpty()) file_name = path;
		const QString directory = QDir::toNativeSeparators(file_info.path());
		auto item = new QTreeWidgetItem(m_recent_projects);
		item->setText(0, file_name);
		item->setText(1, directory);
		item->setData(0, RecentPathRole, path);
		item->setData(0, Qt::AccessibleTextRole, file_name);
		item->setData(0, Qt::AccessibleDescriptionRole, path);
		item->setToolTip(0, path);
		item->setToolTip(1, path);
		const int row_height = qMax(36, fontMetrics().height() + 14);
		item->setSizeHint(0, QSize(0, row_height));
		item->setSizeHint(1, QSize(0, row_height));
		if (m_recent_files && !m_recent_files->iconForFiles().isNull()) {
			item->setIcon(0, m_recent_files->iconForFiles());
		}
	}
	const bool has_recent_projects = !recent_paths.isEmpty();
	m_recent_projects->setVisible(has_recent_projects);
	m_empty_recent_label->setVisible(!has_recent_projects);
	updateRecentSelectionActions();
}

void StartCenterWidget::activateRecentProject(QTreeWidgetItem *item, int)
{
	const QString path = pathForItem(item);
	if (!path.isEmpty()) emit recentProjectRequested(path);
}

void StartCenterWidget::forgetSelectedRecentProject()
{
	if (!m_recent_files || !m_recent_projects) return;
	m_recent_files->forgetFile(pathForItem(m_recent_projects->currentItem()));
}

void StartCenterWidget::showRecentProjectContextMenu(const QPoint &position)
{
	QTreeWidgetItem *item = m_recent_projects->itemAt(position);
	if (!item) return;
	const QString path = pathForItem(item);
	if (path.isEmpty()) return;
	m_recent_projects->setCurrentItem(item);
	QMenu menu(this);
	QAction *open_action = menu.addAction(tr("Ouvrir"));
	QAction *forget_action = menu.addAction(tr("Oublier de la liste"));
	QAction *selected_action = menu.exec(m_recent_projects->viewport()->mapToGlobal(position));
	if (selected_action == open_action) emit recentProjectRequested(path);
	else if (selected_action == forget_action && m_recent_files) {
		m_recent_files->forgetFile(path);
	}
}

void StartCenterWidget::configureActionButton(
	QCommandLinkButton *button,
	QAction *action,
	const QString &accessible_name)
{
	if (!button) return;
	button->setMinimumHeight(60);
	button->setAccessibleName(accessible_name);
	if (!action) {
		button->setEnabled(false);
		return;
	}
	button->setIcon(action->icon());
	button->setEnabled(action->isEnabled());
	connect(button, &QCommandLinkButton::clicked, action, &QAction::trigger);
	connect(action, &QAction::changed, button, [button, action]() {
		button->setIcon(action->icon());
		button->setEnabled(action->isEnabled());
	});
}

QString StartCenterWidget::pathForItem(const QTreeWidgetItem *item) const
{
	return item ? item->data(0, RecentPathRole).toString() : QString();
}

void StartCenterWidget::updateResponsiveLayout()
{
	if (!m_sections_layout) return;
	const QBoxLayout::Direction direction = width() < 760
		? QBoxLayout::TopToBottom
		: QBoxLayout::LeftToRight;
	if (m_sections_layout->direction() != direction) {
		m_sections_layout->setDirection(direction);
	}
}

void StartCenterWidget::updateRecentSelectionActions()
{
	if (m_forget_recent_button && m_recent_projects) {
		m_forget_recent_button->setEnabled(
			m_recent_projects->currentItem() != nullptr);
	}
}
