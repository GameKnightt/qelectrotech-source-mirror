/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "folionavigatordialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>
#include <QStyle>
#include <QVBoxLayout>

FolioNavigatorDialog::FolioNavigatorDialog(QWidget *parent) :
	QDialog(parent),
	m_model(new FolioNavigatorModel(this))
{
	buildUi();
}

void FolioNavigatorDialog::buildUi()
{
	setObjectName(QStringLiteral("folioNavigatorDialog"));
	setWindowTitle(tr("Atteindre un folio"));
	setWindowModality(Qt::NonModal);
	setAttribute(Qt::WA_DeleteOnClose, false);
	setAccessibleName(tr("Navigateur de folios"));
	setAccessibleDescription(tr(
			"Recherche et ouvre un folio du projet courant sans modifier le projet."));

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(10);

	auto *search_label = new QLabel(tr("Rechercher :"), this);
	m_search = new QLineEdit(this);
	m_search->setObjectName(QStringLiteral("folioNavigatorSearch"));
	m_search->setPlaceholderText(tr("Numéro, titre ou propriété du folio"));
	m_search->setClearButtonEnabled(true);
	m_search->setAccessibleName(tr("Rechercher un folio"));
	m_search->setAccessibleDescription(tr(
			"Saisissez un numéro, un titre ou une propriété du cartouche."));
	search_label->setBuddy(m_search);
	layout->addWidget(search_label);
	layout->addWidget(m_search);

	auto *filters = new QFormLayout;
	filters->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	m_scope = new QComboBox(this);
	m_scope->setObjectName(QStringLiteral("folioNavigatorScope"));
	m_scope->addItem(tr("Tous les folios"), int(FolioNavigationIndex::Scope::All));
	m_scope->addItem(tr("Favoris"), int(FolioNavigationIndex::Scope::Favorites));
	m_scope->addItem(tr("Récents"), int(FolioNavigationIndex::Scope::Recent));
	m_scope->setAccessibleName(tr("Portée des folios"));
	filters->addRow(tr("Afficher :"), m_scope);

	m_group = new QComboBox(this);
	m_group->setObjectName(QStringLiteral("folioNavigatorGroup"));
	m_group->setAccessibleName(tr("Groupe installation et localisation"));
	filters->addRow(tr("Groupe :"), m_group);
	layout->addLayout(filters);

	m_results = new QListView(this);
	m_results->setObjectName(QStringLiteral("folioNavigatorResults"));
	m_results->setModel(m_model);
	m_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_results->setSelectionMode(QAbstractItemView::SingleSelection);
	m_results->setUniformItemSizes(true);
	m_results->setWordWrap(true);
	m_results->setAccessibleName(tr("Résultats des folios"));
	m_results->setAccessibleDescription(tr(
			"Utilisez les flèches puis Entrée pour ouvrir le folio sélectionné."));
	m_results->installEventFilter(this);
	layout->addWidget(m_results, 1);

	m_status = new QLabel(this);
	m_status->setObjectName(QStringLiteral("folioNavigatorStatus"));
	m_status->setWordWrap(true);
	m_status->setAccessibleName(tr("État de la recherche"));
	layout->addWidget(m_status);

	auto *navigation_layout = new QHBoxLayout;
	m_back = new QPushButton(
			style()->standardIcon(QStyle::SP_ArrowBack), tr("Retour"), this);
	m_back->setObjectName(QStringLiteral("folioNavigatorBack"));
	m_back->setToolTip(tr("Revenir au folio précédent dans l’historique (Alt+Gauche)"));
	m_back->setAccessibleName(tr("Folio précédent dans l’historique"));
	m_forward = new QPushButton(
			style()->standardIcon(QStyle::SP_ArrowForward), tr("Avance"), this);
	m_forward->setObjectName(QStringLiteral("folioNavigatorForward"));
	m_forward->setToolTip(tr("Avancer dans l’historique des folios (Alt+Droite)"));
	m_forward->setAccessibleName(tr("Folio suivant dans l’historique"));
	m_favorite = new QPushButton(this);
	m_favorite->setObjectName(QStringLiteral("folioNavigatorFavorite"));
	navigation_layout->addWidget(m_back);
	navigation_layout->addWidget(m_forward);
	navigation_layout->addStretch(1);
	navigation_layout->addWidget(m_favorite);
	layout->addLayout(navigation_layout);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	m_open = buttons->addButton(tr("Ouvrir le folio"), QDialogButtonBox::AcceptRole);
	m_open->setObjectName(QStringLiteral("folioNavigatorOpen"));
	m_open->setDefault(true);
	m_open->setAccessibleName(tr("Ouvrir le folio sélectionné"));
	layout->addWidget(buttons);

	m_search->installEventFilter(this);
	connect(m_search, &QLineEdit::textChanged,
			this, &FolioNavigatorDialog::applyFilters);
	connect(m_search, &QLineEdit::returnPressed,
			this, &FolioNavigatorDialog::activateCurrent);
	connect(m_scope, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &FolioNavigatorDialog::applyFilters);
	connect(m_group, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &FolioNavigatorDialog::applyFilters);
	connect(m_results, &QListView::activated,
			this, &FolioNavigatorDialog::activateCurrent);
	connect(m_results->selectionModel(), &QItemSelectionModel::currentChanged,
			this, &FolioNavigatorDialog::updateCurrentActions);
	connect(m_favorite, &QPushButton::clicked,
			this, &FolioNavigatorDialog::toggleCurrentFavorite);
	connect(m_open, &QPushButton::clicked,
			this, &FolioNavigatorDialog::activateCurrent);
	connect(buttons, &QDialogButtonBox::rejected,
			this, &FolioNavigatorDialog::reject);
	connect(m_back, &QPushButton::clicked, this, [this]() {
		emit navigateBackRequested();
		accept();
	});
	connect(m_forward, &QPushButton::clicked, this, [this]() {
		emit navigateForwardRequested();
		accept();
	});
}

