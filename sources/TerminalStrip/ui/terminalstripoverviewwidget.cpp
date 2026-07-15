/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "terminalstripoverviewwidget.h"

#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QSet>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>

namespace {

constexpr int TABLE_PAGE = 0;
constexpr int MESSAGE_PAGE = 1;

}

TerminalStripOverviewWidget::TerminalStripOverviewWidget(QWidget *parent) :
	QWidget(parent),
	m_model(new TerminalStripOverviewModel(this)),
	m_proxy(new TerminalStripOverviewFilterProxyModel(this)),
	m_search_timer(new QTimer(this))
{
	setObjectName(QStringLiteral("terminalStripOverview"));
	setAccessibleName(tr("Vue d’ensemble des borniers et câbles"));
	m_proxy->setSourceModel(m_model);

	auto *root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(18, 14, 18, 14);
	root_layout->setSpacing(10);

	auto *title = new QLabel(tr("Vue d’ensemble des borniers et câbles"), this);
	title->setObjectName(QStringLiteral("terminalStripOverviewTitle"));
	QFont title_font = title->font();
	title_font.setPointSizeF(title_font.pointSizeF() + 3.0);
	title_font.setBold(true);
	title->setFont(title_font);
	root_layout->addWidget(title);

	auto *description = new QLabel(
			tr("Consultez les bornes, conducteurs et informations de câble du projet. "
			   "Cette vue n’apporte aucune modification au schéma."), this);
	description->setWordWrap(true);
	description->setObjectName(QStringLiteral("terminalStripOverviewDescription"));
	root_layout->addWidget(description);

	m_read_only_banner = new QLabel(
			tr("Projet en lecture seule : les borniers peuvent être consultés, "
			   "mais pas modifiés."), this);
	m_read_only_banner->setObjectName(QStringLiteral("terminalStripReadOnlyBanner"));
	m_read_only_banner->setAccessibleName(tr("Information de lecture seule"));
	m_read_only_banner->setWordWrap(true);
	m_read_only_banner->setFrameShape(QFrame::StyledPanel);
	m_read_only_banner->setMargin(8);
	m_read_only_banner->hide();
	root_layout->addWidget(m_read_only_banner);

	auto *filter_frame = new QFrame(this);
	filter_frame->setObjectName(QStringLiteral("terminalStripOverviewFilters"));
	filter_frame->setFrameShape(QFrame::StyledPanel);
	auto *filter_layout = new QGridLayout(filter_frame);
	filter_layout->setContentsMargins(10, 8, 10, 8);
	filter_layout->setHorizontalSpacing(10);
	filter_layout->setVerticalSpacing(6);

	auto *search_label = new QLabel(tr("Rechercher :"), filter_frame);
	m_search = new QLineEdit(filter_frame);
	m_search->setObjectName(QStringLiteral("terminalStripOverviewSearch"));
	m_search->setAccessibleName(tr("Rechercher dans les bornes et câbles"));
	m_search->setAccessibleDescription(
			tr("Recherche insensible aux majuscules et aux accents."));
	m_search->setPlaceholderText(
			tr("Repère, conducteur, référence, câble ou bornier"));
	m_search->setClearButtonEnabled(true);
	search_label->setBuddy(m_search);
	filter_layout->addWidget(search_label, 0, 0);
	filter_layout->addWidget(m_search, 0, 1, 1, 5);

	auto *assignment_label = new QLabel(tr("Affectation :"), filter_frame);
	m_assignment_filter = new QComboBox(filter_frame);
	m_assignment_filter->setObjectName(
			QStringLiteral("terminalStripOverviewAssignmentFilter"));
	m_assignment_filter->setAccessibleName(tr("Filtrer par affectation"));
	m_assignment_filter->setSizeAdjustPolicy(
			QComboBox::AdjustToMinimumContentsLengthWithIcon);
	m_assignment_filter->setMinimumContentsLength(12);
	m_assignment_filter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_assignment_filter->addItem(tr("Toutes les bornes"), 0);
	m_assignment_filter->addItem(tr("Bornes affectées à un bornier"), 1);
	m_assignment_filter->addItem(tr("Bornes indépendantes"), 2);
	assignment_label->setBuddy(m_assignment_filter);
	filter_layout->addWidget(assignment_label, 1, 0);
	filter_layout->addWidget(m_assignment_filter, 1, 1, 1, 5);

	auto *attention_label = new QLabel(tr("État :"), filter_frame);
	m_attention_filter = new QComboBox(filter_frame);
	m_attention_filter->setObjectName(
			QStringLiteral("terminalStripOverviewAttentionFilter"));
	m_attention_filter->setAccessibleName(tr("Filtrer par état"));
	m_attention_filter->setSizeAdjustPolicy(
			QComboBox::AdjustToMinimumContentsLengthWithIcon);
	m_attention_filter->setMinimumContentsLength(12);
	m_attention_filter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_attention_filter->addItem(tr("Tous les états"), 0);
	m_attention_filter->addItem(tr("Points à vérifier"), 1);
	m_attention_filter->addItem(tr("Sans alerte"), 2);
	attention_label->setBuddy(m_attention_filter);
	filter_layout->addWidget(attention_label, 2, 0);
	filter_layout->addWidget(m_attention_filter, 2, 1, 1, 5);
	filter_layout->setColumnStretch(1, 1);
	filter_layout->setColumnStretch(4, 1);
	root_layout->addWidget(filter_frame);

	m_summary = new QLabel(this);
	m_summary->setObjectName(QStringLiteral("terminalStripOverviewSummary"));
	m_summary->setAccessibleName(tr("Résumé des bornes affichées"));
	m_summary->setWordWrap(true);
	m_summary->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	root_layout->addWidget(m_summary);

	m_content = new QStackedWidget(this);
	m_content->setObjectName(QStringLiteral("terminalStripOverviewContent"));

	m_table = new QTableView(m_content);
	m_table->setObjectName(QStringLiteral("terminalStripOverviewTable"));
	m_table->setAccessibleName(tr("Tableau des borniers et câbles"));
	m_table->setAccessibleDescription(
			tr("Une ligne par borne réelle. Entrée affiche la borne dans son folio."));
	m_table->setModel(m_proxy);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setAlternatingRowColors(true);
	m_table->setSortingEnabled(true);
	// Keep the model's deterministic business order (assigned strips first,
	// natural X2/X10 ordering) until the user explicitly sorts a column.
	m_proxy->sort(-1);
	m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_table->verticalHeader()->setVisible(false);
	m_table->horizontalHeader()->setSectionsMovable(true);
	m_table->horizontalHeader()->setStretchLastSection(false);
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
	m_table->setColumnWidth(TerminalStripOverviewModel::Status, 118);
	m_table->setColumnWidth(TerminalStripOverviewModel::Assignment, 140);
	m_table->setColumnWidth(TerminalStripOverviewModel::Installation, 120);
	m_table->setColumnWidth(TerminalStripOverviewModel::Location, 120);
	m_table->setColumnWidth(TerminalStripOverviewModel::Position, 72);
	m_table->setColumnWidth(TerminalStripOverviewModel::Level, 62);
	m_table->setColumnWidth(TerminalStripOverviewModel::Label, 120);
	m_table->setColumnWidth(TerminalStripOverviewModel::Conductor, 130);
	m_table->setColumnWidth(TerminalStripOverviewModel::Xref, 130);
	m_table->setColumnWidth(TerminalStripOverviewModel::Cable, 110);
	m_table->setColumnWidth(TerminalStripOverviewModel::CableWire, 115);
	m_table->setColumnWidth(TerminalStripOverviewModel::Type, 110);
	m_table->setColumnWidth(TerminalStripOverviewModel::Function, 110);
	m_table->setColumnWidth(TerminalStripOverviewModel::Bridge, 65);
	m_content->addWidget(m_table);

	m_message_page = new QWidget(m_content);
	auto *message_layout = new QVBoxLayout(m_message_page);
	message_layout->setContentsMargins(24, 24, 24, 24);
	message_layout->addStretch();
	m_message_title = new QLabel(m_message_page);
	m_message_title->setObjectName(QStringLiteral("terminalStripOverviewMessageTitle"));
	m_message_title->setAlignment(Qt::AlignCenter);
	QFont message_font = m_message_title->font();
	message_font.setPointSizeF(message_font.pointSizeF() + 1.0);
	message_font.setBold(true);
	m_message_title->setFont(message_font);
	m_message_title->setWordWrap(true);
	message_layout->addWidget(m_message_title);
	m_message_detail = new QLabel(m_message_page);
	m_message_detail->setObjectName(QStringLiteral("terminalStripOverviewMessageDetail"));
	m_message_detail->setAlignment(Qt::AlignCenter);
	m_message_detail->setWordWrap(true);
	message_layout->addWidget(m_message_detail);
	message_layout->addStretch();
	m_content->addWidget(m_message_page);
	root_layout->addWidget(m_content, 1);

	auto *actions_layout = new QHBoxLayout;
	m_clear_filters = new QPushButton(tr("Effacer les filtres"), this);
	m_clear_filters->setObjectName(QStringLiteral("terminalStripOverviewClearFilters"));
	m_clear_filters->setAccessibleName(tr("Effacer les filtres"));
	actions_layout->addWidget(m_clear_filters);
	actions_layout->addStretch();
	m_show_in_folio = new QPushButton(tr("Afficher dans le folio"), this);
	m_show_in_folio->setObjectName(QStringLiteral("terminalStripOverviewShowInFolio"));
	m_show_in_folio->setAccessibleName(tr("Afficher la borne sélectionnée dans le folio"));
	m_show_in_folio->setDefault(true);
	actions_layout->addWidget(m_show_in_folio);
	root_layout->addLayout(actions_layout);

	m_search_timer->setSingleShot(true);
	m_search_timer->setInterval(200);
	connect(m_search, &QLineEdit::textChanged, this, [this]() {
		m_search_timer->start();
	});
	connect(m_search_timer, &QTimer::timeout,
			this, &TerminalStripOverviewWidget::applyFilters);
	connect(m_assignment_filter,
			QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, [this]() { applyFilters(); });
	connect(m_attention_filter,
			QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, [this]() { applyFilters(); });
	connect(m_clear_filters, &QPushButton::clicked,
			this, &TerminalStripOverviewWidget::clearFilters);
	connect(m_show_in_folio, &QPushButton::clicked,
			this, &TerminalStripOverviewWidget::activateCurrentRow);
	connect(m_table, &QTableView::doubleClicked,
			this, [this](const QModelIndex &) { activateCurrentRow(); });
	connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
			this, [this]() { updateActionState(); });

	m_search->installEventFilter(this);
	m_table->installEventFilter(this);

	auto *search_action = new QAction(this);
	search_action->setShortcut(QKeySequence::Find);
	search_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	addAction(search_action);
	connect(search_action, &QAction::triggered, this, [this]() {
		m_search->setFocus(Qt::ShortcutFocusReason);
		m_search->selectAll();
	});

	auto *reload_action = new QAction(this);
	reload_action->setShortcut(QKeySequence(Qt::Key_F5));
	reload_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	addAction(reload_action);
	connect(reload_action, &QAction::triggered,
			this, &TerminalStripOverviewWidget::reloadRequested);

	setTabOrder(m_search, m_assignment_filter);
	setTabOrder(m_assignment_filter, m_attention_filter);
	setTabOrder(m_attention_filter, m_table);
	setTabOrder(m_table, m_show_in_folio);
	updateState();
}

