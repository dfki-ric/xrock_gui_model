#include "BasicModelHelper.hpp"

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

        fprintf(stderr, "node: %s\n", node.toYamlString().c_str());
        // add exported interfaces of node to interface list
        for(auto port: node["inputs"])
        {
            if(port.hasKey("interface")) {
                if((int)port["interface"] == 1 || (int)port["interface"] == 2)
                {
                    ConfigMap interface;
                    interface["domain"] = port["domain"];
                    interface["direction"] = port["direction"];
                    interface["multiplicity"] = port["multiplicity"];
                    interface["type"] = port["type"];
                    interface["linkToNode"] = node["name"];
                    interface["linkToInterface"] = port["name"];
                    if(port.hasKey("interfaceExportName"))
                    {
                        interface["name"] = port["interfaceExportName"];
                    }
                    else {
                        interface["name"] = node["name"].getString() + std::string(":") + port["name"].getString();
                    }
                    model["versions"][0]["interfaces"].push_back(interface);
                }
            }
        }

        for(auto port: node["outputs"])
        {
            if(port.hasKey("interface")) {
                if((int)port["interface"] == 1 || (int)port["interface"] == 2)
                {
                    fprintf(stderr, "... Export interface\n");
                    ConfigMap interface;
                    interface["domain"] = port["domain"];
                    interface["direction"] = port["direction"];
                    interface["multiplicity"] = port["multiplicity"];
                    interface["type"] = port["type"];
                    interface["linkToNode"] = node["name"];
                    interface["linkToInterface"] = port["name"];
                    if(port.hasKey("interfaceExportName"))
                    {
                        interface["name"] = port["interfaceExportName"];
                    }
                    else {
                        interface["name"] = node["name"].getString() + std::string(":") + port["name"].getString();
                    }
                    model["versions"][0]["interfaces"].push_back(interface);
                }
            }
        }
    }

} // end of namespace xrock_gui_model
