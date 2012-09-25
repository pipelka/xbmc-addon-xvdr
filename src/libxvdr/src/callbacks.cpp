#include "xvdr/callbacks.h"

using namespace XVDR;

Callbacks* Callbacks::m_callbacks = NULL;

Callbacks::Callbacks()
{
}

Callbacks::~Callbacks()
{
}

void Callbacks::Register(Callbacks* callbacks)
{
  m_callbacks = callbacks;
}

Callbacks* Callbacks::Get()
{
  return m_callbacks;
}
