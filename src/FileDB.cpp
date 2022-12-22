#include "FileDB.hpp"
#include "BasicModelHelper.hpp"

#include <mars/utils/misc.h>
#include <configmaps/ConfigVector.hpp>

#include <iostream>
#include <iomanip>
#include <ctime>

using namespace configmaps;
using namespace mars::utils;

namespace xrock_gui_model
{

    FileDB::FileDB() : dbAddress("")
    {
    }

    FileDB::~FileDB()
    {
    }

    std::vector<std::pair<std::string, std::string>> FileDB::requestModelListByDomain(const std::string &domain)
    {
        std::vector<std::pair<std::string, std::string>> modelList;

        // return content of info.yml
        std::string file = "info.yml";
        handleFilenamePrefix(&file, dbAddress);
        ConfigMap info = ConfigMap::fromYamlFile(file);
        for (auto it : info["models"])
        {
            modelList.push_back(std::make_pair(it["name"], it["type"]));
        }
        return modelList;
    }

    std::vector<std::string> FileDB::requestVersions(const std::string &domain, const std::string &model)
    {
        std::vector<std::string> versionList;

        // return content of info.yml
        std::string file = "info.yml";
        handleFilenamePrefix(&file, dbAddress);
        ConfigMap info = ConfigMap::fromYamlFile(file);
        for (auto it : info["models"])
        {
            if (it["name"] == model)
            {
                for (auto it2 : it["versions"])
                {
                    versionList.push_back(it2["name"]);
                }
                break;
            }
        }
        return versionList;
    }

    ConfigMap FileDB::requestModel(const std::string &domain,
                                   const std::string &model,
                                   const std::string &version,
                                   const bool limit)
    {
        std::vector<std::string> versionList;
        if (limit)
        {
            versionList.push_back(version);
        }
        else
        {
            // get available versions
            std::string file = "info.yml";
            handleFilenamePrefix(&file, dbAddress);
            ConfigMap info = configmaps::ConfigMap::fromYamlFile(file);
            for (auto it : info["models"])
            {
                if (it["name"] == model)
                {
                    for (auto it2 : it["versions"])
                    {
                        versionList.push_back(it2["name"]);
                    }
                    break;
                }
            }
        }

        bool first = true;
        ConfigMap result;
        for (auto it : versionList)
        {
            std::string file = model + "/" + it + "/model.yml";
            handleFilenamePrefix(&file, dbAddress);
            ConfigMap map = configmaps::ConfigMap::fromYamlFile(file);
            if (first)
            {
                result = map;
                first = false;
            }
            else
            {
                result["versions"].push_back(map["versions"][0]);
            }
        }
        BasicModelHelper::convertFromLegacyModelFormat(result);
        return result;
    }

    bool FileDB::storeModel(const ConfigMap &map_)
    {
        ConfigMap map = map_;
        BasicModelHelper::convertToLegacyModelFormat(map);

        std::string model = map["name"];
        std::string type = map["type"];
        std::string version = map["versions"][0]["name"];

        // add to indexing
        std::string file = "info.yml";
        handleFilenamePrefix(&file, dbAddress);
        ConfigMap info = ConfigMap::fromYamlFile(file);
        bool foundModel = false;
        bool foundVersion = false;
        size_t i = 0;
        size_t modelIndex = 0;
        for (auto it : info["models"])
        {
            if (it["name"] == model)
            {
                foundModel = true;
                modelIndex = i;
                for (auto it2 : it["versions"])
                {
                    if (it2["name"] == version)
                    {
                        foundVersion = true;
                        break;
                    }
                }
                break;
            }
            ++i;
        }
        if (!foundModel)
        {
            ConfigMap modelMap;
            modelMap["name"] = model;
            modelMap["type"] = type;
            modelIndex = info["models"].size();
            info["models"].push_back(modelMap);
        }
        if (!foundVersion)
        {
            ConfigMap modelMap;
            modelMap["name"] = version;
            info["models"][modelIndex]["versions"].push_back(modelMap);
            info.toYamlFile(file);
        }

        std::string folder = model + "/" + version;
        handleFilenamePrefix(&folder, dbAddress);
        createDirectory(folder);
        file = folder + "/model.yml";
        map.toYamlFile(file);
        return true;
    }

    void FileDB::set_dbAddress(const std::string &db_Address)
    {
        dbAddress = db_Address;
    }

    configmaps::ConfigMap FileDB::getPropertiesOfComponentModel()
    {
        configmaps::ConfigMap propMap;
        propMap["name"]["value"] = "";
        propMap["type"]["value"] = "";
        propMap["domain"]["value"] = "";
        propMap["domain"]["allowed_values"][0] = "SOFTWARE";
        propMap["project"]["value"] = "";
        return propMap;
    }

    std::vector<std::string> FileDB::getDomains()
    {
        std::vector<std::string> domains;
        domains.push_back("SOFTWARE");
        return domains;
    }

    configmaps::ConfigMap FileDB::getEmptyComponentModel()
    {
        configmaps::ConfigMap emptyModel;
        emptyModel["name"] = "";
        emptyModel["domain"] = "SOFTWARE";
        configmaps::ConfigMap emptyVersion;
        emptyVersion["name"] = "v0.0.0";
        emptyModel["versions"].push_back(emptyVersion);
        return emptyModel;
    }

} // end of namespace xrock_gui_model
