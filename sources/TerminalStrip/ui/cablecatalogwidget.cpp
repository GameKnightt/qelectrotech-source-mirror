/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "cablecatalogwidget.h"

#include "../../cablecatalog/cablecatalogcsvexporter.h"

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextStream>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

constexpr int TREE_PAGE = 0;
constexpr int MESSAGE_PAGE = 1;

}

CableCatalogWidget::CableCatalogWidget(QWidget *parent) :
	QWidget(parent),
	m_model(new CableCatalogModel(this)),
	m_proxy(new CableCatalogFilterProxyModel(this)),
	m_search_timer(new QTimer(this))
{
	setObjectName(QStringLiteral("cableCatalog"));
	setAccessibleName(tr("Catalogue et diagnostics des câbles"));
	m_proxy->setSourceModel(m_model);

	auto *root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(18, 14, 18, 14);
	root_layout->setSpacing(10);

	auto *title = new QLabel(tr("Catalogue et diagnostics des câbles"), this);
	title->setObjectName(QStringLiteral("cableCatalogTitle"));
	QFont title_font = title->font();
	title_font.setPointSizeF(title_font.pointSizeF() + 3.0);
	title_font.setBold(true);
	title->setFont(title_font);
	root_layout->addWidget(title);

	auto *description = new QLabel(
			tr("Analysez les références de câble portées par tous les conducteurs du projet. "
			   "Le catalogue est consultatif et ne modifie pas le schéma."), this);
	description->setObjectName(QStringLiteral("cableCatalogDescription"));
	description->setWordWrap(true);
	root_layout->addWidget(description);

	m_read_only_banner = new QLabel(
			tr("Projet en lecture seule : les câbles peuvent être consultés, "
			   "mais pas modifiés."), this);
	m_read_only_banner->setObjectName(QStringLiteral("cableCatalogReadOnlyBanner"));
	m_read_only_banner->setAccessibleName(tr("Information de lecture seule"));
	m_read_only_banner->setWordWrap(true);
	m_read_only_banner->setFrameShape(QFrame::StyledPanel);
	m_read_only_banner->setMargin(8);
	m_read_only_banner->hide();
	root_layout->addWidget(m_read_only_banner);

	auto *filter_frame = new QFrame(this);
	filter_frame->setObjectName(QStringLiteral("cableCatalogFilters"));
	filter_frame->setFrameShape(QFrame::StyledPanel);
	auto *filter_layout = new QGridLayout(filter_frame);
	filter_layout->setContentsMargins(10, 8, 10, 8);
	filter_layout->setHorizontalSpacing(10);
	filter_layout->setVerticalSpacing(6);

	auto *search_label = new QLabel(tr("Rechercher :"), filter_frame);
	m_search = new QLineEdit(filter_frame);
	m_search->setObjectName(QStringLiteral("cableCatalogSearch"));
	m_search->setAccessibleName(tr("Rechercher dans les câbles"));
	m_search->setAccessibleDescription(
			tr("Recherche insensible aux majuscules et aux accents."));
	m_search->setPlaceholderText(
			tr("Câble, âme, conducteur, folio ou extrémité"));
	m_search->setClearButtonEnabled(true);
	search_label->setBuddy(m_search);
	filter_layout->addWidget(search_label, 0, 0);
	filter_layout->addWidget(m_search, 0, 1, 1, 5);

	auto *health_label = new QLabel(tr("État :"), filter_frame);
	m_health_filter = new QComboBox(filter_frame);
	m_health_filter->setObjectName(QStringLiteral("cableCatalogHealthFilter"));
	m_health_filter->setAccessibleName(tr("Filtrer les câbles par état"));
	m_health_filter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_health_filter->addItem(tr("Tous les états"), 0);
	m_health_filter->addItem(tr("Câbles à vérifier"), 1);
	m_health_filter->addItem(tr("Sans alerte"), 2);
	health_label->setBuddy(m_health_filter);
	filter_layout->addWidget(health_label, 1, 0);
	filter_layout->addWidget(m_health_filter, 1, 1, 1, 2);

	auto *diagnostic_label = new QLabel(tr("Diagnostic :"), filter_frame);
	m_diagnostic_filter = new QComboBox(filter_frame);
	m_diagnostic_filter->setObjectName(QStringLiteral("cableCatalogDiagnosticFilter"));
	m_diagnostic_filter->setAccessibleName(tr("Filtrer par type de diagnostic"));
	m_diagnostic_filter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_diagnostic_filter->addItem(tr("Tous les diagnostics"), 0);
	m_diagnostic_filter->addItem(tr("Âme / couleur non renseignée"), 1);
	m_diagnostic_filter->addItem(tr("Âme / couleur réutilisée"), 2);
	m_diagnostic_filter->addItem(tr("Libellé de câble proche"), 3);
	m_diagnostic_filter->addItem(tr("Topologie ou identité"), 4);
	m_diagnostic_filter->addItem(tr("Informations de cohérence"), 5);
	diagnostic_label->setBuddy(m_diagnostic_filter);
	filter_layout->addWidget(diagnostic_label, 2, 0);
	filter_layout->addWidget(m_diagnostic_filter, 2, 1, 1, 2);

	m_include_unassigned = new QCheckBox(
			tr("Inclure les conducteurs sans câble"), filter_frame);
	m_include_unassigned->setObjectName(QStringLiteral("cableCatalogIncludeUnassigned"));
	m_include_unassigned->setAccessibleName(
			tr("Inclure les conducteurs sans référence de câble"));
	// Keep the long option on its own row. This preserves a usable minimum
	// width with Windows large-text settings instead of forcing the dialog
	// wider than the logical desktop.
	filter_layout->addWidget(m_include_unassigned, 3, 1, 1, 5);
	filter_layout->setColumnStretch(1, 1);
	filter_layout->setColumnStretch(4, 1);
	root_layout->addWidget(filter_frame);

	m_summary = new QLabel(this);
	m_summary->setObjectName(QStringLiteral("cableCatalogSummary"));
	m_summary->setAccessibleName(tr("Résumé des câbles affichés"));
	m_summary->setWordWrap(true);
	m_summary->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	root_layout->addWidget(m_summary);

	m_content = new QStackedWidget(this);
	m_content->setObjectName(QStringLiteral("cableCatalogContent"));
	m_tree = new QTreeView(m_content);
	m_tree->setObjectName(QStringLiteral("cableCatalogTree"));
	m_tree->setAccessibleName(tr("Catalogue hiérarchique des câbles"));
	m_tree->setAccessibleDescription(
			tr("Chaque câble peut être développé pour consulter ses conducteurs. "
			   "Entrée affiche la sélection dans son folio."));
	m_tree->setModel(m_proxy);
	m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	m_tree->setAlternatingRowColors(true);
	m_tree->setSortingEnabled(true);
	m_tree->sortByColumn(CableCatalogModel::Cable, Qt::AscendingOrder);
	m_tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_tree->setUniformRowHeights(true);
	m_tree->header()->setSectionsMovable(true);
	m_tree->header()->setStretchLastSection(false);
	m_tree->header()->setSectionResizeMode(QHeaderView::Interactive);
	m_tree->setColumnWidth(CableCatalogModel::Status, 105);
	m_tree->setColumnWidth(CableCatalogModel::Cable, 155);
	m_tree->setColumnWidth(CableCatalogModel::WireColor, 125);
	m_tree->setColumnWidth(CableCatalogModel::Conductor, 125);
	m_tree->setColumnWidth(CableCatalogModel::Folio, 90);
	m_tree->setColumnWidth(CableCatalogModel::From, 140);
	m_tree->setColumnWidth(CableCatalogModel::To, 140);
	m_tree->setColumnWidth(CableCatalogModel::Section, 80);
	m_tree->setColumnWidth(CableCatalogModel::Function, 110);
	m_tree->setColumnWidth(CableCatalogModel::TensionProtocol, 130);
	m_tree->setColumnWidth(CableCatalogModel::Diagnostics, 260);
	m_content->addWidget(m_tree);

	auto *message_page = new QWidget(m_content);
	auto *message_layout = new QVBoxLayout(message_page);
	message_layout->setContentsMargins(24, 24, 24, 24);
	message_layout->addStretch();
	m_message_title = new QLabel(message_page);
	m_message_title->setObjectName(QStringLiteral("cableCatalogMessageTitle"));
	m_message_title->setAlignment(Qt::AlignCenter);
	QFont message_font = m_message_title->font();
	message_font.setPointSizeF(message_font.pointSizeF() + 1.0);
	message_font.setBold(true);
	m_message_title->setFont(message_font);
	m_message_title->setWordWrap(true);
	message_layout->addWidget(m_message_title);
	m_message_detail = new QLabel(message_page);
	m_message_detail->setObjectName(QStringLiteral("cableCatalogMessageDetail"));
	m_message_detail->setAlignment(Qt::AlignCenter);
	m_message_detail->setWordWrap(true);
	message_layout->addWidget(m_message_detail);
	message_layout->addStretch();
	m_content->addWidget(message_page);
	root_layout->addWidget(m_content, 1);

	auto *actions_layout = new QGridLayout;
	actions_layout->setContentsMargins(0, 0, 0, 0);
	actions_layout->setHorizontalSpacing(8);
	actions_layout->setVerticalSpacing(6);
	m_clear_filters = new QPushButton(tr("Effacer les filtres"), this);
	m_clear_filters->setObjectName(QStringLiteral("cableCatalogClearFilters"));
	m_clear_filters->setAccessibleName(tr("Effacer les filtres du catalogue"));
	actions_layout->addWidget(m_clear_filters, 0, 0);
	m_export = new QPushButton(tr("Exporter le catalogue…"), this);
	m_export->setObjectName(QStringLiteral("cableCatalogExport"));
	m_export->setAccessibleName(tr("Exporter le catalogue des câbles en CSV"));
	actions_layout->addWidget(m_export, 0, 1);
	actions_layout->setColumnStretch(2, 1);
	m_show_in_folio = new QPushButton(tr("Afficher dans le folio"), this);
	m_show_in_folio->setObjectName(QStringLiteral("cableCatalogShowInFolio"));
	m_show_in_folio->setAccessibleName(
			tr("Afficher le câble ou le conducteur sélectionné dans son folio"));
	m_show_in_folio->setDefault(true);
	actions_layout->addWidget(m_show_in_folio, 1, 0, 1, 3, Qt::AlignRight);
	root_layout->addLayout(actions_layout);

	m_search_timer->setSingleShot(true);
	m_search_timer->setInterval(200);
	connect(m_search, &QLineEdit::textChanged, this,
			[this]() { m_search_timer->start(); });
	connect(m_search_timer, &QTimer::timeout,
			this, &CableCatalogWidget::applyFilters);
	connect(m_health_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, [this]() { applyFilters(); });
	connect(m_diagnostic_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, [this]() { applyFilters(); });
	connect(m_include_unassigned, &QCheckBox::toggled,
			this, [this]() { applyFilters(); });
	connect(m_clear_filters, &QPushButton::clicked,
			this, &CableCatalogWidget::clearFilters);
	connect(m_export, &QPushButton::clicked,
			this, &CableCatalogWidget::exportCsv);
	connect(m_show_in_folio, &QPushButton::clicked,
			this, &CableCatalogWidget::activateCurrentRow);
	connect(m_tree, &QTreeView::doubleClicked,
			this, &CableCatalogWidget::handleDoubleClick);
	connect(m_tree->selectionModel(), &QItemSelectionModel::selectionChanged,
			this, [this]() { updateActionState(); });

	m_search->installEventFilter(this);
	m_tree->installEventFilter(this);
	auto *find_action = new QAction(this);
	find_action->setShortcut(QKeySequence::Find);
	find_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	addAction(find_action);
	connect(find_action, &QAction::triggered, this, [this]() {
		m_search->setFocus(Qt::ShortcutFocusReason);
		m_search->selectAll();
	});
	auto *reload_action = new QAction(this);
	reload_action->setShortcut(QKeySequence(Qt::Key_F5));
	reload_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	addAction(reload_action);
	connect(reload_action, &QAction::triggered,
			this, &CableCatalogWidget::reloadRequested);

	setTabOrder(m_search, m_health_filter);
	setTabOrder(m_health_filter, m_diagnostic_filter);
	setTabOrder(m_diagnostic_filter, m_include_unassigned);
	setTabOrder(m_include_unassigned, m_tree);
	setTabOrder(m_tree, m_show_in_folio);
	updateState();
}

