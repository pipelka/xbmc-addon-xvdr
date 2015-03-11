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

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "xvdr/demux.h"
#include "xvdr/msgpacket.h"
#include "xvdr/command.h"

using namespace XVDR;

Demux::Demux(ClientInterface* client, PacketBuffer* buffer) : Connection(client), mPriority(50),
	mPaused(false), mTimeShiftMode(false), mChannelUID(0), mBuffer(buffer),
	mIFrameStart(false) {
	mCanSeekStream = (mBuffer != NULL);

	// create a small memory buffer as queue
	if(mBuffer == NULL) {
		mBuffer = PacketBuffer::create(10 * 1024 * 1024);
	}
}

Demux::~Demux() {
	// wait for pending requests
	MutexLock lock(&mLock);
	delete mBuffer;
}

Demux::SwitchStatus Demux::OpenChannel(const std::string& hostname, uint32_t channeluid, const std::string& clientname) {
	if(!Open(hostname, clientname)) {
		return SC_ERROR;
	}

	mPaused = false;
	mTimeShiftMode = false;

	return SwitchChannel(channeluid);
}

void Demux::CloseChannel() {
    Close();
    CleanupPacketQueue();
}

StreamProperties Demux::GetStreamProperties() {
	MutexLock lock(&mLock);
	return mStreams;
}

void Demux::CleanupPacketQueue() {
	MutexLock lock(&mLock);
	mBuffer->clear();
}

void Demux::Abort() {
	mStreams.clear();
	Connection::Abort();
	CleanupPacketQueue();
	mCondition.Signal();
}

Packet* Demux::Read() {
	if(ConnectionLost() || Aborting()) {
		return NULL;
	}

	Packet* p = NULL;
	MsgPacket* pkt = NULL;

	// request packets in timeshift mode
	if(mTimeShiftMode) {
		MsgPacket req(XVDR_CHANNELSTREAM_REQUEST, XVDR_CHANNEL_STREAM);

		if(!Session::TransmitMessage(&req)) {
			return NULL;
		}
	}

	// fetch packet from packetbuffer (queue))
	{
		MutexLock lock(&mLock);
		pkt = mBuffer->get();
	}

	// empty queue -> return empty packet
	if(pkt == NULL) {
		mCondition.Wait(100);
		p = m_client->AllocatePacket(0);
		return p;
	}

	if(pkt->getMsgID() == XVDR_STREAM_CHANGE) {
		StreamChange(pkt);
		p = m_client->StreamChange(mStreams);
	}
	else {
		uint16_t id = pkt->get_U16();
		int64_t pts = pkt->get_S64();
		int64_t dts = pkt->get_S64();
		uint32_t duration = pkt->get_U32();
		uint32_t length = pkt->get_U32();
		uint8_t* payload = pkt->consume(length);

		if(mStreams.find(id) == mStreams.end()) {
			p = m_client->AllocatePacket(0);
		}
		else {
			Stream& stream = mStreams[id];
			p = m_client->AllocatePacket(length);
			m_client->SetPacketData(p, payload, stream.Index, dts, pts, duration);
		}
	}

	{
		MutexLock lock(&mLock);
		mBuffer->release(pkt);
	}

	return p;
}

bool Demux::OnResponsePacket(MsgPacket* resp) {
	if(resp->getType() != XVDR_CHANNEL_STREAM) {
		return false;
	}

	switch(resp->getMsgID()) {
		case XVDR_STREAM_DETACH:
			m_client->OnDetach();
			Abort();
			break;

		case XVDR_STREAM_STATUS:
			StreamStatus(resp);
			break;

		case XVDR_STREAM_SIGNALINFO:
			StreamSignalInfo(resp);
			break;

		case XVDR_STREAM_CHANGE:
		case XVDR_STREAM_MUXPKT: {
				MutexLock lock(&mLock);
				mBuffer->put(resp);
				mCondition.Signal();
			}

			return true;

			// discard unknown packet types
		default:
			break;
	}

	return false;
}

Demux::SwitchStatus Demux::SwitchChannel(uint32_t channeluid) {
	m_client->Log(DEBUG, "changing to channel %d (priority %i)", channeluid, mPriority);

	CleanupPacketQueue();

	{
		MutexLock lock(&mLock);
		mStreams.clear();
	}

	mCondition.Signal();

	MsgPacket vrp(XVDR_CHANNELSTREAM_OPEN);
	vrp.put_U32(channeluid);
	vrp.put_S32(mPriority);
	vrp.put_U8(mIFrameStart);

	MsgPacket* vresp = ReadResult(&vrp);

	mPaused = false;
	mTimeShiftMode = false;

	SwitchStatus status = SC_OK;

	if(vresp != NULL) {
		status = (SwitchStatus)vresp->get_U32();
	}

	delete vresp;

	if(status == SC_OK) {
		mChannelUID = channeluid;
		m_client->Log(INFO, "sucessfully switched channel");
	}
	else {
		m_client->Log(FAILURE, "%s - failed to set channel (status: %i)", __FUNCTION__, status);
	}

	mCondition.Signal();

	return status;
}