void TerminalStripOverviewWidget::setRows(
		const QVector<TerminalStripOverviewRow> &rows)
{
	const QString stable_key = selectedStableKey();
	m_model->setRows(rows);
	restoreSelection(stable_key);
	updateState();
}

void TerminalStripOverviewWidget::setReadOnly(bool read_only)
{
	m_read_only_banner->setVisible(read_only);
}

void TerminalStripOverviewWidget::setProjectAvailable(bool available)
{
	m_project_available = available;
	updateState();
}

void TerminalStripOverviewWidget::clearFilters()
{
	m_search_timer->stop();
	m_search->clear();
	m_assignment_filter->setCurrentIndex(0);
	m_attention_filter->setCurrentIndex(0);
	applyFilters();
	m_search->setFocus(Qt::ShortcutFocusReason);
}

TerminalStripOverviewModel *TerminalStripOverviewWidget::model() const
{
	return m_model;
}

TerminalStripOverviewFilterProxyModel *
TerminalStripOverviewWidget::proxyModel() const
{
	return m_proxy;
}

QTableView *TerminalStripOverviewWidget::tableView() const
{
	return m_table;
}

QUuid TerminalStripOverviewWidget::selectedElementUuid() const
{
	const QModelIndex current = m_table->currentIndex();
	return current.isValid()
			? current.data(TerminalStripOverviewModel::ElementUuidRole).toUuid()
			: QUuid();
}

