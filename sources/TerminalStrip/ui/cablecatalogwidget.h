/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef CABLECATALOGWIDGET_H
#define CABLECATALOGWIDGET_H

#include "cablecatalogmodel.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTimer;
class QTreeView;

class CableCatalogWidget final : public QWidget
{
	Q_OBJECT

	public:
		explicit CableCatalogWidget(QWidget *parent = nullptr);

		void setSnapshot(const CableCatalogSnapshot &snapshot);
		void setReadOnly(bool read_only);
		void setProjectAvailable(bool available);
		void setLoadFailed(bool failed);
		void clearFilters();
		bool exportCsvToFile(const QString &file_path) const;

		CableCatalogModel *model() const;
		CableCatalogFilterProxyModel *proxyModel() const;
		QTreeView *treeView() const;
		CableNavigationTarget selectedTarget() const;
		QVector<CableNavigationTarget> selectedTargets() const;

	signals:
		void showConductorRequested(const CableNavigationTarget &target);
		void editConductorsRequested(
				const QVector<CableNavigationTarget> &targets);
		void reloadRequested();

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;

	private:
		void applyFilters();
		void updateState();
		void updateSummary();
		void updateActionState();
		void activateCurrentRow();
		void requestEditSelectedConductors();
		void handleDoubleClick(const QModelIndex &index);
		void selectFirstVisibleRow();
		void restoreSelection(const QString &stable_key);
		QString selectedStableKey() const;
		void exportCsv();

		CableCatalogModel *m_model = nullptr;
		CableCatalogFilterProxyModel *m_proxy = nullptr;
		QLabel *m_read_only_banner = nullptr;
		QLineEdit *m_search = nullptr;
		QComboBox *m_health_filter = nullptr;
		QComboBox *m_diagnostic_filter = nullptr;
		QCheckBox *m_include_unassigned = nullptr;
		QLabel *m_summary = nullptr;
		QStackedWidget *m_content = nullptr;
		QTreeView *m_tree = nullptr;
		QLabel *m_message_title = nullptr;
		QLabel *m_message_detail = nullptr;
		QPushButton *m_clear_filters = nullptr;
		QPushButton *m_export = nullptr;
		QPushButton *m_edit_conductors = nullptr;
		QPushButton *m_show_in_folio = nullptr;
		QTimer *m_search_timer = nullptr;
		bool m_read_only = false;
		bool m_project_available = true;
		bool m_load_failed = false;
};

#endif // CABLECATALOGWIDGET_H
