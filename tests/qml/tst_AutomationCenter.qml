import QtQuick 2.15
import QtTest 1.2
import "../../sources/ai/ui/qml"

Item {
    id: root
    width: 720
    height: 820

    QtObject {
        id: fakeController
        property string projectTitle: "Projet de test"
        property string projectPath: "C:/QET/projet.qet"
        property string scopeDirectory: "C:/QET"
        property int folioCount: 12
        property int elementCount: 96
        property int conductorCount: 124
        property bool writeAccess: false
        property string serverCommand: "\"QElectroTech.exe\" --mcp-stdio --mcp-root \"C:/QET\""
        property string configurationSnippet: "{\"mcpServers\":{}}"
        property string statusMessage: ""
        property var availableTools: [
            "qet.project.inspect",
            "qet.project.validate",
            "qet.project.create",
            "qet.project.set_titleblock"
        ]
        property string protocolVersion: "MCP 2025-11-25 · stdio"
        function copyCommand() {}
        function copyConfiguration() {}
    }

    Component {
        id: centerComponent
        AutomationCenter {
            width: root.width
            height: root.height
            controller: fakeController
        }
    }

    TestCase {
        name: "AutomationCenter"
        when: windowShown

        function createCenter(width, height) {
            const center = createTemporaryObject(
                        centerComponent,
                        root,
                        {"width": width, "height": height})
            verify(!!center, "AutomationCenter exists")
            return center
        }

        function init() {
            fakeController.projectTitle = "Projet de test"
            fakeController.writeAccess = false
            fakeController.folioCount = 12
            fakeController.elementCount = 96
            fakeController.conductorCount = 124
        }

        function test_loadsProjectAndCommand() {
            const center = createCenter(720, 820)
            const title = findChild(center, "automationCenterTitle")
            verify(!!title, "Title exists")
            compare(title.text, "Automatisation et IA")

            const command = findChild(center, "serverCommandField")
            verify(!!command, "Command field exists")
            compare(command.text, fakeController.serverCommand)
            compare(command.textFormat, TextEdit.PlainText)
        }

        function test_projectTitleIsLiteralPlainText() {
            fakeController.projectTitle = "<b>Projet littéral</b>"
            const center = createCenter(720, 820)
            const projectTitle = findChild(center, "projectTitleValue")
            verify(!!projectTitle, "Project title exists")
            compare(projectTitle.text, "<b>Projet littéral</b>")
            compare(projectTitle.textFormat, Text.PlainText)
        }

        function test_writeAccessRequiresExplicitToggle() {
            const center = createCenter(720, 820)
            const access = findChild(center, "accessSwitch")
            verify(!!access, "Access switch exists")
            compare(access.checked, false)
            mouseClick(access)
            tryCompare(fakeController, "writeAccess", true)
        }

        function test_keyboardFocusAndNarrowLayout() {
            const center = createCenter(360, 300)
            const access = findChild(center, "accessSwitch")
            verify(!!access, "Access switch exists")
            access.forceActiveFocus()
            verify(access.activeFocus, "Access switch accepts keyboard focus")

            const command = findChild(center, "serverCommandField")
            verify(!!command, "Command remains available at narrow width")
            verify(command.width > 250, "Command field remains readable")

            const configuration = findChild(center,
                                            "copyConfigurationButton")
            configuration.forceActiveFocus()
            tryVerify(function() {
                return scrollContentY(center) > 0
            }, 1000, "Focused controls are revealed inside the viewport")
        }

        function scrollContentY(center) {
            const scroll = findChild(center, "automationCenterScroll")
            return scroll ? scroll.contentY : 0
        }

        function test_largeCountersWrapAtNarrowWidth() {
            fakeController.folioCount = 1000000
            fakeController.elementCount = 1000000
            fakeController.conductorCount = 1000000
            const center = createCenter(360, 640)
            const counters = findChild(center, "projectCounters")
            verify(!!counters, "Project counters exist")
            verify(counters.height > 18,
                   "Large translated counters wrap instead of overflowing")
            for (let index = 0; index < counters.children.length; ++index) {
                const counter = counters.children[index]
                verify(counter.x + counter.width <= counters.width + 1,
                       "Every counter remains inside the card")
            }
        }

        function test_counterSingularAndPluralLabels() {
            fakeController.folioCount = 1
            fakeController.elementCount = 2
            fakeController.conductorCount = 1
            const center = createCenter(720, 820)
            compare(findChild(center, "folioCountText").text,
                    "1 folio traduit")
            compare(findChild(center, "elementCountText").text,
                    "2 éléments traduits")
            compare(findChild(center, "conductorCountText").text,
                    "1 conducteur traduit")
        }

        function test_scrollIndicatorStaysInsideViewport() {
            const center = createCenter(360, 120)
            const scroll = findChild(center, "automationCenterScroll")
            const indicator = findChild(center, "scrollIndicator")
            verify(!!scroll, "Scrollable viewport exists")
            verify(!!indicator, "Scroll indicator exists")
            verify(scroll.contentHeight > scroll.height,
                   "Fixture is tall enough to scroll")

            scroll.contentY = scroll.contentHeight - scroll.height
            const indicatorPosition = indicator.mapToItem(center, 0, 0)
            verify(indicatorPosition.y >= 0,
                   "Scroll indicator remains below the viewport top")
            verify(indicatorPosition.y + indicator.height <= center.height + 1,
                   "Scroll indicator remains above the viewport bottom")
        }

        function test_scrollIndicatorFitsViewportBelowMinimumThumbHeight() {
            const center = createCenter(360, 20)
            const indicator = findChild(center, "scrollIndicator")
            verify(!!indicator, "Scroll indicator exists")
            verify(indicator.height <= center.height,
                   "Scroll indicator is never taller than its viewport")
            const indicatorPosition = indicator.mapToItem(center, 0, 0)
            verify(indicatorPosition.y + indicator.height <= center.height + 1,
                   "Tiny viewport keeps the complete indicator visible")
        }

        function test_keyboardEndScrollsToBottom() {
            const center = createCenter(360, 300)
            const scroll = findChild(center, "automationCenterScroll")
            scroll.forceActiveFocus()
            keyClick(Qt.Key_End)
            compare(Math.round(scroll.contentY),
                    Math.round(scroll.contentHeight - scroll.height))
        }
    }
}
