/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef CONDUCTORPROPERTIESEDITOR_H
#define CONDUCTORPROPERTIESEDITOR_H

#include "../PropertiesEditor/propertieseditorwidget.h"

#include <QColor>
#include <QList>
#include <QPointer>
#include <QSet>

class Conductor;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;

class ConductorPropertiesEditor : public PropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit ConductorPropertiesEditor(
				const QList<Conductor *> &conductors,
				QWidget *parent = nullptr);

		void setConductors(const QList<Conductor *> &conductors);
		QString title() const override;
		void updateUi() override;
		bool setLiveEdit(bool live_edit) override;

	private:
		enum class Field {
			Type,
			Formula,
			Function,
			TensionProtocol,
			WireColor,
			WireSection,
			Color,
			Style,
			Size,
			ShowText
		};

		void buildUi();
		void clearConnections();
		void connectEditors();
		void applyField(Field field);
		void chooseColor();
		void openAdvancedEditor();
		void updateColorButton(const QColor &color, bool mixed);
		QList<Conductor *> liveConductors() const;

		QList<QPointer<Conductor>> m_conductors;
		QList<QMetaObject::Connection> m_connections;
		bool m_updating = false;
		QColor m_selected_color;
		bool m_color_mixed = false;
		QSet<int> m_touched_text_fields;

		QLabel *m_selection_summary = nullptr;
		QLabel *m_resolved_text = nullptr;
		QComboBox *m_type = nullptr;
		QLineEdit *m_formula = nullptr;
		QLineEdit *m_function = nullptr;
		QLineEdit *m_tension_protocol = nullptr;
		QLineEdit *m_wire_color = nullptr;
		QLineEdit *m_wire_section = nullptr;
		QPushButton *m_color_button = nullptr;
		QComboBox *m_style = nullptr;
		QDoubleSpinBox *m_size = nullptr;
		QCheckBox *m_show_text = nullptr;
		QPushButton *m_advanced = nullptr;
};

#endif // CONDUCTORPROPERTIESEDITOR_H
