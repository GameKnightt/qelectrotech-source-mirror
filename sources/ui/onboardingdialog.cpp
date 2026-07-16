/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "onboardingdialog.h"

#include <QApplication>
#include <QDebug>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>
#include <QWindow>

namespace {

QLabel *createTextLabel(
		const QString &text,
		QWidget *parent,
		const QString &object_name = QString())
{
	auto label = new QLabel(text, parent);
	label->setWordWrap(true);
	label->setTextFormat(Qt::PlainText);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse);
	if (!object_name.isEmpty()) label->setObjectName(object_name);
	return label;
}

}

OnboardingDialog::OnboardingDialog(QWidget *parent) :
	QDialog(parent)
{
	setObjectName(QStringLiteral("onboardingDialog"));
	setWindowTitle(tr("Bien démarrer avec QElectroTech"));
	QIcon application_icon = parent ? parent->windowIcon() : QApplication::windowIcon();
	if (application_icon.isNull()) {
		application_icon = style()->standardIcon(QStyle::SP_FileDialogInfoView);
	}
	setWindowIcon(application_icon);
	setWindowModality(Qt::ApplicationModal);
	setSizeGripEnabled(true);
	setMinimumSize(420, 320);
	resize(760, 560);
	setAccessibleName(windowTitle());
	setAccessibleDescription(tr(
		"Introduction aux principales zones de travail et aux premières "
		"étapes de création d'un schéma."));

	auto root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(0, 0, 0, 0);
	root_layout->setSpacing(0);

	auto accent_line = new QFrame(this);
	accent_line->setObjectName(QStringLiteral("onboardingAccentLine"));
	accent_line->setFixedHeight(5);
	accent_line->setFrameShape(QFrame::NoFrame);
	root_layout->addWidget(accent_line);

	auto content = new QWidget(this);
	auto content_layout = new QVBoxLayout(content);
	content_layout->setContentsMargins(28, 22, 28, 22);
	content_layout->setSpacing(16);

	auto header_layout = new QHBoxLayout;
	header_layout->setContentsMargins(0, 0, 0, 0);
	header_layout->setSpacing(14);

	auto logo = new QLabel(content);
	const int icon_size = qMax(
		48,
		style()->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, this));
	logo->setPixmap(application_icon.pixmap(icon_size, icon_size));
	logo->setFixedSize(icon_size, icon_size);
	logo->setAccessibleName(tr("QElectroTech"));
	header_layout->addWidget(logo, 0, Qt::AlignTop);

	auto heading_layout = new QVBoxLayout;
	heading_layout->setContentsMargins(0, 0, 0, 0);
	heading_layout->setSpacing(4);
	auto heading = createTextLabel(
		tr("Bien démarrer avec QElectroTech"),
		content,
		QStringLiteral("onboardingHeading"));
	QFont heading_font = heading->font();
	heading_font.setWeight(QFont::DemiBold);
	if (heading_font.pointSizeF() > 0) {
		heading_font.setPointSizeF(heading_font.pointSizeF() + 4.0);
	}
	heading->setFont(heading_font);
	m_step_label = createTextLabel(
		QString(),
		content,
		QStringLiteral("onboardingStepLabel"));
	m_step_label->setForegroundRole(QPalette::Text);
	heading_layout->addWidget(heading);
	heading_layout->addWidget(m_step_label);
	header_layout->addLayout(heading_layout, 1);
	content_layout->addLayout(header_layout);

	m_progress = new QProgressBar(content);
	m_progress->setObjectName(QStringLiteral("onboardingProgress"));
	m_progress->setRange(1, 4);
	m_progress->setTextVisible(false);
	m_progress->setAccessibleName(tr("Progression de l'introduction"));
	content_layout->addWidget(m_progress);

	m_pages = new QStackedWidget(content);
	m_pages->setObjectName(QStringLiteral("onboardingPages"));
	m_pages->addWidget(createPage(
		tr("Bienvenue dans votre atelier de schémas"),
		tr(
			"QElectroTech réunit dans un même espace les outils nécessaires "
			"pour créer et maintenir des schémas techniques professionnels."),
		{
			{
				tr("Plusieurs disciplines"),
				tr(
					"Électricité, automatisme, pneumatique, hydraulique et "
					"process utilisent le même projet structuré.")
			},
			{
				tr("Vos formats restent compatibles"),
				tr(
					"Le fork conserve les projets .qet, les éléments .elmt, "
					"les cartouches XML et les collections existantes.")
			},
			{
				tr("Une interface progressive"),
				tr(
					"Le profil Essentiel met les actions fréquentes en avant. "
					"Le profil Classique reste disponible pour les usages avancés.")
			}
		}));
	m_pages->addWidget(createPage(
		tr("Repérez les trois zones principales"),
		tr(
			"L'espace de travail garde les informations importantes près du "
			"schéma afin de limiter les allers-retours entre fenêtres."),
		{
			{
				tr("Projets et folios"),
				tr(
					"Naviguez dans la structure du dossier, renommez les folios "
					"et retrouvez rapidement les références croisées.")
			},
			{
				tr("Composants"),
				tr(
					"Recherchez un symbole puis déposez-le sur le folio. Les "
					"collections commune, entreprise et personnelle restent séparées.")
			},
			{
				tr("Propriétés"),
				tr(
					"Sélectionnez un élément ou un conducteur pour afficher "
					"uniquement les réglages utiles au contexte.")
			}
		}));
	m_pages->addWidget(createPage(
		tr("Créez un premier schéma sans friction"),
		tr(
			"Le parcours suivant suffit pour produire un premier projet "
			"enregistrable et prêt à être revu."),
		{
			{
				tr("1. Créer ou ouvrir"),
				tr("Utilisez Ctrl+N pour un projet neuf ou Ctrl+O pour ouvrir une copie.")
			},
			{
				tr("2. Placer et renseigner"),
				tr(
					"Déposez les composants, puis double-cliquez pour compléter "
					"leurs propriétés et références fabricant.")
			},
			{
				tr("3. Relier et contrôler"),
				tr(
					"Reliez les bornes, vérifiez les conducteurs et utilisez "
					"Annuler/Rétablir dès qu'une modification doit être reprise.")
			},
			{
				tr("4. Enregistrer et exporter"),
				tr(
					"Ctrl+S enregistre le projet. Le centre d'export regroupe "
					"les sorties PDF, image et les données métier disponibles.")
			}
		}));
	m_pages->addWidget(createPage(
		tr("Travaillez en confiance"),
		tr(
			"Les fonctions de sécurité et d'automatisation restent sous votre "
			"contrôle et peuvent être utilisées progressivement."),
		{
			{
				tr("État de sauvegarde visible"),
				tr(
					"La barre d'état distingue un projet modifié, en cours "
					"d'enregistrement, sauvegardé ou associé à une récupération.")
			},
			{
				tr("Automatisation facultative"),
				tr(
					"Le centre Automatisation et IA prépare une connexion MCP "
					"locale. Aucun modèle ni aucune clé API n'est intégré au logiciel.")
			},
			{
				tr("Cette introduction reste accessible"),
				tr(
					"Vous pourrez la relancer à tout moment depuis "
					"Aide > Bien démarrer avec QElectroTech.")
			}
		}));
	content_layout->addWidget(m_pages, 1);

	auto button_layout = new QHBoxLayout;
	button_layout->setContentsMargins(0, 0, 0, 0);
	button_layout->setSpacing(8);
	m_skip_button = new QPushButton(tr("Passer l'introduction"), content);
	m_skip_button->setObjectName(QStringLiteral("onboardingSkipButton"));
	m_skip_button->setAccessibleDescription(tr(
		"Ferme l'introduction et ne l'affiche plus automatiquement."));
	m_back_button = new QPushButton(tr("Précédent"), content);
	m_back_button->setObjectName(QStringLiteral("onboardingBackButton"));
	m_next_button = new QPushButton(tr("Suivant"), content);
	m_next_button->setObjectName(QStringLiteral("onboardingNextButton"));
	m_next_button->setProperty("primaryAction", true);
	m_next_button->setDefault(true);
	button_layout->addWidget(m_skip_button);
	button_layout->addStretch(1);
	button_layout->addWidget(m_back_button);
	button_layout->addWidget(m_next_button);
	content_layout->addLayout(button_layout);
	root_layout->addWidget(content, 1);

	connect(
		m_skip_button,
		&QPushButton::clicked,
		this,
		&OnboardingDialog::skipIntroduction);
	connect(
		m_back_button,
		&QPushButton::clicked,
		this,
		&OnboardingDialog::goBack);
	connect(
		m_next_button,
		&QPushButton::clicked,
		this,
		&OnboardingDialog::goForward);

	QWidget::setTabOrder(m_skip_button, m_back_button);
	QWidget::setTabOrder(m_back_button, m_next_button);
	updateNavigation();
}