SignalStatus Demux::GetSignalStatus() {
	MutexLock lock(&mLock);
	return mSignalStatus;
}

void Demux::GetContentFromType(const std::string& type, std::string& content) {
	if(type == "AC3") {
		content = "AUDIO";
	}
	else if(type == "MPEG2AUDIO") {
		content = "AUDIO";
	}
	else if(type == "AAC") {
		content = "AUDIO";
	}
	else if(type == "EAC3") {
		content = "AUDIO";
	}
	else if(type == "MPEG2VIDEO") {
		content = "VIDEO";
	}
	else if(type == "H264") {
		content = "VIDEO";
	}
	else if(type == "DVBSUB") {
		content = "SUBTITLE";
	}
	else if(type == "TELETEXT") {
		content = "TELETEXT";
	}
	else {
		content = "UNKNOWN";
	}
}

void Demux::StreamChange(MsgPacket* resp) {
	MutexLock lock(&mLock);
	mStreams.clear();

	int index = 0;
	uint32_t composition_id;
	uint32_t ancillary_id;

	while(!resp->eop()) {
		Stream stream;

		stream.Index = index++;
		stream.PhysicalId = resp->get_U32();
		stream.Type = resp->get_String();

		GetContentFromType(stream.Type, stream.Content);

		stream.Identifier = -1;

		if(stream.Content == "AUDIO") {
			stream.Language = resp->get_String();
			stream.Channels = resp->get_U32();
			stream.SampleRate = resp->get_U32();
			stream.BlockAlign = resp->get_U32();
			stream.BitRate = resp->get_U32();
			stream.BitsPerSample = resp->get_U32();
		}
		else if(stream.Content == "VIDEO") {
			stream.FpsScale = resp->get_U32();
			stream.FpsRate = resp->get_U32();
			stream.Height = resp->get_U32();
			stream.Width = resp->get_U32();
			stream.Aspect = (double)resp->get_S64() / 10000.0;
		}
		else if(stream.Content == "SUBTITLE") {
			stream.Language = resp->get_String();
			composition_id = resp->get_U32();
			ancillary_id   = resp->get_U32();
			stream.Identifier = (composition_id & 0xffff) | ((ancillary_id & 0xffff) << 16);
		}

		if(index > 16) {
			m_client->Log(FAILURE, "%s - max amount of streams reached", __FUNCTION__);
			break;
		}

		mStreams[stream.PhysicalId] = stream;
	}
}

void Demux::StreamStatus(MsgPacket* resp) {
	uint32_t status = resp->get_U32();

	switch(status) {
		case XVDR_STREAM_STATUS_SIGNALLOST:
			m_client->OnSignalLost();
			break;

		case XVDR_STREAM_STATUS_SIGNALRESTORED:
			m_client->OnSignalRestored();
			break;

		default:
			break;
	}
}

void Demux::StreamSignalInfo(MsgPacket* resp) {
	MutexLock lock(&mLock);
	mSignalStatus << resp;
}

void Demux::OnDisconnect() {
}

void Demux::OnReconnect() {
}

void Demux::SetPriority(int priority) {
	if(priority < -1 || priority > 99) {
		priority = 50;
	}

	mPriority = priority;
}

void Demux::Pause(bool on) {
	if(mCanSeekStream) {
		return;
	}

	MsgPacket req(XVDR_CHANNELSTREAM_PAUSE);
	req.put_U32(on);

	MsgPacket* vresp = ReadResult(&req);
	delete vresp;

	{
		MutexLock lock(&mLock);

		if(!on && mPaused) {
			mTimeShiftMode = true;
		}

		mPaused = on;
	}

	mCondition.Signal();
}

void Demux::RequestSignalInfo() {
	if(!mLastSignal.TimedOut()) {
		return;
	}

	MsgPacket req(XVDR_CHANNELSTREAM_SIGNAL);
	Session::TransmitMessage(&req);

	// signal status timeout
	mLastSignal.Set(5000);
}

bool Demux::CanSeekStream() {
	return mCanSeekStream;
}

bool Demux::SeekTime(int time, bool backwards, double* startpts) {
	MutexLock lock(&mLock);

	if(!mCanSeekStream) {
		return false;
	}

	return mBuffer->seek(time, backwards, startpts);
}

void Demux::SetStartWithIFrame(bool on) {
	mIFrameStart = on;
}