void FolioNavigatorDialog::openForProject(
		const QString &project_title,
		const QVector<FolioNavigationEntry> &entries,
		const QUuid &active_diagram,
		bool can_navigate_back,
		bool can_navigate_forward,
		bool preserve_filters)
{
	const QUuid preferred = preserve_filters ? currentDiagramId() : active_diagram;
	if (!preserve_filters)
	{
		m_search->clear();
		m_scope->setCurrentIndex(0);
	}
	m_model->setEntries(entries);
	rebuildGroups();
	if (!preserve_filters) {
		m_group->setCurrentIndex(0);
	}
	applyFilters();
	selectPreferred(preferred);
	m_back->setEnabled(can_navigate_back);
	m_forward->setEnabled(can_navigate_forward);
	setWindowTitle(project_title.trimmed().isEmpty()
			? tr("Atteindre un folio")
			: tr("Atteindre un folio — %1").arg(project_title));

	if (QScreen *screen = QGuiApplication::screenAt(mapToGlobal(rect().center())))
	{
		const QSize available = screen->availableGeometry().size();
		resize(qMin(680, available.width()), qMin(500, available.height()));
	}
	show();
	raise();
	activateWindow();
	m_search->setFocus(Qt::ShortcutFocusReason);
	m_search->selectAll();
}

FolioNavigatorModel *FolioNavigatorDialog::model()
{
	return m_model;
}

bool FolioNavigatorDialog::eventFilter(QObject *watched, QEvent *event)
{
	if ((watched == m_search || watched == m_results)
			&& event->type() == QEvent::KeyPress)
	{
		auto *key_event = static_cast<QKeyEvent *>(event);
		switch (key_event->key())
		{
			case Qt::Key_Down:
				moveCurrentRow(1);
				return true;
			case Qt::Key_Up:
				moveCurrentRow(-1);
				return true;
			case Qt::Key_Return:
			case Qt::Key_Enter:
				activateCurrent();
				return true;
			case Qt::Key_Escape:
				reject();
				return true;
			default:
				break;
		}
	}
	return QDialog::eventFilter(watched, event);
}

void FolioNavigatorDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	m_search->setFocus(Qt::ShortcutFocusReason);
}

void FolioNavigatorDialog::applyFilters()
{
	const QUuid selected = currentDiagramId();
	m_model->setFilters(
			m_search->text(),
			m_group->currentData().toString(),
			currentScope());
	selectPreferred(selected);

	const int visible = m_model->rowCount();
	if (m_model->totalEntryCount() == 0) {
		m_status->setText(tr("Ce projet ne contient aucun folio."));
	} else if (visible == 0) {
		m_status->setText(tr("Aucun folio correspondant."));
	} else {
		m_status->setText(visible == 1
				? tr("1 folio correspondant")
				: tr("%1 folios correspondants").arg(visible));
	}
	updateCurrentActions();
}

void FolioNavigatorDialog::activateCurrent()
{
	const QUuid diagram_id = currentDiagramId();
	if (diagram_id.isNull()) {
		return;
	}
	accept();
	emit folioActivated(diagram_id);
}

void FolioNavigatorDialog::toggleCurrentFavorite()
{
	const QModelIndex current = m_results->currentIndex();
	const FolioNavigationEntry *entry = m_model->entryAt(current.row());
	if (!entry) {
		return;
	}
	const QUuid diagram_id = entry->diagram_id;
	const bool favorite = !entry->favorite;
	if (m_model->setFavorite(diagram_id, favorite))
	{
		emit favoriteChanged(diagram_id, favorite);
		selectPreferred(diagram_id);
		updateCurrentActions();
	}
}

void FolioNavigatorDialog::updateCurrentActions()
{
	const FolioNavigationEntry *entry = m_model->entryAt(
			m_results->currentIndex().row());
	const bool has_entry = entry != nullptr;
	m_open->setEnabled(has_entry);
	m_favorite->setEnabled(has_entry);
	if (!has_entry) {
		m_favorite->setText(tr("★ Ajouter aux favoris"));
		return;
	}
	m_favorite->setText(entry->favorite
			? tr("★ Retirer des favoris")
			: tr("★ Ajouter aux favoris"));
	m_favorite->setAccessibleName(entry->favorite
			? tr("Retirer le folio sélectionné des favoris")
			: tr("Ajouter le folio sélectionné aux favoris"));
}

void FolioNavigatorDialog::rebuildGroups()
{
	const QString previous = m_group->currentData().toString();
	m_group->blockSignals(true);
	m_group->clear();
	m_group->addItem(tr("Tous les groupes"), QString());
	for (const QString &group : m_model->groups()) {
		m_group->addItem(group, group);
	}
	const int previous_index = m_group->findData(previous);
	m_group->setCurrentIndex(previous_index >= 0 ? previous_index : 0);
	m_group->setEnabled(m_group->count() > 1);
	m_group->blockSignals(false);
}

void FolioNavigatorDialog::selectPreferred(const QUuid &preferred_id)
{
	const int row = m_model->preferredRow(preferred_id);
	if (row < 0)
	{
		m_results->setCurrentIndex(QModelIndex());
		return;
	}
	const QModelIndex index = m_model->index(row, 0);
	m_results->setCurrentIndex(index);
	m_results->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

void FolioNavigatorDialog::moveCurrentRow(int delta)
{
	if (m_model->rowCount() == 0) {
		return;
	}
	int row = m_results->currentIndex().row();
	if (row < 0) {
		row = 0;
	} else {
		row = qBound(0, row + delta, m_model->rowCount() - 1);
	}
	m_results->setCurrentIndex(m_model->index(row, 0));
	m_results->scrollTo(m_results->currentIndex());
}

QUuid FolioNavigatorDialog::currentDiagramId() const
{
	return m_model->diagramIdAt(m_results->currentIndex().row());
}

FolioNavigationIndex::Scope FolioNavigatorDialog::currentScope() const
{
	return static_cast<FolioNavigationIndex::Scope>(m_scope->currentData().toInt());
}