void CableCatalogWidget::setSnapshot(const CableCatalogSnapshot &snapshot)
{
	const QString key = selectedStableKey();
	m_model->setSnapshot(snapshot);
	restoreSelection(key);
	updateState();
}

void CableCatalogWidget::setReadOnly(bool read_only)
{
	m_read_only_banner->setVisible(read_only);
}

void CableCatalogWidget::setProjectAvailable(bool available)
{
	m_project_available = available;
	updateState();
}

void CableCatalogWidget::setLoadFailed(bool failed)
{
	m_load_failed = failed;
	updateState();
}

void CableCatalogWidget::clearFilters()
{
	m_search_timer->stop();
	m_search->clear();
	m_health_filter->setCurrentIndex(0);
	m_diagnostic_filter->setCurrentIndex(0);
	m_include_unassigned->setChecked(false);
	applyFilters();
	m_search->setFocus(Qt::ShortcutFocusReason);
}

CableCatalogModel *CableCatalogWidget::model() const { return m_model; }
CableCatalogFilterProxyModel *CableCatalogWidget::proxyModel() const { return m_proxy; }
QTreeView *CableCatalogWidget::treeView() const { return m_tree; }

CableNavigationTarget CableCatalogWidget::selectedTarget() const
{
	const QModelIndex current = m_tree->currentIndex();
	return current.isValid()
			? current.data(CableCatalogModel::NavigationTargetRole)
					.value<CableNavigationTarget>()
			: CableNavigationTarget();
}

