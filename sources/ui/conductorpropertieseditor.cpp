/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "conductorpropertieseditor.h"

#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../undocommand/changeconductorspropertiescommand.h"
#include "conductorpropertiesdialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <functional>
#include <optional>

namespace
{
	const QString MixedValuePlaceholder = QStringLiteral("—");

	template<typename Value, typename Getter>
	std::optional<Value> commonValue(
			const QList<Conductor *> &conductors,
			Getter getter)
	{
		if (conductors.isEmpty()) {
			return std::nullopt;
		}
		const Value first = getter(conductors.first()->properties());
		for (const Conductor *conductor : conductors)
		{
			if (getter(conductor->properties()) != first) {
				return std::nullopt;
			}
		}
		return first;
	}

	void setMixedLineEdit(QLineEdit *editor, const std::optional<QString> &value)
	{
		editor->setText(value.value_or(QString()));
		editor->setPlaceholderText(value ? QString() : QObject::tr("Valeurs multiples"));
	}
}

ConductorPropertiesEditor::ConductorPropertiesEditor(
		const QList<Conductor *> &conductors,
		QWidget *parent) :
	PropertiesEditorWidget(parent)
{
	buildUi();
	setConductors(conductors);
}

void ConductorPropertiesEditor::buildUi()
{
	setObjectName(QStringLiteral("conductorPropertiesEditor"));
	setAccessibleName(tr("Propriétés des conducteurs sélectionnés"));

	auto *main_layout = new QVBoxLayout(this);
	main_layout->setContentsMargins(8, 8, 8, 8);
	main_layout->setSpacing(8);

	m_selection_summary = new QLabel(this);
	m_selection_summary->setObjectName(QStringLiteral("conductorSelectionSummary"));
	m_selection_summary->setWordWrap(true);
	main_layout->addWidget(m_selection_summary);

	auto *tabs = new QTabWidget(this);
	tabs->setDocumentMode(true);
	tabs->setAccessibleName(tr("Catégories de propriétés du conducteur"));
	main_layout->addWidget(tabs);

	auto *identity_page = new QWidget(tabs);
	auto *identity_form = new QFormLayout(identity_page);
	identity_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	m_type = new QComboBox(identity_page);
	m_type->setObjectName(QStringLiteral("conductorTypeEditor"));
	m_type->addItem(tr("Valeurs multiples"), -1);
	m_type->addItem(tr("Multifilaire"), int(ConductorProperties::Multi));
	m_type->addItem(tr("Unifilaire"), int(ConductorProperties::Single));
	identity_form->addRow(tr("Type :"), m_type);

	m_formula = new QLineEdit(identity_page);
	m_formula->setObjectName(QStringLiteral("conductorFormulaEditor"));
	m_formula->setClearButtonEnabled(true);
	identity_form->addRow(tr("Repère / formule :"), m_formula);

	m_resolved_text = new QLabel(identity_page);
	m_resolved_text->setObjectName(QStringLiteral("conductorResolvedText"));
	m_resolved_text->setTextInteractionFlags(Qt::TextSelectableByMouse);
	m_resolved_text->setWordWrap(true);
	identity_form->addRow(tr("Texte affiché :"), m_resolved_text);

	m_function = new QLineEdit(identity_page);
	m_function->setObjectName(QStringLiteral("conductorFunctionEditor"));
	identity_form->addRow(tr("Fonction :"), m_function);

	m_tension_protocol = new QLineEdit(identity_page);
	m_tension_protocol->setObjectName(QStringLiteral("conductorProtocolEditor"));
	identity_form->addRow(tr("Tension / protocole :"), m_tension_protocol);

	m_wire_color = new QLineEdit(identity_page);
	m_wire_color->setObjectName(QStringLiteral("conductorWireColorEditor"));
	identity_form->addRow(tr("Couleur du fil :"), m_wire_color);

	m_wire_section = new QLineEdit(identity_page);
	m_wire_section->setObjectName(QStringLiteral("conductorWireSectionEditor"));
	identity_form->addRow(tr("Section :"), m_wire_section);

	tabs->addTab(identity_page, tr("Identification"));

	auto *appearance_page = new QWidget(tabs);
	auto *appearance_form = new QFormLayout(appearance_page);
	appearance_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	m_color_button = new QPushButton(appearance_page);
	m_color_button->setObjectName(QStringLiteral("conductorColorEditor"));
	appearance_form->addRow(tr("Couleur du trait :"), m_color_button);

	m_style = new QComboBox(appearance_page);
	m_style->setObjectName(QStringLiteral("conductorStyleEditor"));
	m_style->addItem(tr("Valeurs multiples"), -1);
	m_style->addItem(tr("Trait plein"), int(Qt::SolidLine));
	m_style->addItem(tr("Trait en pointillés"), int(Qt::DashLine));
	m_style->addItem(tr("Traits et points"), int(Qt::DashDotLine));
	appearance_form->addRow(tr("Style :"), m_style);

	m_size = new QDoubleSpinBox(appearance_page);
	m_size->setObjectName(QStringLiteral("conductorSizeEditor"));
	m_size->setRange(0.0, 50.0);
	m_size->setDecimals(1);
	m_size->setSingleStep(0.5);
	m_size->setSpecialValueText(tr("Valeurs multiples"));
	m_size->setSuffix(tr(" pt"));
	appearance_form->addRow(tr("Épaisseur :"), m_size);

	m_show_text = new QCheckBox(tr("Afficher le repère"), appearance_page);
	m_show_text->setObjectName(QStringLiteral("conductorShowTextEditor"));
	m_show_text->setTristate(true);
	appearance_form->addRow(QString(), m_show_text);

	tabs->addTab(appearance_page, tr("Apparence"));

	m_advanced = new QPushButton(tr("Propriétés avancées…"), this);
	m_advanced->setObjectName(QStringLiteral("conductorAdvancedProperties"));
	m_advanced->setToolTip(tr("Ouvrir toutes les propriétés du conducteur sélectionné"));
	main_layout->addWidget(m_advanced);
}

