QT += widgets

CONFIG += c++17

HEADERS       = \
                board.h \
                bot.h \
                graphic.h \
                imagew.h \
                mainwindow.h \
                renderthread.h
SOURCES       = main.cpp \
                bot.cpp \
                graphic.cpp \
                imagew.cpp \
                mainwindow.cpp \
                renderthread.cpp

unix:!mac:!vxworks:!integrity:!haiku:LIBS += -lm

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/threads/mandelbrot
INSTALLS += target

DISTFILES +=

FORMS += \
    graphic.ui \
    imagew.ui \
    mainwindow.ui
