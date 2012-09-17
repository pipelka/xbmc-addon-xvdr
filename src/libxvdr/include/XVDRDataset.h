#pragma once

#include <stdint.h>
#include <string>
#include <sstream>
#include <map>

class cXVDRDatasetItem {
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

	operator int() const {
		return value<int>();
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

template<> void cXVDRDatasetItem::operator=(std::string v);
template<> void cXVDRDatasetItem::operator=(const char* v);

template<class K>
class cXVDRDataset {
public:

	typedef std::map<K, cXVDRDatasetItem> maptype;

	cXVDRDatasetItem& operator[](const K& key) {
		return m_map[key];
	}

	const cXVDRDatasetItem& operator[](const K& key) const {
		typename std::map<K, cXVDRDatasetItem>::const_iterator i = m_map.find(key);
		static cXVDRDatasetItem empty;

		if(i == m_map.end()) {
			return empty;
		}

		return i->second;
	}

	bool equals(const cXVDRDataset& rhs) const {
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
} cXVDREpgKeyType;

class cXVDREpg : public cXVDRDataset<cXVDREpgKeyType> {
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
} cXVDRChannelKeyType;

class cXVDRChannel : public cXVDRDataset<cXVDRChannelKeyType> {
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
} cXVDRTimerKeyType;

class cXVDRTimer : public cXVDRDataset<cXVDRTimerKeyType> {
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
} cXVDRRecordingKeyType;

class cXVDRRecordingEntry : public cXVDRDataset<cXVDRRecordingKeyType> {
};

typedef enum {
	channelgroup_name,
	channelgroup_isradio
} cXVDRChannelGroupKeyType;

class cXVDRChannelGroup : public cXVDRDataset<cXVDRChannelGroupKeyType> {
};

typedef enum {
	channelgroupmember_name,
	channelgroupmember_uid,
	channelgroupmember_number
}  cXVDRChannelGroupMemberKeyType;

class cXVDRChannelGroupMember : public cXVDRDataset<cXVDRChannelGroupMemberKeyType> {
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
} cXVDRSignalStatusKeyType;

class cXVDRSignalStatus : public cXVDRDataset<cXVDRSignalStatusKeyType> {
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
} cXVDRStreamKeyType;

class cXVDRStream : public cXVDRDataset<cXVDRStreamKeyType> {
public:
};

bool operator==(cXVDRStream const& lhs, cXVDRStream const& rhs);
bool operator==(cXVDRDatasetItem const& lhs, cXVDRDatasetItem const& rhs);

class cXVDRStreamProperties : public std::map<uint32_t, cXVDRStream> {
};
