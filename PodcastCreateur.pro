QT       += core gui widgets multimedia

VERSION = 2.3.0    # 2.3.0 = version dist 1.0
QMAKE_TARGET_COMPANY = "Mon Studio Podcast"
QMAKE_TARGET_PRODUCT = "Podcast Createur"
QMAKE_TARGET_DESCRIPTION = "Editeur audio pour podcast"
QMAKE_TARGET_COPYRIGHT = "Copyright 2026"

TARGET = PodcastCreateur
TEMPLATE = app
CONFIG += c++17

DEFINES += USE_DIALOG_MODE
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    main.cpp \
    homedialog.cpp \
    projectwindow.cpp \
    chroniclewidget.cpp \
    scripteditor.cpp \
    audiorecorder.cpp \
    audioeditor.cpp \
    waveformwidget.cpp \
    audiomerger.cpp \
    customtooltip.cpp \
    draggablelistwidget.cpp \
    jinglemanager.cpp \
    jingleselectordialog.cpp \
    settingsdialog.cpp \
    projectutils.cpp \    
    jinglewidget.cpp  \
    projectmanager.cpp \
    waveformloader.cpp
HEADERS += \
    homedialog.h \
    projectwindow.h \
    chroniclewidget.h \
    scripteditor.h \
    audiorecorder.h \
    audioeditor.h \
    waveformwidget.h \
    audiomerger.h \
    customtooltip.h \
    draggablelistwidget.h \
    jinglemanager.h \
    jingleselectordialog.h \
    settingsdialog.h \
    projectutils.h \    
    jinglewidget.h  \
    projectmanager.h \   
    waveformloader.h

FORMS += \
    audioeditor.ui

RESOURCES += resources.qrc




# Configuration spécifique OS (reprise de votre ancien projet)
win32 {
    RC_ICONS = icones/icon.ico
}
macx {
    ICON = icones/icon.icns
}

