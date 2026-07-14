/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "startcenterpagecontroller.h"

#include "startcenterwidget.h"

#include <QStackedWidget>
#include <QTimer>

StartCenterPageController::StartCenterPageController(
	QStackedWidget *pages,
	StartCenterWidget *start_center,
	QWidget *editor_page) :
	m_pages(pages),
	m_start_center(start_center),
	m_editor_page(editor_page)
{
}

void StartCenterPageController::setHasOpenProjects(bool has_open_projects)
{
	if (!m_pages || !m_start_center || !m_editor_page) return;
	QWidget *target_page = has_open_projects
		? m_editor_page
		: static_cast<QWidget *>(m_start_center);
	if (m_pages->currentWidget() != target_page) {
		m_pages->setCurrentWidget(target_page);
	}
	if (!has_open_projects) {
		QStackedWidget *pages = m_pages;
		StartCenterWidget *start_center = m_start_center;
		QTimer::singleShot(0, start_center, [pages, start_center]() {
			if (pages->currentWidget() == start_center) {
				start_center->focusPrimaryAction();
			}
		});
	}
}

bool StartCenterPageController::isStartCenterVisible() const
{
	return m_pages && m_start_center
		&& m_pages->currentWidget() == m_start_center;
}
