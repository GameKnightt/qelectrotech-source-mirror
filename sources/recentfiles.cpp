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
#include "recentfiles.h"
#include <QMenu>

/**
	Constructeur
	@param identifier prefixe a utiliser pour recuperer les fichiers recents
	dans la configuration de l'application
	@param size Nombre de fichiers recents a retenir
	@param parent QObject parent
*/
RecentFiles::RecentFiles(const QString &identifier, int size, QObject *parent) :
	QObject(parent),
	identifier_(identifier.isEmpty() ? "unnamed" : identifier),
	size_(size > 0 ? size : 10),
	menu_(nullptr)
{
	extractFilesFromSettings();
	buildMenu();
}

/**
	Destructeur
	@todo determiner s'il faut detruire ou non le menu
*/
RecentFiles::~RecentFiles()
{
#if TODO_LIST
#pragma message("@TODO determiner s'il faut detruire ou non le menu")
#endif
	delete menu_;
}

/**
	@return le nombre de fichiers a retenir
*/
int RecentFiles::size() const
{
	return(size_);
}

/**
	@return the recent files, newest first
*/
QStringList RecentFiles::files() const
{
	return list_;
}

/**
	@return un menu listant les derniers fichiers ouverts
*/
QMenu *RecentFiles::menu() const
{
	return(menu_);
}

/**
	@return l'icone affichee a cote de chaque fichier, ou une QIcon nulle si
	aucune icone n'est utilisee.
*/
QIcon RecentFiles::iconForFiles() const
{
	return(files_icon_);
}

/**
	Definit l'icone a afficher a cote de chaque fichier. Si une QIcon nulle
	est fournie, aucune icone n'est utilisee.
	@param icon Icone a afficher a cote de chaque fichier
*/
void RecentFiles::setIconForFiles(const QIcon &icon) {
	files_icon_ = icon;
	buildMenu();
}

/**
	Oublie les fichiers recents
*/
void RecentFiles::clear()
{
	if (list_.isEmpty()) return;
	list_.clear();
	saveFilesToSettings();
	buildMenu();
	emit filesChanged();
}

/**
	Forget a single recent file without touching the file on disk.
*/
void RecentFiles::forgetFile(const QString &filepath)
{
	const QString native_path = QDir::toNativeSeparators(filepath);
	if (native_path.isEmpty() || !list_.removeAll(native_path)) return;
	saveFilesToSettings();
	buildMenu();
	emit filesChanged();
}

/**
	Sauvegarde les fichiers recents dans la configuration
*/
void RecentFiles::save()
{
	saveFilesToSettings();
}

/**
	Gere les actions sur le menu
*/
void RecentFiles::handleMenuRequest(const QString &filepath) {
	emit(fileOpeningRequested(filepath));
}

/**
	Gere le fait qu'un fichier ait ete ouvert
	@param filepath Chemin du fichier ouvert
*/
void RecentFiles::fileWasOpened(const QString &filepath) {
	if (!insertFile(filepath)) return;
	saveFilesToSettings();
	buildMenu();
	emit filesChanged();
}

/**
	@brief RecentFiles::extractFilesFromSettings
	Read the list of recent file from settings
*/
void RecentFiles::extractFilesFromSettings()
{
		//Forget the list of recent files
	list_.clear();

		//Get the last opened file from the settings
	QSettings settings;
	for (int i = size_ ; i >= 1  ; -- i)
	{
		QString key(identifier_ % "-recentfiles/file" % QString::number(i));
		QString value(settings.value(key, QString()).toString());
		insertFile(value);
	}
}

/**
	Insere un fichier dans la liste des fichiers recents
*/
bool RecentFiles::insertFile(const QString &filepath) {
	// s'assure que le chemin soit exprime avec des separateurs conformes au systeme
	QString filepath_ns = QDir::toNativeSeparators(filepath);

	// evite d'inserer un chemin de fichier vide ou en double
	if (filepath_ns.isEmpty()) return false;
	const QStringList previous_list = list_;
	list_.removeAll(filepath_ns);

	// insere le chemin de fichier
	list_.push_front(filepath_ns);

	// s'assure que l'on ne retient pas plus de fichiers que necessaire
	while (list_.count() > size_) list_.removeLast();
	return list_ != previous_list;
}

/**
	@brief RecentFiles::saveFilesToSettings
	Write the list of recent files to settings
*/
void RecentFiles::saveFilesToSettings()
{
	QSettings settings;
	for (int i = 0 ; i < size_ ; ++ i)
	{
		QString key(identifier_ % "-recentfiles/file" % QString::number(i + 1));
		if (i < list_.count()) settings.setValue(key, list_[i]);
		else settings.remove(key);
	}
}

/**
	Construit le menu
*/
void RecentFiles::buildMenu()
{
	// reinitialise le menu
	if (!menu_) {
		menu_ = new QMenu;
	} else {
		const QList<QAction *> previous_actions = menu_->actions();
		menu_ -> clear();
		for (QAction *action : previous_actions) action->deleteLater();
	}

	// remplit le menu
	foreach (QString filepath, list_) {
		// creee une nouvelle action pour le fichier
		QAction *action = new QAction(filepath, this);
		if (!files_icon_.isNull()) {
			action -> setIcon(files_icon_);
		}
		menu_ -> addAction(action);

		connect(action, &QAction::triggered, this, [this, filepath]() {
			handleMenuRequest(filepath);
		});
	}
}
