#pragma once
#include "ofURLFileLoader.h"
#include <json.hpp>

ofHttpResponse sendPutRequest(const std::string & url, const nlohmann::json & content, long timeout);
