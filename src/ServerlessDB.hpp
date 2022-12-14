/**
 * \file ServerlessDB.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with the database
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#include <xdbi/Serverless.hpp>
#include <xtypes_generator/XTypeRegistry.hpp>
#include <memory>
#include "DBInterface.hpp"

namespace xrock_gui_model
{

    class ServerlessDB : public DBInterface
    {

    public:
        explicit ServerlessDB(const fs::path &db_path);
        ~ServerlessDB();

        virtual std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain) override;
        virtual std::vector<std::string> requestVersions(const std::string &domain, const std::string &model) override;
        virtual configmaps::ConfigMap requestModel(const std::string &domain,
                                                   const std::string &model,
                                                   const std::string &version,
                                                   const bool limit = false) override;
        virtual bool storeModel(const configmaps::ConfigMap &map) override;

        virtual void set_dbGraph(const std::string &_dbGraph) override;
        virtual void set_dbPath(const fs::path &_dbPath);
        virtual configmaps::ConfigMap getPropertiesOfComponentModel() override;
        virtual std::vector<std::string> getDomains() override;

    private:
        std::unique_ptr<xdbi::Serverless> serverless;
        xtypes::XTypeRegistryPtr registry;
    };
} // end of namespace xrock_gui_model

