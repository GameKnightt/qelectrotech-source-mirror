/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
*/
#include "../../sources/ui/folionavigatordialog.h"
#include "../../sources/ui/folionavigatormodel.h"
#include "../../sources/utils/folionavigationindex.h"

#include <QComboBox>
#include <QFont>
#include <QGuiApplication>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QScreen>
#include <QSignalSpy>
#include <QtTest>

namespace
{
	FolioNavigationEntry entry(
			int position,
			const QString &folio,
			const QString &title)
	{
		FolioNavigationEntry result;
		result.diagram_id = QUuid::createUuid();
		result.position = position;
		result.folio_label = folio;
		result.raw_folio_label = folio;
		result.title = title;
		return result;
	}
}

class FolioNavigatorTest : public QObject
{
	Q_OBJECT

	private slots:
		void normalizesAccentsAndSpaces();
		void ranksExactAliasesBeforeTextMatches();
		void matchesAllTokensAcrossMetadata();
		void preservesDuplicateTitles();
		void filtersGroupsFavoritesAndRecents();
		void enforcesDeterministicScaleBudgets();
		void buildsOneThousandDistinctGroups();
		void modelKeepsIdentityAndActiveSelection();
		void keyboardMovesWithoutActivatingThenEnterActivates();
		void escapeAndEmptyResultsDoNotActivate();
		void favoriteActionIsExplicit();
		void exposesAccessibleNamesAndFitsScreen();
		void exposesAccessibleNamesAndFitsScreenWithLargeText();
};

void FolioNavigatorTest::normalizesAccentsAndSpaces()
{
	QCOMPARE(
			FolioNavigationIndex::normalized(QStringLiteral("  Énergie   générale ")),
			QStringLiteral("energie generale"));
}

void FolioNavigatorTest::ranksExactAliasesBeforeTextMatches()
{
	QVector<FolioNavigationEntry> entries {
		entry(0, QStringLiteral("A-01"), QStringLiteral("Armoire 12")),
		entry(11, QStringLiteral("B-12"), QStringLiteral("Distribution")),
		entry(2, QStringLiteral("12"), QStringLiteral("Commande"))
	};
	const QVector<int> result = FolioNavigationIndex::filteredIndexes(
			entries, QStringLiteral("12"));
	QCOMPARE(result.size(), 3);
	QCOMPARE(result.at(0), 1);
	QCOMPARE(result.at(1), 2);
}

void FolioNavigatorTest::matchesAllTokensAcrossMetadata()
{
	FolioNavigationEntry first = entry(
			0, QStringLiteral("M-01"), QStringLiteral("Commande moteur"));
	first.plant = QStringLiteral("Atelier nord");
	first.location = QStringLiteral("Armoire A3");
	first.additional_fields << QStringLiteral("discipline automatisme");
	FolioNavigationEntry second = entry(
			1, QStringLiteral("P-01"), QStringLiteral("Pompe"));
	QVector<FolioNavigationEntry> entries {first, second};

	const QVector<int> result = FolioNavigationIndex::filteredIndexes(
			entries, QStringLiteral("moteur nord a3 automatisme"));
	QCOMPARE(result, QVector<int>({0}));
	QVERIFY(FolioNavigationIndex::filteredIndexes(
			entries, QStringLiteral("moteur sud")).isEmpty());
}

void FolioNavigatorTest::preservesDuplicateTitles()
{
	QVector<FolioNavigationEntry> entries {
		entry(0, QStringLiteral("1"), QStringLiteral("Alimentation")),
		entry(1, QStringLiteral("2"), QStringLiteral("Alimentation"))
	};
	const QVector<int> result = FolioNavigationIndex::filteredIndexes(
			entries, QStringLiteral("alimentation"));
	QCOMPARE(result, QVector<int>({0, 1}));
	QVERIFY(entries.at(result.at(0)).diagram_id
			!= entries.at(result.at(1)).diagram_id);
}

void FolioNavigatorTest::filtersGroupsFavoritesAndRecents()
{
	FolioNavigationEntry first = entry(0, QStringLiteral("1"), QStringLiteral("A"));
	first.group = QStringLiteral("Atelier / A1");
	first.favorite = true;
	first.recent_rank = 1;
	FolioNavigationEntry second = entry(1, QStringLiteral("2"), QStringLiteral("B"));
	second.group = QStringLiteral("Atelier / A1");
	second.recent_rank = 0;
	FolioNavigationEntry third = entry(2, QStringLiteral("3"), QStringLiteral("C"));
	third.group = QStringLiteral("Process / P1");
	QVector<FolioNavigationEntry> entries {first, second, third};

	QCOMPARE(FolioNavigationIndex::filteredIndexes(
			entries, QString(), QStringLiteral("Atelier / A1")),
			QVector<int>({0, 1}));
	QCOMPARE(FolioNavigationIndex::filteredIndexes(
			entries, QString(), QString(), FolioNavigationIndex::Scope::Favorites),
			QVector<int>({0}));
	QCOMPARE(FolioNavigationIndex::filteredIndexes(
			entries, QString(), QString(), FolioNavigationIndex::Scope::Recent),
			QVector<int>({1, 0}));
}

