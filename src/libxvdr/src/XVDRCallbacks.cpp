#include "XVDRCallbacks.h"

cXVDRCallbacks* cXVDRCallbacks::m_callbacks = NULL;

cXVDRCallbacks::cXVDRCallbacks()
{
}

cXVDRCallbacks::~cXVDRCallbacks()
{
}

void cXVDRCallbacks::Register(cXVDRCallbacks* callbacks)
{
  m_callbacks = callbacks;
}

cXVDRCallbacks* cXVDRCallbacks::Get()
{
  return m_callbacks;
}
