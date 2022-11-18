/**
 * \file MultiDB.hpp
 * \brief Fuctions to communicate with the multi-database
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#include <xdbi/MultiDbClient.hpp>
#include <xtypes_generator/XTypeRegistry.hpp>
#include <memory>
#include "DBInterface.hpp"

namespace xrock_gui_model
{

    class MultiDB : public DBInterface
    {

    public:
        explicit MultiDB(const nl::json &config);
        ~MultiDB();

        virtual std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain) override;
        virtual std::vector<std::string> requestVersions(const std::string &domain, const std::string &model) override;
        virtual configmaps::ConfigMap requestModel(const std::string &domain,
                                                   const std::string &model,
                                                   const std::string &version,
                                                   const bool limit = false) override;
        virtual bool storeModel(const configmaps::ConfigMap &map)override;;

        virtual void set_dbGraph(const std::string &_dbGraph) override;

    private:
        std::unique_ptr<xdbi::MultiDbClient> multidb;
        xtypes::XTypeRegistryPtr registry;
    };
} // end of namespace xrock_gui_model

