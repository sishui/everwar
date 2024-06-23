/*
 * This file is part of the UeCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/** \addtogroup u2w User to World Communication
 * @{
 * \file WorldSocket.h
 * \author Derex <derex101@gmail.com>
 */

#ifndef _WORLDSOCKET_H
#define _WORLDSOCKET_H

#include "Common.h"
#include "Auth/AuthCrypt.h"
#include "Auth/BigNumber.h"
#include "Network/Socket.hpp"

#include <chrono>
#include <functional>

class WorldPacket;
class WorldSession;

/**
 * WorldSocket.
 *
 * This class is responsible for the communication with
 * remote clients.
 * Most methods return -1 on failure.
 * The class uses reference counting.
 *
 * For output the class uses one buffer (64K usually) and
 * a queue where it stores packet if there is no place on
 * the queue. The reason this is done, is because the server
 * does really a lot of small-size writes to it, and it doesn't
 * scale well to allocate memory for every. When something is
 * written to the output buffer the socket is not immediately
 * activated for output (again for the same reason), there
 * is 10ms celling (thats why there is Update() override method).
 * This concept is similar to TCP_CORK, but TCP_CORK
 * uses 200ms celling. As result overhead generated by
 * sending packets from "producer" threads is minimal,
 * and doing a lot of writes with small size is tolerated.
 *
 * The calls to Update () method are managed by WorldSocketMgr
 * and ReactorRunnable.
 *
 * For input ,the class uses one 1024 bytes buffer on stack
 * to which it does recv() calls. And then received data is
 * distributed where its needed. 1024 matches pretty well the
 * traffic generated by client for now.
 *
 * The input/output do speculative reads/writes (AKA it tryes
 * to read all data available in the kernel buffer or tryes to
 * write everything available in userspace buffer),
 * which is ok for using with Level and Edge Triggered IO
 * notification.
 *
 */

class WorldSocket : public MaNGOS::Socket
{
    private:
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif
        struct ClientPktHeader
        {
            uint16 size;
            uint32 cmd;
        };
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

        /// Time in which the last ping was received
        std::chrono::system_clock::time_point m_lastPingTime;

        /// Keep track of over-speed pings ,to prevent ping flood.
        uint32 m_overSpeedPings;

        ClientPktHeader m_existingHeader;
        bool m_useExistingHeader;

        /// Class used for managing encryption of the headers
        AuthCrypt m_crypt;

        /// Session to which received packets are routed
        WorldSession *m_session;
        bool m_sessionFinalized;

        const uint32 m_seed;

        BigNumber m_s;

        /// process one incoming packet.
        virtual bool ProcessIncomingData() override;

        /// Called by ProcessIncoming() on CMSG_AUTH_SESSION.
        bool HandleAuthSession(WorldPacket &recvPacket);

        /// Called by ProcessIncoming() on CMSG_PING.
        bool HandlePing(WorldPacket &recvPacket);

    public:
        WorldSocket(boost::asio::io_service &service, std::function<void (Socket *)> closeHandler);

        // send a packet \o/
        void SendPacket(const WorldPacket& pct, bool immediate = false);

        void FinalizeSession() { m_session = nullptr; }

        virtual bool Open() override;

        /// Return the session key
        BigNumber &GetSessionKey() { return m_s; }

};

#endif  /* _WORLDSOCKET_H */

/// @}
