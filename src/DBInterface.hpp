/**
 * \file DBInterface.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with databases
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#include <xdbi/DbInterface.hpp>

namespace xrock_gui_model
{

    class DBInterface
    {

    public:
        DBInterface() {}
        virtual ~DBInterface() {}

        virtual std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain) = 0;
        virtual std::vector<std::string> requestVersions(const std::string &domain, const std::string &model) = 0;
        virtual configmaps::ConfigMap requestModel(const std::string &domain,
                                                   const std::string &model,
                                                   const std::string &version,
                                                   const bool limit = false) = 0;
        virtual bool storeModel(const configmaps::ConfigMap &map) = 0;
        virtual void set_dbGraph(const std::string &_dbGraph){};
        virtual void set_dbAddress(const std::string &_dbAddress){};
        static std::vector<std::string> loadBackends()
        {
            return xdbi::DbInterface::get_available_backends();
        }
        virtual bool isConnected() { return false; };
    };
} // end of namespace xrock_gui_model

