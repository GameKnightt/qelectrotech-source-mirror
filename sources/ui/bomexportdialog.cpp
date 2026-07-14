/*
   Copyright 2006-2026 The QElectroTech Team
   This file is part of QElectroTech.

   QElectroTech is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   QElectroTech is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "bomexportdialog.h"

#include "../dataBase/ui/elementquerywidget.h"
#include "../qetapp.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../utils/exportutils.h"
#include "ui_bomexportdialog.h"

#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

/**
	@brief BOMExportDialog::BOMExportDialog
	@param project
	@param parent
*/
BOMExportDialog::BOMExportDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::BOMExportDialog),
	m_project(project)
{
	ui->setupUi(this);

	m_query_widget = new ElementQueryWidget(this);
	ui->m_main_layout->insertWidget(0, m_query_widget);
		//By default format as bom is clicked
	on_m_format_as_bom_clicked(true);
}

/**
	@brief BOMExportDialog::~BOMExportDialog
*/
BOMExportDialog::~BOMExportDialog()
{
	delete ui;
}

/**
	@brief BOMExportDialog::exec
	@return
*/
int BOMExportDialog::exec()
{
	const int result = QDialog::exec();
	if (result != QDialog::Accepted) {
		return result;
	}

	QString csv;
	QString error_message;
	if (!getBom(csv, error_message)) {
		QMessageBox::critical(
			this,
			tr("Erreur"),
			tr("Impossible de générer la nomenclature.\n\n%1")
				.arg(error_message));
		return QDialog::Rejected;
	}

	// Save in a CSV file in the same directory as the project by default.
	QString dir = m_project->currentDir();
	if (dir.isEmpty()) dir = QETApp::documentDir();
	const QString file_name = dir % "/" % tr("nomenclature_") %
		QString(m_project->title() % ".csv");
	const QString file_path = QFileDialog::getSaveFileName(
		this,
		tr("Enregister sous... "),
		file_name,
		tr("Fichiers csv (*.csv)"));
	if (file_path.isEmpty()) {
		return QDialog::Rejected;
	}

	if (!ExportUtils::writeTextAtomically(file_path, csv, &error_message)) {
		QMessageBox::critical(
			this,
			tr("Erreur"),
			tr("Impossible d'écrire le fichier sans remplacer la version existante.\n\n"
			   "Destination : %1\n%2")
				.arg(file_path, error_message));
		return QDialog::Rejected;
	}

	return QDialog::Accepted;
}

bool BOMExportDialog::getBom(QString &csv, QString &error_message)
{
	csv.clear();
	error_message.clear();
	const auto update_result = m_project->dataBase()->updateDB();
	if (!update_result.isOk()) {
		error_message = tr(
			"La base de données du projet n'a pas pu être actualisée. "
			"Aucun fichier n'a été généré.\n%1")
			.arg(update_result.diagnostic());
		return false;
	}
	auto query_ = m_project->dataBase()->newQuery(m_query_widget->queryStr());

	if (!query_.exec()) {
		error_message = query_.lastError().text();
		return false;
	}

	// HEADERS
	if (ui->m_include_headers->isChecked())
	{
		const auto record_ = query_.record();
		QStringList header_names;
		for (int i = 0; i < record_.count(); ++i)
		{
			const QString field_name = record_.fieldName(i);
			QString header;
			if (field_name == "position") {
				header = tr("Position");
			} else if (field_name == "diagram_position") {
				header = tr("Position du folio");
			} else if (field_name == "designation_qty") {
				header = tr(
					"Quantité numéro d'article",
					"Special field with name : designation quantity");
			} else {
				header = QETInformation::translatedInfoKey(field_name);
				if (header.isEmpty()) {
					header = field_name;
				}
			}
			header_names << ExportUtils::csvField(header);
		}
		csv = header_names.join(";") % "\n";
	}

	// ROWS
	const int column_count = query_.record().count();
	while (query_.next())
	{
		QStringList values;
		for (int i = 0; i < column_count; ++i)
		{
			const QDate date = query_.value(i).toDate();
			if (!date.isNull()) {
				values << ExportUtils::csvField(
					QLocale::system().toString(date, QLocale::ShortFormat));
			} else {
				values << ExportUtils::csvField(query_.value(i).toString());
			}
		}
		csv += values.join(";") % "\n";
	}

	if (query_.lastError().isValid()) {
		error_message = query_.lastError().text();
		csv.clear();
		return false;
	}

	return true;
}

/**
	@brief BOMExportDialog::on_m_format_as_bom_clicked
	@param checked
*/
void BOMExportDialog::on_m_format_as_bom_clicked(bool checked) {
	m_query_widget->setGroupBy("designation", checked);
	m_query_widget->setCount("COUNT(*) AS designation_qty", checked);
}
