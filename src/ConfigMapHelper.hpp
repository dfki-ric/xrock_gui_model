/**
 * \file ConfigMapHelper.hpp
 * \author Malte Langosz
 * \brief Some helper functions to operate on config maps
 **/

#ifndef XROCK_GUI_MODEL_CONFIG_MAP_HELPER_HPP
#define XROCK_GUI_MODEL_CONFIG_MAP_HELPER_HPP

#include <configmaps/ConfigData.h>

namespace xrock_gui_model {

  class ConfigMapHelper {
  public:
    ConfigMapHelper() {}
    ~ConfigMapHelper() {}

    static void packSubmodel(configmaps::ConfigMap &target, configmaps::ConfigVector &source);
    static void unpackSubmodel(configmaps::ConfigMap &target, configmaps::ConfigVector &source);
    static configmaps::ConfigItem* getSubItem(configmaps::ConfigMap &map,
                                              std::vector<std::string> path);
    static configmaps::ConfigItem* getSubItem(configmaps::ConfigItem *item,
                                              std::vector<std::string> path);

  };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_CONFIG_MAP_HELPER_HPP
