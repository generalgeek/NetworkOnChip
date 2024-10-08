/******************************************************************************************
 * Universidade de Brasília – UnB
 * Instituto de Ciências Exatas – IE
 * Departamento de Ciência da Computação – CIC
 * Modelagem de Sistemas em Silício – Professor R. Jacobi
 *
 * Projeto: Simple System-C NoC.
 *
 * Nome: Jessé Barreto de Barros, Javier Urresty Sanches, João Carlos Passos
 * Matrícula: 17/0067033
 * Copyright 2017 - All rights reserved
 ******************************************************************************************
 */

#include "router.h"

#include <cstdlib>

#include "noccommon.h"
#include "nocdebug.h"
#include "nocrouting.h"

Router::Router(sc_module_name name, unsigned routerId):
    sc_module(name),
    _routerId(routerId),
    _srcLocal(Link::Local),
    _srcNorth(Link::Local),
    _srcSouth(Link::Local),
    _srcEast(Link::Local),
    _srcWest(Link::Local) {
    _initChannelBuffers();

    _mutexInputChannels.push_back(new sc_mutex("LocalChannelMutex"));
    _mutexInputChannels.push_back(new sc_mutex("NorthChannelMutex"));
    _mutexInputChannels.push_back(new sc_mutex("SouthChannelMutex"));
    _mutexInputChannels.push_back(new sc_mutex("EastChannelMutex"));
    _mutexInputChannels.push_back(new sc_mutex("WestChannelMutex"));

    _arbiterLinkId = 0;

    SC_THREAD(_localChannelReadThread);
    SC_THREAD(_northChannelReadThread);
    SC_THREAD(_southChannelReadThread);
    SC_THREAD(_eastChannelReadThread);
    SC_THREAD(_westChannelReadThread);
    SC_THREAD(_localChannelWriteThread);
    SC_THREAD(_northChannelWriteThread);
    SC_THREAD(_southChannelWriteThread);
    SC_THREAD(_eastChannelWriteThread);
    SC_THREAD(_westChannelWriteThread);
    SC_THREAD(_arbiterThread);
}

Router::~Router() {
    // Destroy the Flit buffers.
    for (std::pair<sc_fifo<Flit*>*, Link>& pair : _inputBuffers) {
        if (!pair.first) {
            if (!pair.first->num_available()) {
                Flit* flit;
                for (int i = 0; i < pair.first->num_available(); i++) {
                    pair.first->nb_read(flit);
                    if (!flit) {
                        delete flit;
                        flit = nullptr;
                    }
                }
            }
            delete pair.first;
            pair.first = nullptr;
        }
    }

    // Destroy the Mutexes
    for (sc_mutex* mutex : _mutexInputChannels) {
        if (mutex != nullptr) {
            delete mutex;
        }
    }
}

std::string Router::getName() {
    return std::string(this->name());
}

const int Router::getIdNumber() {
    return _routerId;
}

void Router::_arbiterThread() {
    for (;;) {
        std::pair<sc_fifo<Flit*>*, Link> pair = _inputBuffers[_arbiterLinkId];
        if (pair.first->num_available() != 0) {
            switch (pair.second) {
            case Link::West:
                if (_mutexInputChannels[_arbiterLinkId]->trylock() != -1) {
                    _writeWest.notify();
                    _srcWest = Link(_arbiterLinkId);
                }
                break;
            case Link::East:
                if (_mutexInputChannels[_arbiterLinkId]->trylock() != -1) {
                    _writeEast.notify();
                    _srcEast = Link(_arbiterLinkId);
                }
                break;
            case Link::South:
                if (_mutexInputChannels[_arbiterLinkId]->trylock() != -1) {
                    _writeSouth.notify();
                    _srcSouth = Link(_arbiterLinkId);
                }
                break;
            case Link::North:
                if (_mutexInputChannels[_arbiterLinkId]->trylock() != -1) {
                    _writeNorth.notify();
                    _srcNorth = Link(_arbiterLinkId);
                }
                break;
            case Link::Local:
            default:
                if (_mutexInputChannels[_arbiterLinkId]->trylock() != -1) {
                    _writeLocal.notify();
                    _srcLocal = Link(_arbiterLinkId);
                }
                break;
            }
        }
        _arbiterLinkId = _arbiterLinkId == 4 ? 0 : _arbiterLinkId + 1;
        // wait(SC_ZERO_TIME); 
        // 上面这东西导致仿真时间无法推进, 会一直卡在这里, 导致test_src/test_routerchannel.cpp里的时钟周期无法推进
        wait(100, SC_PS);
    }
}

void Router::_localChannelReadThread() {
    sc_fifo<Flit*>* localBuffer = _inputBuffers[Link::Local].first;
    Link& localBufferFlitsDstLink = _inputBuffers[Link::Local].second;
    for (;;) {
        // Checks whether the internal buffer is empty
        if (localBuffer->num_available() == 0) {
            _readFromChannel(&localChannelIn, localBuffer, localBufferFlitsDstLink);
        }
    }
}

void Router::_localChannelWriteThread() {
    for (;;) {
        wait(_writeLocal);
        sc_fifo<Flit*>* localBuffer = _inputBuffers[_srcLocal].first;
        localChannelOut->sendFlit(localBuffer->read());
        if (localBuffer->num_available() == 0) {
            _mutexInputChannels[_srcLocal]->unlock();
        }
    }
}

