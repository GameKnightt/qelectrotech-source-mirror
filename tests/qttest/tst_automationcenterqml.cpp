/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include <QtQuickTest/quicktest.h>

#include <QCoreApplication>
#include <QTranslator>

class AutomationCenterTranslator final : public QTranslator
{
public:
	QString translate(
		const char *context,
		const char *source_text,
		const char *disambiguation = nullptr,
		int n = -1) const override
	{
		Q_UNUSED(disambiguation)

		if (qstrcmp(context, "AutomationCenter") != 0 || n < 0) {
			return {};
		}

		const QString source = QString::fromUtf8(source_text);
		if (source == QStringLiteral("%n folio(s)")) {
			return n < 2
				? QStringLiteral("%1 folio traduit").arg(n)
				: QStringLiteral("%1 folios traduits").arg(n);
		}
		if (source == QStringLiteral("%n élément(s)")) {
			return n < 2
				? QStringLiteral("%1 élément traduit").arg(n)
				: QStringLiteral("%1 éléments traduits").arg(n);
		}
		if (source == QStringLiteral("%n conducteur(s)")) {
			return n < 2
				? QStringLiteral("%1 conducteur traduit").arg(n)
				: QStringLiteral("%1 conducteurs traduits").arg(n);
		}

		return {};
	}
};

class AutomationCenterTestSetup final : public QObject
{
	Q_OBJECT

public slots:
	void applicationAvailable()
	{
		QCoreApplication::installTranslator(&m_translator);
	}

	void cleanupTestCase()
	{
		QCoreApplication::removeTranslator(&m_translator);
	}

private:
	AutomationCenterTranslator m_translator;
};

QUICK_TEST_MAIN_WITH_SETUP(
	qet_automation_center,
	AutomationCenterTestSetup)

#include "tst_automationcenterqml.moc"
