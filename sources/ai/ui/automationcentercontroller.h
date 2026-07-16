/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#ifndef AUTOMATION_CENTER_CONTROLLER_H
#define AUTOMATION_CENTER_CONTROLLER_H

#include <QObject>
#include <QStringList>

class AutomationCenterController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString projectTitle READ projectTitle NOTIFY projectChanged)
	Q_PROPERTY(QString projectPath READ projectPath NOTIFY projectChanged)
	Q_PROPERTY(QString scopeDirectory READ scopeDirectory NOTIFY projectChanged)
	Q_PROPERTY(int folioCount READ folioCount NOTIFY projectChanged)
	Q_PROPERTY(int elementCount READ elementCount NOTIFY projectChanged)
	Q_PROPERTY(int conductorCount READ conductorCount NOTIFY projectChanged)
	Q_PROPERTY(bool writeAccess READ writeAccess WRITE setWriteAccess NOTIFY writeAccessChanged)
	Q_PROPERTY(QString serverCommand READ serverCommand NOTIFY commandChanged)
	Q_PROPERTY(QString configurationSnippet READ configurationSnippet NOTIFY commandChanged)
	Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
	Q_PROPERTY(QStringList availableTools READ availableTools CONSTANT)
	Q_PROPERTY(QString protocolVersion READ protocolVersion CONSTANT)

	public:
		explicit AutomationCenterController(QObject *parent = nullptr);

		QString projectTitle() const;
		QString projectPath() const;
		QString scopeDirectory() const;
		int folioCount() const;
		int elementCount() const;
		int conductorCount() const;
		bool writeAccess() const;
		QString serverCommand() const;
		QString configurationSnippet() const;
		QString statusMessage() const;
		QStringList availableTools() const;
		QString protocolVersion() const;

		void setProjectSnapshot(
			const QString &title,
			const QString &path,
			int folios,
			int elements,
			int conductors);

	public slots:
		void setWriteAccess(bool enabled);

	public:
		Q_INVOKABLE void copyCommand();
		Q_INVOKABLE void copyConfiguration();

	signals:
		void projectChanged();
		void writeAccessChanged();
		void commandChanged();
		void statusMessageChanged();

	private:
		QString quoted(const QString &value) const;
		void setStatusMessage(const QString &message);

		QString m_project_title;
		QString m_project_path;
		QString m_scope_directory;
		int m_folio_count = 0;
		int m_element_count = 0;
		int m_conductor_count = 0;
		bool m_write_access = false;
		QString m_status_message;
};

#endif // AUTOMATION_CENTER_CONTROLLER_H
