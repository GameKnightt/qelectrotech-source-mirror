/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef TERMINALSTRIPOVERVIEWWIDGET_H
#define TERMINALSTRIPOVERVIEWWIDGET_H

#include "terminalstripoverviewmodel.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableView;
class QTimer;

class TerminalStripOverviewWidget : public QWidget
{
	Q_OBJECT

	public:
		explicit TerminalStripOverviewWidget(QWidget *parent = nullptr);

		void setRows(const QVector<TerminalStripOverviewRow> &rows);
		void setReadOnly(bool read_only);
		void setProjectAvailable(bool available);
		void clearFilters();

		TerminalStripOverviewModel *model() const;
		TerminalStripOverviewFilterProxyModel *proxyModel() const;
		QTableView *tableView() const;
		QUuid selectedElementUuid() const;

	signals:
		void showElementRequested(const QUuid &element_uuid);
		void reloadRequested();

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;

	private:
		void applyFilters();
		void updateState();
		void updateSummary();
		void updateActionState();
		void activateCurrentRow();
		void selectFirstVisibleRow();
		void restoreSelection(const QString &stable_key);
		QString selectedStableKey() const;

		TerminalStripOverviewModel *m_model = nullptr;
		TerminalStripOverviewFilterProxyModel *m_proxy = nullptr;
		QLabel *m_read_only_banner = nullptr;
		QLineEdit *m_search = nullptr;
		QComboBox *m_assignment_filter = nullptr;
		QComboBox *m_attention_filter = nullptr;
		QLabel *m_summary = nullptr;
		QStackedWidget *m_content = nullptr;
		QTableView *m_table = nullptr;
		QWidget *m_message_page = nullptr;
		QLabel *m_message_title = nullptr;
		QLabel *m_message_detail = nullptr;
		QPushButton *m_clear_filters = nullptr;
		QPushButton *m_show_in_folio = nullptr;
		QTimer *m_search_timer = nullptr;
		bool m_project_available = true;
};

#endif // TERMINALSTRIPOVERVIEWWIDGET_H
