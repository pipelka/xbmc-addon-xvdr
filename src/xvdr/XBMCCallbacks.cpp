/*
 *      Copyright (C) 2012 Alexander Pipelka
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "XBMCCallbacks.h"
#include "XBMCAddon.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "DVDDemuxPacket.h"

using namespace ADDON;
using namespace XVDR;

#define MSG_MAXLEN 512

cXBMCCallbacks::cXBMCCallbacks() : m_handle(NULL)
{
  m_msg = (char*)malloc(MSG_MAXLEN);
}

cXBMCCallbacks::~cXBMCCallbacks()
{
  free(m_msg);
}

void cXBMCCallbacks::SetHandle(ADDON_HANDLE handle)
{
  m_handle = handle;
}

void cXBMCCallbacks::Log(XVDR_LOGLEVEL level, const std::string& text, ...)
{
  addon_log_t lt;
  switch(level)
  {
    case XVDR_DEBUG:
      lt = LOG_DEBUG;
      break;
    default:
    case XVDR_INFO:
      lt = LOG_INFO;
      break;
    case XVDR_NOTICE:
      lt = LOG_NOTICE;
      break;
    case XVDR_ERROR:
      lt = LOG_ERROR;
      break;
  }

  va_list ap;
  va_start(ap, &text);

  vsnprintf(m_msg, MSG_MAXLEN, text.c_str(), ap);
  va_end(ap);

  XBMC->Log(lt, m_msg);
}

void cXBMCCallbacks::Notification(XVDR_LOGLEVEL level, const std::string& text, ...)
{
  queue_msg qm;
  switch(level)
  {
    default:
    case XVDR_INFO:
      qm = QUEUE_INFO;
      break;
    case XVDR_ERROR:
      qm = QUEUE_ERROR;
      break;
    case XVDR_WARNING:
      qm = QUEUE_WARNING;
      break;
  }

  va_list ap;
  va_start(ap, &text);

  vsnprintf(m_msg, MSG_MAXLEN, text.c_str(), ap);
  va_end(ap);

  XBMC->QueueNotification(qm, m_msg);
}

void cXBMCCallbacks::Recording(const std::string& line1, const std::string& line2, bool on)
{
  PVR->Recording(line1.c_str(), line2.c_str(), on);
}

void cXBMCCallbacks::ConvertToUTF8(std::string& text)
{
  XBMC->UnknownToUTF8(text);
}

std::string cXBMCCallbacks::GetLanguageCode()
{
  const char* code = XBMC->GetDVDMenuLanguage();
  return (code == NULL) ? "" : code;
}

const char* cXBMCCallbacks::GetLocalizedString(int id)
{
  return XBMC->GetLocalizedString(id);
}

void cXBMCCallbacks::TriggerChannelUpdate()
{
  PVR->TriggerChannelUpdate();
}

void cXBMCCallbacks::TriggerRecordingUpdate()
{
  PVR->TriggerRecordingUpdate();
}

void cXBMCCallbacks::TriggerTimerUpdate()
{
  PVR->TriggerTimerUpdate();
}

void cXBMCCallbacks::TransferChannelEntry(const Channel& channel)
{
  PVR_CHANNEL pvrchannel;
  pvrchannel << channel;

  PVR->TransferChannelEntry(m_handle, &pvrchannel);
}

void cXBMCCallbacks::TransferEpgEntry(const Epg& epg)
{
  EPG_TAG pvrepg;
  pvrepg << epg;

  PVR->TransferEpgEntry(m_handle, &pvrepg);
}

void cXBMCCallbacks::TransferTimerEntry(const Timer& timer)
{
  PVR_TIMER pvrtimer;
  pvrtimer << timer;

  PVR->TransferTimerEntry(m_handle, &pvrtimer);
}

void cXBMCCallbacks::TransferRecordingEntry(const RecordingEntry& rec)
{
  PVR_RECORDING pvrrec;
  pvrrec << rec;

  PVR->TransferRecordingEntry(m_handle, &pvrrec);
}

void cXBMCCallbacks::TransferChannelGroup(const ChannelGroup& group)
{
  PVR_CHANNEL_GROUP pvrgroup;
  pvrgroup << group;

  PVR->TransferChannelGroup(m_handle, &pvrgroup);
}

void cXBMCCallbacks::TransferChannelGroupMember(const ChannelGroupMember& member)
{
  PVR_CHANNEL_GROUP_MEMBER pvrmember;
  pvrmember << member;

  PVR->TransferChannelGroupMember(m_handle, &pvrmember);
}

Packet* cXBMCCallbacks::AllocatePacket(int s)
{
  DemuxPacket* d = PVR->AllocateDemuxPacket(s);
  if(d == NULL)
    return NULL;

  d->iSize = s;
  return (Packet*)d;
}

void cXBMCCallbacks::SetPacketData(Packet* packet, uint8_t* data, int streamid, uint64_t dts, uint64_t pts)
{
  if (packet == NULL)
    return;

  DemuxPacket* d = static_cast<DemuxPacket*>(packet);

  d->iStreamId = streamid;
  d->duration  = 0;
  d->dts       = (double)dts * DVD_TIME_BASE / 1000000;
  d->pts       = (double)pts * DVD_TIME_BASE / 1000000;

  if (data != NULL)
    memcpy(d->pData, data, d->iSize);
}

void cXBMCCallbacks::FreePacket(Packet* packet)
{
  PVR->FreeDemuxPacket((DemuxPacket*)packet);
}

Packet* cXBMCCallbacks::StreamChange(const StreamProperties& p) {
  Packet* pkt = AllocatePacket(0);
  if (pkt != NULL)
    SetPacketData(pkt, NULL, DMX_SPECIALID_STREAMCHANGE, 0, 0);

  return pkt;
}

Packet* cXBMCCallbacks::ContentInfo(const StreamProperties& p) {
  Packet* pkt = AllocatePacket(sizeof(PVR_STREAM_PROPERTIES));

  if (pkt != NULL) {
    PVR_STREAM_PROPERTIES props;
    props << p;
    SetPacketData(pkt, (uint8_t*)&props, DMX_SPECIALID_STREAMINFO, 0, 0);
  }

  return pkt;
}

void cXBMCCallbacks::Lock() {
  m_mutex.Lock();
}

void cXBMCCallbacks::Unlock() {
  m_mutex.Unlock();
}

PVR_CHANNEL& operator<< (PVR_CHANNEL& lhs, const Channel& rhs)
{
	memset(&lhs, 0, sizeof(lhs));

	lhs.bIsHidden = rhs[channel_ishidden];
	lhs.bIsRadio = rhs[channel_isradio];
	lhs.iChannelNumber = rhs[channel_number];
	lhs.iEncryptionSystem = rhs[channel_encryptionsystem];
	lhs.iUniqueId = rhs[channel_uid];
	strncpy(lhs.strChannelName, rhs[channel_name].c_str(), sizeof(lhs.strChannelName));
	strncpy(lhs.strIconPath, rhs[channel_iconpath].c_str(), sizeof(lhs.strIconPath));
	strncpy(lhs.strInputFormat, rhs[channel_inputformat].c_str(), sizeof(lhs.strInputFormat));
	strncpy(lhs.strStreamURL, rhs[channel_streamurl].c_str(), sizeof(lhs.strStreamURL));

	return lhs;
}

EPG_TAG& operator<< (EPG_TAG& lhs, const Epg& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.endTime = rhs[epg_endtime];
	lhs.iChannelNumber = rhs[epg_uid];
	lhs.iGenreSubType = rhs[epg_genresubtype];
	lhs.iGenreType = rhs[epg_genretype];
	lhs.iParentalRating = rhs[epg_parentalrating];
	lhs.iUniqueBroadcastId = rhs[epg_broadcastid];
	lhs.startTime = rhs[epg_starttime];

	lhs.strPlot = rhs[epg_plot].c_str();
	lhs.strPlotOutline = rhs[epg_plotoutline].c_str();
	lhs.strTitle = rhs[epg_title].c_str();

	return lhs;
}

Timer& operator<< (Timer& lhs, const PVR_TIMER& rhs) {
	lhs[timer_isrepeating] = rhs.bIsRepeating;
	lhs[timer_endtime] = rhs.endTime;
	lhs[timer_firstday] = rhs.firstDay;
	lhs[timer_channeluid] = rhs.iClientChannelUid;
	lhs[timer_index] = rhs.iClientIndex;
	lhs[timer_epguid] = rhs.iEpgUid;
	lhs[timer_lifetime] = rhs.iLifetime;
	lhs[timer_marginend] = rhs.iMarginEnd;
	lhs[timer_marginstart] = rhs.iMarginStart;
	lhs[timer_priority] = rhs.iPriority;
	lhs[timer_weekdays] = rhs.iWeekdays;
	lhs[timer_starttime] = rhs.startTime;
	lhs[timer_state] = (rhs.state == PVR_TIMER_STATE_RECORDING);
	lhs[timer_directory] = rhs.strDirectory;
	lhs[timer_summary] = rhs.strSummary;
	lhs[timer_title] = rhs.strTitle;

	return lhs;
}

PVR_TIMER& operator<< (PVR_TIMER& lhs, const Timer& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.bIsRepeating = rhs[timer_isrepeating];
	lhs.endTime = rhs[timer_endtime];
	lhs.firstDay = rhs[timer_firstday];
	lhs.iClientChannelUid = rhs[timer_channeluid];
	lhs.iClientIndex = rhs[timer_index];
	lhs.iEpgUid = rhs[timer_epguid];
	lhs.iLifetime = rhs[timer_lifetime];
	lhs.iMarginEnd = rhs[timer_marginend];
	lhs.iMarginStart = rhs[timer_marginstart];
	lhs.iPriority = rhs[timer_priority];
	lhs.iWeekdays = rhs[timer_weekdays];
	lhs.startTime = rhs[timer_starttime];
	lhs.state = (rhs[timer_state] == 1) ? PVR_TIMER_STATE_RECORDING : PVR_TIMER_STATE_COMPLETED;
	strncpy(lhs.strDirectory, rhs[timer_directory].c_str(), sizeof(lhs.strDirectory));
	strncpy(lhs.strSummary, rhs[timer_summary].c_str(), sizeof(lhs.strSummary));
	strncpy(lhs.strTitle, rhs[timer_title].c_str(), sizeof(lhs.strTitle));

	return lhs;
}

RecordingEntry& operator<< (RecordingEntry& lhs, const PVR_RECORDING& rhs) {
	lhs[recording_duration] = rhs.iDuration;
	lhs[recording_genresubtype] = rhs.iGenreSubType;
	lhs[recording_genretype] = rhs.iGenreType;
	lhs[recording_lifetime] = rhs.iLifetime;
	lhs[recording_playcount] = rhs.iPlayCount;
	lhs[recording_priority] = rhs.iPriority;
	lhs[recording_time] = rhs.recordingTime;
	lhs[recording_channelname] = rhs.strChannelName;
	lhs[recording_directory] = rhs.strDirectory;
	lhs[recording_plot] = rhs.strPlot;
	lhs[recording_plotoutline] = rhs.strPlotOutline;
	lhs[recording_id] = rhs.strRecordingId;
	lhs[recording_streamurl] = rhs.strStreamURL;
	lhs[recording_title] = rhs.strTitle;
	lhs[recording_title] = rhs.strTitle;

	return lhs;
}

PVR_RECORDING& operator<< (PVR_RECORDING& lhs, const RecordingEntry& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.iDuration = rhs[recording_duration];
	lhs.iGenreSubType = rhs[recording_genresubtype];
	lhs.iGenreType = rhs[recording_genretype];
	lhs.iLifetime = rhs[recording_lifetime];
	lhs.iPlayCount = rhs[recording_playcount];
	lhs.iPriority = rhs[recording_priority];
	lhs.recordingTime = rhs[recording_time];
	lhs.iPlayCount = rhs[recording_playcount];
	strncpy(lhs.strChannelName, rhs[recording_channelname].c_str(), sizeof(lhs.strChannelName));
	strncpy(lhs.strDirectory, rhs[recording_directory].c_str(), sizeof(lhs.strDirectory));
	strncpy(lhs.strPlot, rhs[recording_plot].c_str(), sizeof(lhs.strPlot));
	strncpy(lhs.strPlotOutline, rhs[recording_plotoutline].c_str(), sizeof(lhs.strPlotOutline));
	strncpy(lhs.strRecordingId, rhs[recording_id].c_str(), sizeof(lhs.strRecordingId));
	strncpy(lhs.strStreamURL, rhs[recording_streamurl].c_str(), sizeof(lhs.strStreamURL));
	strncpy(lhs.strTitle, rhs[recording_title].c_str(), sizeof(lhs.strTitle));

	return lhs;
}

PVR_CHANNEL_GROUP& operator<< (PVR_CHANNEL_GROUP& lhs, const ChannelGroup& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.bIsRadio = rhs[channelgroup_isradio];
	strncpy(lhs.strGroupName, rhs[channelgroup_name].c_str(), sizeof(lhs.strGroupName));

	return lhs;
}

PVR_CHANNEL_GROUP_MEMBER& operator<< (PVR_CHANNEL_GROUP_MEMBER& lhs, const ChannelGroupMember& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.iChannelNumber = rhs[channelgroupmember_number];
	lhs.iChannelUniqueId = rhs[channelgroupmember_uid];
	strncpy(lhs.strGroupName, rhs[channelgroupmember_name].c_str(), sizeof(lhs.strGroupName));

	return lhs;
}

PVR_STREAM_PROPERTIES::PVR_STREAM& operator<< (PVR_STREAM_PROPERTIES::PVR_STREAM& lhs, const Stream& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.fAspect = rhs[stream_aspect];
	lhs.iBitRate = rhs[stream_bitrate];
	lhs.iBitsPerSample = rhs[stream_bitspersample];
	lhs.iBlockAlign = rhs[stream_blockalign];
	lhs.iChannels = rhs[stream_channels];
	lhs.iCodecId = rhs[stream_codecid];
	lhs.iCodecType = rhs[stream_codectype];
	lhs.iFPSRate = rhs[stream_fpsrate];
	lhs.iFPSScale = rhs[stream_fpsscale];
	lhs.iHeight = rhs[stream_height];
	lhs.iIdentifier = rhs[stream_identifier];
	lhs.iPhysicalId = rhs[stream_physicalid];
	lhs.iSampleRate = rhs[stream_samplerate];
	lhs.iStreamIndex = rhs[stream_index];
	lhs.iWidth = rhs[stream_width];
	strncpy(lhs.strLanguage, rhs[stream_language].c_str(), sizeof(lhs.strLanguage));

	return lhs;
}

PVR_STREAM_PROPERTIES& operator<< (PVR_STREAM_PROPERTIES& lhs, const StreamProperties& rhs) {
	lhs.iStreamCount = rhs.size();

	for(StreamProperties::const_iterator i = rhs.begin(); i != rhs.end(); i++) {
	  int index = (*i).second[stream_index];
		lhs.stream[index] << (*i).second;
	}

	return lhs;
}

PVR_SIGNAL_STATUS& operator<< (PVR_SIGNAL_STATUS& lhs, const SignalStatus& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.dAudioBitrate = rhs[signal_audiobitrate];
	lhs.dDolbyBitrate = rhs[signal_dolbybitrate];
	lhs.dVideoBitrate = rhs[signal_videobitrate];
	lhs.iBER = rhs[signal_ber];
	lhs.iSNR = rhs[signal_snr];
	lhs.iSignal = rhs[signal_strength];
	lhs.iUNC = rhs[signal_unc];
	strncpy(lhs.strAdapterName, rhs[signal_adaptername].c_str(), sizeof(lhs.strAdapterName));
	strncpy(lhs.strAdapterStatus, rhs[signal_adapterstatus].c_str(), sizeof(lhs.strAdapterStatus));

	return lhs;
}
