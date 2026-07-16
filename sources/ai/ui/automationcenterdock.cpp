/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "automationcenterdock.h"

#include "automationcentercontroller.h"

#include <QDebug>
#include <QLabel>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>

AutomationCenterDock::AutomationCenterDock(QWidget *parent) :
	QDockWidget(tr("Automatisation et IA"), parent),
	m_controller(new AutomationCenterController(this)),
	m_quick_widget(new QQuickWidget(this))
{
	setObjectName(QStringLiteral("automation_ai_dock"));
	setAccessibleName(tr("Panneau Automatisation et IA"));
	setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	setFeatures(
		QDockWidget::DockWidgetClosable
		| QDockWidget::DockWidgetMovable
		| QDockWidget::DockWidgetFloatable);
	setMinimumWidth(360);

	m_quick_widget->setObjectName(QStringLiteral("automationCenterQuickWidget"));
	m_quick_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
	m_quick_widget->setClearColor(Qt::transparent);
	m_quick_widget->setSource(
		QUrl(QStringLiteral("qrc:/ai/AutomationCenter.qml")));
	if (m_quick_widget->status() == QQuickWidget::Error) {
		for (const QQmlError &error : m_quick_widget->errors())
			qWarning() << "Automation center QML:" << error.toString();
		QLabel *fallback = new QLabel(
			tr("Le centre Automatisation et IA n’a pas pu être chargé.\n"
			   "Vérifiez que les modules Qt Quick du paquet sont présents, "
			   "puis relancez QElectroTech."),
			this);
		fallback->setObjectName(
			QStringLiteral("automationCenterLoadError"));
		fallback->setAccessibleName(
			tr("Erreur de chargement du centre Automatisation et IA"));
		fallback->setWordWrap(true);
		fallback->setMargin(16);
		fallback->setTextInteractionFlags(
			Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
		delete m_quick_widget;
		m_quick_widget = nullptr;
		setWidget(fallback);
		return;
	}
	if (QQuickItem *root = m_quick_widget->rootObject()) {
		root->setProperty(
			"controller",
			QVariant::fromValue<QObject *>(m_controller));
	}
	setWidget(m_quick_widget);
}

AutomationCenterController *AutomationCenterDock::controller()
{
	return m_controller;
}
