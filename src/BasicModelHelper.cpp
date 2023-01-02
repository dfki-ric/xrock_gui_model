#include "BasicModelHelper.hpp"

#include <mars/utils/misc.h>

using namespace configmaps;

namespace xrock_gui_model
{

    void BasicModelHelper::updateExportedInterfacesFromModel(ConfigMap &node, ConfigMap &model)
    {
        // exposed interfaces are stored in the input data within the bagel_gui
        // so we have to create this information from the model interfaces
        if(model["versions"][0].hasKey("interfaces"))
        {
            ConfigVector interfaces = model["versions"][0]["interfaces"];
            for(auto interface: model["versions"][0]["interfaces"])
            {
                if(interface.hasKey("linkToNode") && interface["linkToNode"] == node["name"])
                {
                    // search for interface
                    if(interface["direction"] == "INCOMING" || interface["direction"] == "BIDIRECTIONAL")
                    {
                        for(ConfigVector::iterator input = node["inputs"].begin();
                            input < node["inputs"].end(); ++input)
                        {
                            if((*input)["name"] == interface["linkToInterface"])
                            {
                                (*input)["interface"] = 1;
                                (*input)["interfaceExportName"] = interface["name"];
                            }
                        }
                    } else {
                        for(ConfigVector::iterator output = node["outputs"].begin();
                            output < node["outputs"].end(); ++output)
                        {
                            if((*output)["name"] == interface["linkToInterface"])
                            {
                                (*output)["interface"] = 1;
                                (*output)["interfaceExportName"] = interface["name"];
                            }
                        }
                    }
                }
            }
        }
    }

    // TODO: Check if this function is needed anymore. The updateExportedInterfacesToModel() function does not depend on it anymore!
    void BasicModelHelper::clearExportedInterfacesInModel(ConfigMap &model)
    {
        ConfigVector interfaces;
        if(model["versions"][0].hasKey("interfaces"))
        {
            for(auto interface: model["versions"][0]["interfaces"])
            {
                if(interface.hasKey("linkToNode"))
                {
                    continue;
                }
                interfaces.push_back(interface);
            }
        }
        model["versions"][0]["interfaces"] = interfaces;
    }

    void BasicModelHelper::updateExportedInterfacesToModel(ConfigMap &node, ConfigMap &model)
    {
        // add exported interfaces of node to interface list
        const std::string& nodeName(node["name"].getString());
        const std::string& nodeAlias(node["alias"].getString());
        for(auto port: node["inputs"])
        {
            const std::string& portName(port["name"].getString());
            const std::string& portAlias(port["alias"].getString());
            if(port.hasKey("interface")) {
                const int interfaceId = port["interface"];
                // TODO: What meaning has value 2?
                if((interfaceId == 1) || (interfaceId == 2))
                {
                    // Search for the matching external interface first
                    bool found = false;
                    for (auto& interface : model["versions"][0]["interfaces"])
                    {
                        if (interface["name"] == port["interfaceExportName"])
                        {
                            // Interface already exists. Just update alias!
                            interface["alias"] = (nodeAlias.empty() ? nodeName : nodeAlias) + std::string(":") + (portAlias.empty() ? portName : portAlias);
                            found = true;
                            break;
                        }
                    }
                    // If the interface already exists, we are done here
                    if (found) continue;

                    // The interface does not yet exist, so we create a NEW one
                    ConfigMap interface;
                    interface["domain"] = port["domain"];
                    interface["direction"] = port["direction"];
                    interface["multiplicity"] = port["multiplicity"];
                    interface["type"] = port["type"];
                    interface["linkToNode"] = nodeName;
                    interface["linkToInterface"] = portName;
                    interface["name"] = nodeName + std::string(":") + portName;
                    interface["alias"] = (nodeAlias.empty() ? nodeName : nodeAlias) + std::string(":") + (portAlias.empty() ? portName : portAlias);
                    model["versions"][0]["interfaces"].push_back(interface);
                }
                else if (interfaceId == 0)
                {
                    // In this case the external interface shall be removed. We do this by copiing every interface except for matching ones
                    ConfigVector keep;
                    for (auto interface : model["versions"][0]["interfaces"])
                    {
                        if (interface["name"] == port["interfaceExportName"])
                            continue;
                        keep.push_back(interface);
                    }
                    model["versions"][0]["interfaces"] = keep;
                }
            }
        }

        for(auto port: node["outputs"])
        {
            const std::string& portName(port["name"].getString());
            const std::string& portAlias(port["alias"].getString());
            if(port.hasKey("interface")) {
                const int interfaceId = port["interface"];
                // TODO: What meaning has value 2?
                if((interfaceId == 1) || (interfaceId == 2))
                {
                    // Search for the matching external interface first
                    bool found = false;
                    for (auto& interface : model["versions"][0]["interfaces"])
                    {
                        if (interface["name"] == port["interfaceExportName"])
                        {
                            // Interface already exists. Just update alias!
                            interface["alias"] = (nodeAlias.empty() ? nodeName : nodeAlias) + std::string(":") + (portAlias.empty() ? portName : portAlias);
                            found = true;
                            break;
                        }
                    }
                    // If the interface already exists, we are done here
                    if (found) continue;

                    // The interface does not yet exist, so we create a NEW one
                    ConfigMap interface;
                    interface["domain"] = port["domain"];
                    interface["direction"] = port["direction"];
                    interface["multiplicity"] = port["multiplicity"];
                    interface["type"] = port["type"];
                    interface["linkToNode"] = node["name"];
                    interface["linkToInterface"] = port["name"];
                    interface["name"] = nodeName + std::string(":") + portName;
                    interface["alias"] = (nodeAlias.empty() ? nodeName : nodeAlias) + std::string(":") + (portAlias.empty() ? portName : portAlias);
                    model["versions"][0]["interfaces"].push_back(interface);
                }
                else if (interfaceId == 0)
                {
                    // In this case the external interface shall be removed. We do this by copiing every interface except for matching ones
                    ConfigVector keep;
                    for (auto interface : model["versions"][0]["interfaces"])
                    {
                        if (interface["name"] == port["interfaceExportName"])
                            continue;
                        keep.push_back(interface);
                    }
                    model["versions"][0]["interfaces"] = keep;
                }
            }
        }
    }

