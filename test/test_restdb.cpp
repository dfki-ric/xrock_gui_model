
#include <configmaps/ConfigMap.hpp>
#include <xdbi/Client.hpp>
#include <memory>

#include <mars/utils/misc.h>
#include <xtypes/ComponentModel.hpp>

#include <iostream>
#include <iomanip>
#include <ctime>

using namespace configmaps;


int main(int argv, char** argc) {
  std::unique_ptr<xdbi::Client> client;

  client = std::make_unique<xdbi::Client>();
  client->setDbAddress("http://localhost:8183");
  client->setDbUser("");
  client->setDbPassword("");
  client->setWorkingGraph("graph_test");
  
  std::vector<std::pair<std::string, std::string>> result = client->requestModelListByDomain("ComponentModel", mars::utils::toupper("software"));

  for(auto r: result) {
    fprintf(stderr, "%s: %s\n", r.first.c_str(), r.second.c_str());
  }
  return 0;
}

/*
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

    try {
      client->update(nl::json::parse(model.toJsonString()));
    } catch (...) {
        fprintf(stderr, "ERROR: Problem with database communication\n");
        return false;
    }
    return true;
  }

*/
