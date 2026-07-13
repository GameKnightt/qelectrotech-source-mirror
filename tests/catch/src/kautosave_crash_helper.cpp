#include "kautosavefile.h"

#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <QUrl>

int main(int argc, char **argv)
{
	QCoreApplication application(argc, argv);
	if (application.arguments().size() < 4
			|| application.arguments().size() > 5) {
		return 1;
	}

	if (application.arguments().size() == 5
			&& application.arguments().at(4) == QStringLiteral("test-mode")) {
		QStandardPaths::setTestModeEnabled(true);
	}
	QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech"));
	QCoreApplication::setApplicationName(application.arguments().at(2));

	const auto managed_path = application.arguments().at(1);
	const auto payload = QByteArray::fromBase64(
			application.arguments().at(3).toLatin1());
	KAutoSaveFile backup(QUrl::fromLocalFile(managed_path));
	if (!backup.open(QIODevice::WriteOnly
					 | QIODevice::Truncate
					 | QIODevice::Text)) {
		return 2;
	}
	if (backup.write(payload) != payload.size() || !backup.flush()) {
		return 3;
	}

	QTextStream output(stdout);
	output << "READY\n";
	output.flush();

	for (;;) {
		QThread::sleep(60);
	}
}
