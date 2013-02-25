/*
 *      xbmc-addon-xvdr - XVDR addon for XBMC
 *
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

#include "XBMCClient.h"
#include "XBMCAddon.h"
#include "GUIDialogChannelScanner.h"
#include "GUIDialogOk.h"
#include "GUIDialogYesNo.h"

#include <stdio.h>
#include <stdlib.h>
#include "DVDDemuxPacket.h"

using namespace ADDON;
using namespace XVDR;

#define MSG_MAXLEN 512

cXBMCClient::cXBMCClient() : XVDR::Connection(this), m_handle(NULL), m_settings(cXBMCSettings::GetInstance()), m_emptyChannelsSeen(false)
{
  m_scanner = new CGUIDialogChannelScanner(GUI, this);
}

cXBMCClient::~cXBMCClient()
{
  delete m_scanner;
}

int cXBMCClient::GetChannelsCount() {
  int count = XVDR::Connection::GetChannelsCount();

  Lock();
  bool seen = m_emptyChannelsSeen;
  Unlock();

  if(!SupportChannelScan() || count > 0 || seen) {
    return count;
  }

  Lock();
  m_emptyChannelsSeen = true;
  Unlock();

  if(CGUIDialogYesNo::ShowAndGetInput(GUI, GetLocalizedString(30008).c_str(), GetLocalizedString(30075).c_str(), GetLocalizedString(30076).c_str())) {
    DialogChannelScan();
  }

  return count;
}

void cXBMCClient::SetHandle(ADDON_HANDLE handle)
{
  m_handle = handle;
}

void cXBMCClient::OnLog(LOGLEVEL level, const char* msg)
{
  addon_log_t lt;
  switch(level)
  {
    case DEBUG:
      lt = LOG_DEBUG;
      break;
    default:
    case INFO:
      lt = LOG_INFO;
      break;
    case NOTICE:
      lt = LOG_NOTICE;
      break;
    case FAILURE:
      lt = LOG_ERROR;
      break;
  }

  XBMC->Log(lt, msg);
}

void cXBMCClient::OnNotification(LOGLEVEL level, const char* msg)
{
  queue_msg qm;
  switch(level)
  {
    default:
    case INFO:
      qm = QUEUE_INFO;
      break;
    case FAILURE:
      qm = QUEUE_ERROR;
      break;
    case WARNING:
      qm = QUEUE_WARNING;
      break;
  }

  XBMC->QueueNotification(qm, msg);
}

void cXBMCClient::Recording(const std::string& line1, const std::string& line2, bool on)
{
  PVR->Recording(line1.c_str(), line2.c_str(), on);
}

std::string cXBMCClient::GetLanguageCode()
{
  std::string code;

  char* string = XBMC->GetDVDMenuLanguage();

  if(string != NULL) {
    code = string;
    Log(DEBUG, "Possible leak caused by workaround in %s", __FUNCTION__);
#if 0
    XBMC->FreeString(string);
#endif
  }

  return code;
}

std::string cXBMCClient::GetLocalizedString(int id)
{
  std::string result;

  char* string = XBMC->GetLocalizedString(id);

  if(string != NULL) {
    result = string;
    XBMC->FreeString(string);
  }

  return result;

}

void cXBMCClient::TriggerChannelUpdate()
{
  PVR->TriggerChannelUpdate();
}

void cXBMCClient::TriggerRecordingUpdate()
{
  PVR->TriggerRecordingUpdate();
}

void cXBMCClient::TriggerTimerUpdate()
{
  PVR->TriggerTimerUpdate();
}

void cXBMCClient::TransferChannelEntry(const Channel& channel)
{
  PVR_CHANNEL pvrchannel;

  // local picons ?
  if(!m_settings.PiconPath().empty()) {
    Channel c(channel);
    c.IconPath = m_settings.PiconPath();
    if(c.IconPath[c.IconPath.length()-1] != '/') {
      c.IconPath += "/";
    }
    c.IconPath += channel.ServiceReference + ".png";
    pvrchannel << c;
  }
  else {
    pvrchannel << channel;
  }

  PVR->TransferChannelEntry(m_handle, &pvrchannel);
}

void cXBMCClient::TransferEpgEntry(const EpgItem& epg)
{
  EPG_TAG pvrepg;
  pvrepg << epg;

  PVR->TransferEpgEntry(m_handle, &pvrepg);
}

void cXBMCClient::TransferTimerEntry(const Timer& timer)
{
  PVR_TIMER pvrtimer;
  pvrtimer << timer;

  PVR->TransferTimerEntry(m_handle, &pvrtimer);
}

void cXBMCClient::TransferRecordingEntry(const RecordingEntry& rec)
{
  PVR_RECORDING pvrrec;
  pvrrec << rec;

  PVR->TransferRecordingEntry(m_handle, &pvrrec);
}

void cXBMCClient::TransferChannelGroup(const ChannelGroup& group)
{
  PVR_CHANNEL_GROUP pvrgroup;
  pvrgroup << group;

  PVR->TransferChannelGroup(m_handle, &pvrgroup);
}

void cXBMCClient::TransferChannelGroupMember(const ChannelGroupMember& member)
{
  PVR_CHANNEL_GROUP_MEMBER pvrmember;
  pvrmember << member;

  PVR->TransferChannelGroupMember(m_handle, &pvrmember);
}

Packet* cXBMCClient::AllocatePacket(int s)
{
  DemuxPacket* d = PVR->AllocateDemuxPacket(s);
  if(d == NULL)
    return NULL;

  d->iSize = s;
  return (Packet*)d;
}

void cXBMCClient::SetPacketData(Packet* packet, uint8_t* data, int streamid, uint64_t dts, uint64_t pts, uint32_t duration)
{
  if (packet == NULL)
    return;

  DemuxPacket* d = static_cast<DemuxPacket*>(packet);

  d->iStreamId = streamid;
  d->duration  = duration;
  d->dts       = (double)dts * DVD_TIME_BASE / 1000000;
  d->pts       = (double)pts * DVD_TIME_BASE / 1000000;

  if (data != NULL)
    memcpy(d->pData, data, d->iSize);
}

void cXBMCClient::FreePacket(Packet* packet)
{
  PVR->FreeDemuxPacket((DemuxPacket*)packet);
}

Packet* cXBMCClient::StreamChange(const StreamProperties& p) {
  Packet* pkt = AllocatePacket(0);
  if (pkt != NULL)
    SetPacketData(pkt, NULL, DMX_SPECIALID_STREAMCHANGE, 0, 0);

  return pkt;
}

Packet* cXBMCClient::ContentInfo(const StreamProperties& p) {
  Packet* pkt = AllocatePacket(sizeof(PVR_STREAM_PROPERTIES));

  if (pkt != NULL) {
    PVR_STREAM_PROPERTIES props;
    props << p;
    SetPacketData(pkt, (uint8_t*)&props, DMX_SPECIALID_STREAMINFO, 0, 0);
  }

  return pkt;
}

void cXBMCClient::OnDisconnect() {
  Log(FAILURE, "%s - connection lost !!!", __FUNCTION__);
  Notification(FAILURE, GetLocalizedString(30044));
}

void cXBMCClient::OnReconnect() {
  Log(INFO, "%s - connection restored", __FUNCTION__);
  Notification(INFO, GetLocalizedString(30045));

  TriggerTimerUpdate();
  TriggerRecordingUpdate();
}

void cXBMCClient::OnSignalLost() {
  Notification(FAILURE, GetLocalizedString(30047));
}

void cXBMCClient::OnSignalRestored() {
  Notification(INFO, GetLocalizedString(30048));
}

void cXBMCClient::OnChannelScannerStatus(const XVDR::ChannelScannerStatus& status) {
  XBMC->QueueNotification(QUEUE_INFO, GetLocalizedString(30038).c_str(), status.progress, status.numChannels);
}

void cXBMCClient::DialogChannelScan() {
  if(!m_scanner->LoadSetup()) {
    CGUIDialogOk::Show(GUI, GetLocalizedString(30008).c_str(), NULL, GetLocalizedString(30074).c_str());
  }
  m_scanner->DoModal();
}

PVR_CHANNEL& operator<< (PVR_CHANNEL& lhs, const Channel& rhs)
{
	memset(&lhs, 0, sizeof(lhs));

	lhs.bIsHidden = rhs.IsHidden;
	lhs.bIsRadio = rhs.IsRadio;
	lhs.iChannelNumber = rhs.Number;
	lhs.iEncryptionSystem = rhs.EncryptionSystem;
	lhs.iUniqueId = rhs.UID;
	strncpy(lhs.strChannelName, rhs.Name.c_str(), sizeof(lhs.strChannelName));
	strncpy(lhs.strIconPath, rhs.IconPath.c_str(), sizeof(lhs.strIconPath));
	lhs.strInputFormat[0] = 0;
	lhs.strStreamURL[0] = 0;

	return lhs;
}

EPG_TAG& operator<< (EPG_TAG& lhs, const EpgItem& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.endTime = rhs.EndTime;
	lhs.iChannelNumber = rhs.UID;
	lhs.iGenreSubType = rhs.GenreSubType;
	lhs.iGenreType = rhs.GenreType;
	lhs.iParentalRating = rhs.ParentalRating;
	lhs.iUniqueBroadcastId = rhs.BroadcastID;
	lhs.startTime = rhs.StartTime;

	lhs.strPlot = rhs.Plot.c_str();
	lhs.strPlotOutline = rhs.PlotOutline.c_str();
	lhs.strTitle = rhs.Title.c_str();

	return lhs;
}

Timer& operator<< (Timer& lhs, const PVR_TIMER& rhs) {
	lhs.IsRepeating = rhs.bIsRepeating;
	lhs.EndTime = rhs.endTime + rhs.iMarginEnd * 60;
	lhs.FirstDay = rhs.firstDay;
	lhs.ChannelUID = rhs.iClientChannelUid;
	lhs.Index = rhs.iClientIndex;
	lhs.EpgUID = rhs.iEpgUid;
	lhs.LifeTime = rhs.iLifetime;
	lhs.Priority = rhs.iPriority;
	lhs.WeekDays = rhs.iWeekdays;
	lhs.StartTime = rhs.startTime - rhs.iMarginStart * 60;
	lhs.Directory = rhs.strDirectory;
	lhs.Summary = rhs.strSummary;
	lhs.Title = rhs.strTitle;

	if(rhs.state == PVR_TIMER_STATE_RECORDING)
	  lhs.State = 8;
  if(rhs.state == PVR_TIMER_STATE_SCHEDULED)
    lhs.State = 1;
  if(rhs.state == PVR_TIMER_STATE_NEW)
    lhs.State = 0;

	return lhs;
}

PVR_TIMER& operator<< (PVR_TIMER& lhs, const Timer& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.bIsRepeating = rhs.IsRepeating;
	lhs.endTime = rhs.EndTime;
	lhs.firstDay = rhs.FirstDay;
	lhs.iClientChannelUid = rhs.ChannelUID;
	lhs.iClientIndex = rhs.Index;
	lhs.iEpgUid = rhs.EpgUID;
	lhs.iLifetime = rhs.LifeTime;
	lhs.iMarginEnd = 0;
	lhs.iMarginStart = 0;
	lhs.iPriority = rhs.Priority;
	lhs.iWeekdays = rhs.WeekDays;
	lhs.startTime = rhs.StartTime;

  if(rhs.State == 0)
    lhs.state = PVR_TIMER_STATE_NEW;
  if(rhs.State & 1)
    lhs.state = PVR_TIMER_STATE_SCHEDULED;
  if(rhs.State & 1024)
    lhs.state = PVR_TIMER_STATE_CONFLICT_OK;
  if(rhs.State & 2048)
    lhs.state = PVR_TIMER_STATE_CONFLICT_NOK;
  if(rhs.State & 8)
    lhs.state = PVR_TIMER_STATE_RECORDING;

	strncpy(lhs.strDirectory, rhs.Directory.c_str(), sizeof(lhs.strDirectory));
	strncpy(lhs.strSummary, rhs.Summary.c_str(), sizeof(lhs.strSummary));
	strncpy(lhs.strTitle, rhs.Title.c_str(), sizeof(lhs.strTitle));

	return lhs;
}

RecordingEntry& operator<< (RecordingEntry& lhs, const PVR_RECORDING& rhs) {
	lhs.Duration = rhs.iDuration;
	lhs.GenreSubType = rhs.iGenreSubType;
	lhs.GenreType = rhs.iGenreType;
	lhs.LifeTime = rhs.iLifetime;
	lhs.PlayCount = rhs.iPlayCount;
	lhs.Priority = rhs.iPriority;
	lhs.Time = rhs.recordingTime;
	lhs.ChannelName = rhs.strChannelName;
	lhs.Directory = rhs.strDirectory;
	lhs.Plot = rhs.strPlot;
	lhs.PlotOutline = rhs.strPlotOutline;
	lhs.Id = rhs.strRecordingId;
	lhs.Title = rhs.strTitle;

	return lhs;
}

PVR_RECORDING& operator<< (PVR_RECORDING& lhs, const RecordingEntry& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.iDuration = rhs.Duration;
	lhs.iGenreSubType = rhs.GenreSubType;
	lhs.iGenreType = rhs.GenreType;
	lhs.iLifetime = rhs.LifeTime;
	lhs.iPlayCount = rhs.PlayCount;
	lhs.iPriority = rhs.Priority;
	lhs.recordingTime = rhs.Time;
	strncpy(lhs.strChannelName, rhs.ChannelName.c_str(), sizeof(lhs.strChannelName));
	strncpy(lhs.strDirectory, rhs.Directory.c_str(), sizeof(lhs.strDirectory));
	strncpy(lhs.strPlot, rhs.Plot.c_str(), sizeof(lhs.strPlot));
	strncpy(lhs.strPlotOutline, rhs.PlotOutline.c_str(), sizeof(lhs.strPlotOutline));
	strncpy(lhs.strRecordingId, rhs.Id.c_str(), sizeof(lhs.strRecordingId));
	strncpy(lhs.strTitle, rhs.Title.c_str(), sizeof(lhs.strTitle));
	lhs.strStreamURL[0] = 0;

	return lhs;
}

PVR_CHANNEL_GROUP& operator<< (PVR_CHANNEL_GROUP& lhs, const ChannelGroup& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.bIsRadio = rhs.IsRadio;
	strncpy(lhs.strGroupName, rhs.Name.c_str(), sizeof(lhs.strGroupName));

	return lhs;
}

PVR_CHANNEL_GROUP_MEMBER& operator<< (PVR_CHANNEL_GROUP_MEMBER& lhs, const ChannelGroupMember& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.iChannelNumber = rhs.Number;
	lhs.iChannelUniqueId = rhs.UID;
	strncpy(lhs.strGroupName, rhs.Name.c_str(), sizeof(lhs.strGroupName));

	return lhs;
}

PVR_STREAM_PROPERTIES::PVR_STREAM& operator<< (PVR_STREAM_PROPERTIES::PVR_STREAM& lhs, const Stream& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.fAspect = rhs.Aspect;
	lhs.iBitRate = rhs.BitRate;
	lhs.iBitsPerSample = rhs.BitsPerSample;
	lhs.iBlockAlign = rhs.BlockAlign;
	lhs.iChannels = rhs.Channels;
	lhs.iCodecId = rhs.CodecId;
	lhs.iCodecType = rhs.CodecType;
	lhs.iFPSRate = rhs.FpsRate;
	lhs.iFPSScale = rhs.FpsScale;
	lhs.iHeight = rhs.Height;
	lhs.iIdentifier = rhs.Identifier;
	lhs.iPhysicalId = rhs.PhysicalId;
	lhs.iSampleRate = rhs.SampleRate;
	lhs.iWidth = rhs.Width;
	strncpy(lhs.strLanguage, rhs.Language.c_str(), sizeof(lhs.strLanguage));

	return lhs;
}

PVR_STREAM_PROPERTIES& operator<< (PVR_STREAM_PROPERTIES& lhs, const StreamProperties& rhs) {
	lhs.iStreamCount = rhs.size();

	for(StreamProperties::const_iterator i = rhs.begin(); i != rhs.end(); i++) {
	  int index = (*i).second.Index;
		lhs.stream[index] << (*i).second;
	}

	return lhs;
}

PVR_SIGNAL_STATUS& operator<< (PVR_SIGNAL_STATUS& lhs, const SignalStatus& rhs) {
	memset(&lhs, 0, sizeof(lhs));

	lhs.dAudioBitrate = 0;
	lhs.dDolbyBitrate = 0;
	lhs.dVideoBitrate = 0;
	lhs.iBER = rhs.BER;
  lhs.iSNR = rhs.SNR;
  lhs.iSignal = rhs.Strength;
	lhs.iUNC = rhs.UNC;
	strncpy(lhs.strAdapterName, rhs.AdapterName.c_str(), sizeof(lhs.strAdapterName));
	strncpy(lhs.strAdapterStatus, rhs.AdapterStatus.c_str(), sizeof(lhs.strAdapterStatus));

	return lhs;
}
