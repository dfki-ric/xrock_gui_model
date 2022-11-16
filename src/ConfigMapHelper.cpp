#include "ConfigMapHelper.hpp"

using namespace configmaps;

namespace xrock_gui_model {

  void ConfigMapHelper::unpackSubmodel(ConfigMap &target, const ConfigVector &source) {
    size_t i = 0;
    for(auto it: source) {
      target["submodel"][i]["name"] = it["name"];
      if(it.hasKey("data")) {
        try {
          if (it["data"].isMap())
            target["submodel"][i]["data"] = it["data"];
          else
            target["submodel"][i]["data"] = ConfigMap::fromYamlString(it["data"]);
        }
        catch (...) {
          fprintf(stderr, "ERROR: unpack submodel\n");
        }
      }
      if(it.hasKey("submodel")) {
        unpackSubmodel(target["submodel"][i], it["submodel"]);
      }
      ++i;
    }
  }

  void ConfigMapHelper::packSubmodel(ConfigMap &target, const ConfigVector &source) {
    size_t i = 0;
    for(auto it: source) {
      target["submodel"][i]["name"] = it["name"];
      if(it.hasKey("data")) {
        try {
          target["submodel"][i]["data"] = it["data"].toYamlString();
        } catch(...) {
          fprintf(stderr, "ERROR: pack submodel\n");
        }
      }
      if(it.hasKey("submodel")) {
        packSubmodel(target["submodel"][i], it["submodel"]);
      }
      ++i;
    }
  }

  // todo: add vector handling and then move to configmaps itself
  configmaps::ConfigItem* ConfigMapHelper::getSubItem(configmaps::ConfigMap &map,
                                                      std::vector<std::string> path) {
    if(map.hasKey(path[0])) {
      ConfigItem *ptr = map[path[0]];
      path.erase(path.begin());
      return getSubItem(ptr, path);
    }
    return NULL;
  }

  configmaps::ConfigItem* ConfigMapHelper::getSubItem(configmaps::ConfigItem *item,
                                                      std::vector<std::string> path) {
    ConfigItem *ptr = item;
    for(auto key: path) {
      if(ptr->isMap() and ptr->hasKey(key)) {
        ptr = (*ptr)[key];
      }
      else {
        fprintf(stderr, "ERROR: accessing submap: %s", key.c_str());
        return NULL;
      }
    }
    return ptr;
  }

} // end of namespace xrock_gui_model
