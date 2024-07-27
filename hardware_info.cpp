#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <windows.h>
#include <winhttp.h>
#include <algorithm>

#pragma comment(lib, "winhttp.lib")

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) throw std::runtime_error("_popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

std::string trim(const std::string& str) {
    const char* whitespace = " \t\n\r\f\v";
    std::string::size_type start = str.find_first_not_of(whitespace);
    std::string::size_type end = str.find_last_not_of(whitespace);

    return start == std::string::npos ? "" : str.substr(start, end - start + 1);
}

std::string json_escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default: result += c; break;
        }
    }
    return result;
}

void send_to_discord(const std::wstring& webhook_url, const std::string& json_data) {
    HINTERNET hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession) {
        URL_COMPONENTS urlComp;
        memset(&urlComp, 0, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);

        wchar_t hostName[256];
        wchar_t urlPath[256];

        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

        if (WinHttpCrackUrl(webhook_url.c_str(), 0, 0, &urlComp)) {
            HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
            if (hConnect) {
                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath,
                    NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                    (urlComp.nPort == INTERNET_DEFAULT_HTTPS_PORT) ? WINHTTP_FLAG_SECURE : 0);

                if (hRequest) {
                    std::wstring headers = L"Content-Type: application/json\r\n";
                    BOOL bResults = WinHttpSendRequest(hRequest,
                        headers.c_str(), -1, (LPVOID)json_data.c_str(), json_data.size(), json_data.size(), 0);

                    if (bResults) {
                        bResults = WinHttpReceiveResponse(hRequest, NULL);
                        if (bResults) {
                            std::cout << "Data sent to Discord webhook successfully." << std::endl;
                        }
                        else {
                            std::cerr << "WinHttpReceiveResponse failed with error: " << GetLastError() << std::endl;
                        }
                    }
                    else {
                        std::cerr << "WinHttpSendRequest failed with error: " << GetLastError() << std::endl;
                    }

                    WinHttpCloseHandle(hRequest);
                }
                else {
                    std::cerr << "WinHttpOpenRequest failed with error: " << GetLastError() << std::endl;
                }
                WinHttpCloseHandle(hConnect);
            }
            else {
                std::cerr << "WinHttpConnect failed with error: " << GetLastError() << std::endl;
            }
        }
        else {
            std::cerr << "WinHttpCrackUrl failed with error: " << GetLastError() << std::endl;
        }
        WinHttpCloseHandle(hSession);
    }
    else {
        std::cerr << "WinHttpOpen failed with error: " << GetLastError() << std::endl;
    }
}

int main() {
    // replace this with ur actual webhook url
    const std::wstring webhook_url = L"URL Here";

    std::string disk_drive = trim(exec("wmic diskdrive get model, serialnumber"));
    std::string cpu = trim(exec("wmic cpu get serialnumber"));
    std::string bios = trim(exec("wmic bios get serialnumber"));
    std::string motherboard = trim(exec("wmic baseboard get serialnumber"));
    std::string smbios_uuid = trim(exec("wmic path win32_computersystemproduct get uuid"));
    std::string mac_address = trim(exec("getmac"));

    auto clean_data = [](const std::string& data) {
        std::istringstream stream(data);
        std::string line;
        std::string result;
        while (std::getline(stream, line)) {
            if (line.empty() || line.find("Model") != std::string::npos || line.find("SerialNumber") != std::string::npos || line.find("UUID") != std::string::npos || line.find("Physical Address") != std::string::npos) {
                continue;
            }
            result += trim(line) + " ";
        }
        return trim(result);
        };

    std::string clean_disk_drive = clean_data(disk_drive);
    std::string clean_cpu = clean_data(cpu);
    std::string clean_bios = clean_data(bios);
    std::string clean_motherboard = clean_data(motherboard);
    std::string clean_smbios_uuid = clean_data(smbios_uuid);
    std::string clean_mac_address = clean_data(mac_address);

    std::ostringstream json_data;
    json_data << R"({
      "content": null,
      "embeds": [
        {
          "title": "Gathered Serials",
          "color": null,
          "fields": [
            { "name": "Disk Drive", "value": "```)" << json_escape(clean_disk_drive) << R"(```" },
            { "name": "CPU", "value": "```)" << json_escape(clean_cpu) << R"(```" },
            { "name": "BIOS", "value": "```)" << json_escape(clean_bios) << R"(```" },
            { "name": "Motherboard", "value": "```)" << json_escape(clean_motherboard) << R"(```" },
            { "name": "SMBios UUID", "value": "```)" << json_escape(clean_smbios_uuid) << R"(```" },
            { "name": "MAC Address", "value": "```)" << json_escape(clean_mac_address) << R"(```" }
          ]
        }
      ]
    })";

    send_to_discord(webhook_url, json_data.str());

    std::cout << "Data sent to Discord webhook successfully." << std::endl;

    std::cin.get();

    return 0;
}
