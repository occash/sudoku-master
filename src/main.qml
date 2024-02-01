import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Sudoku 1.0

Window {
    id: root

    visible: true
    width: 400
    height: 500
    title: qsTr("Sudoku Master")

    QtObject {
        id: d
        readonly property var sizes: [
            {
                side: 4,
                boxSize: Qt.size(2, 2),
                alphabet: ["1", "2", "3", "4"]
            },
            {
                side: 6,
                boxSize: Qt.size(3, 2),
                alphabet: ["1", "2", "3", "4", "5", "6"]
            },
            {
                side: 9,
                boxSize: Qt.size(3, 3),
                alphabet: ["1", "2", "3", "4", "5", "6", "7", "8", "9"]
            },
            {
                side: 12,
                boxSize: Qt.size(4, 3),
                alphabet: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C"]
            },
            {
                side: 16,
                boxSize: Qt.size(4, 4),
                alphabet: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G"]
            }
        ]
        readonly property var current: sizes[slider.value]
        readonly property var colors: ["#f2b179", "#ede0c6"]
    }

    BoardModel {
        id: board
        side: d.current.side
        boxSize: d.current.boxSize
    }

    Rectangle {
        id: field
        height: width
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 8
        }
        radius: 4
        color: "#bbada0"

        GridLayout {
            id: boxLayout

            anchors.fill: parent
            anchors.margins: 4
            columns: board.side / board.boxSize.width
            rows: board.side / board.boxSize.height

            Repeater {
                model: board
                delegate: Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.row: row
                    Layout.column: column
                    color: "#8f7a66"
                    radius: 4

                    GridLayout {
                        id: cellLayout

                        anchors.fill: parent
                        anchors.margins: 4
                        columns: model.width
                        rows: model.height

                        Repeater {
                            model: cells
                            delegate: Rectangle {
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                Layout.row: modelData.row
                                Layout.column: modelData.column
                                color: modelData.selection < 0 ? "#eee4da" : d.colors[modelData.selection]
                                radius: 4

                                Text {
                                    anchors.fill: parent
                                    text: modelData.value < 0 ? "" : d.current.alphabet[modelData.value]
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 20
                                    color: "#776e65"
                                    visible: modelData.filled
                                }

                                TapHandler {
                                    onTapped: board.select(modelData.x, modelData.y)
                                }
                            }
                        }
                    }
                }
            }
        }


    }

    ColumnLayout {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: field.bottom
            margins: 8
        }

        Slider {
            id: slider
            Layout.fillWidth: true
            from: 0
            to: 4
            value: 2
            stepSize: 1
            snapMode: Slider.SnapAlways
        }

        Button {
            Layout.fillWidth: true
            text: "Refill"
            onClicked: board.fill()
        }
    }
}
