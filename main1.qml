import QtQuick 2.12

Rectangle {
    visible: true
    width: 640
    height: 480
    //title: qsTr("Hello World")
    //opacity: 0.5
    //color: "blue"
    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "red";}
        GradientStop { position: 1.0; color: "blue" }
    }


    Rectangle {
        anchors.centerIn: parent
        width: 100; height: 100; color: "green"
        RotationAnimation on rotation {
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: 2000
        }
    }
}
