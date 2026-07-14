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
#include "searchandreplaceworker.h"
#include "conductorpropertytransform.h"
#include "../qetproject.h"
#include "../diagram.h"
#include "../diagramcommands.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetinformation.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "../undocommand/changetitleblockcommand.h"

#include <QDebug>

SearchAndReplaceWorker::SearchAndReplaceWorker()
{
	m_conductor_properties = invalidConductorProperties();
}

/**
	@brief SearchAndReplaceWorker::replaceDiagram
	Replace all properties of each diagram in diagram_list,
	by the current titleblock properties of this worker
	@param diagram_list : list of diagram to be changed,
	all diagrams must belong to the same project;
*/
void SearchAndReplaceWorker::replaceDiagram(QList<Diagram *> diagram_list)
{
	if (diagram_list.isEmpty()) {
		return;
	}

	QETProject *project = diagram_list.first()->project();
	for (Diagram *d : diagram_list) {
		if (d->project() != project) {
			return;
		}
	}

	QUndoStack *us = project->undoStack();
	us->beginMacro(QObject::tr("Chercher/remplacer les propriétés de folio"));
	for (Diagram *d : diagram_list)
	{
		TitleBlockProperties old_propertie = d->border_and_titleblock.exportTitleBlock();
		TitleBlockProperties new_properties = old_propertie;

		new_properties.title = applyChange(new_properties.title, m_titleblock_properties.title);
		new_properties.author = applyChange(new_properties.author, m_titleblock_properties.author);
		new_properties.filename = applyChange(new_properties.filename, m_titleblock_properties.filename);
		new_properties.plant = applyChange(new_properties.plant, m_titleblock_properties.plant);
		new_properties.locmach = applyChange(new_properties.locmach, m_titleblock_properties.locmach);
		new_properties.indexrev = applyChange(new_properties.indexrev, m_titleblock_properties.indexrev);
		new_properties.folio = applyChange(new_properties.folio, m_titleblock_properties.folio);

		if (m_titleblock_properties.date.isValid())
		{
			if (m_titleblock_properties.date == eraseDate()) {
				new_properties.date = QDate();
			} else {
				new_properties.date = m_titleblock_properties.date;
			}
		}

		new_properties.context.add(m_titleblock_properties.context);

		if (old_propertie != new_properties) {
			project->undoStack()->push(new ChangeTitleBlockCommand(d, old_propertie, new_properties));
		}
	}
	us->endMacro();
}

void SearchAndReplaceWorker::replaceDiagram(Diagram *diagram)
{
	QList<Diagram *> list;
	list.append(diagram);
	replaceDiagram(list);
}

/**
	@brief SearchAndReplaceWorker::replaceElement
	Replace all properties of each elements in list
	All element must belong to the same project,
	if not this function do nothing.
	All change are made through a undo command append
	to undo list of the project.
	@param list
*/
void SearchAndReplaceWorker::replaceElement(QList<Element *> list)
{
	if (list.isEmpty() || !list.first()->diagram()) {
		return;
	}

	QETProject *project_ = list.first()->diagram()->project();
	for (Element *elmt : list)
	{
		if (elmt->diagram()) {
			if (elmt->diagram()->project() != project_) {
				return;
			}
		}
	}

	project_->undoStack()->beginMacro(QObject::tr("Chercher/remplacer les propriétés d'éléments."));
	for (Element *elmt : list)
	{
			//We apply change only for master, slave, and terminal element.
		if (elmt->linkType() == Element::Master ||
			elmt->linkType() == Element::Simple ||
			elmt->linkType() == Element::Terminale ||
			elmt->linkType() == Element::Thumbnail)
		{
			DiagramContext old_context;
			DiagramContext new_context =  old_context = elmt->elementInformations();
			for (QString key : QETInformation::elementInfoKeys())
			{
				new_context.addValue(key, applyChange(old_context.value(key).toString(),
													  m_element_context.value(key).toString()));
			}

			if (old_context != new_context)
			{
				ChangeElementInformationCommand *undo = new ChangeElementInformationCommand(elmt, old_context, new_context);
				project_->undoStack()->push(undo);
			}
		}
	}
	project_->undoStack()->endMacro();
}

void SearchAndReplaceWorker::replaceElement(Element *element)
{
	QList<Element *>list;
	list.append(element);
	replaceElement(list);
}

/**
	@brief SearchAndReplaceWorker::replaceIndiText
	Replace all displayed text of independent text of list
	Each must belong to the same project, if not this function do nothing
	@param list
*/
void SearchAndReplaceWorker::replaceIndiText(QList<IndependentTextItem *> list)
{
	if (list.isEmpty() || !list.first()->diagram()) {
		return;
	}
	QETProject *project_ = list.first()->diagram()->project();
	for (IndependentTextItem *text : list) {
		if (!text->diagram() ||
			text->diagram()->project() != project_) {
			return;
		}
	}

	project_->undoStack()->beginMacro(QObject::tr("Chercher/remplacer des textes independants"));
	for (IndependentTextItem *text : list)
	{
		QString before = text->toPlainText();
		text->setPlainText(m_indi_text);
		project_->undoStack()->push(new ChangeDiagramTextCommand(text, before, m_indi_text));
	}
	project_->undoStack()->endMacro();
}

