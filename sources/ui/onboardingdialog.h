/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef ONBOARDING_DIALOG_H
#define ONBOARDING_DIALOG_H

#include <QDialog>
#include <QList>
#include <QMetaObject>
#include <QPair>
#include <QString>

class QLabel;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QShowEvent;
class QRect;
class QScreen;

/**
	@brief First-run introduction to the main QElectroTech workflows.

	Completion is persisted only when the user explicitly finishes or skips the
	introduction. Closing it keeps the first-run invitation available at the
	next launch. The dialog can always be reopened from the Help menu.
*/
class OnboardingDialog final : public QDialog
{
	Q_OBJECT

	public:
		explicit OnboardingDialog(QWidget *parent = nullptr);

		static QString completionSettingsKey();
		static QString eligibilitySettingsKey();
		static bool initializeFirstRunEligibility();
		static bool shouldShowOnStartup();
		void fitToAvailableGeometry(const QRect &available_geometry);

	protected:
		void showEvent(QShowEvent *event) override;

	private slots:
		void goBack();
		void goForward();
		void skipIntroduction();

	private:
		QWidget *createPage(
				const QString &title,
				const QString &summary,
				const QList<QPair<QString, QString>> &sections);
		void updateNavigation();
		void completeIntroduction();
		void fitToAvailableScreen();
		void followScreen(QScreen *screen);

		QLabel *m_step_label = nullptr;
		QProgressBar *m_progress = nullptr;
		QStackedWidget *m_pages = nullptr;
		QPushButton *m_skip_button = nullptr;
		QPushButton *m_back_button = nullptr;
		QPushButton *m_next_button = nullptr;
		QMetaObject::Connection m_available_geometry_connection;
		bool m_screen_tracking_initialized = false;
};

#endif // ONBOARDING_DIALOG_H