QString OnboardingDialog::completionSettingsKey()
{
	return QStringLiteral("onboarding/introduction-completed");
}

QString OnboardingDialog::eligibilitySettingsKey()
{
	return QStringLiteral("onboarding/first-run-eligible");
}

bool OnboardingDialog::initializeFirstRunEligibility()
{
	QSettings settings;
	if (settings.contains(eligibilitySettingsKey())) {
		return settings.value(eligibilitySettingsKey()).toBool();
	}

	const bool eligible = settings.allKeys().isEmpty();
	settings.setValue(eligibilitySettingsKey(), eligible);
	settings.sync();
	if (settings.status() != QSettings::NoError) {
		qWarning() << "Unable to persist onboarding eligibility";
	}
	return eligible;
}

bool OnboardingDialog::shouldShowOnStartup()
{
	QSettings settings;
	return settings.value(eligibilitySettingsKey(), true).toBool()
		&& !settings.value(completionSettingsKey(), false).toBool();
}

void OnboardingDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	if (windowHandle() && !m_screen_tracking_initialized) {
		connect(
			windowHandle(),
			&QWindow::screenChanged,
			this,
			[this](QScreen *screen) {
				followScreen(screen);
			});
		m_screen_tracking_initialized = true;
	}
	fitToAvailableScreen();
	m_next_button->setFocus(Qt::OtherFocusReason);
}

