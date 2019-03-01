import QtQuick 2.11

Rectangle {
    visible: true
    width: 640
    height: 480
    //title: qsTr("Hello World")
    //opacity: 0.5
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#00FFFF00";}
        GradientStop { position: 1.0; color: "#FFFF00FF" }
    }


    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 20
        width: 100; height: 100;
        color: mousearea.containsMouse ? "blue" : "orange"

        MouseArea {
            id: mousearea
            anchors.fill: parent
            hoverEnabled: true

        }

        PropertyAnimation on y {
            loops: Animation.Infinite
            from: 0
            to: 600
            duration: 3000
        }
    }

    Text{
        anchors.centerIn: parent
        text: "dans le bon sens"
    }
}