void Router::_northChannelReadThread() {
    sc_fifo<Flit*>* localBuffer = _inputBuffers[Link::North].first;
    Link& localBufferFlitsDstLink = _inputBuffers[Link::North].second;
    for (;;) {
        // Checks whether the internal buffer is empty
        if (localBuffer->num_available() == 0) {
            _readFromChannel(&northChannelIn, localBuffer, localBufferFlitsDstLink);
        }
    }
}

void Router::_northChannelWriteThread() {
    for (;;) {
        wait(_writeNorth);
        sc_fifo<Flit*>* localBuffer = _inputBuffers[_srcNorth].first;
        northChannelOut->sendFlit(localBuffer->read());
        if (localBuffer->num_available() == 0) {
            _mutexInputChannels[_srcNorth]->unlock();
        }
    }
}

void Router::_southChannelReadThread() {
    sc_fifo<Flit*>* localBuffer = _inputBuffers[Link::South].first;
    Link& localBufferFlitsDstLink = _inputBuffers[Link::South].second;
    for (;;) {
        // Checks whether the internal buffer is empty
        if (localBuffer->num_available() == 0) {
            _readFromChannel(&southChannelIn, localBuffer, localBufferFlitsDstLink);
        }
    }
}

void Router::_southChannelWriteThread() {
    for (;;) {
        wait(_writeSouth);
        sc_fifo<Flit*>* localBuffer = _inputBuffers[_srcSouth].first;
        southChannelOut->sendFlit(localBuffer->read());
        if (localBuffer->num_available() == 0) {
            _mutexInputChannels[_srcSouth]->unlock();
        }
    }
}

void Router::_eastChannelReadThread() {
    for (;;) {
        sc_fifo<Flit*>* localBuffer = _inputBuffers[Link::East].first;
        Link& localBufferFlitsDstLink = _inputBuffers[Link::East].second;
        // Checks whether the internal buffer is empty
        if (localBuffer->num_available() == 0) {
            _readFromChannel(&eastChannelIn, localBuffer, localBufferFlitsDstLink);
        }
    }
}

void Router::_eastChannelWriteThread() {
    for (;;) {
        wait(_writeEast);
        sc_fifo<Flit*>* localBuffer = _inputBuffers[_srcEast].first;
        eastChannelOut->sendFlit(localBuffer->read());
        if (localBuffer->num_available() == 0) {
            _mutexInputChannels[_srcEast]->unlock();
        }
    }
}

void Router::_westChannelReadThread() {
    sc_fifo<Flit*>* localBuffer = _inputBuffers[Link::West].first;
    Link& localBufferFlitsDstLink = _inputBuffers[Link::West].second;
    for (;;) {
        // Checks whether the internal buffer is empty
        if (localBuffer->num_available() == 0) {
            _readFromChannel(&westChannelIn, localBuffer, localBufferFlitsDstLink);
        }
    }
}

void Router::_westChannelWriteThread() {
    for (;;) {
        wait(_writeWest);
        sc_fifo<Flit*>* localBuffer = _inputBuffers[_srcWest].first;
        westChannelOut->sendFlit(localBuffer->read());
        if (localBuffer->num_available() == 0) {
            _mutexInputChannels[_srcWest]->unlock();
        }
    }
}

Link Router::_routingMethod(Flit* flit) {   // 路由器数据转发决策
    if (flit == nullptr) {
        NoCDebug::printDebug("Router Id-" + std::to_string(_routerId) + " flit is nullptr.", NoCDebug::Router, true);
        std::cerr << "file: " << __FILE__ << " line: " << __LINE__ << std::endl;
    }

    flit_t flitData = flit->getData();
    unsigned flitDstId = flitData.range(23, 16);
    routingInfo info = routingXY(_routerId, flitDstId);
    Link ret;
    switch (info.link) {
    case 4:
        ret = Link::West;
        break;
    case 3:
        ret = Link::East;
        break;
    case 2:
        ret = Link::South;
        break;
    case 1:
        ret = Link::North;
        break;
    case 0:
    default:
        ret = Link::Local;
        break;
    }
    return ret;
}

void Router::_initChannelBuffers() {
    for (int i = 0; i < 5; i++) {
        _inputBuffers.push_back(std::make_pair(new sc_fifo<Flit*>, Link::Local));
    }
}

void Router::_readFromChannel(sc_port<IRouterChannel>* inputChannel, sc_fifo<Flit*>* localBuffer,
                              Link& localBufferFlitsDstLink) {
    // Receives the header flit.
    Flit* dataFlit = nullptr;
    dataFlit = (*inputChannel)->receiveFlit();

    // Checks whether link this flit should go
    localBufferFlitsDstLink = _routingMethod(dataFlit);

    // Put the header flit to the buffer
    localBuffer->write(dataFlit);

    // Put the other flits to the buffer
    int packageSize = dataFlit->getData().range(15, 0);
    for (int i = 0; i < packageSize; i++) {
        dataFlit = (*inputChannel)->receiveFlit();
        localBuffer->write(dataFlit);
    }

    // When it finishes to read the package
    for (; localBuffer->num_available();) {
        wait(localBuffer->data_read_event());
    }
}
