/**
 * \file ConfigMapHelper.hpp
 * \author Malte Langosz
 * \brief Some helper functions to operate on config maps
 **/

#pragma once
#include <configmaps/ConfigData.h>

namespace xrock_gui_model
{

    class ConfigMapHelper
    {
    public:
        ConfigMapHelper() {}
        ~ConfigMapHelper() {}

        static void packSubmodel(configmaps::ConfigMap &target, const configmaps::ConfigVector &source);
        static void unpackSubmodel(configmaps::ConfigMap &target, const configmaps::ConfigVector &source);
        static configmaps::ConfigItem *getSubItem(configmaps::ConfigMap &map,
                                                  std::vector<std::string> path);
        static configmaps::ConfigItem *getSubItem(configmaps::ConfigItem *item,
                                                  std::vector<std::string> path);
    };
} // end of namespace xrock_gui_model

