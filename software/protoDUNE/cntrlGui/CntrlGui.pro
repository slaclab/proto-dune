
QMAKE_CXXFLAGS += -g
TEMPLATE = app
FORMS    = 
HEADERS  = XmlClient.h SystemWindow.h MainWindow.h CommandHolder.h CommandWindow.h VariableHolder.h VariableWindow.h SummaryWindow.h GenericWindow.h PgpLinkWindow.h
SOURCES  = XmlClient.cpp CntrlGui.cpp SystemWindow.cpp MainWindow.cpp CommandHolder.cpp CommandWindow.cpp  VariableHolder.cpp VariableWindow.cpp SummaryWindow.cpp GenericWindow.cpp PgpLinkWindow.cpp
TARGET   = ../bin/cntrlGui
QT       += network xml script

