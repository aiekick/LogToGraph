#pragma once

#include <iostream>
#include <chrono>
#include <string>

#include <EzLibs/EzLog.hpp>
#include <curl/curl.h>

using namespace std::chrono;

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

class CurlWrapper {
public:
    bool init() {
        auto ret = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (ret != CURLE_OK) {
            LogVarError("Curl Wrapper error : %s", curl_easy_strerror(ret));
            LogVarInfo("Failing to Initialize");
            return false;
        }

        LogVarInfo("Initialized");
        return true;
    }

    void unit() {
        curl_global_cleanup();
    }

    std::string DownloadDatas(const std::string& url, std::string& vErrorCode) {
        std::string ret;
        auto p_curl = curl_easy_init();
        if (p_curl) {
            curl_easy_setopt(p_curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(p_curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(p_curl, CURLOPT_WRITEDATA, &ret);
            curl_easy_setopt(p_curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(p_curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(p_curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:89.0) Gecko/20100101 Firefox/89.0");
            auto curl_code = curl_easy_perform(p_curl);
            if (curl_code != CURLE_OK) {
                vErrorCode = curl_easy_strerror(curl_code);
                LogVarError("Yahoo brocker error : %s", curl_easy_strerror(curl_code));
            } else {
                vErrorCode.clear();
            }
            curl_easy_cleanup(p_curl);
        }
        return ret;
    }
};
