#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

// Callback: receive data chunks from libcurl
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Fetch JSON data from API
string fetch_weather_data(const string& url) {
    CURL* curl = curl_easy_init();
    CURLcode result;
    string buffer;

    if (curl) {
        // API endpoint
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Ignore SSL (for testing only)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Add header to request JSON response
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Capture response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        result = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        cout << "HTTP Response Code: " << http_code << endl;

        if (result == CURLE_OK && http_code == 200) {
            cout << "Request Successful ✅\n";
            cout << "Raw Response:\n" << buffer << endl;
        }
        else {
            cout << "Request Failed: " << curl_easy_strerror(result) << endl;
        }

        curl_easy_cleanup(curl);
    }

    return buffer;
}


// Save parsed JSON data into CSV
void save_to_csv(const json& data, const string& filename) {
    ofstream csv(filename);
    if (!csv.is_open()) {
        cerr << "Cannot open CSV file for writing.\n";
        return;
    }

    csv << "Location,Date,Morning,Afternoon,Night,Summary,SummaryWhen,MinTemp,MaxTemp\n";

    for (const auto& item : data) {
        string location = item["location"].value("location_name", "N/A");
        string date = item.value("date", "N/A");
        string morning = item.value("morning_forecast", "N/A");
        string afternoon = item.value("afternoon_forecast", "N/A");
        string night = item.value("night_forecast", "N/A");
        string summary = item.value("summary_forecast", "N/A");
        string summary_when = item.value("summary_when", "N/A");
        int min_temp = item.value("min_temp", 0);
        int max_temp = item.value("max_temp", 0);

        csv << location << ","
            << date << ","
            << morning << ","
            << afternoon << ","
            << night << ","
            << summary << ","
            << summary_when << ","
            << min_temp << ","
            << max_temp << "\n";
    }

    csv.close();
    cout << "✅ Saved CSV: " << filename << endl;
}


// Save parsed JSON data into JSON file
void save_to_json(const json& data, const string& filename) {
    json output = json::array();

    for (const auto& item : data) {
        json entry;
        entry["Location"] = item["location"].value("location_name", "N/A");
        entry["Date"] = item.value("date", "N/A");
        entry["Morning"] = item.value("morning_forecast", "N/A");
        entry["Afternoon"] = item.value("afternoon_forecast", "N/A");
        entry["Night"] = item.value("night_forecast", "N/A");
        entry["Summary"] = item.value("summary_forecast", "N/A");
        entry["SummaryWhen"] = item.value("summary_when", "N/A");
        entry["MinTemp"] = item.value("min_temp", 0);
        entry["MaxTemp"] = item.value("max_temp", 0);

        output.push_back(entry);
    }

    ofstream json_file(filename);
    if (!json_file.is_open()) {
        cerr << "❌ Cannot open JSON file for writing.\n";
        return;
    }

    json_file << setw(4) << output << endl;
    json_file.close();
    cout << "✅ Saved JSON: " << filename << endl;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    string url = "https://api.data.gov.my/weather/forecast";
    string response = fetch_weather_data(url);

    if (!response.empty()) {
        try {
            json j = json::parse(response);

            if (j.is_array()) {
                save_to_csv(j, "forecast.csv");
                save_to_json(j, "forecast.json");
            }
            else {
                cerr << "❌ Unexpected JSON structure, expected an array.\n";
            }

        }
        catch (json::parse_error& e) {
            cerr << "❌ JSON Parse Error: " << e.what() << endl;
        }
    }

    curl_global_cleanup();
    return 0;
}