void ConductorPropertiesEditor::setConductors(
		const QList<Conductor *> &conductors)
{
	clearConnections();
	m_conductors.clear();
	for (Conductor *conductor : conductors)
	{
		if (conductor) {
			m_conductors.append(QPointer<Conductor>(conductor));
		}
	}
	updateUi();
	connectEditors();
}

QString ConductorPropertiesEditor::title() const
{
	return tr("Propriétés des conducteurs");
}

QList<Conductor *> ConductorPropertiesEditor::liveConductors() const
{
	QList<Conductor *> result;
	for (const QPointer<Conductor> &conductor : m_conductors)
	{
		if (conductor) {
			result.append(conductor.data());
		}
	}
	return result;
}

void ConductorPropertiesEditor::updateUi()
{
	const QList<Conductor *> conductors = liveConductors();
	if (conductors.isEmpty()) {
		return;
	}

	m_updating = true;
	m_touched_text_fields.clear();
	m_selection_summary->setText(conductors.size() == 1
			? tr("1 conducteur sélectionné")
			: tr("%1 conducteurs sélectionnés — seules les propriétés modifiées seront harmonisées.")
					.arg(conductors.size()));

	const auto type = commonValue<ConductorProperties::ConductorType>(
			conductors, [](const ConductorProperties &p) { return p.type; });
	m_type->setCurrentIndex(type ? m_type->findData(int(*type)) : 0);

	setMixedLineEdit(m_formula, commonValue<QString>(
			conductors, [](const ConductorProperties &p) { return p.m_formula; }));
	setMixedLineEdit(m_function, commonValue<QString>(
			conductors, [](const ConductorProperties &p) { return p.m_function; }));
	setMixedLineEdit(m_tension_protocol, commonValue<QString>(
			conductors, [](const ConductorProperties &p) { return p.m_tension_protocol; }));
	setMixedLineEdit(m_wire_color, commonValue<QString>(
			conductors, [](const ConductorProperties &p) { return p.m_wire_color; }));
	setMixedLineEdit(m_wire_section, commonValue<QString>(
			conductors, [](const ConductorProperties &p) { return p.m_wire_section; }));

	const auto resolved_text = commonValue<QString>(
			conductors, [](const ConductorProperties &p) { return p.text; });
	m_resolved_text->setText(resolved_text.value_or(tr("Valeurs multiples")));

	const auto color = commonValue<QColor>(
			conductors, [](const ConductorProperties &p) { return p.color; });
	m_color_mixed = !color.has_value();
	m_selected_color = color.value_or(QColor());
	updateColorButton(m_selected_color, m_color_mixed);

	const auto style = commonValue<Qt::PenStyle>(
			conductors, [](const ConductorProperties &p) { return p.style; });
	m_style->setCurrentIndex(style ? m_style->findData(int(*style)) : 0);

	const auto size = commonValue<double>(
			conductors, [](const ConductorProperties &p) { return p.cond_size; });
	m_size->setValue(size.value_or(0.0));

	const auto show_text = commonValue<bool>(
			conductors, [](const ConductorProperties &p) { return p.m_show_text; });
	m_show_text->setCheckState(show_text
			? (*show_text ? Qt::Checked : Qt::Unchecked)
			: Qt::PartiallyChecked);

	const bool read_only = conductors.first()->diagram()
			&& conductors.first()->diagram()->isReadOnly();
	for (QWidget *editor : QList<QWidget *> {
			m_type, m_formula, m_function, m_tension_protocol, m_wire_color,
			m_wire_section, m_color_button, m_style, m_size, m_show_text}) {
		editor->setEnabled(!read_only);
	}
	m_advanced->setEnabled(conductors.size() == 1 && !read_only);
	m_advanced->setAccessibleDescription(conductors.size() == 1
			? tr("Ouvre le dialogue complet des propriétés du conducteur")
			: tr("Disponible lorsqu’un seul conducteur est sélectionné"));
	m_updating = false;
}