void FolioNavigatorTest::enforcesDeterministicScaleBudgets()
{
	for (const int count : {500, 1000})
	{
		QVector<FolioNavigationEntry> entries;
		entries.reserve(count);
		for (int index = 0; index < count; ++index)
		{
			FolioNavigationEntry item = entry(
					index,
					QStringLiteral("F-%1").arg(
							index + 1, 5, 10, QLatin1Char('0')),
					QStringLiteral("Commande moteur industriel"));
			item.diagram_id = QUuid(QStringLiteral(
					"{00000000-0000-0000-0000-%1}").arg(
							index + 1, 12, 16, QLatin1Char('0')));
			item.plant = QStringLiteral("Atelier nord");
			item.location = QStringLiteral("Armoire A3");
			item.additional_fields << QStringLiteral("discipline automatisme");
			FolioNavigationIndex::prepareEntry(item);
			entries.append(item);
		}

		FolioNavigationStats empty_stats;
		const QVector<int> all = FolioNavigationIndex::filteredIndexes(
				entries, QString(), QString(),
				FolioNavigationIndex::Scope::All, &empty_stats);
		QCOMPARE(all.size(), count);
		QCOMPARE(empty_stats.entries_visited, qsizetype(count));
		QCOMPARE(empty_stats.match_evaluations, qsizetype(0));
		QCOMPARE(empty_stats.sort_comparisons, qsizetype(0));

		FolioNavigationStats absent_stats;
		QVERIFY(FolioNavigationIndex::filteredIndexes(
				entries, QStringLiteral("zzzz-absent"), QString(),
				FolioNavigationIndex::Scope::All, &absent_stats).isEmpty());
		QCOMPARE(absent_stats.entries_visited, qsizetype(count));
		QCOMPARE(absent_stats.match_evaluations, qsizetype(count));
		QCOMPARE(absent_stats.searchable_text_lookups, qsizetype(count));
		QCOMPARE(absent_stats.prepared_search_hits, qsizetype(count));
		QCOMPARE(absent_stats.fallback_search_builds, qsizetype(0));
		QCOMPARE(absent_stats.token_checks, qsizetype(count));
		QCOMPARE(absent_stats.sort_comparisons, qsizetype(0));

		QVector<FolioNavigationEntry> unprepared_entries = entries;
		for (FolioNavigationEntry &item : unprepared_entries) {
			item.search_index_ready = false;
		}
		FolioNavigationStats fallback_stats;
		QVERIFY(FolioNavigationIndex::filteredIndexes(
				unprepared_entries, QStringLiteral("zzzz-absent"), QString(),
				FolioNavigationIndex::Scope::All, &fallback_stats).isEmpty());
		QCOMPARE(fallback_stats.prepared_search_hits, qsizetype(0));
		QCOMPARE(fallback_stats.fallback_search_builds, qsizetype(count));

		FolioNavigationStats broad_stats;
		const QVector<int> broad = FolioNavigationIndex::filteredIndexes(
				entries, QStringLiteral("commande atelier automatisme"),
				QString(), FolioNavigationIndex::Scope::All, &broad_stats);
		QCOMPARE(broad.size(), count);
		QCOMPARE(broad_stats.entries_visited, qsizetype(count));
		QCOMPARE(broad_stats.prepared_search_hits, qsizetype(count));
		QCOMPARE(broad_stats.fallback_search_builds, qsizetype(0));
		QCOMPARE(broad_stats.token_checks, qsizetype(3 * count));
		const int logarithm = count == 500 ? 9 : 10;
		QVERIFY(broad_stats.sort_comparisons
				<= qsizetype(4 * count * logarithm * logarithm));

		FolioNavigationStats exact_stats;
		const QString last_folio = QStringLiteral("F-%1").arg(
				count, 5, 10, QLatin1Char('0'));
		const QVector<int> exact = FolioNavigationIndex::filteredIndexes(
				entries, last_folio, QString(),
				FolioNavigationIndex::Scope::All, &exact_stats);
		QCOMPARE(exact, QVector<int>({count - 1}));
		QCOMPARE(exact_stats.entries_visited, qsizetype(count));
		QCOMPARE(exact_stats.matches, qsizetype(1));
	}
}

