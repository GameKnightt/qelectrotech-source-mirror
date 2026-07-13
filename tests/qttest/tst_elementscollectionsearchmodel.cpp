/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
*/
#include "elementcollectionroles.h"
#include "elementscollectionsearchmodel.h"

#include <QAbstractItemModelTester>
#include <QMimeData>
#include <QStandardItemModel>
#include <QtTest>

class ElementsCollectionSearchModelTest : public QObject
{
	Q_OBJECT

	private:
		static QStandardItem *element(const QString &name,
									  const QString &location,
									  const QString &metadata = QString())
		{
			auto *item = new QStandardItem(name);
			item->setData(true, ElementCollectionRoles::IsElementRole);
			item->setData(location, ElementCollectionRoles::CollectionPathRole);
			item->setData(metadata + QLatin1Char(' ') + name,
						  ElementCollectionRoles::SearchTextRole);
			item->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
			return item;
		}

		static QStandardItemModel *sourceModel(QObject *parent)
		{
			auto *model = new QStandardItemModel(parent);
			auto *root = new QStandardItem(QStringLiteral("Collection QET"));
			auto *electric = new QStandardItem(QStringLiteral("Électrique"));
			auto *sensors = new QStandardItem(QStringLiteral("Capteurs"));
			sensors->appendRow(element(
				QStringLiteral("Pompe électrique"),
				QStringLiteral("common://10_electric/pompe.elmt"),
				QStringLiteral("moteur industriel")));
			electric->appendRow(sensors);
			root->appendRow(electric);
			model->appendRow(root);

			auto *custom_root = new QStandardItem(QStringLiteral("Collection utilisateur"));
			auto *pneumatic = new QStandardItem(QStringLiteral("Pneumatique"));
			pneumatic->appendRow(element(
				QStringLiteral("Pompe électrique"),
				QStringLiteral("custom://50_pneumatic/pompe.elmt"),
				QStringLiteral("air comprimé")));
			custom_root->appendRow(pneumatic);
			model->appendRow(custom_root);
			return model;
		}

	private slots:
		void flatAccentInsensitiveAndFiltering()
		{
			QScopedPointer<QStandardItemModel> source(sourceModel(this));
			ElementsCollectionSearchModel model;
			QAbstractItemModelTester tester(
				&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
			model.setSourceModel(source.data());

			model.setQuery(QStringLiteral("electrique moteur"));
			QCOMPARE(model.rowCount(), 1);
			QVERIFY(!model.index(0, 0).parent().isValid());
			QCOMPARE(model.index(0, 0).data(
				ElementsCollectionSearchModel::LocationRole).toString(),
				QStringLiteral("common://10_electric/pompe.elmt"));

			model.setQuery(QStringLiteral("pompe"));
			QCOMPARE(model.rowCount(), 2);
			model.setQuery(QStringLiteral("pompe hydraulique"));
			QCOMPARE(model.rowCount(), 0);
			model.setQuery(QStringLiteral("p"));
			QCOMPARE(model.rowCount(), 0);
		}

		void metadataAndMimeData()
		{
			QScopedPointer<QStandardItemModel> source(sourceModel(this));
			ElementsCollectionSearchModel model;
			model.setSourceModel(source.data());
			model.setQuery(QStringLiteral("moteur"));
			const QModelIndex result = model.index(0, 0);

			QCOMPARE(result.data(ElementsCollectionSearchModel::NameRole).toString(),
				QStringLiteral("Pompe électrique"));
			QCOMPARE(result.data(ElementsCollectionSearchModel::DisciplineRole).toString(),
				QStringLiteral("Électrique"));
			QCOMPARE(result.data(ElementsCollectionSearchModel::ProvenanceRole).toString(),
				QStringLiteral("QET"));
			QCOMPARE(result.data(ElementsCollectionSearchModel::FullPathRole).toString(),
				QStringLiteral("Électrique / Capteurs / Pompe électrique"));
			QVERIFY(result.data(Qt::AccessibleTextRole).toString().contains(
				QStringLiteral("Capteurs")));

			QScopedPointer<QMimeData> mime(model.mimeData(QModelIndexList() << result));
			QVERIFY(mime->hasFormat(QStringLiteral("application/x-qet-element-uri")));
			QCOMPARE(mime->text(), QStringLiteral("common://10_electric/pompe.elmt"));
		}

		void sourceChangesRefreshResults()
		{
			QScopedPointer<QStandardItemModel> source(sourceModel(this));
			ElementsCollectionSearchModel model;
			model.setSourceModel(source.data());
			model.setQuery(QStringLiteral("vanne"));
			QCOMPARE(model.rowCount(), 0);

			QStandardItem *discipline = source->item(0)->child(0);
			discipline->appendRow(element(
				QStringLiteral("Vanne"),
				QStringLiteral("common://10_electric/vanne.elmt")));
			QCOMPARE(model.rowCount(), 1);
			discipline->removeRow(discipline->rowCount() - 1);
			QCOMPARE(model.rowCount(), 0);
		}

		void lazyPreviewDoesNotResetResults()
		{
			QScopedPointer<QStandardItemModel> source(sourceModel(this));
			ElementsCollectionSearchModel model;
			model.setSourceModel(source.data());
			model.setQuery(QStringLiteral("pompe"));
			QCOMPARE(model.rowCount(), 2);

			QSignalSpy reset_spy(&model, &QAbstractItemModel::modelReset);
			QStandardItem *element_item = source->item(0)->child(0)->child(0)->child(0);
			element_item->setIcon(QIcon::fromTheme(QStringLiteral("edit-find")));

			QCOMPARE(reset_spy.count(), 0);
			QCOMPARE(model.rowCount(), 2);
		}

		void sortingModesAreDeterministic()
		{
			QScopedPointer<QStandardItemModel> source(sourceModel(this));
			ElementsCollectionSearchModel model;
			model.setSourceModel(source.data());
			model.setQuery(QStringLiteral("pompe"));

			model.setSortMode(ElementsCollectionSearchModel::SortByDiscipline);
			QCOMPARE(model.index(0, 0).data(
				ElementsCollectionSearchModel::DisciplineRole).toString(),
				QStringLiteral("Électrique"));
			model.setSortMode(ElementsCollectionSearchModel::SortByProvenance);
			QCOMPARE(model.index(0, 0).data(
				ElementsCollectionSearchModel::ProvenanceRole).toString(),
				QStringLiteral("QET"));
		}

		void searchDoesNotExpandSourceTree()
		{
			QScopedPointer<QStandardItemModel> source(sourceModel(this));
			ElementsCollectionSearchModel model;
			model.setSourceModel(source.data());
			const QPersistentModelIndex collapsed_branch(source->index(0, 0));

			model.setQuery(QStringLiteral("pompe"));
			model.setQuery(QStringLiteral("pneumatique"));
			model.setQuery(QString());

			QVERIFY(collapsed_branch.isValid());
			QCOMPARE(source->rowCount(collapsed_branch), 1);
		}
};

QTEST_MAIN(ElementsCollectionSearchModelTest)
#include "tst_elementscollectionsearchmodel.moc"