void SearchAndReplaceWorker::replaceIndiText(IndependentTextItem *text)
{
	QList<IndependentTextItem *>list;
	list.append(text);
	replaceIndiText(list);
}

/**
	@brief SearchAndReplaceWorker::replaceConductor
	Replace all properties of each conductor in list
	All conductor must belong to the same project,
	if not this function do nothing.
	All change are made through a undo command append
	to undo list of the project.
	@param list
*/
void SearchAndReplaceWorker::replaceConductor(QList<Conductor *> list)
{
	const ConductorChangePlan plan = conductorChangePlan(list);
	const ConductorChangePlan::Result result = applyConductorChangePlan(plan);
	if (!result.canApply() && !result.isNoOp()) {
		qWarning() << ConductorChangePlan::resultMessage(result);
	}
}

void SearchAndReplaceWorker::replaceConductor(Conductor *conductor)
{
	QList<Conductor *>list;
	list.append(conductor);
	replaceConductor(list);
}

ConductorChangePlan SearchAndReplaceWorker::conductorChangePlan(
	const QList<Conductor *> &list,
	bool includePropertyPatch,
	bool includeAdvancedReplacement) const
{
	QETProject *project = nullptr;
	if (!list.isEmpty() && list.first() && list.first()->diagram()) {
		project = list.first()->diagram()->project();
	}
	const ConductorProperties patch = m_conductor_properties;
	const advancedReplaceStruct advanced = m_advanced_struct;
	return ConductorChangePlan::build(
		project,
		list,
		[patch, advanced, includePropertyPatch, includeAdvancedReplacement](
			const ConductorProperties &before) {
			return transformConductorProperties(
				before,
				patch,
				advanced,
				includePropertyPatch,
				includeAdvancedReplacement);
		});
}

ConductorChangePlan::Result SearchAndReplaceWorker::applyConductorChangePlan(
	const ConductorChangePlan &plan) const
{
	return plan.applyAtomically(
		QObject::tr("Chercher/remplacer les propriétés de conducteurs"));
}

/**
	@brief SearchAndReplaceWorker::replaceAdvanced
	Apply the change of text according to the current advancedStruct
	All items in the 4 list must belong to the same QETProject,
	if not this function do nothing
	@param diagrams :
	@param elements :
	@param texts :
	@param conductors :
*/
void SearchAndReplaceWorker::replaceAdvanced(
		QList<Diagram *> diagrams,
		QList<Element *> elements,
		QList<IndependentTextItem *> texts,
		QList<Conductor *> conductors)
{
	QETProject *project_ = nullptr;

	//Some test to check if a least one list have one item
	//and if all items belong to the same project
	if (!diagrams.isEmpty() && diagrams.first()) {
		project_ = diagrams.first()->project();
	} else if (!elements.isEmpty() && elements.first()
			   && elements.first()->diagram()) {
		project_ = elements.first()->diagram()->project();
	} else if (!texts.isEmpty() && texts.first()
			   && texts.first()->diagram()) {
		project_ = texts.first()->diagram()->project();
	} else if (!conductors.isEmpty() && conductors.first()
			   && conductors.first()->diagram()) {
		project_ = conductors.first()->diagram()->project();
	} else {
		return;
	}

	for (Diagram *dd : diagrams) {
		if (!dd || dd->project() != project_) {
			return;
		}
	}
	for (Element *elmt : elements) {
		if (!elmt || !elmt->diagram()
			|| elmt->diagram()->project() != project_) {
			return;
		}
	}
	for (IndependentTextItem *text : texts) {
		if (!text || !text->diagram()
			|| text->diagram()->project() != project_) {
			return;
		}
	}
	for (Conductor *cc : conductors) {
		if (!cc || !cc->diagram()
			|| cc->diagram()->project() != project_) {
			return;
		}
	}
	//The end of the test

	int who = m_advanced_struct.who;
	if (who == -1) {
		return;
	}
	if (who == 2)
	{
		if (conductors.isEmpty()) return;
		const ConductorChangePlan plan = conductorChangePlan(
			conductors,
			false,
			true);
		const ConductorChangePlan::Result result = plan.applyAtomically(
			QObject::tr("Rechercher / remplacer avancé dans les conducteurs"));
		if (!result.canApply() && !result.isNoOp()) {
			qWarning() << ConductorChangePlan::resultMessage(result);
		}
		return;
	}

	project_->undoStack()->beginMacro(QObject::tr("Rechercher / remplacer avancé"));
	if (who == 0)
	{
		for (Diagram *diagram : diagrams)
		{
			TitleBlockProperties old_properties = diagram->border_and_titleblock.exportTitleBlock();
			TitleBlockProperties new_properties = replaceAdvanced(diagram);
			if (old_properties != new_properties) {
				project_->undoStack()->push(new ChangeTitleBlockCommand(diagram, old_properties, new_properties));
			}
		}
	}
	else if (who == 1)
	{
		for (Element *element : elements)
		{
			DiagramContext old_context = element->elementInformations();
			DiagramContext new_context = replaceAdvanced(element);
			if (old_context != new_context) {
				project_->undoStack()->push(new ChangeElementInformationCommand(element, old_context, new_context));
			}
		}
	}
	else if (who == 3)
	{
		for (IndependentTextItem *text : texts)
		{
			QRegularExpression rx(m_advanced_struct.search);
			if (!rx.isValid())
			{
				qWarning() <<QObject::tr("this is an error in the code")
					  << rx.errorString()
					  << rx.patternErrorOffset();
			}
			QString replace = m_advanced_struct.replace;
			QString after = text->toPlainText();
			after = after.replace(rx, replace);

			if (after != text->toPlainText()) {
				project_->undoStack()->push(new ChangeDiagramTextCommand(text, text->toPlainText(), after));
			}
		}
	}
	project_->undoStack()->endMacro();
}

