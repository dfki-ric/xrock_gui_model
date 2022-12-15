/**
 * \file IOLibrary.hpp
 * \author Malte Langosz
 * \brief Factory interface to provide interfaces to other databases
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#include <xdbi/DbInterface.hpp>
#include "DBInterface.hpp"

namespace xrock_gui_model
{

    class XRockIOLibrary
    {

    public:
        XRockIOLibrary() {}
        ~XRockIOLibrary() {}
        virtual DBInterface* getDB(configmaps::ConfigMap &env) = 0;
        virtual std::vector<std::string> getBackends() = 0;
    };
} // end of namespace xrock_gui_model