bool ConductorPropertiesEditor::setLiveEdit(bool live_edit)
{
	m_live_edit = live_edit;
	clearConnections();
	connectEditors();
	return true;
}

void ConductorPropertiesEditor::connectEditors()
{
	if (liveConductors().isEmpty()) {
		return;
	}

	m_connections << connect(m_advanced, &QPushButton::clicked,
			this, &ConductorPropertiesEditor::openAdvancedEditor);

	for (Conductor *conductor : liveConductors())
	{
		m_connections << connect(conductor, &Conductor::propertiesChange,
				this, &ConductorPropertiesEditor::updateUi, Qt::QueuedConnection);
		m_connections << connect(conductor, &QObject::destroyed,
				this, [this]() { updateUi(); });
	}

	if (!m_live_edit) {
		return;
	}

	m_connections << connect(m_type, QOverload<int>::of(&QComboBox::activated),
			this, [this](int) { applyField(Field::Type); });
	const auto connect_text_editor = [this](QLineEdit *editor, Field field)
	{
		m_connections << connect(editor, &QLineEdit::textEdited, this,
				[this, field]() { m_touched_text_fields.insert(int(field)); });
		m_connections << connect(editor, &QLineEdit::editingFinished, this,
				[this, field]() {
					if (m_touched_text_fields.remove(int(field))) {
						applyField(field);
					}
				});
	};
	connect_text_editor(m_formula, Field::Formula);
	connect_text_editor(m_function, Field::Function);
	connect_text_editor(m_tension_protocol, Field::TensionProtocol);
	connect_text_editor(m_wire_color, Field::WireColor);
	connect_text_editor(m_wire_section, Field::WireSection);
	m_connections << connect(m_color_button, &QPushButton::clicked,
			this, &ConductorPropertiesEditor::chooseColor);
	m_connections << connect(m_style, QOverload<int>::of(&QComboBox::activated),
			this, [this](int) { applyField(Field::Style); });
	m_connections << connect(m_size, &QDoubleSpinBox::editingFinished,
			this, [this]() { applyField(Field::Size); });
	m_connections << connect(m_show_text, &QCheckBox::clicked,
			this, [this]() { applyField(Field::ShowText); });
}

