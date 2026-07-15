#include "../../../sources/utils/titleblockcontextresolver.h"

#include <catch2/catch.hpp>

TEST_CASE("Title block custom properties use non-empty folio overrides")
{
	DiagramContext project;
	project.addValue("revision", "A");
	project.addValue("project-only", "shared");

	SECTION("an absent folio value uses the project value")
	{
		const DiagramContext effective =
				QET::effectiveTitleBlockContext(project, DiagramContext());
		CHECK(effective.value("revision").toString() == QString("A"));
		CHECK(effective.value("project-only").toString() == QString("shared"));
	}

	SECTION("an empty folio value falls back to the project value")
	{
		DiagramContext folio;
		folio.addValue("revision", "");

		const DiagramContext effective =
				QET::effectiveTitleBlockContext(project, folio);
		CHECK(effective.value("revision").toString() == QString("A"));
	}

	SECTION("a non-empty folio value overrides the project value")
	{
		DiagramContext folio;
		folio.addValue("revision", "B");

		const DiagramContext effective =
				QET::effectiveTitleBlockContext(project, folio);
		CHECK(effective.value("revision").toString() == QString("B"));
	}

	SECTION("an empty folio-only value remains available and empty")
	{
		DiagramContext folio;
		folio.addValue("folio-only", "");

		const DiagramContext effective =
				QET::effectiveTitleBlockContext(project, folio);
		CHECK(effective.contains("folio-only"));
		CHECK(effective.value("folio-only").toString().isEmpty());
	}
}
