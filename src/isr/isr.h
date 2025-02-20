#pragma once

#include <../utils/cstring_view.h>

class ISR
{
};

class ISROr : public ISR
{
public:
  cstring_view *terms;
};
