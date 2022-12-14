/**
 * \file RestDB.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with the database
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#include <xdbi/Client.hpp>
#include <xtypes_generator/XTypeRegistry.hpp>
#include <memory>
#include "DBInterface.hpp"

namespace xrock_gui_model
{

    class RestDB : public DBInterface
    {

    public:
        RestDB();
        ~RestDB();

        virtual std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain) override;
        virtual std::vector<std::string> requestVersions(const std::string &domain, const std::string &model) override;
        virtual configmaps::ConfigMap requestModel(const std::string &domain,
                                                   const std::string &model,
                                                   const std::string &version,
                                                   const bool limit = false) override;
        virtual bool storeModel(const configmaps::ConfigMap &map) override;

        virtual void set_dbAddress(const std::string &_dbAddress) override;
        virtual void set_dbGraph(const std::string &_dbGraph) override;
        virtual void set_dbUser(const std::string &_dbUser);
        virtual void set_dbPassword(const std::string &_dbPassword);
        virtual bool isConnected() override;
        virtual configmaps::ConfigMap getPropertiesOfComponentModel() override;
        virtual std::vector<std::string> getDomains() override;

    private:
        std::unique_ptr<xdbi::Client> client;
        xtypes::XTypeRegistryPtr registry;
    };
} // end of namespace xrock_gui_model
