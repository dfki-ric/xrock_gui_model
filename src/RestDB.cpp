#include "RestDB.hpp"
#include <mars/utils/misc.h>

#include <cpr/cpr.h>
#include <iostream>
#include <iomanip>
#include <ctime>

using namespace configmaps;

namespace xrock_gui_model {



  RestDB::RestDB() {
    dbAddress = "http://localhost:8095";
    dbUser = "";
    dbPassword = "";
  }


  RestDB::~RestDB() {

  }

  std::vector<std::pair<std::string, std::string>> RestDB::requestModelListByDomain(const std::string &domain) {
    ConfigMap request;
    request["dbRequest2"]["id"] = 1;
    request["dbRequest2"]["username"] = "nn";
    request["dbRequest2"]["password"] = "nn";
    request["dbRequest2"]["domain"] = mars::utils::toupper(domain);
    request["dbRequest2"]["modelDeepness"] = "versionOnly";

    std::string json_string = request.toJsonString();

    //fprintf(stderr, "\nSTART requestModelListByDomain: %s\n\n", domain.c_str());
    fprintf(stderr, "request message: %s\n\n", json_string.c_str());
    fprintf(stderr, "request db: %s\n", dbAddress.c_str());
    auto r = cpr::Post(cpr::Url{dbAddress},
                       cpr::Body{{json_string}},
                       cpr::Header{{"content-type", "application/json"}});
    //fprintf(stderr, "response: %s\n\n", r.text.c_str());
    ConfigMap response = ConfigMap::fromJsonString(r.text);
    //fprintf(stderr, "%s\n", result.toYamlString().c_str());
    ConfigMap &result = response["response"];
    std::vector<std::pair<std::string, std::string>> modelList;
    if(result["results"].isVector()) {
      for(int i=0; i < result["results"].size(); ++i) {
        std::string modelName = result["results"][i]["name"];
        std::string type = result["results"][i]["type"];
        modelList.push_back(std::make_pair(modelName, type));
        fprintf(stderr, "modelName: %s\n", modelName.c_str());
      }
    }
    //fprintf(stderr, "\nEND requestModelListByDomain \n\n");
    return modelList;
  }


  std::vector<std::string> RestDB::requestVersions(const std::string &domain, const std::string &model) {
    ConfigMap request;
    request["dbRequest2"]["id"] = 1;
    request["dbRequest2"]["username"] = dbUser;
    request["dbRequest2"]["password"] = dbPassword;
    request["dbRequest2"]["domain"] = mars::utils::toupper(domain);
    request["dbRequest2"]["modelDeepness"] = "versionOnly";
    request["dbRequest2"]["name"] = model;

    std::string json_string = request.toJsonString();

    //fprintf(stderr, "\nSTART requestVersions: %s %s\n\n", domain.c_str(), model.c_str());
    //fprintf(stderr, "request message: %s\n\n", json_string.c_str());
    auto r = cpr::Post(cpr::Url{dbAddress},
                       cpr::Body{{json_string}},
                       cpr::Header{{"content-type", "application/json"}});
    //fprintf(stderr, "response: %s\n\n", r.text.c_str());
    ConfigMap response = ConfigMap::fromJsonString(r.text);
    ConfigMap &result = response["response"];

    std::vector<std::string> versionList;

    if(result["results"].isVector()) {
      if(result["results"].size() > 0) {
        for(int i=0; i < result["results"][0]["versions"].size(); ++i) {
          std::string versionName = result["results"][0]["versions"][i]["name"];
          versionList.push_back(versionName);
          //fprintf(stderr, "versionName: %s\n", versionName.c_str());
        }
      }
    }
    //fprintf(stderr, "\nEND requestVersions \n\n");
    return versionList;
  }


  ConfigMap RestDB::requestModel(const std::string &domain,
                                  const std::string &model,
                                  const std::string &version,
                                  const bool limit) {
    ConfigMap request;
    request["dbRequest2"]["id"] = 1;
    request["dbRequest2"]["username"] = dbUser;
    request["dbRequest2"]["password"] = dbPassword;
    request["dbRequest2"]["domain"] = mars::utils::toupper(domain);
    request["dbRequest2"]["name"] = model;
    request["dbRequest2"]["version"] = version;

    std::string json_string = request.toJsonString();

    fprintf(stderr, "\nSTART requestModel: %s %s %s\n\n", domain.c_str(), model.c_str(), version.c_str());
    fprintf(stderr, "request message: %s\n\n", json_string.c_str());
    auto r = cpr::Post(cpr::Url{dbAddress},
                       cpr::Body{{json_string}},
                       cpr::Header{{"content-type", "application/json"}});
    fprintf(stderr, "response: %s\n\n", r.text.c_str());

    ConfigMap response = ConfigMap::fromJsonString(r.text);
    ConfigMap &result = response["response"];

    if(!result["results"].isVector() or result["results"].size() == 0) {
      //fprintf(stderr, "error in database result\n");
      //fprintf(stderr, "\nEND requestModel \n\n");
      return ConfigMap();
    }
    //fprintf(stderr, "\nEND requestModel \n\n");
    return result["results"][0];
  }


  bool RestDB::storeModel(const ConfigMap &map) {
    fprintf(stderr, "\nSTART storeModel: \n\n");
    ConfigMap model = map;
    // here we assume that the maps fits to the json representation required by the db
    
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto date = oss.str();
    
    for(int i = 0; i < model["versions"].size(); ++i) {
      model["versions"][i]["date"] = date;
    }

    ConfigMap request;
    //request["dbInsert"]["id"] = 1;
    request["dbInsert"]["username"] = dbUser;
    request["dbInsert"]["password"] = dbPassword;
    request["dbInsert"]["overwriteAll"] = true;
    request["dbInsert"]["basicInsert"] = model;

    std::string json_string = request.toJsonString();
    //std::cout << "storeModel: " << map.toJsonString() << std::endl;
    //fprintf(stderr, "request message: %s\n\n", json_string.c_str());
    try {
      auto r = cpr::Post(cpr::Url{dbAddress},
                         cpr::Body{{json_string}},
                         cpr::Header{{"content-type", "application/json"}});

      //fprintf(stderr, "\nEND storeModel \n\n");
      ConfigMap rMap = ConfigMap::fromJsonString(r.text);
      if(rMap["error"].hasKey("errorID")) {
        fprintf(stderr, "ERROR: %s\n\n", rMap["error"]["text"].getString().c_str());
        return false;
      }
    } catch (...) {
        fprintf(stderr, "ERROR: Problem with database communication\n");
        return false;
    }
    return true;
  }


  void RestDB::set_dbAddress(const std::string &_db_Address) {
    dbAddress = _db_Address;
  }


  void RestDB::set_dbUser(const std::string &_dbUser) {
    dbUser = _dbUser;
  }


  void RestDB::set_dbPassword(const std::string &_dbPassword) {
    dbPassword = _dbPassword;
  }

} // end of namespace xrock_gui_model
