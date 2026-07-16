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
#include "configdialog.h"
#include "ui/configpage/configpage.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QShowEvent>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWindow>

#include <algorithm>

/**
	Constructeur
	@param parent QWidget parent
*/
ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent) {
	setObjectName(QStringLiteral("qetConfigDialog"));
	setAccessibleName(tr("Configuration de QElectroTech"));
	// liste des pages
	pages_list = new QListWidget(this);
	pages_list->setObjectName(QStringLiteral("configPagesList"));
	pages_list -> setViewMode(QListView::ListMode);
	// Keep this size in logical pixels. Scaling it with the physical screen
	// height makes the navigation excessively large on high-DPI displays.
	pages_list -> setIconSize(QSize(32, 32));
	pages_list -> setMovement(QListView::Static);
	pages_list -> setMinimumWidth(220);
	pages_list -> setMaximumWidth(232);
	pages_list -> setSpacing(2);
	pages_list -> setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	pages_list -> setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	pages_list -> setTextElideMode(Qt::ElideRight);
	pages_list -> setWordWrap(true);
	pages_list -> setAccessibleName(tr("Configuration"));

	// pages
	page_title = new QLabel(this);
	page_title->setObjectName(QStringLiteral("configPageTitle"));
	page_title->setTextFormat(Qt::PlainText);
	page_title->setAccessibleName(tr("Configuration"));
	QFont title_font = page_title->font();
	title_font.setWeight(QFont::DemiBold);
	if (title_font.pointSizeF() > 0) {
		title_font.setPointSizeF(title_font.pointSizeF() + 3.0);
	} else if (title_font.pixelSize() > 0) {
		title_font.setPixelSize(title_font.pixelSize() + 4);
	}
	page_title->setFont(title_font);

	pages_widget = new QStackedWidget(this);
	pages_widget->setObjectName(QStringLiteral("configPagesStack"));
	// boutons
	buttons = new QDialogButtonBox(
				QDialogButtonBox::Ok
					|QDialogButtonBox::Cancel,
				Qt::Horizontal,
				this);
	buttons->setObjectName(QStringLiteral("configDialogButtonBox"));

	// layouts
	QHBoxLayout *hlayout1 = new QHBoxLayout();
	hlayout1->setContentsMargins(0, 0, 0, 0);
	hlayout1->setSpacing(18);
	hlayout1 -> addWidget(pages_list);
	QVBoxLayout *page_layout = new QVBoxLayout();
	page_layout->setContentsMargins(0, 0, 0, 0);
	page_layout->setSpacing(12);
	page_layout->addWidget(page_title);
	page_layout->addWidget(pages_widget, 1);
	hlayout1 -> addLayout(page_layout, 1);

	// Add a layout for QDialog
	QVBoxLayout *dialog_layout = new QVBoxLayout(this);
	dialog_layout->setContentsMargins(16, 16, 16, 16);
	dialog_layout->setSpacing(14);
	dialog_layout -> addLayout(hlayout1, 1);
	dialog_layout -> addWidget(buttons);
	setLayout(dialog_layout);
	setSizeGripEnabled(true);

	// connexion signaux / slots
	connect(buttons, SIGNAL(accepted()), this, SLOT(applyConf()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	connect(pages_list, SIGNAL(currentRowChanged(int)),
		pages_widget, SLOT(setCurrentIndex(int)));
	connect(
			pages_list,
			&QListWidget::currentRowChanged,
			this,
			[this](int index) {
				page_title->setText(
						index >= 0 && index < pages.size()
							? pages.at(index)->title()
							: QString());
			});

	// A comfortable initial size which will be bounded to the logical available
	// geometry of the actual screen in showEvent().
	resize(m_desired_size);

#ifdef Q_OS_MACOS
	if (parent) {
		setWindowFlags(Qt::Sheet);
	}
#endif
}

/// Destructeur
ConfigDialog::~ConfigDialog()
{
}

/**
	Construit la liste des pages sur la gauche
*/
void ConfigDialog::buildPagesList()
{
	pages_list -> clear();
	foreach(ConfigPage *page, pages) {
		addPageToList(page);
	}
}

/**
	Add the \a page ConfigPage to this configuration dialog.
*/
void ConfigDialog::addPageToList(ConfigPage *page) {
	QListWidgetItem *new_button = new QListWidgetItem(pages_list);
	new_button -> setIcon(page -> icon());
	new_button -> setText(page -> title());
	new_button -> setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	new_button -> setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	const int text_width =
		(std::max)(120, pages_list->minimumWidth() - 72);
	const int text_height = QFontMetrics(pages_list->font()).boundingRect(
			QRect(0, 0, text_width, 1000),
			Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
			page->title()).height();
	new_button->setSizeHint(
		QSize(0, (std::max)(52, text_height + 20)));
	new_button->setData(Qt::AccessibleTextRole, page->title());
}

/**
	Applique la configuration de toutes les pages
*/
void ConfigDialog::applyConf()
{
	foreach(ConfigPage *page, pages) {
		page -> applyConf();
	}
	accept();
}

/**
	Ajoute une page au dialogue de configuration
*/
void ConfigDialog::addPage(ConfigPage *page) {
	if (!page || pages.contains(page)) return;
	pages << page;

	// Each page owns its scroll position. Keeping the navigation outside these
	// areas makes both the current section and the action buttons permanently
	// reachable, even when a page has a large size hint or minimum size.
	QScrollArea *page_scroll = new QScrollArea(pages_widget);
	page_scroll->setObjectName(QStringLiteral("configPageScrollArea"));
	page_scroll->setWidgetResizable(true);
	page_scroll->setFrameShape(QFrame::NoFrame);
	page_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	page_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	page_scroll->setFocusPolicy(Qt::NoFocus);
	page_scroll->setWidget(page);
	pages_widget -> addWidget(page_scroll);
	addPageToList(page);

	if (pages.size() == 1) {
		pages_list->setCurrentRow(0);
		page_title->setText(page->title());
	}
}

/**
	@brief ConfigDialog::setCurrentPage
	Set the current index to index
	@param index
*/
void ConfigDialog::setCurrentPage(const int index) {
	pages_list->setCurrentRow(index);
}

/**
	@brief ConfigDialog::fitToAvailableGeometry
	Keep the dialog inside the logical available geometry supplied by Qt. This
	geometry already accounts for the task bar and the device scale factor.
*/
void ConfigDialog::fitToAvailableGeometry(const QRect &available_geometry)
{
	applyAvailableGeometry(available_geometry, false);
}

void ConfigDialog::applyAvailableGeometry(
		const QRect &available_geometry,
		bool center_dialog)
{
	if (!available_geometry.isValid()) return;

	const QSize maximum_size {
		(std::max)(1, static_cast<int>(available_geometry.width() * 0.94)),
		(std::max)(1, static_cast<int>(available_geometry.height() * 0.94))
	};
	m_geometry_update_in_progress = true;
	setMaximumSize(maximum_size);
	resize(m_desired_size.boundedTo(maximum_size));

	const QRect current_frame = frameGeometry();
	QRect target_frame = current_frame;
	if (center_dialog) {
		target_frame.moveCenter(available_geometry.center());
	} else {
		const int maximum_left =
				available_geometry.right() - current_frame.width() + 1;
		const int maximum_top =
				available_geometry.bottom() - current_frame.height() + 1;
		target_frame.moveTopLeft(QPoint(
				std::clamp(
						current_frame.left(),
						available_geometry.left(),
						(std::max)(available_geometry.left(), maximum_left)),
				std::clamp(
						current_frame.top(),
						available_geometry.top(),
						(std::max)(available_geometry.top(), maximum_top))));
	}
	move(pos() + target_frame.topLeft() - current_frame.topLeft());
	m_geometry_update_in_progress = false;
}

/**
	@brief ConfigDialog::availableGeometry
	Return the available geometry of the screen which actually contains the
	dialog or its parent. The primary screen is only a last-resort fallback.
*/
QRect ConfigDialog::availableGeometry() const
{
	QScreen *screen = nullptr;
	if (windowHandle()) {
		screen = windowHandle()->screen();
	}
	if (!screen && parentWidget() && parentWidget()->windowHandle()) {
		screen = parentWidget()->windowHandle()->screen();
	}
	if (!screen && parentWidget()) {
		screen = QGuiApplication::screenAt(
				parentWidget()->frameGeometry().center());
	}
	if (!screen) {
		screen = QGuiApplication::primaryScreen();
	}

	return screen ? screen->availableGeometry() : QRect();
}

/**
	@brief ConfigDialog::followScreen
	Follow both screen changes and task-bar/available-geometry changes while the
	dialog is open. The desired user size is restored when space becomes
	available again.
*/
void ConfigDialog::followScreen(QScreen *screen, bool center_dialog)
{
	QObject::disconnect(m_available_geometry_connection);
	if (!screen) {
		screen = QGuiApplication::primaryScreen();
	}
	if (!screen) return;

	m_available_geometry_connection = connect(
			screen,
			&QScreen::availableGeometryChanged,
			this,
			[this](const QRect &geometry) {
				applyAvailableGeometry(geometry, false);
			});
	applyAvailableGeometry(screen->availableGeometry(), center_dialog);
}

void ConfigDialog::resizeEvent(QResizeEvent *event)
{
	QDialog::resizeEvent(event);
	if (!m_geometry_update_in_progress) {
		m_desired_size = event->size();
	}
}

void ConfigDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	const int current_page = pages_list->currentRow();
	page_title->setText(
			current_page >= 0 && current_page < pages.size()
				? pages.at(current_page)->title()
				: QString());
	if (windowHandle() && !m_screen_tracking_initialized) {
		connect(
				windowHandle(),
				&QWindow::screenChanged,
				this,
				[this](QScreen *screen) {
					followScreen(screen, false);
				});
		m_screen_tracking_initialized = true;
	}

	QScreen *screen = windowHandle() ? windowHandle()->screen() : nullptr;
	if (!screen && parentWidget() && parentWidget()->windowHandle()) {
		screen = parentWidget()->windowHandle()->screen();
	}
	if (screen) {
		followScreen(screen, true);
	} else {
		applyAvailableGeometry(availableGeometry(), true);
	}
}