bool TerminalStripOverviewWidget::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		auto *key_event = static_cast<QKeyEvent *>(event);
		if (watched == m_search && key_event->key() == Qt::Key_Down)
		{
			selectFirstVisibleRow();
			return true;
		}
		if ((watched == m_table || watched == m_search)
				&& (key_event->key() == Qt::Key_Return
					|| key_event->key() == Qt::Key_Enter))
		{
			activateCurrentRow();
			return true;
		}
		if (key_event->key() == Qt::Key_Escape)
		{
			if (!m_search->text().isEmpty())
			{
				m_search->clear();
				applyFilters();
			}
			else if (m_assignment_filter->currentIndex() != 0
					 || m_attention_filter->currentIndex() != 0)
			{
				m_assignment_filter->setCurrentIndex(0);
				m_attention_filter->setCurrentIndex(0);
				applyFilters();
			}
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void TerminalStripOverviewWidget::applyFilters()
{
	const QString stable_key = selectedStableKey();
	m_proxy->setSearchText(m_search->text());
	m_proxy->setAssignmentScope(
			static_cast<TerminalStripOverviewFilterProxyModel::AssignmentScope>(
					m_assignment_filter->currentData().toInt()));
	m_proxy->setAttentionScope(
			static_cast<TerminalStripOverviewFilterProxyModel::AttentionScope>(
					m_attention_filter->currentData().toInt()));
	restoreSelection(stable_key);
	updateState();
}

void TerminalStripOverviewWidget::updateState()
{
	if (!m_project_available)
	{
		m_message_title->setText(tr("Le projet n’est plus disponible."));
		m_message_detail->setText(
				tr("Fermez cette fenêtre et ouvrez de nouveau le gestionnaire "
				   "depuis un projet actif."));
		m_content->setCurrentIndex(MESSAGE_PAGE);
	}
	else if (m_model->rowCount() == 0)
	{
		m_message_title->setText(
				tr("Aucune borne n’a été trouvée dans ce projet."));
		m_message_detail->setText(
				tr("Placez des bornes dans un folio, puis affectez-les à un "
				   "bornier si nécessaire."));
		m_content->setCurrentIndex(MESSAGE_PAGE);
	}
	else if (m_proxy->rowCount() == 0)
	{
		m_message_title->setText(
				tr("Aucune borne ne correspond à votre recherche ou au filtre sélectionné."));
		m_message_detail->setText(tr("Utilisez « Effacer les filtres » pour tout afficher."));
		m_content->setCurrentIndex(MESSAGE_PAGE);
	}
	else
	{
		m_content->setCurrentIndex(TABLE_PAGE);
		if (!m_table->currentIndex().isValid()) selectFirstVisibleRow();
	}
	updateSummary();
	updateActionState();
}

void TerminalStripOverviewWidget::updateSummary()
{
	int attention_count = 0;
	for (int row = 0; row < m_proxy->rowCount(); ++row)
	{
		if (m_proxy->index(row, 0).data(
				TerminalStripOverviewModel::AttentionRole).toBool())
			++attention_count;
	}

	QSet<QUuid> strips;
	for (const TerminalStripOverviewRow &row : m_model->rows())
	{
		if (!row.strip_uuid.isNull()) strips.insert(row.strip_uuid);
	}
	const int visible_count = m_proxy->rowCount();
	const int total_count = m_model->rowCount();
	const int strip_count = strips.size();
	const QString visible_text = visible_count == 1
			? tr("1 borne affichée")
			: tr("%1 bornes affichées").arg(visible_count);
	const QString total_text = total_count == 1
			? tr("1 borne au total")
			: tr("%1 bornes au total").arg(total_count);
	const QString strips_text = strip_count == 1
			? tr("1 bornier")
			: tr("%1 borniers").arg(strip_count);
	const QString attention_text = attention_count == 1
			? tr("1 point à vérifier")
			: tr("%1 points à vérifier").arg(attention_count);
	m_summary->setText(tr("%1 sur %2 • %3 • %4")
			.arg(visible_text, total_text, strips_text, attention_text));
}

void TerminalStripOverviewWidget::updateActionState()
{
	const QModelIndex current = m_table->currentIndex();
	const bool navigable = m_project_available
			&& current.isValid()
			&& current.data(TerminalStripOverviewModel::NavigationAvailableRole).toBool()
			&& !current.data(TerminalStripOverviewModel::ElementUuidRole).toUuid().isNull();
	m_show_in_folio->setEnabled(navigable);
	const bool filters_active = !m_search->text().isEmpty()
			|| m_assignment_filter->currentIndex() != 0
			|| m_attention_filter->currentIndex() != 0;
	m_clear_filters->setEnabled(filters_active);
}

void TerminalStripOverviewWidget::activateCurrentRow()
{
	if (!m_show_in_folio->isEnabled()) return;
	const QUuid uuid = selectedElementUuid();
	if (!uuid.isNull()) emit showElementRequested(uuid);
}

void TerminalStripOverviewWidget::selectFirstVisibleRow()
{
	if (m_proxy->rowCount() <= 0) return;
	const QModelIndex index = m_proxy->index(0, 0);
	m_table->setCurrentIndex(index);
	m_table->selectionModel()->select(
			index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	m_table->scrollTo(index);
	m_table->setFocus(Qt::TabFocusReason);
}

void TerminalStripOverviewWidget::restoreSelection(const QString &stable_key)
{
	if (!stable_key.isEmpty())
	{
		for (int row = 0; row < m_proxy->rowCount(); ++row)
		{
			const QModelIndex index = m_proxy->index(row, 0);
			if (index.data(TerminalStripOverviewModel::StableKeyRole).toString()
					== stable_key)
			{
				m_table->setCurrentIndex(index);
				m_table->selectionModel()->select(
						index,
						QItemSelectionModel::ClearAndSelect
								| QItemSelectionModel::Rows);
				return;
			}
		}
	}
	if (m_proxy->rowCount() > 0)
	{
		const QModelIndex index = m_proxy->index(0, 0);
		m_table->setCurrentIndex(index);
		m_table->selectionModel()->select(
				index,
				QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	}
}

QString TerminalStripOverviewWidget::selectedStableKey() const
{
	const QModelIndex current = m_table->currentIndex();
	return current.isValid()
			? current.data(TerminalStripOverviewModel::StableKeyRole).toString()
			: QString();
}