void FolioNavigatorTest::buildsOneThousandDistinctGroups()
{
	QVector<FolioNavigationEntry> entries;
	entries.reserve(1000);
	for (int index = 0; index < 1000; ++index)
	{
		FolioNavigationEntry item = entry(
				index,
				QString::number(index + 1),
				QStringLiteral("Folio"));
		item.group = QStringLiteral("Groupe %1").arg(
				index, 4, 10, QLatin1Char('0'));
		entries.append(item);
	}
	FolioNavigatorModel model;
	model.setEntries(entries);
	const QStringList groups = model.groups();
	QCOMPARE(groups.size(), 1000);
	QCOMPARE(groups.first(), QStringLiteral("Groupe 0000"));
	QCOMPARE(groups.last(), QStringLiteral("Groupe 0999"));
}

void FolioNavigatorTest::modelKeepsIdentityAndActiveSelection()
{
	QVector<FolioNavigationEntry> entries {
		entry(0, QStringLiteral("1"), QStringLiteral("Premier")),
		entry(1, QStringLiteral("2"), QStringLiteral("Deuxième"))
	};
	entries[1].active = true;
	FolioNavigatorModel model;
	model.setEntries(entries);
	QCOMPARE(model.rowCount(), 2);
	QCOMPARE(model.preferredRow(), 1);
	QCOMPARE(model.data(model.index(1), FolioNavigatorModel::DiagramIdRole).toUuid(),
			entries.at(1).diagram_id);
	QVERIFY(model.data(model.index(1), Qt::DisplayRole).toString().contains(
			QStringLiteral("Actif")));
	model.setFilters(QStringLiteral("premier"), QString(), FolioNavigationIndex::Scope::All);
	QCOMPARE(model.rowCount(), 1);
	QCOMPARE(model.preferredRow(entries.at(1).diagram_id), 0);
}

void FolioNavigatorTest::keyboardMovesWithoutActivatingThenEnterActivates()
{
	QVector<FolioNavigationEntry> entries {
		entry(0, QStringLiteral("1"), QStringLiteral("Premier")),
		entry(1, QStringLiteral("2"), QStringLiteral("Deuxième")),
		entry(2, QStringLiteral("3"), QStringLiteral("Troisième"))
	};
	entries[0].active = true;
	FolioNavigatorDialog dialog;
	QSignalSpy activated(&dialog, &FolioNavigatorDialog::folioActivated);
	bool closed_before_activation = false;
	connect(&dialog, &FolioNavigatorDialog::folioActivated,
			&dialog, [&dialog, &closed_before_activation]() {
		closed_before_activation = !dialog.isVisible();
	});
	dialog.openForProject(
			QStringLiteral("Test"), entries, entries.at(0).diagram_id, false, false);
	auto *search = dialog.findChild<QLineEdit *>(QStringLiteral("folioNavigatorSearch"));
	QVERIFY(search);
	QTest::keyClick(search, Qt::Key_Down);
	QCOMPARE(activated.count(), 0);
	QCOMPARE(dialog.model()->diagramIdAt(
			dialog.findChild<QListView *>(QStringLiteral("folioNavigatorResults"))
					->currentIndex().row()), entries.at(1).diagram_id);
	QTest::keyClick(search, Qt::Key_Return);
	QCOMPARE(activated.count(), 1);
	QCOMPARE(activated.at(0).at(0).toUuid(), entries.at(1).diagram_id);
	QVERIFY(closed_before_activation);
	QVERIFY(!dialog.isVisible());
}

void FolioNavigatorTest::escapeAndEmptyResultsDoNotActivate()
{
	QVector<FolioNavigationEntry> entries {
		entry(0, QStringLiteral("1"), QStringLiteral("Premier"))
	};
	FolioNavigatorDialog dialog;
	QSignalSpy activated(&dialog, &FolioNavigatorDialog::folioActivated);
	dialog.openForProject(
			QStringLiteral("Test"), entries, entries.at(0).diagram_id, false, false);
	auto *search = dialog.findChild<QLineEdit *>(QStringLiteral("folioNavigatorSearch"));
	search->setText(QStringLiteral("absent"));
	QCOMPARE(dialog.model()->rowCount(), 0);
	QTest::keyClick(search, Qt::Key_Return);
	QCOMPARE(activated.count(), 0);
	QVERIFY(dialog.isVisible());
	QTest::keyClick(search, Qt::Key_Escape);
	QCOMPARE(activated.count(), 0);
	QVERIFY(!dialog.isVisible());
}

