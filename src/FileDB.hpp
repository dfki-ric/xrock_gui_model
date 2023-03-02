/**
 * \file FileDB.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with the database
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#include "DBInterface.hpp"

namespace xrock_gui_model
{

    class FileDB : public DBInterface
    {

    public:
        FileDB();
        ~FileDB();

        std::vector<std::pair<std::string, std::string>> requestModelListByDomain(const std::string &domain) override;
        std::vector<std::string> requestVersions(const std::string &domain, const std::string &model) override;
        configmaps::ConfigMap requestModel(const std::string &domain,
                                           const std::string &model,
                                           const std::string &version,
                                           const bool limit = false) override;
        bool storeModel(const configmaps::ConfigMap &map_) override;

        void setDbAddress(const std::string & db_Address) override;
        virtual configmaps::ConfigMap getPropertiesOfComponentModel() override;
        virtual std::vector<std::string> getDomains() override;
        virtual configmaps::ConfigMap getEmptyComponentModel() override;

    private:
        std::string dbAddress;
    };
} // end of namespace xrock_gui_model
