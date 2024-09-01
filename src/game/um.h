#ifndef _UM_H
#define _UM_H

#include "hwn.h"

class UM: public HWN
{
private:
  const std::string gameName = "Ultraman";

public:
  const std::string& getGameName() override { return gameName; }
};

#endif