QWidget *OnboardingDialog::createPage(
		const QString &title,
		const QString &summary,
		const QList<QPair<QString, QString>> &sections)
{
	auto scroll_area = new QScrollArea(m_pages);
	scroll_area->setObjectName(QStringLiteral("onboardingPageScrollArea"));
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	auto page = new QWidget(scroll_area);
	auto page_layout = new QVBoxLayout(page);
	page_layout->setContentsMargins(2, 8, 10, 8);
	page_layout->setSpacing(12);

	auto page_title = createTextLabel(
		title,
		page,
		QStringLiteral("onboardingPageTitle"));
	QFont title_font = page_title->font();
	title_font.setWeight(QFont::DemiBold);
	if (title_font.pointSizeF() > 0) {
		title_font.setPointSizeF(title_font.pointSizeF() + 2.0);
	}
	page_title->setFont(title_font);
	page_layout->addWidget(page_title);

	auto page_summary = createTextLabel(
		summary,
		page,
		QStringLiteral("onboardingPageSummary"));
	page_summary->setForegroundRole(QPalette::Text);
	page_layout->addWidget(page_summary);

	for (const auto &section : sections) {
		auto card = new QFrame(page);
		card->setProperty("onboardingCard", true);
		card->setFrameShape(QFrame::StyledPanel);
		auto card_layout = new QVBoxLayout(card);
		card_layout->setContentsMargins(14, 11, 14, 11);
		card_layout->setSpacing(4);
		auto section_title = createTextLabel(section.first, card);
		QFont section_font = section_title->font();
		section_font.setWeight(QFont::DemiBold);
		section_title->setFont(section_font);
		auto section_text = createTextLabel(section.second, card);
		section_text->setForegroundRole(QPalette::Text);
		card_layout->addWidget(section_title);
		card_layout->addWidget(section_text);
		page_layout->addWidget(card);
	}
	page_layout->addStretch(1);
	scroll_area->setWidget(page);
	return scroll_area;
}

void OnboardingDialog::goBack()
{
	if (m_pages->currentIndex() <= 0) return;
	m_pages->setCurrentIndex(m_pages->currentIndex() - 1);
	updateNavigation();
}

void OnboardingDialog::goForward()
{
	if (m_pages->currentIndex() + 1 < m_pages->count()) {
		m_pages->setCurrentIndex(m_pages->currentIndex() + 1);
		updateNavigation();
		return;
	}
	completeIntroduction();
}

void OnboardingDialog::skipIntroduction()
{
	completeIntroduction();
}

void OnboardingDialog::updateNavigation()
{
	const int page = m_pages->currentIndex();
	const int count = m_pages->count();
	m_step_label->setText(tr("Étape %1 sur %2").arg(page + 1).arg(count));
	m_progress->setRange(1, count);
	m_progress->setValue(page + 1);
	m_progress->setAccessibleDescription(m_step_label->text());
	m_back_button->setEnabled(page > 0);
	m_next_button->setText(
		page + 1 == count
			? tr("Commencer")
			: tr("Suivant"));
}

void OnboardingDialog::completeIntroduction()
{
	QSettings settings;
	settings.setValue(completionSettingsKey(), true);
	settings.sync();
	if (settings.status() != QSettings::NoError) {
		QMessageBox::warning(
			this,
			tr("Préférences non enregistrées"),
			tr(
				"QElectroTech n'a pas pu enregistrer ce choix. "
				"Vérifiez les droits d'accès aux préférences puis réessayez."));
		return;
	}
	accept();
}

void OnboardingDialog::fitToAvailableScreen()
{
	QScreen *screen = nullptr;
	if (windowHandle()) screen = windowHandle()->screen();
	if (!screen && parentWidget() && parentWidget()->windowHandle()) {
		screen = parentWidget()->windowHandle()->screen();
	}
	if (!screen && parentWidget()) {
		screen = QGuiApplication::screenAt(
				parentWidget()->frameGeometry().center());
	}
	if (!screen) screen = QGuiApplication::primaryScreen();
	followScreen(screen);
}

void OnboardingDialog::followScreen(QScreen *screen)
{
	QObject::disconnect(m_available_geometry_connection);
	if (!screen) screen = QGuiApplication::primaryScreen();
	if (!screen) return;

	m_available_geometry_connection = connect(
		screen,
		&QScreen::availableGeometryChanged,
		this,
		[this](const QRect &available_geometry) {
			fitToAvailableGeometry(available_geometry);
		});
	fitToAvailableGeometry(screen->availableGeometry());
}

void OnboardingDialog::fitToAvailableGeometry(const QRect &available)
{
	if (!available.isValid()) return;

	const QSize maximum_size(
		qMax(1, static_cast<int>(available.width() * 0.92)),
		qMax(1, static_cast<int>(available.height() * 0.92)));
	setMaximumSize(maximum_size);
	resize(size().boundedTo(maximum_size));

	QRect target = frameGeometry();
	target.moveCenter(available.center());
	move(pos() + target.topLeft() - frameGeometry().topLeft());
}
