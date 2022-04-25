#include "RestDB.hpp"
#include <mars/utils/misc.h>

#include <cpr/cpr.h>
#include <iostream>
#include <iomanip>
#include <ctime>
using namespace configmaps;

#include <nlohmann/json.hpp>


namespace xrock_gui_model
{

  RestDB::RestDB()
  {
    dbAddress = "http://localhost:8183";
    dbUser = "";
    dbPassword = "";
  }

  RestDB::~RestDB()
  {
  }

  std::vector<std::pair<std::string, std::string>> RestDB::requestModelListByDomain(const std::string &domain)
  {
    std::vector<std::pair<std::string, std::string>> result;

    ConfigMap dbRequest;
    dbRequest["graph"] = "graph_production";
    dbRequest["type"] = "find";
    dbRequest["classname"] = "ComponentModel";
    ConfigMap props;
    props["domain"] = mars::utils::toupper(domain);
    dbRequest["properties"] = props;
    fprintf(stderr, "request db: %s\n", dbAddress.c_str());

    auto r = cpr::Post(cpr::Url{dbAddress},
                       cpr::Body{{dbRequest.toJsonString()}},
                       cpr::Header{{"content-type", "application/json"}});

    ConfigMap response = ConfigMap::fromJsonString(r.text);
    if (response["status"].getString() == "finished")
    {
      for (auto &model_str : response["result"])
      {
        ConfigMap model = ConfigMap::fromYamlString(model_str.toString());
        std::string type = model["type"].getString();
        // LOG("type " << type);
        std::string name = model["name"].getString();
        // LOG("name " << name);
        result.push_back(std::make_pair(name, type));
                fprintf(stderr, "modelName: %s\n", name.c_str());
      }
    }
    else
      fprintf(stderr, "response error: => %s", response["error"].getString().c_str());
    return result;
  }

  std::vector<std::string> RestDB::requestVersions(const std::string &domain, const std::string &model)
  {
    std::vector<std::string> result;
    ConfigMap dbRequest;
    dbRequest["graph"] = "graph_production";
    dbRequest["type"] = "find";
    dbRequest["classname"] = "ComponentModel";
    ConfigMap props;
    props["name"] = model;
    props["domain"] = mars::utils::toupper(domain);
    dbRequest["properties"] = props;

    auto r = cpr::Post(cpr::Url{dbAddress},
                       cpr::Body{{dbRequest.toJsonString()}},
                       cpr::Header{{"content-type", "application/json"}});
    // LOG("response => " << r.text);

    ConfigMap response = ConfigMap::fromJsonString(r.text);
    if (response["status"].getString() == "finished")
    {

      for (auto &model_str : response["result"])
      {
  
        ConfigMap model = ConfigMap::fromYamlString(model_str.toString());
        for (auto &version_yml : model["versions"])
        {
          std::string version = version_yml["version"].getString();
          std::cout << "version: " << version << std::endl;
          result.push_back(version);
        }
      }
    }
    else
      fprintf(stderr, "response error: => %s", response["error"].getString().c_str());

    return result;
  }

  ConfigMap RestDB::requestModel(const std::string &domain,
                                 const std::string &model,
                                 const std::string &version,
                                 const bool limit)
  {
    ConfigMap dbRequest;
    dbRequest["graph"] = "graph_production";
    dbRequest["type"] = "find";
    dbRequest["classname"] = "ComponentModel";
    ConfigMap props;
    props["name"] = model;
    props["domain"] = mars::utils::toupper(domain);
    if (version.size() > 0)
    {
      props["version"] = version;
    }
    dbRequest["properties"] = props;
    fprintf(stderr, "\nSTART requestModel: %s %s %s\n\n", domain.c_str(), model.c_str(), version.c_str());
    auto r = cpr::Post(cpr::Url{dbAddress},
                       cpr::Body{{dbRequest.toJsonString()}},
                       cpr::Header{{"content-type", "application/json"}});
 
     fprintf(stderr, "response: %s\n\n", r.text.c_str());
    ConfigMap response = ConfigMap::fromJsonString(r.text);
    if (response["status"].getString() == "finished")
    {

      if (!response["result"].isVector() or response["result"].size() == 0)
      {
        // fprintf(stderr, "error in database result\n");
        // fprintf(stderr, "\nEND requestModel \n\n");
  

        return ConfigMap();
      }
    }
    else
      fprintf(stderr, "response error: => %s", response["error"].getString().c_str());


    auto result = ConfigMap::fromYamlString(response["result"][0].getString());
    // fprintf(stderr, "\nEND requestModel \n\n");
    return result;
  }

  bool RestDB::storeModel(const ConfigMap &map)
  {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    fprintf(stderr, "\nSTART storeModel: \n\n");
    ConfigMap model = map;
    // here we assume that the maps fits to the json representation required by the db

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto date = oss.str();

    for (int i = 0; i < model["versions"].size(); ++i)
    {
      model["versions"][i]["date"] = date;
    }

    ConfigMap dbRequest;
    dbRequest["graph"] = "graph_production";
    dbRequest["type"] = "add";
    dbRequest["models"] = model;

    try
    {
      auto r = cpr::Post(cpr::Url{dbAddress},
                         cpr::Body{{dbRequest.toJsonString()}},
                         cpr::Header{{"content-type", "application/json"}});

      ConfigMap response = ConfigMap::fromJsonString(r.text);
      if (response["status"].getString() != "finished")
      {
        fprintf(stderr, "ERROR: %s\n\n", response["error"].getString().c_str());
        return false;
      }
    }
    catch (...)
    {
      fprintf(stderr, "ERROR: Problem with database communication\n");
      return false;
    }
    return true;
  }

  void RestDB::set_dbAddress(const std::string &_db_Address)
  {
    dbAddress = _db_Address;
  }

  void RestDB::set_dbUser(const std::string &_dbUser)
  {
    dbUser = _dbUser;
  }

  void RestDB::set_dbPassword(const std::string &_dbPassword)
  {
    dbPassword = _dbPassword;
  }

} // end of namespace xrock_gui_model
