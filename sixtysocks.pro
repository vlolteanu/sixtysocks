TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -lpthread

QMAKE_CXXFLAGS += -std=c++11

SOURCES += \
    core/listenreactor.cc \
    core/poller.cc \
    core/reactor.cc \
    core/streamreactor.cc \
    proxifier/proxifier.cc \
    proxifier/proxifierdownstreamer.cc \
    proxifier/proxifierupstreamer.cc \
    proxy/proxy.cc \
    proxy/proxyupstreamer.cc \
    proxy/simpleproxydownstreamer.cc \
    proxy/connectproxydownstreamer.cc \
    core/authenticationreactor.cc \
    authentication/authserver.cc \
    sixtysocks.cc

HEADERS += \
    core/poller.hh \
    core/listenreactor.hh \
    core/reactor.hh \
    core/streamreactor.hh \
    proxifier/proxifier.hh \
    proxifier/proxifierdownstreamer.hh \
    proxifier/proxifierupstreamer.hh \
    proxy/proxy.hh \
    core/exfd.h \
    proxy/proxyupstreamer.hh \
    proxy/simpleproxydownstreamer.hh \
    proxy/connectproxydownstreamer.hh \
    core/spinlock.hh \
    authentication/authserver.hh \
    core/authenticationreactor.hh

LIBS = -lsocks6msg -lsocks6util -lpthread -lboost_system