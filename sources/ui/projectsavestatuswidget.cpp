/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "projectsavestatuswidget.h"

#include <QAccessible>
#include <QAccessibleEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

ProjectSaveStatusWidget::ProjectSaveStatusWidget(QWidget *parent) :
	QWidget(parent),
	m_icon_label(new QLabel(this)),
	m_text_label(new QLabel(this))
{
	setObjectName(QStringLiteral("projectSaveStatus"));
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	setFocusPolicy(Qt::NoFocus);

	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(8, 0, 8, 0);
	layout->setSpacing(5);
	layout->addWidget(m_icon_label);
	layout->addWidget(m_text_label);

	const int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
	m_icon_label->setFixedSize(icon_size, icon_size);
	m_icon_label->setScaledContents(true);
	m_icon_label->setFocusPolicy(Qt::NoFocus);
	m_text_label->setTextInteractionFlags(Qt::NoTextInteraction);
	m_text_label->setFocusPolicy(Qt::NoFocus);
	setState(State::NoProject);
}

ProjectSaveStatusWidget::State ProjectSaveStatusWidget::state() const
{
	return m_state;
}

QString ProjectSaveStatusWidget::statusText() const
{
	return m_text_label->text();
}

void ProjectSaveStatusWidget::setState(
	State state,
	const QString &project_name,
	const QString &file_path,
	const QString &detail,
	const QString &recovery_path,
	const QString &recovery_error)
{
	// A project that has never been written to disk must never be presented as
	// saved, even if its in-memory modified flag is currently clean.
	if (state == State::Saved && file_path.isEmpty())
		state = State::Modified;
	m_state = state;
	QString text;
	QString description;
	QStyle::StandardPixmap icon = QStyle::SP_DialogApplyButton;

	switch (state) {
	case State::NoProject:
		setVisible(false);
		setAccessibleName(QString());
		setAccessibleDescription(QString());
		setToolTip(QString());
		return;
	case State::Saved:
		text = tr("Sauvegardé");
		description = file_path.isEmpty()
			? tr("Le projet « %1 » est enregistré.").arg(project_name)
			: tr("Le projet « %1 » est enregistré dans « %2 ».").arg(project_name, file_path);
		icon = QStyle::SP_DialogApplyButton;
		break;
	case State::Modified:
		text = tr("Modifié");
		description = tr("Le projet « %1 » contient des modifications non enregistrées.").arg(project_name);
		icon = QStyle::SP_MessageBoxInformation;
		break;
	case State::Saving:
		text = tr("Enregistrement…");
		description = file_path.isEmpty()
			? tr("Enregistrement du projet « %1 » en cours.").arg(project_name)
			: tr("Enregistrement du projet « %1 » en cours vers « %2 ».").arg(project_name, file_path);
		icon = QStyle::SP_BrowserReload;
		break;
	case State::Error:
		text = tr("Erreur d’enregistrement");
		description = tr("Le projet « %1 » n’a pas pu être enregistré. Les modifications restent non enregistrées.").arg(project_name);
		if (!detail.isEmpty())
			description += QStringLiteral("\n") + detail;
		icon = QStyle::SP_MessageBoxCritical;
		break;
	}

	if ((state == State::Modified || state == State::Error)
		&& !recovery_path.isEmpty()) {
		description += QStringLiteral("\n")
			+ tr("Copie de récupération disponible : %1.").arg(recovery_path);
	} else if ((state == State::Modified || state == State::Error)
		&& !recovery_error.isEmpty()) {
		description += QStringLiteral("\n")
			+ tr("La copie de récupération n’a pas pu être mise à jour : %1.")
				.arg(recovery_error);
	}

	setVisible(true);
	m_text_label->setText(text);
	const int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
	if (m_icon_label->size() != QSize(icon_size, icon_size))
		m_icon_label->setFixedSize(icon_size, icon_size);
	m_icon_label->setPixmap(style()->standardIcon(icon).pixmap(icon_size, icon_size));
	setToolTip(description);
	setAccessibleName(tr("État de sauvegarde : %1 — projet « %2 »")
		.arg(text, project_name));
	setAccessibleDescription(description);

	QAccessibleEvent name_changed(this, QAccessible::NameChanged);
	QAccessible::updateAccessibility(&name_changed);
	QAccessibleEvent description_changed(this, QAccessible::DescriptionChanged);
	QAccessible::updateAccessibility(&description_changed);
}
