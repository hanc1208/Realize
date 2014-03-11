#pragma once
#include "stdafx.h"
#include "Macro.h"

NS_REALIZE_BEGIN

string stringf(const char* format, ...);
char* getIPFromSocket(SOCKET socket);

NS_REALIZE_END