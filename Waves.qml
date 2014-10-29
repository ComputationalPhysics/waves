import QtQuick 2.0
import Waves 1.0

Item {
    id: wavesRoot
    width: 1280
    height: 720

    Waves {
        id: waves
        anchors.fill: parent
        pan: 30
        tilt: 30
        zoom: -5

        PinchArea {
            id: pinchArea
            anchors.fill: parent
            enabled: true

            property double previousScale: 1.0

            onPinchStarted: {
                previousScale = (pinch.scale - 1.0)
            }

            onPinchUpdated: {
                var correctScale = pinch.scale >= 1.0 ? (pinch.scale - 1.0) : (-1.0 / pinch.scale + 1.0)
                var deltaScale = correctScale - previousScale
                waves.zoom += 10 * deltaScale
                previousScale = correctScale
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                property point lastPosition
                property point pressedPosition
                property real pressedTime
                property bool movedTooFarToHide: false

                onPressed: {
                    lastPosition = Qt.point(mouse.x, mouse.y)
                    pressedPosition = Qt.point(mouse.x, mouse.y)
                    pressedTime = Date.now()
                    movedTooFarToHide = false
                }

                onPositionChanged: {
                    var pressedDeltaX = mouse.x - pressedPosition.x
                    var pressedDeltaY = mouse.y - pressedPosition.y
                    if(pressedDeltaX*pressedDeltaX + pressedDeltaY*pressedDeltaY > wavesRoot.width * 0.1) {
                        movedTooFarToHide = true
                    }

                    var deltaX = mouse.x - lastPosition.x
                    var deltaY = mouse.y - lastPosition.y
                    var deltaPan = deltaX / width * 360 // max 3 rounds
                    var deltaTilt = deltaY / height * 180 // max 0.5 round

                    waves.pan += deltaPan
                    waves.tilt += deltaTilt
                    waves.tilt = Math.max(-90, Math.min(90, waves.tilt))
                    lastPosition = Qt.point(mouse.x, mouse.y)
                }

                onWheel: {
                    waves.zoom += wheel.angleDelta.y / 180
                }
            }
        }

        Timer {
            id: timer
            property real lastTime: Date.now()
            property real lastSampleTime: Date.now()
            running: true
            repeat: true
            interval: 1
            onTriggered: {
                if(!waves.previousStepCompleted) {
                    return
                }

                var currentTime = Date.now()
                var dt = currentTime - lastTime
                dt /= 1000
                waves.step(dt)
                lastTime = currentTime
            }
        }
    }
}
