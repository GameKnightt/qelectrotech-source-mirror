/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef START_CENTER_PAGE_CONTROLLER_H
#define START_CENTER_PAGE_CONTROLLER_H

class QStackedWidget;
class QWidget;
class StartCenterWidget;

class StartCenterPageController
{
	public:
		StartCenterPageController(
			QStackedWidget *pages,
			StartCenterWidget *start_center,
			QWidget *editor_page);

		void setHasOpenProjects(bool has_open_projects);
		bool isStartCenterVisible() const;

	private:
		QStackedWidget *m_pages = nullptr;
		StartCenterWidget *m_start_center = nullptr;
		QWidget *m_editor_page = nullptr;
};

#endif
