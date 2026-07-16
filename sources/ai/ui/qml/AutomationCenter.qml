import QtQuick 2.15

Item {
    id: root
    property QtObject controller: null

    readonly property color canvasColor: "#f3f5f7"
    readonly property color cardColor: "#ffffff"
    readonly property color inkColor: "#18222c"
    readonly property color mutedColor: "#5f6b76"
    readonly property color lineColor: "#d9e0e6"
    readonly property color accentColor: "#176b5b"
    readonly property color warningColor: "#9a5a10"
    objectName: "automationCenterRoot"

    Rectangle {
        anchors.fill: parent
        color: root.canvasColor
    }

    Flickable {
        id: scrollView
        objectName: "automationCenterScroll"
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentColumn.height + 28
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        function reveal(item) {
            const position = item.mapToItem(contentColumn, 0, 0)
            const margin = 12
            const top = Math.max(0, position.y - margin)
            const bottom = position.y + item.height + margin
            if (top < contentY)
                contentY = top
            else if (bottom > contentY + height)
                contentY = Math.max(
                            0,
                            Math.min(contentHeight - height,
                                     bottom - height))
        }

        Keys.onPressed: {
            if (event.key === Qt.Key_Up)
                contentY = Math.max(0, contentY - 40)
            else if (event.key === Qt.Key_Down)
                contentY = Math.max(
                            0,
                            Math.min(contentHeight - height,
                                     contentY + 40))
            else if (event.key === Qt.Key_PageUp)
                contentY = Math.max(0, contentY - height)
            else if (event.key === Qt.Key_PageDown)
                contentY = Math.max(
                            0,
                            Math.min(contentHeight - height,
                                     contentY + height))
            else if (event.key === Qt.Key_Home)
                contentY = 0
            else if (event.key === Qt.Key_End)
                contentY = Math.max(0, contentHeight - height)
            else
                return
            event.accepted = true
        }

        Column {
            id: contentColumn
            x: 18
            y: 12
            width: Math.max(scrollView.width - 36, 320)
            spacing: 14

            Column {
                width: parent.width
                spacing: 4

                Text {
                    objectName: "automationCenterTitle"
                    width: parent.width
                    text: qsTr("Automatisation et IA")
                    color: root.inkColor
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                    wrapMode: Text.WordWrap
                    textFormat: Text.PlainText
                }

                Text {
                    width: parent.width
                    text: qsTr("Connectez Claude, ChatGPT, Gemini ou un autre client MCP au moteur natif de QElectroTech.")
                    color: root.mutedColor
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    textFormat: Text.PlainText
                }
            }

            Rectangle {
                width: parent.width
                height: projectLayout.height + 28
                radius: 10
                color: root.cardColor
                border.color: root.lineColor

                Column {
                    id: projectLayout
                    x: 14
                    y: 14
                    width: parent.width - 28
                    height: childrenRect.height
                    spacing: 9

                    Text {
                        width: parent.width
                        text: qsTr("Projet et périmètre autorisé")
                        color: root.inkColor
                        font.weight: Font.DemiBold
                        font.pixelSize: 15
                        wrapMode: Text.WordWrap
                        textFormat: Text.PlainText
                    }

                    Text {
                        objectName: "projectTitleValue"
                        width: parent.width
                        text: root.controller && root.controller.projectTitle
                              ? root.controller.projectTitle
                              : qsTr("Aucun projet enregistré sélectionné")
                        color: root.inkColor
                        font.pixelSize: 14
                        elide: Text.ElideMiddle
                        textFormat: Text.PlainText
                    }

                    Text {
                        width: parent.width
                        text: root.controller ? root.controller.scopeDirectory : ""
                        color: root.mutedColor
                        font.family: "Consolas"
                        font.pixelSize: 11
                        wrapMode: Text.WrapAnywhere
                        textFormat: Text.PlainText
                        Accessible.name: qsTr("Répertoire MCP autorisé")
                    }

                    Flow {
                        id: projectCounters
                        objectName: "projectCounters"
                        width: parent.width
                        height: childrenRect.height
                        spacing: 8

                        Text {
                            objectName: "folioCountText"
                            readonly property int count: root.controller
                                                         ? root.controller.folioCount
                                                         : 0
                            width: Math.min(implicitWidth,
                                            projectCounters.width)
                            text: qsTr("%n folio(s)", "", count)
                                  .replace("(s)", count < 2 ? "" : "s")
                            color: root.mutedColor
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            textFormat: Text.PlainText
                        }
                        Text {
                            objectName: "elementCountText"
                            readonly property int count: root.controller
                                                         ? root.controller.elementCount
                                                         : 0
                            width: Math.min(implicitWidth,
                                            projectCounters.width)
                            text: qsTr("%n élément(s)", "", count)
                                  .replace("(s)", count < 2 ? "" : "s")
                            color: root.mutedColor
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            textFormat: Text.PlainText
                        }
                        Text {
                            objectName: "conductorCountText"
                            readonly property int count: root.controller
                                                         ? root.controller.conductorCount
                                                         : 0
                            width: Math.min(implicitWidth,
                                            projectCounters.width)
                            text: qsTr("%n conducteur(s)", "", count)
                                  .replace("(s)", count < 2 ? "" : "s")
                            color: root.mutedColor
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            textFormat: Text.PlainText
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: accessLayout.height + 28
                radius: 10
                color: root.cardColor
                border.color: root.lineColor

                Column {
                    id: accessLayout
                    x: 14
                    y: 14
                    width: parent.width - 28
                    height: childrenRect.height
                    spacing: 10

                    Item {
                        width: parent.width
                        height: Math.max(accessText.height, accessSwitch.height)

                        Column {
                            id: accessText
                            width: parent.width - 62
                            spacing: 2

                            Text {
                                width: parent.width
                                text: qsTr("Droits du serveur")
                                color: root.inkColor
                                font.weight: Font.DemiBold
                                font.pixelSize: 15
                                textFormat: Text.PlainText
                            }
                            Text {
                                width: parent.width
                                text: root.controller && root.controller.writeAccess
                                      ? qsTr("Lecture + création de nouvelles copies")
                                      : qsTr("Lecture seule — recommandé")
                                color: root.controller && root.controller.writeAccess
                                       ? root.warningColor : root.accentColor
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                                textFormat: Text.PlainText
                            }
                        }

                        FocusScope {
                            id: accessSwitch
                            readonly property bool checked: root.controller
                                                            ? root.controller.writeAccess
                                                            : false
                            objectName: "accessSwitch"
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 48
                            height: 28
                            activeFocusOnTab: true
                            Accessible.role: Accessible.CheckBox
                            Accessible.name: qsTr("Autoriser les outils d’écriture MCP")
                            Accessible.checked: checked
                            Keys.onSpacePressed: toggle()
                            Keys.onEnterPressed: toggle()
                            Keys.onReturnPressed: toggle()
                            onActiveFocusChanged: {
                                if (activeFocus)
                                    scrollView.reveal(accessSwitch)
                            }

                            function toggle() {
                                if (root.controller)
                                    root.controller.writeAccess = !checked
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: height / 2
                                color: accessSwitch.checked
                                       ? root.warningColor : "#d8dde2"
                                border.color: accessSwitch.activeFocus
                                              ? root.accentColor : root.lineColor
                                border.width: accessSwitch.activeFocus ? 2 : 1
                            }

                            Rectangle {
                                width: 22
                                height: 22
                                radius: 11
                                y: 3
                                x: accessSwitch.checked
                                   ? accessSwitch.width - width - 3 : 3
                                color: "#ffffff"
                                border.color: "#b8c0c7"
                                Behavior on x {
                                    NumberAnimation { duration: 120 }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    accessSwitch.forceActiveFocus()
                                    accessSwitch.toggle()
                                }
                            }
                        }
                    }

                    Text {
                        width: parent.width
                        visible: root.controller && root.controller.writeAccess
                        text: qsTr("Protection maintenue : destination distincte, fichier inexistant et confirm=true obligatoires.")
                        color: root.warningColor
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        textFormat: Text.PlainText
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: connectionLayout.height + 28
                radius: 10
                color: root.cardColor
                border.color: root.lineColor

                Column {
                    id: connectionLayout
                    x: 14
                    y: 14
                    width: parent.width - 28
                    height: childrenRect.height
                    spacing: 9

                    Item {
                        width: parent.width
                        height: Math.max(connectionTitle.implicitHeight,
                                         protocolLabel.implicitHeight)

                        Text {
                            id: connectionTitle
                            anchors.left: parent.left
                            text: qsTr("Connexion locale")
                            color: root.inkColor
                            font.weight: Font.DemiBold
                            font.pixelSize: 15
                            textFormat: Text.PlainText
                        }
                        Text {
                            id: protocolLabel
                            anchors.right: parent.right
                            text: root.controller
                                  ? root.controller.protocolVersion : ""
                            color: root.accentColor
                            font.pixelSize: 11
                            textFormat: Text.PlainText
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 86
                        color: "#f7f9fa"
                        radius: 7
                        border.color: root.lineColor

                        TextEdit {
                            id: commandField
                            objectName: "serverCommandField"
                            anchors.fill: parent
                            anchors.margins: 10
                            text: root.controller
                                  ? root.controller.serverCommand : ""
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.WrapAnywhere
                            color: root.inkColor
                            selectionColor: root.accentColor
                            selectedTextColor: "#ffffff"
                            font.family: "Consolas"
                            font.pixelSize: 11
                            textFormat: TextEdit.PlainText
                            Accessible.name: qsTr("Commande de lancement du serveur MCP")
                            onActiveFocusChanged: {
                                if (activeFocus)
                                    scrollView.reveal(commandField)
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        height: 40
                        spacing: 8

                        FocusScope {
                            id: copyCommandButton
                            objectName: "copyCommandButton"
                            width: (parent.width - parent.spacing) / 2
                            height: 40
                            activeFocusOnTab: true
                            Accessible.role: Accessible.Button
                            Accessible.name: copyCommandText.text
                            Keys.onSpacePressed: activate()
                            Keys.onEnterPressed: activate()
                            Keys.onReturnPressed: activate()
                            onActiveFocusChanged: {
                                if (activeFocus)
                                    scrollView.reveal(copyCommandButton)
                            }

                            function activate() {
                                if (root.controller)
                                    root.controller.copyCommand()
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: 7
                                color: copyCommandMouse.pressed
                                       ? "#dfe5e9" : "#e8ecef"
                                border.color: copyCommandButton.activeFocus
                                              ? root.accentColor : root.lineColor
                                border.width: copyCommandButton.activeFocus ? 2 : 1
                            }
                            Text {
                                id: copyCommandText
                                anchors.centerIn: parent
                                width: parent.width - 12
                                text: qsTr("Copier la commande")
                                color: root.inkColor
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                textFormat: Text.PlainText
                            }
                            MouseArea {
                                id: copyCommandMouse
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    copyCommandButton.forceActiveFocus()
                                    copyCommandButton.activate()
                                }
                            }
                        }

                        FocusScope {
                            id: copyConfigurationButton
                            objectName: "copyConfigurationButton"
                            width: (parent.width - parent.spacing) / 2
                            height: 40
                            activeFocusOnTab: true
                            Accessible.role: Accessible.Button
                            Accessible.name: copyConfigurationText.text
                            Keys.onSpacePressed: activate()
                            Keys.onEnterPressed: activate()
                            Keys.onReturnPressed: activate()
                            onActiveFocusChanged: {
                                if (activeFocus)
                                    scrollView.reveal(copyConfigurationButton)
                            }

                            function activate() {
                                if (root.controller)
                                    root.controller.copyConfiguration()
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: 7
                                color: copyConfigurationMouse.pressed
                                       ? "#dfe5e9" : "#e8ecef"
                                border.color: copyConfigurationButton.activeFocus
                                              ? root.accentColor : root.lineColor
                                border.width: copyConfigurationButton.activeFocus
                                              ? 2 : 1
                            }
                            Text {
                                id: copyConfigurationText
                                anchors.centerIn: parent
                                width: parent.width - 12
                                text: qsTr("Copier la configuration JSON")
                                color: root.inkColor
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                textFormat: Text.PlainText
                            }
                            MouseArea {
                                id: copyConfigurationMouse
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    copyConfigurationButton.forceActiveFocus()
                                    copyConfigurationButton.activate()
                                }
                            }
                        }
                    }

                    Text {
                        width: parent.width
                        visible: root.controller
                                 && root.controller.statusMessage
                        text: root.controller
                              ? root.controller.statusMessage : ""
                        color: root.accentColor
                        wrapMode: Text.WordWrap
                        font.pixelSize: 12
                        textFormat: Text.PlainText
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: toolsLayout.height + 28
                radius: 10
                color: root.cardColor
                border.color: root.lineColor

                Column {
                    id: toolsLayout
                    x: 14
                    y: 14
                    width: parent.width - 28
                    height: childrenRect.height
                    spacing: 7

                    Text {
                        width: parent.width
                        text: qsTr("Outils exposés")
                        color: root.inkColor
                        font.weight: Font.DemiBold
                        font.pixelSize: 15
                        textFormat: Text.PlainText
                    }

                    Repeater {
                        model: root.controller
                               ? root.controller.availableTools : []

                        delegate: Item {
                            width: toolsLayout.width
                            height: Math.max(14, toolText.implicitHeight)

                            Rectangle {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 8
                                height: 8
                                radius: 4
                                color: modelData.indexOf("create") >= 0
                                       || modelData.indexOf("set_titleblock") >= 0
                                       ? root.warningColor : root.accentColor
                            }
                            Text {
                                id: toolText
                                x: 16
                                width: parent.width - 16
                                text: modelData
                                color: root.inkColor
                                font.family: "Consolas"
                                font.pixelSize: 11
                                wrapMode: Text.WrapAnywhere
                                textFormat: Text.PlainText
                            }
                        }
                    }
                }
            }

            Text {
                width: parent.width
                text: qsTr("Le MCP n’envoie aucune donnée à un fournisseur : le client IA choisi par l’utilisateur décide des échanges et doit afficher les appels d’outils.")
                color: root.mutedColor
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
            }

            Item {
                width: parent.width
                height: 18
            }
        }
    }

    Rectangle {
        objectName: "scrollIndicator"
        anchors.right: parent.right
        anchors.rightMargin: 3
        y: {
            const scrollRange = Math.max(
                                  0,
                                  scrollView.contentHeight - scrollView.height)
            if (scrollRange === 0)
                return 0
            const progress = Math.max(
                               0,
                               Math.min(1, scrollView.contentY / scrollRange))
            return progress * Math.max(0, scrollView.height - height)
        }
        width: 4
        height: Math.min(scrollView.height,
                         Math.max(28,
                                  scrollView.visibleArea.heightRatio
                                  * scrollView.height))
        radius: 2
        color: "#aeb8c0"
        visible: scrollView.contentHeight > scrollView.height
        opacity: 0.8
    }
}