void FolioNavigatorTest::favoriteActionIsExplicit()
{
	QVector<FolioNavigationEntry> entries {
		entry(0, QStringLiteral("1"), QStringLiteral("Premier"))
	};
	FolioNavigatorDialog dialog;
	QSignalSpy favorite(&dialog, &FolioNavigatorDialog::favoriteChanged);
	dialog.openForProject(
			QStringLiteral("Test"), entries, entries.at(0).diagram_id, false, false);
	auto *button = dialog.findChild<QPushButton *>(
			QStringLiteral("folioNavigatorFavorite"));
	QVERIFY(button);
	QTest::mouseClick(button, Qt::LeftButton);
	QCOMPARE(favorite.count(), 1);
	QCOMPARE(favorite.at(0).at(0).toUuid(), entries.at(0).diagram_id);
	QCOMPARE(favorite.at(0).at(1).toBool(), true);
	QVERIFY(button->text().contains(QStringLiteral("Retirer")));
}

void FolioNavigatorTest::exposesAccessibleNamesAndFitsScreen()
{
	FolioNavigatorDialog dialog;
	QVector<FolioNavigationEntry> entries {
		entry(0, QStringLiteral("1"), QStringLiteral("Premier"))
	};
	dialog.openForProject(
			QStringLiteral("Test"), entries, entries.at(0).diagram_id, true, true);
	QVERIFY(!dialog.accessibleName().isEmpty());
	QVERIFY(!dialog.findChild<QLineEdit *>(
			QStringLiteral("folioNavigatorSearch"))->accessibleName().isEmpty());
	QVERIFY(!dialog.findChild<QListView *>(
			QStringLiteral("folioNavigatorResults"))->accessibleName().isEmpty());
	QScreen *screen = dialog.screen();
	QVERIFY(screen);
	QVERIFY2(
			screen->availableGeometry().contains(dialog.frameGeometry()),
			qPrintable(QStringLiteral("available=%1,%2 %3x%4 frame=%5,%6 %7x%8 minimum=%9x%10")
					.arg(screen->availableGeometry().x())
					.arg(screen->availableGeometry().y())
					.arg(screen->availableGeometry().width())
					.arg(screen->availableGeometry().height())
					.arg(dialog.frameGeometry().x())
					.arg(dialog.frameGeometry().y())
					.arg(dialog.frameGeometry().width())
					.arg(dialog.frameGeometry().height())
					.arg(dialog.minimumSizeHint().width())
					.arg(dialog.minimumSizeHint().height())));
	QVERIFY(dialog.minimumSizeHint().width()
			<= screen->availableGeometry().width());
	QVERIFY(dialog.minimumSizeHint().height()
			<= screen->availableGeometry().height());
	dialog.close();
}

void FolioNavigatorTest::exposesAccessibleNamesAndFitsScreenWithLargeText()
{
	const QFont original_font = QGuiApplication::font();
	struct FontRestorer {
		QFont font;
		~FontRestorer() { QGuiApplication::setFont(font); }
	} restorer {original_font};
	QFont large_font = original_font;
	const qreal base_size = original_font.pointSizeF() > 0
			? original_font.pointSizeF() : 9.0;
	large_font.setPointSizeF(base_size * 1.5);
	QGuiApplication::setFont(large_font);

	FolioNavigatorDialog dialog;
	QVector<FolioNavigationEntry> entries {
		entry(0, QStringLiteral("1"), QStringLiteral("Premier"))
	};
	dialog.openForProject(
			QStringLiteral("Test"), entries, entries.at(0).diagram_id, true, true);
	QVERIFY(!dialog.accessibleName().isEmpty());
	QVERIFY(!dialog.findChild<QLineEdit *>(
			QStringLiteral("folioNavigatorSearch"))->accessibleName().isEmpty());
	QVERIFY(!dialog.findChild<QListView *>(
			QStringLiteral("folioNavigatorResults"))->accessibleName().isEmpty());
	const QSize target_logical_size(1280, 720);
	QVERIFY2(
			dialog.minimumSizeHint().width() <= target_logical_size.width(),
			"The dialog must fit a 1920x1080 display at 150% scaling");
	QVERIFY2(
			dialog.minimumSizeHint().height() <= target_logical_size.height(),
			"The dialog must fit a 1920x1080 display at 150% scaling");
	dialog.close();
}

QTEST_MAIN(FolioNavigatorTest)
#include "tst_folionavigator.moc"