bool CableCatalogWidget::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		auto *key_event = static_cast<QKeyEvent *>(event);
		if (watched == m_search && key_event->key() == Qt::Key_Down)
		{
			selectFirstVisibleRow();
			return true;
		}
		if ((watched == m_search || watched == m_tree)
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
			else if (m_health_filter->currentIndex() != 0
					 || m_diagnostic_filter->currentIndex() != 0
					 || m_include_unassigned->isChecked())
			{
				m_health_filter->setCurrentIndex(0);
				m_diagnostic_filter->setCurrentIndex(0);
				m_include_unassigned->setChecked(false);
				applyFilters();
			}
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void CableCatalogWidget::applyFilters()
{
	const QString key = selectedStableKey();
	m_proxy->setSearchText(m_search->text());
	m_proxy->setHealthScope(
			static_cast<CableCatalogFilterProxyModel::HealthScope>(
					m_health_filter->currentData().toInt()));
	m_proxy->setDiagnosticScope(
			static_cast<CableCatalogFilterProxyModel::DiagnosticScope>(
					m_diagnostic_filter->currentData().toInt()));
	m_proxy->setIncludeUnassigned(m_include_unassigned->isChecked());
	restoreSelection(key);
	updateState();
}

void CableCatalogWidget::updateState()
{
	if (!m_project_available)
	{
		m_message_title->setText(tr("Le projet n’est plus disponible."));
		m_message_detail->setText(
				tr("Fermez cette fenêtre et ouvrez de nouveau le gestionnaire "
				   "depuis un projet actif."));
		m_content->setCurrentIndex(MESSAGE_PAGE);
	}
	else if (m_load_failed)
	{
		m_message_title->setText(tr("Impossible de charger le catalogue des câbles."));
		m_message_detail->setText(
				tr("Le projet n’a pas été modifié. Essayez de recharger la vue."));
		m_content->setCurrentIndex(MESSAGE_PAGE);
	}
	else if (m_model->snapshot().assigned_conductor_count == 0
			&& !m_include_unassigned->isChecked())
	{
		m_message_title->setText(tr("Aucun câble n’a été trouvé dans ce projet."));
		m_message_detail->setText(
				tr("Renseignez le champ « Câble » dans les propriétés d’un conducteur "
				   "pour le faire apparaître ici."));
		m_content->setCurrentIndex(MESSAGE_PAGE);
	}
	else if (m_proxy->rowCount() == 0)
	{
		m_message_title->setText(
				tr("Aucun câble ne correspond à votre recherche ou aux filtres sélectionnés."));
		m_message_detail->setText(
				tr("Utilisez « Effacer les filtres » pour tout afficher."));
		m_content->setCurrentIndex(MESSAGE_PAGE);
	}
	else
	{
		m_content->setCurrentIndex(TREE_PAGE);
		if (!m_tree->currentIndex().isValid()) selectFirstVisibleRow();
	}
	updateSummary();
	updateActionState();
}

void CableCatalogWidget::updateSummary()
{
	int visible = 0;
	int attention = 0;
	for (int row = 0; row < m_proxy->rowCount(); ++row)
	{
		const QModelIndex index = m_proxy->index(row, 0);
		if (!index.data(CableCatalogModel::AssignedRole).toBool()
				&& !m_include_unassigned->isChecked()) continue;
		++visible;
		if (index.data(CableCatalogModel::AttentionRole).toBool()) ++attention;
	}
	int total = 0;
	for (const CableCatalogEntry &entry : m_model->snapshot().entries)
	{
		if (entry.assigned || m_include_unassigned->isChecked()) ++total;
	}
	const QString visible_text = visible == 1
			? tr("1 câble affiché") : tr("%1 câbles affichés").arg(visible);
	const QString total_text = total == 1
			? tr("1 câble au total") : tr("%1 câbles au total").arg(total);
	const QString attention_text = attention == 1
			? tr("1 câble à vérifier") : tr("%1 câbles à vérifier").arg(attention);
	m_summary->setText(tr("%1 sur %2 • %3")
			.arg(visible_text, total_text, attention_text));
}

void CableCatalogWidget::updateActionState()
{
	const QModelIndex current = m_tree->currentIndex();
	m_show_in_folio->setEnabled(m_project_available && current.isValid()
			&& current.data(CableCatalogModel::NavigationAvailableRole).toBool());
	const bool filters_active = !m_search->text().isEmpty()
			|| m_health_filter->currentIndex() != 0
			|| m_diagnostic_filter->currentIndex() != 0
			|| m_include_unassigned->isChecked();
	m_clear_filters->setEnabled(filters_active);
	m_export->setEnabled(m_project_available && !m_load_failed
			&& m_model->snapshot().conductor_count > 0);
}

void CableCatalogWidget::activateCurrentRow()
{
	if (!m_show_in_folio->isEnabled()) return;
	const CableNavigationTarget target = selectedTarget();
	if (target.isValid()) emit showConductorRequested(target);
}

void CableCatalogWidget::handleDoubleClick(const QModelIndex &index)
{
	if (!index.isValid()) return;
	if (index.data(CableCatalogModel::NodeTypeRole).toInt()
			== CableCatalogModel::CableNode)
	{
		m_tree->setExpanded(index, !m_tree->isExpanded(index));
		return;
	}
	activateCurrentRow();
}

void CableCatalogWidget::selectFirstVisibleRow()
{
	if (m_proxy->rowCount() <= 0) return;
	const QModelIndex index = m_proxy->index(0, 0);
	m_tree->setCurrentIndex(index);
	m_tree->selectionModel()->select(
			index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
	m_tree->scrollTo(index);
	m_tree->setFocus(Qt::TabFocusReason);
}

void CableCatalogWidget::restoreSelection(const QString &stable_key)
{
	if (!stable_key.isEmpty())
	{
		for (int row = 0; row < m_proxy->rowCount(); ++row)
		{
			const QModelIndex parent_index = m_proxy->index(row, 0);
			if (parent_index.data(CableCatalogModel::StableKeyRole).toString()
					== stable_key)
			{
				m_tree->setCurrentIndex(parent_index);
				return;
			}
			for (int child = 0; child < m_proxy->rowCount(parent_index); ++child)
			{
				const QModelIndex child_index = m_proxy->index(child, 0, parent_index);
				if (child_index.data(CableCatalogModel::StableKeyRole).toString()
						== stable_key)
				{
					m_tree->expand(parent_index);
					m_tree->setCurrentIndex(child_index);
					return;
				}
			}
		}
	}
	if (m_proxy->rowCount() > 0)
		m_tree->setCurrentIndex(m_proxy->index(0, 0));
}

QString CableCatalogWidget::selectedStableKey() const
{
	const QModelIndex current = m_tree->currentIndex();
	return current.isValid()
			? current.data(CableCatalogModel::StableKeyRole).toString()
			: QString();
}

bool CableCatalogWidget::exportCsvToFile(const QString &file_path) const
{
	QFile file(file_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
	QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	stream.setCodec("UTF-8");
#endif
	stream << CableCatalogCsvExporter::toCsv(
			m_model->snapshot(), m_include_unassigned->isChecked());
	return stream.status() == QTextStream::Ok;
}

void CableCatalogWidget::exportCsv()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setWindowTitle(tr("Exporter le catalogue des câbles"));
	dialog.setDefaultSuffix(QStringLiteral("csv"));
	dialog.setNameFilter(tr("Fichiers CSV (*.csv)"));
	dialog.selectFile(QStringLiteral("catalogue-cables.csv"));
	if (dialog.exec() != QDialog::Accepted) return;
	if (!exportCsvToFile(dialog.selectedFiles().first()))
	{
		QMessageBox::warning(this, tr("Export impossible"),
				tr("Le fichier n’a pas pu être écrit."));
		return;
	}
	QMessageBox::information(this, tr("Export terminé"),
			tr("Le catalogue des câbles a été exporté."));
}
