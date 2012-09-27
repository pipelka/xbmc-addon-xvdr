#pragma once

#include <stdint.h>
#include <string>
#include <sstream>
#include <map>

namespace XVDR {

class DatasetItem {
public:

	bool empty() const {
		return m_value.empty();
	}

	template<class T>
	void operator=(T v) {
		std::stringstream str;
		str << v;
		str >> m_value;
	}

	operator std::string() const {
		return m_value;
	}

	operator int64_t() const {
		return value<int64_t>();
	}

	template<class T>
	T value() const {
		T v;
		GetValue(v);
		return v;
	}

	const char* c_str() const {
		return m_value.c_str();
	}

protected:

	template<class T>
	void GetValue(T& v) const {
		std::stringstream str;
		str << m_value;
		str >> v;
	}

private:

	std::string m_value;
};

template<> void DatasetItem::operator=(std::string v);
template<> void DatasetItem::operator=(const char* v);

template<class K>
class Dataset {
public:

	typedef std::map<K, DatasetItem> maptype;

	DatasetItem& operator[](const K& key) {
		return m_map[key];
	}

	const DatasetItem& operator[](const K& key) const {
		typename std::map<K, DatasetItem>::const_iterator i = m_map.find(key);
		static DatasetItem empty;

		if(i == m_map.end()) {
			return empty;
		}

		return i->second;
	}

	bool equals(const Dataset& rhs) const {
		if(m_map.size() != rhs.m_map.size()) {
			return false;
		}

		for(typename maptype::const_iterator i = m_map.begin(); i != m_map.end(); i++) {
			typename maptype::const_iterator j = rhs.m_map.find(i->first);
			if(j == rhs.m_map.end()) {
				return false;
			}
			if(i->second != j->second) {
				return false;
			}
		}

		return true;
	}

private:

	maptype m_map;
};

typedef enum {
	epg_uid,
	epg_broadcastid,
	epg_starttime,
	epg_endtime,
	epg_genretype,
	epg_genresubtype,
	epg_genredescription,
	epg_parentalrating,
	epg_title,
	epg_plotoutline,
	epg_plot
} EpgKeyType;

class Epg : public Dataset<EpgKeyType> {
};

typedef enum {
	channel_uid,
    channel_name,
    channel_number,
    channel_encryptionsystem,
    channel_isradio,
    channel_inputformat,
    channel_streamurl,
    channel_iconpath,
    channel_ishidden
} ChannelKeyType;

class Channel : public Dataset<ChannelKeyType> {
};

typedef enum {
	timer_index,
	timer_epguid,
	timer_state,
	timer_priority,
	timer_lifetime,
	timer_channeluid,
	timer_starttime,
	timer_endtime,
	timer_firstday,
	timer_weekdays,
	timer_isrepeating,
	timer_marginstart,
	timer_marginend,
	timer_title,
	timer_directory,
	timer_summary
} TimerKeyType;

class Timer : public Dataset<TimerKeyType> {
};

typedef enum {
	recording_time,
	recording_duration,
	recording_priority,
	recording_lifetime,
	recording_channelname,
	recording_title,
	recording_plotoutline,
	recording_plot,
	recording_directory,
	recording_id,
	recording_streamurl,
	recording_genretype,
	recording_genresubtype,
	recording_playcount
} RecordingKeyType;

class RecordingEntry : public Dataset<RecordingKeyType> {
};

typedef enum {
	channelgroup_name,
	channelgroup_isradio
} ChannelGroupKeyType;

class ChannelGroup : public Dataset<ChannelGroupKeyType> {
};

typedef enum {
	channelgroupmember_name,
	channelgroupmember_uid,
	channelgroupmember_number
}  ChannelGroupMemberKeyType;

class ChannelGroupMember : public Dataset<ChannelGroupMemberKeyType> {
};

typedef enum {
    signal_adaptername,
    signal_adapterstatus,
    signal_snr,
    signal_strength,
    signal_ber,
    signal_unc,
    signal_videobitrate,
    signal_audiobitrate,
    signal_dolbybitrate,
} SignalStatusKeyType;

class SignalStatus : public Dataset<SignalStatusKeyType> {
};

typedef enum {
  stream_index,
  stream_identifier,
  stream_physicalid,
  stream_codectype,
  stream_codecid,
  stream_language,
  stream_fpsscale,
  stream_fpsrate,
  stream_height,
  stream_width,
  stream_aspect,
  stream_channels,
  stream_samplerate,
  stream_blockalign,
  stream_bitrate,
  stream_bitspersample
} StreamKeyType;

class Stream : public Dataset<StreamKeyType> {
public:
};

class StreamProperties : public std::map<uint32_t, Stream> {
};

bool operator==(Stream const& lhs, Stream const& rhs);
bool operator==(DatasetItem const& lhs, DatasetItem const& rhs);

} // namespace XVDR
