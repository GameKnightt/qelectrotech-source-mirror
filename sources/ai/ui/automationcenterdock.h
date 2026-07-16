/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef AUTOMATION_CENTER_DOCK_H
#define AUTOMATION_CENTER_DOCK_H

#include <QDockWidget>

class AutomationCenterController;
class QQuickWidget;

class AutomationCenterDock : public QDockWidget
{
	Q_OBJECT

	public:
		explicit AutomationCenterDock(QWidget *parent = nullptr);

		AutomationCenterController *controller();

	private:
		AutomationCenterController *m_controller = nullptr;
		QQuickWidget *m_quick_widget = nullptr;
};

#endif // AUTOMATION_CENTER_DOCK_H
