#pragma once
/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2012 Alexander Pipelka
 *
 *      https://github.com/pipelka/xbmc-addon-xvdr
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <queue>

#include "xvdr/clientinterface.h"
#include "xvdr/connection.h"
#include "xvdr/dataset.h"
#include "xvdr/command.h"
#include "xvdr/packetbuffer.h"

class MsgPacket;

namespace XVDR {

class ClientInterface;

/**
 * Demux class.
 * Receives the demuxed stream of a TV channel.
 */
class Demux : public Connection {
public:

	// channel switch return codes
	typedef enum {
	    SC_OK = XVDR_RET_OK,                            /* !< channel switch successful */
	    SC_ACTIVE_RECORDING = XVDR_RET_RECRUNNING,      /* !< active recording blocks channel switch */
	    SC_DEVICE_BUSY = XVDR_RET_DATALOCKED,           /* !< all devices busy */
	    SC_ENCRYPTED = XVDR_RET_ENCRYPTED,              /* !< encrypted channel cannot be decrypted */
	    SC_ERROR = XVDR_RET_ERROR,                      /* !< server (communication) error */
	    SC_INVALID_CHANNEL = XVDR_RET_DATAINVALID       /* !< invalid channel */
	} SwitchStatus;

public:

    /**
     * Demuxer constructor.
     * Create a new Demux object.
     * @param client pointer to client callback interface
     * @param buffer pointer to custom PacketBuffer object, may be NULL
     */
	Demux(ClientInterface* client, PacketBuffer* buffer = NULL);

	~Demux();

    /**
     * Open a TV channel.
     * Starts streaming of a TV channel.
     * @param hostname IP-Address or hostname of the backend to connect to
     * @param channeluid unique id of the channel to stream
     * @param clientname optional name of the demux client
     * @return Status of the operation
     */
	SwitchStatus OpenChannel(const std::string& hostname, uint32_t channeluid, const std::string& clientname = "");

    /**
     * Close a TV channel.
     * Stops streaming of a previously opened channel.
     */
	void CloseChannel();

    /**
     * Abort connection.
     * Immediately tears-down the backend connection.
     */
	void Abort();

    /**
     * Read a packet.
     * @return the next available stream packet
     */
	Packet* Read();

    /**
     * Templatized read function.
     * @return casted type of a packet
     */
	template<class T>T* Read() {
		return (T*)Read();
	}

    /**
     * Switch to a different TV channel.
     * Changes the channel on the backend without re-connecting.
     * @param channeluid unique id of the new channel
     * @return Status of the switch operation.
     */
	SwitchStatus SwitchChannel(uint32_t channeluid);

    /**
     * Set priority of stream receiver.
     * Changes the backend priority of the stream receiver. Higher values
     * block receivers with lower priority on the backend.
     * @param priority backend priority of the stream receiver
     */
	void SetPriority(int priority);

    /**
     * Wait for I-Frame on stream start.
     * Sets the flag if streaming should always start with an I-Frame
     * @param on true - streaming starts with an I-Frame
     */
	void SetStartWithIFrame(bool on);

    /**
     * Get stream properties.
     * Returns information about all available streams of the current TV channel
     * @return the stream properties structure
     */
	StreamProperties GetStreamProperties();

    /**
     * Get the signal status.
     * Returns the signal status information of the current TV channel
     * @return the signal status structure
     */
	SignalStatus GetSignalStatus();

    /**
     * Pause current TV channel.
     * @param on true - pause channel / false - continue streaming
     */
	void Pause(bool on);

    /**
     * Request to fetch signal status information.
     * Requests the backend to transmit signal status information of
     * the current TV channel
     */
	void RequestSignalInfo();

    /**
     * Channel is seekable.
     * Returns the status if the current channel is seekable (aka time-shift)
     * @return the seekable status
     */
	bool CanSeekStream();

    /**
     * Seek in current stream.
     * Jumps to a timestamp within the current playing TV channel
     * @param time the timestamp to jump to
     * @param backwards true - the jump is backwards in time
     * @param startpts returns the PTS of the new jump destination
     * @return true on success, otherwise false
     */
	bool SeekTime(int time, bool backwards, double* startpts);

protected:

	void OnDisconnect();

	void OnReconnect();

	bool OnResponsePacket(MsgPacket* resp);

	void StreamChange(MsgPacket* resp);

	void StreamStatus(MsgPacket* resp);

	void StreamSignalInfo(MsgPacket* resp);

private:

	void GetContentFromType(const std::string& type, std::string& content);

	void CleanupPacketQueue();

	StreamProperties mStreams;

	SignalStatus mSignalStatus;

	int mPriority;

	uint32_t mChannelUID;

	PacketBuffer* mBuffer;

	Mutex mLock;

	CondWait mCondition;

	bool mPaused;

	bool mTimeShiftMode;

	TimeMs mLastSignal;

	bool mIFrameStart;

	bool mCanSeekStream;
};

} // namespace XVDR
