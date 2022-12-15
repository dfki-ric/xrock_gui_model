/**
 * \file DBInterface.hpp
 * \author Malte Langosz
 * \brief Fuctions to communicate with databases
 **/

#pragma once
#include <configmaps/ConfigMap.hpp>
#include <xdbi/DbInterface.hpp>
#include <xtypes/ComponentModel.hpp>

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

        configmaps::ConfigMap getPropertiesOfComponentModel() {
            configmaps::ConfigMap propMap;
            const auto cm = xtypes::ComponentModel();
            const nl::json props = cm.get_properties();
            for (auto it = props.begin(); it != props.end(); ++it)
            {
                if (it->is_null())
                    continue; // skip for now..

                const std::string key = it.key();
                propMap[key]["value"] = std::string(it.value());
                const auto allowed_values = cm.get_allowed_property_values(key);
                if (allowed_values.size() > 0)
                {
                    //propMap[key]["allowed_values"] = ConfigVector();
                    size_t i = 0;
                    for (const auto &allowed : allowed_values)
                    {
                        std::string s = allowed;
                        propMap[key]["allowed_values"][i++] = s;
                    }
                }
            }
            return propMap;
        };

        std::vector<std::string> getDomains() {
            std::vector<std::string> domains;
            const auto cm = xtypes::ComponentModel();
            for (const auto &d : cm.get_allowed_property_values("domain"))
            {
                domains.push_back(d.get<std::string>());
            }
            return domains;
        }
    };
} // end of namespace xrock_gui_model

