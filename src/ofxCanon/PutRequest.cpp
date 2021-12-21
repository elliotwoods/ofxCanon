#include "PutRequest.h"

#include <curl/curl.h>

size_t readBody_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
	auto body = (std::string*)userdata;

	if (size * nmemb < 1) {
		return 0;
	}

	if (!body->empty()) {
		auto sent = std::min(size * nmemb, body->size());
		memcpy(ptr, body->c_str(), sent);
		*body = body->substr(sent);
		return sent;
	}

	return 0;                          /* no more data left to deliver */
}

size_t saveToMemory_cb(void* buffer, size_t size, size_t nmemb, void* userdata) {
	auto response = (ofHttpResponse*)userdata;
	response->data.append((const char*)buffer, size * nmemb);
	return size * nmemb;
}

ofHttpResponse sendPutRequest(const std::string & url, const nlohmann::json & requestBody, long timeout)
{
	auto hnd = curl_easy_init();
	
	curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());
	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "PUT");

	std::string requestBodyString = requestBody.dump();
	curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, requestBodyString.data());
	curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE, requestBodyString.size());

	curl_easy_setopt(hnd, CURLOPT_TIMEOUT, timeout);

	// start request and receive response
	ofHttpResponse response;
	CURLcode err = CURLE_OK;
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, saveToMemory_cb);
	err = curl_easy_perform(hnd);
	
	if (err == CURLE_OK) {
		long http_code = 0;
		curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &http_code);
		response.status = http_code;
	}
	else {
		response.error = curl_easy_strerror(err);
		response.status = -1;
	}

	if (headers) {
		curl_slist_free_all(headers);
	}

	return response;
}