/**
	@brief SearchAndReplaceWorker::setupLineEdit
	With search and replace, when the variable to edit is a text,
	the editor is always the same no matter if it is for a folio,
	element or conductor.
	The editor is a QLineEdit to edit the text
	and checkbox to erase the text if checked.
	This function fill the editor, from the current string
	@param l
	@param cb
	@param str
*/
void SearchAndReplaceWorker::setupLineEdit(QLineEdit *l,
					   QCheckBox *cb,
					   QString str)
{
	l->setText(str);
	cb->setChecked(str == eraseText() ? true : false);
	l->setDisabled(str == eraseText() ? true : false);
}

ConductorProperties SearchAndReplaceWorker::invalidConductorProperties()
{
	ConductorProperties cp;

		//init with invalid value the conductor properties
	cp.text_size = 0;
	cp.text.clear();
	cp.m_vertical_alignment = Qt::AlignAbsolute;
	cp.m_horizontal_alignment = Qt::AlignAbsolute;
	cp.verti_rotate_text = -1;
	cp.horiz_rotate_text = -1;
	cp.color = QColor();
	cp.style = Qt::NoPen;
	cp.cond_size = 0;
	cp.m_color_2 = QColor();
	cp.m_dash_size = 0;

	return cp;
}

/**
	@brief SearchAndReplaceWorker::applyChange
	@param original : the original properties
	@param change : the change properties, to be merged with original
	@return a new conductor properties with the change applyed.
*/
ConductorProperties SearchAndReplaceWorker::applyChange(
		const ConductorProperties &original,
		const ConductorProperties &change)
{
	return applyConductorPropertyPatch(original, change);
}

/**
	@brief SearchAndReplaceWorker::applyChange
	@param original : the original string
	@param change : the changed string:
	@return the string to be use in the properties
*/
QString SearchAndReplaceWorker::applyChange(const QString &original,
					    const QString &change)
{
	if (change.isEmpty())           {return original;}
	else if (change == eraseText()) {return QString();}
	else                            {return change;}
}

/**
	@brief SearchAndReplaceWorker::replaceAdvanced
	@param diagram
	@return the titleblock properties with the change applied,
	according to the state of m_advanced_struct
*/
TitleBlockProperties SearchAndReplaceWorker::replaceAdvanced(Diagram *diagram)
{
	TitleBlockProperties p = diagram->border_and_titleblock.exportTitleBlock();

	if (m_advanced_struct.who == 0)
	{
		QRegularExpression rx(m_advanced_struct.search);
		if (!rx.isValid())
		{
			qWarning() <<QObject::tr("this is an error in the code")
				  << rx.errorString()
				  << rx.patternErrorOffset();
		}
		QString replace = m_advanced_struct.replace;
		QString what = m_advanced_struct.what;
		if (what == "title") {p.title = p.title.replace(rx, replace);}
		else if (what == "author") {p.author = p.author.replace(rx, replace);}
		else if (what == "filename") {p.filename = p.filename.replace(rx, replace);}
		else if (what == "folio") {p.folio = p.folio.replace(rx, replace);}
		else if (what == "plant") {p.plant = p.plant.replace(rx, replace);}
		else if (what == "locmach") {p.locmach = p.locmach.replace(rx, replace);}
		else if (what == "indexrev") {p.indexrev = p.indexrev.replace(rx, replace);}
	}
	return p;
}

/**
	@brief SearchAndReplaceWorker::replaceAdvanced
	@param element
	@return The diagram context with the change applied,
	according to the state of m_advanced_struct
*/
DiagramContext SearchAndReplaceWorker::replaceAdvanced(Element *element)
{
	DiagramContext context = element->elementInformations();

	if (m_advanced_struct.who == 1)
	{
		QString what = m_advanced_struct.what;
		if (context.contains(what))
		{
			QRegularExpression rx(m_advanced_struct.search);
			if (!rx.isValid())
			{
				qWarning() <<QObject::tr("this is an error in the code")
					  << rx.errorString()
					  << rx.patternErrorOffset();
			}
			QString replace = m_advanced_struct.replace;
			QString value = context[what].toString();
			context.addValue(what, value.replace(rx, replace));
		}
	}

	return context;
}
