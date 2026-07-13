/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef PROJECT_SAVE_STATUS_WIDGET_H
#define PROJECT_SAVE_STATUS_WIDGET_H

#include <QWidget>

class QLabel;

class ProjectSaveStatusWidget : public QWidget
{
	Q_OBJECT

	public:
		enum class State { NoProject, Saved, Modified, Saving, Error };

		explicit ProjectSaveStatusWidget(QWidget *parent = nullptr);

		State state() const;
		QString statusText() const;
		void setState(
			State state,
			const QString &project_name = QString(),
			const QString &file_path = QString(),
			const QString &detail = QString());

	private:
		State m_state = State::NoProject;
		QLabel *m_icon_label = nullptr;
		QLabel *m_text_label = nullptr;
};

#endif
