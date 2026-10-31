QT     += core gui  network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The Fluent toolkit and the shared client sources are built once by
# ChatClient.pro as static libs; this project only adds main().
LIBS += -L$$OUT_PWD/lib -lchatcore -lElaWidgetTools
include(third_party/ElaWidgetTools/ElaWidgetToolsDep.pri)

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
RC_ICONS = icon.ico
DESTDIR = $$OUT_PWD/bin

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    config.ini

CONFIG(debug, debug|release) {
        #debug
    message("debug mode")
    #the dir to copy is all dll/lib files under the project's release dir, e.g. project at D:\QT\Test
    #PWD is D:/QT/Test, DllFile = D:/QT/Test/release/*.dll
    TargetConfig = $${PWD}/config.ini
    #replace "/" in the input dir with "\"
    TargetConfig = $$replace(TargetConfig, /, \\)
    #replace "/" in the output dir with "\"
    OutputDir =  $$DESTDIR
    OutputDir = $$replace(OutputDir, /, \\)
    #run the copy command
    win32: QMAKE_POST_LINK += copy /Y \"$$TargetConfig\" \"$$OutputDir\" &

    # First, define the path of the static folder
    StaticDir = $${PWD}/static
    # replace "/" in the path with "\"
    StaticDir = $$replace(StaticDir, /, \\)
    #message($${StaticDir})
    # use the xcopy command to copy the folder; /E copies subdirs and their contents including empty ones, /I creates the target dir if missing, /Y overwrites without prompting.
    win32: QMAKE_POST_LINK += xcopy /Y /E /I \"$$StaticDir\" \"$$OutputDir\\static\\\"
}else{
    #release
    message("release mode")
    #the dir to copy is all dll/lib files under the project's release dir, e.g. project at D:\QT\Test
    #PWD is D:/QT/Test, DllFile = D:/QT/Test/release/*.dll
    TargetConfig = $${PWD}/config.ini
    #replace "/" in the input dir with "\"
    TargetConfig = $$replace(TargetConfig, /, \\)
    #replace "/" in the output dir with "\"
    OutputDir =  $$DESTDIR
    OutputDir = $$replace(OutputDir, /, \\)
    #run the copy command
    win32: QMAKE_POST_LINK += copy /Y \"$$TargetConfig\" \"$$OutputDir\"

    # First, define the path of the static folder
    StaticDir = $${PWD}/static
    # replace "/" in the path with "\"
    StaticDir = $$replace(StaticDir, /, \\)
    #message($${StaticDir})
    # use the xcopy command to copy the folder; /E copies subdirs and their contents including empty ones, /I creates the target dir if missing, /Y overwrites without prompting.
    win32: QMAKE_POST_LINK += xcopy /Y /E /I \"$$StaticDir\" \"$$OutputDir\\static\\\"
}

win32-msvc*:QMAKE_CXXFLAGS += /wd"4819" /utf-8
