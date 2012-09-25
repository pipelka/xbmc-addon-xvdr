#include "xvdr/dataset.h"

using namespace XVDR;

bool XVDR::operator==(Stream const& lhs, Stream const& rhs) {
	return lhs.equals(rhs);
}

bool operator==(DatasetItem const& lhs, DatasetItem const& rhs) {
	return ((std::string)lhs == (std::string)rhs);
}

template<> void DatasetItem::operator=(std::string v) {
	m_value = v;
}

template<> void DatasetItem::operator=(const char* v) {
	m_value = v;
}
