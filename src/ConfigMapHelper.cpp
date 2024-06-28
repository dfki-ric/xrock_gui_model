#include "ConfigMapHelper.hpp"

using namespace configmaps;

namespace xrock_gui_model
{

    void ConfigMapHelper::unpackSubmodel(ConfigMap &target, const ConfigVector &source)
    {
        size_t i = 0;
        for (auto it : source)
        {
            target["submodel"][i]["name"] = it["name"];
            if (it.hasKey("data"))
            {
                try
                {
                    if (it["data"].isMap())
                        target["submodel"][i]["data"] = it["data"];
                    else
                        target["submodel"][i]["data"] = ConfigMap::fromYamlString(it["data"]);
                }
                catch (...)
                {
                    std::cerr << "ERROR: unpack submodel" << std::endl;
                }
            }
            if (it.hasKey("submodel"))
            {
                unpackSubmodel(target["submodel"][i], it["submodel"]);
            }
            ++i;
        }
    }

    void ConfigMapHelper::packSubmodel(ConfigMap &target, const ConfigVector &source)
    {
        size_t i = 0;
        for (auto it : source)
        {
            target["submodel"][i]["name"] = it["name"];
            if (it.hasKey("data"))
            {
                try
                {
                    // If target already contains a 'data' entry, we have to check if it is a string or a map and then convert the new value to string or directly assign it
                    if (target.hasKey("submodel") && target["submodel"][i].hasKey("data"))
                    {
                        // Assign data depending on type of already existing data entry
                        if (target["submodel"][i]["data"].isMap())
                            target["submodel"][i]["data"] = it["data"];
                        else
                            target["submodel"][i]["data"] = it["data"].toYamlString();
                    } else {
                        // Standard behavior: store map directly
                        target["submodel"][i]["data"] = it["data"];
                    }
                }
                catch (...)
                {
                    std::cerr << "ERROR: pack submodel" << std::endl;
                }
            }
            if (it.hasKey("submodel"))
            {
                packSubmodel(target["submodel"][i], it["submodel"]);
            }
            ++i;
        }
    }

    // todo: add vector handling and then move to configmaps itself
    configmaps::ConfigItem *ConfigMapHelper::getSubItem(configmaps::ConfigMap &map,
                                                        std::vector<std::string> path)
    {
        if (map.hasKey(path[0]))
        {
            ConfigItem *ptr = map[path[0]];
            path.erase(path.begin());
            return getSubItem(ptr, path);
        }
        return NULL;
    }

    configmaps::ConfigItem *ConfigMapHelper::getSubItem(configmaps::ConfigItem *item,
                                                        std::vector<std::string> path)
    {
        ConfigItem *ptr = item;
        for (auto key : path)
        {
            if (ptr->isMap() and ptr->hasKey(key))
            {
                ptr = (*ptr)[key];
            }
            else
            {
                std::cerr << "ERROR: accessing submap: " << key.c_str() << std::endl;
                return NULL;
            }
        }
        return ptr;
    }

} // end of namespace xrock_gui_model
