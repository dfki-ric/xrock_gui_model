/**
 * \file BasicModelHelper.hpp
 * \author Malte Langosz
 * \brief Some helper functions to convert between different BasicModel/ComponentModel versions
 *        and BagelGUI data types.
 **/

#pragma once
#include <configmaps/ConfigData.h>

namespace xrock_gui_model
{

    class BasicModelHelper
    {
    public:
        BasicModelHelper() {}
        ~BasicModelHelper() {}

        // searches for interfaces of the node that are exporeted to the model and adds these information
        // to the node data, that it can be displayed correctly by the gui
        static void updateExportedInterfacesFromModel(configmaps::ConfigMap &node, configmaps::ConfigMap &model, bool overrideExportName);

        // remove all model interfaces that are linked to component interfaces        
        static void clearExportedInterfacesInModel(configmaps::ConfigMap &model);

        // If node ports are configured in the gui to be exposed we have to create a linked interface
        // in the model itself
        static void updateExportedInterfacesToModel(configmaps::ConfigMap &node, configmaps::ConfigMap &model);

        // Converts from the old basic model to the new representation:
        //  - Store model information in sub-map
        //  - Convert old domainData keys
        //  - Check the types of annotation data in the model
        static void convertFromLegacyModelFormat(configmaps::ConfigMap &model);

        // Reverse conversion from convertFromLegacyModelFormat
        static void convertToLegacyModelFormat(configmaps::ConfigMap &model);
    };
} // end of namespace xrock_gui_model

