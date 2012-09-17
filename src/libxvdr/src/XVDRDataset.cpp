#include "XVDRDataset.h"

bool operator==(cXVDRStream const& lhs, cXVDRStream const& rhs) {
	return lhs.equals(rhs);
}

bool operator==(cXVDRDatasetItem const& lhs, cXVDRDatasetItem const& rhs) {
	return ((std::string)lhs == (std::string)rhs);
}

template<> void cXVDRDatasetItem::operator=(std::string v) {
	m_value = v;
}

template<> void cXVDRDatasetItem::operator=(const char* v) {
	m_value = v;
}
