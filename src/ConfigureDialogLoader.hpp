/**
 * \file ConfigureDialogLoader.hpp
 * \author Malte Langosz
 * \brief Interface to allow plugin-based configuration dialogs
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#include <QDialog>

namespace xrock_gui_model
{

    class ConfigureDialogLoader
    {
    public:
        ConfigureDialogLoader(){}
        virtual ~ConfigureDialogLoader(){}
        virtual QDialog* createDialog(configmaps::ConfigMap *configuration,
                                      configmaps::ConfigMap &env,
                                      configmaps::ConfigMap &globalConfig) = 0;
    };
} // end of namespace xrock_gui_model

