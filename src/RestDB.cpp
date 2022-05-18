#include "RestDB.hpp"
#include <mars/utils/misc.h>
#include <xtypes/ComponentModel.hpp>

#include <iostream>
#include <iomanip>
#include <ctime>

using namespace configmaps;

namespace xrock_gui_model {



  RestDB::RestDB() {
    client = std::make_unique<xdbi::Client>();
    client->setDbAddress("http://localhost:8183");
    client->setDbUser("");
    client->setDbPassword("");
    client->setDefaultGraph("graph_test");
  }


  RestDB::~RestDB() {
  }

  std::vector<std::pair<std::string, std::string>> RestDB::requestModelListByDomain(const std::string& domain) {
    return client->requestModelListByDomain("ComponentModel", mars::utils::toupper(domain));
  }


  std::vector<std::string> RestDB::requestVersions(const std::string& domain, const std::string &model) {
    // ToDo as soon as modelDeepness is integrated, we can simplify this
    // request["modelDeepness"] = "versionOnly";
    return client->requestVersions("ComponentModel", mars::utils::toupper(domain), model);
  }


  ConfigMap RestDB::requestModel(const std::string &domain,
                                  const std::string &model,
                                  const std::string &version,
                                  const bool limit) {
    ConfigMap props;
    props["domain"] = mars::utils::toupper(domain);
    props["name"] = model;
    if(version.size() > 0) {
      props["version"] = version;
    }

    std::vector<XTypePtr> xtypes = client->findByJsonPropString("ComponentModel", props.toJsonString());
    if (xtypes.size() == 0) {
      std::cerr<<"ComponentModel with props: "<<props.toJsonString()<<" not loaded"<<std::endl;
      abort();
    }
    return ConfigMap::fromJsonString(std::static_pointer_cast<ComponentModel>(xtypes[0])->exportToBasicModelJSON());
  }


  bool RestDB::storeModel(const ConfigMap &map) {
    ConfigMap model = ConfigMap(map);
    // here we assume that the maps fits to the json representation required by the db
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto date = oss.str();

    for(unsigned int i = 0; i < model["versions"].size(); ++i) {
      model["versions"][(int)i]["date"] = date;
    }

    // In xdbi-proxy we set the type property of model via type2uri here
    // Before we can completely load the given component model, we have to find any part models!
    std::string serialized_model = model.toJsonString();
    std::vector<ComponentModelPtr> needed_part_models = ComponentModel::extract_part_model_info(serialized_model);
    std::vector<ComponentModelPtr> full_part_models{};
    for (const auto& part_model : needed_part_models) {
        // For proper instantation we need at least ComponentModel -> Interface -> InterfaceModel. So this means recursion depth >= 2
        std::vector<XTypePtr> full_models = client->find("ComponentModel", nl::json{{"uuid", std::to_string(part_model->uuid())}});//, limit_recursion=True, recursion_depth=3)
        for (const auto& full_model : full_models){
            full_part_models.push_back(std::static_pointer_cast<ComponentModel>(full_model));
          }
    }
    // # import from JSON and export to DB
    std::vector<ComponentModelPtr> models = ComponentModel::importFromBasicModelJSON(serialized_model, full_part_models);
    std::vector<XTypePtr> db_models{};
    for (const auto& m : models)
      db_models.push_back(std::static_pointer_cast<XType>(m));

    try {
      client->update(db_models);
    } catch (...) {
        fprintf(stderr, "ERROR: Problem with database communication\n");
        return false;
    }
    return true;
  }


  void RestDB::set_dbAddress(const std::string &_db_Address) {
    client->setDbAddress(_db_Address);
  }


  void RestDB::set_dbUser(const std::string &_dbUser) {
    client->setDbUser(_dbUser);
  }


  void RestDB::set_dbPassword(const std::string &_dbPassword) {
    client->setDbPassword(_dbPassword);
  }


  void RestDB::set_dbGraph(const std::string &_dbGraph) {
    client->setDefaultGraph(_dbGraph);
  }

} // end of namespace xrock_gui_model