    void BasicModelHelper::convertFromLegacyModelFormat(configmaps::ConfigMap &model)
    {
        //  - Store model information in sub-map
        model["model"] = ConfigMap(model);

        //  - Convert old domainData keys
        std::string domainData = mars::utils::tolower(model["domain"].getString()) + "Data";
        if(model["versions"][0].hasKey(domainData))
        {
            if(model["versions"][0][domainData].hasKey("data"))
            {
                if(model["versions"][0][domainData]["data"].isMap())
                {
                    model["versions"][0]["data"] = model["versions"][0][domainData]["data"];
                    ((ConfigMap)model["versions"][0]).erase(domainData);
                }
                else
                {
                    model["versions"][0]["data"] = ConfigMap::fromYamlString(model["versions"][0][domainData]["data"]);
                    ((ConfigMap)model["versions"][0]).erase(domainData);
                }
            }
        }

        //  - Check the types of annotation data in the model
        //    - check if data properties of edges are in string format and convert them to map
        if(model["versions"][0].hasKey("components"))
        {
            if(model["versions"][0]["components"].hasKey("edges"))
            {
                ConfigVector &edges = model["versions"][0]["components"]["edges"];
                for(ConfigVector::iterator edge = edges.begin(); edge != edges.end(); ++edge)
                {
                    if(edge->hasKey("data") && !(*edge)["data"].isMap())
                    {
                        (*edge)["data"] = ConfigMap::fromYamlString((*edge)["data"]);
                    }
                }
            }
        }
    }

    // ToDo:
    void BasicModelHelper::convertToLegacyModelFormat(configmaps::ConfigMap &model)
    {
        //  - Erase model information from sub-map
        model.erase("model");

        //  - Convert data maps back to strings
        std::string domainData = mars::utils::tolower(model["domain"].getString()) + "Data";
        if(model["versions"][0].hasKey("data"))
        {
            std::string dataString;
            if(model["versions"][0]["data"].isMap())
            {
                dataString = model["versions"][0]["data"].toYamlString();
            }
            else
            {
                dataString << model["versions"][0]["data"];
            }
            model["versions"][0][domainData]["data"] = dataString;
            ConfigMap &m = model["versions"][0];
            m.erase("data");
        }

        //  - Check the types of annotation data in the model
        if(model["versions"][0].hasKey("components"))
        {
            if(model["versions"][0]["components"].hasKey("edges"))
            {
                ConfigVector &edges = model["versions"][0]["components"]["edges"];
                for(ConfigVector::iterator edge = edges.begin(); edge != edges.end(); ++edge)
                {
                    if(edge->hasKey("data") && (*edge)["data"].isMap())
                    {
                        std::string dataString = (*edge)["data"].toYamlString();
                        ConfigMap &e = *edge;
                        e.erase("data");
                        e["data"] = dataString;
                    }
                }
            }
        }
    }

} // end of namespace xrock_gui_model
