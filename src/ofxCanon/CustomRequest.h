#pragma once
#include "ofURLFileLoader.h"
#include <json.hpp>

ofHttpResponse sendCustomRequest(const std::string & url, const nlohmann::json & content, long timeout, std::string requestType);