void ConductorPropertiesEditor::clearConnections()
{
	for (const QMetaObject::Connection &connection : m_connections) {
		disconnect(connection);
	}
	m_connections.clear();
}

void ConductorPropertiesEditor::applyField(Field field)
{
	if (m_updating || !m_live_edit) {
		return;
	}
	const QList<Conductor *> conductors = liveConductors();
	if (conductors.isEmpty() || !conductors.first()->diagram()
			|| conductors.first()->diagram()->isReadOnly()) {
		return;
	}

	QVector<ChangeConductorsPropertiesCommand::Change> changes;

	for (Conductor *conductor : conductors)
	{
		ConductorProperties old_properties = conductor->properties();
		ConductorProperties new_properties = old_properties;
		switch (field)
		{
			case Field::Type:
				if (m_type->currentData().toInt() < 0) continue;
				new_properties.type = static_cast<ConductorProperties::ConductorType>(
						m_type->currentData().toInt());
				break;
			case Field::Formula: new_properties.m_formula = m_formula->text(); break;
			case Field::Function: new_properties.m_function = m_function->text(); break;
			case Field::TensionProtocol: new_properties.m_tension_protocol = m_tension_protocol->text(); break;
			case Field::WireColor: new_properties.m_wire_color = m_wire_color->text(); break;
			case Field::WireSection: new_properties.m_wire_section = m_wire_section->text(); break;
			case Field::Color:
				if (!m_selected_color.isValid()) continue;
				new_properties.color = m_selected_color;
				break;
			case Field::Style:
				if (m_style->currentData().toInt() < 0) continue;
				new_properties.style = static_cast<Qt::PenStyle>(m_style->currentData().toInt());
				break;
			case Field::Size:
				if (m_size->value() <= 0.0) continue;
				new_properties.cond_size = m_size->value();
				break;
			case Field::ShowText:
				if (m_show_text->checkState() == Qt::PartiallyChecked) continue;
				new_properties.m_show_text = m_show_text->isChecked();
				break;
		}

		if (new_properties != old_properties) {
			changes.append({conductor, old_properties, new_properties});
		}
	}

	auto *undo = new ChangeConductorsPropertiesCommand(changes);
	if (!undo->isEmpty()) {
		conductors.first()->diagram()->undoStack().push(undo);
	} else {
		delete undo;
	}
}

void ConductorPropertiesEditor::chooseColor()
{
	const QColor initial = m_selected_color.isValid() ? m_selected_color : Qt::black;
	const QColor selected = QColorDialog::getColor(
			initial, this, tr("Couleur du conducteur"), QColorDialog::ShowAlphaChannel);
	if (!selected.isValid()) {
		return;
	}
	m_selected_color = selected;
	m_color_mixed = false;
	updateColorButton(selected, false);
	applyField(Field::Color);
}

void ConductorPropertiesEditor::openAdvancedEditor()
{
	const QList<Conductor *> conductors = liveConductors();
	if (conductors.size() == 1 && conductors.first()->diagram()
			&& !conductors.first()->diagram()->isReadOnly()) {
		ConductorPropertiesDialog::PropertiesDialog(conductors.first(), this);
	}
}

void ConductorPropertiesEditor::updateColorButton(
		const QColor &color, bool mixed)
{
	if (mixed || !color.isValid())
	{
		m_color_button->setText(tr("Valeurs multiples"));
		m_color_button->setStyleSheet(QString());
		m_color_button->setAccessibleDescription(tr("Plusieurs couleurs sont sélectionnées"));
		return;
	}

	m_color_button->setText(color.name(QColor::HexRgb).toUpper());
	const QColor foreground = color.lightness() < 128 ? Qt::white : Qt::black;
	m_color_button->setStyleSheet(QStringLiteral(
			"QPushButton { background-color: %1; color: %2; }")
			.arg(color.name(QColor::HexArgb), foreground.name()));
	m_color_button->setAccessibleDescription(
			tr("Couleur actuelle : %1").arg(color.name(QColor::HexRgb)));
}
