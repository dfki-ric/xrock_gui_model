/**
 * \file ComponentModelInterface.cpp
 * \author Malte Langosz
 */

#include "ComponentModelInterface.hpp"
#include "ConfigMapHelper.hpp"
#include <osg_graph_viz/Node.hpp>
#include <bagel_gui/BagelGui.hpp>
#include <QMessageBox>

#include <mars/utils/misc.h>
#include <dirent.h>
#include <iostream>
using namespace bagel_gui;
using namespace configmaps;
using namespace mars::utils;

namespace xrock_gui_model
{

    ComponentModelInterface::ComponentModelInterface(BagelGui *bagelGui) : ModelInterface(bagelGui)
    {
        std::string confDir = bagelGui->getConfigDir();
        ConfigMap config = ConfigMap::fromYamlFile(confDir + "/config_default.yml", true);
        if (mars::utils::pathExists(confDir + "/config.yml"))
        {
            config.append(ConfigMap::fromYamlFile(confDir + "/config.yml", true));
        }
        // 20221110 MS: What are xrock_node_definitions?
        ConfigVector::iterator it = config["xrock_node_definitions"].begin();
        std::vector<std::string> searchPaths;
        for (; it != config["xrock_node_definitions"].end(); ++it)
        {
            std::string filename = (*it);
            if (filename[0] == '/')
            {
                searchPaths.push_back(filename);
            }
            else
            {
                searchPaths.push_back(confDir + "/" + filename);
            }
        }

        {
            std::vector<std::string>::iterator it2 = searchPaths.begin();
            for (; it2 != searchPaths.end(); ++it2)
            {
                loadNodeInfo(*it2);
            }
        }
        
        // 20221110 MS: Is this node still needed? It is a description node? Why do we need it? We can add a description property to the XType(s) instead.
        osg_graph_viz::NodeInfo info;
        info.numInputs = 0;
        info.numOutputs = 0;
        info.map["name"] = "";
        info.map["type"] = "DES";
        info.map["text"] = "";
        info.map["font_size"] = 28.;
        info.type = "DES";
        info.map["NodeClass"] = "GUINode";
        nodeInfoMap[info.type] = info;


        // 20221110 MS: This functionality is not needed and clutters this class. We use orogen_to_xrock for this.
        if (config.hasKey("OrogenFolder"))
        {
            std::string orogenFolder = confDir + "/";
            if (config.hasKey("RockRoot"))
            {
                orogenFolder << config["RockRoot"];
                if (orogenFolder.back() != '/')
                {
                    orogenFolder += "/";
                }
                if (orogenFolder[0] != '/')
                {
                    orogenFolder = confDir + "/" + orogenFolder;
                }
            }
            orogenFolder += (std::string)config["OrogenFolder"];
            loadNodeInfo(orogenFolder, true);
        }
    }

    // TODO: Check whether all of this config maps need to be copied over.
    ComponentModelInterface::ComponentModelInterface(const ComponentModelInterface *other)
        : ModelInterface(other->bagelGui),
          nodeMap(other->nodeMap),
          edgeMap(other->edgeMap),
          nodeInfoMap(other->nodeInfoMap),
          basicModel(other->basicModel)
    {
    }

    ComponentModelInterface::~ComponentModelInterface()
    {
    }

    ModelInterface *ComponentModelInterface::clone()
    {
        ComponentModelInterface *newModel = new ComponentModelInterface(this);
        return newModel;
    }

    // 20221110 MS: As far as i can see it, this stuff is needed for bagel only. It has nothing to do with XRock, right?
    void ComponentModelInterface::loadNodeInfo(std::string path, bool orogen)
    {
        if (path[path.size() - 1] != '/')
        {
            path += "/";
        }
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(path.c_str())) != NULL)
        {
            // go through all entities
            while ((ent = readdir(dir)) != NULL)
            {
                std::string file = ent->d_name;

                if (file.find(".yml", file.size() - 4, 4) != std::string::npos)
                {
                    // try to load the yaml-file
                    ConfigMap map = ConfigMap::fromYamlFile(path + file);
                    if (orogen)
                    {
                        addOrogenInfo(map);
                    }
                    else
                    {
                        addNodeInfo(map);
                    }
                }
                else if (file.find(".", 0, 1) != std::string::npos)
                {
                    // skip ".*"
                }
                else
                {
                    // go into the next dir
                    loadNodeInfo(path + file + "/", orogen);
                }
            }
            closedir(dir);
        }
        else
        {
            // this is not a directory
            std::cerr << "Specified path " << path.c_str() << " is not a valid directory" << std::endl;
        }
        return;
    }

    // This function adds a node inside the GUI? This whole function is undocumented and a mystery!
    // As far as i see it it generates/updates an nodeInfoMap entry from the original model
    // So it should be named updateInfoMap()
    bool ComponentModelInterface::addNodeInfo(ConfigMap &model, std::string version)
    {
        if (!model.hasKey("domain"))
            return false;
        // try to use the template to generate bagel node info
        osg_graph_viz::NodeInfo info;
        int numInputs = 0;
        int numOutputs = 0;
        int versionIndex = 0;
        std::string domain = model["domain"];
        //domain = tolower(domain); // 20221110 MS: Has been removed. We want the domain to be exactly the same as in the original model
        // 20221110 MS: Why is the name the type here? Why don't we use the URI to identify the node?
        std::string type = model["name"];

        if (!version.empty())
        {
            unsigned int i = 0;
            for (auto it : model["versions"])
            {
                if (it["name"] == version)
                {
                    versionIndex = i;
                    break;
                }
                ++i;
            }
            if (i >= model["versions"].size())
            {
                std::cerr << "version " << version.c_str() << "is not part of model" << type.c_str() << std::endl;
                return false;
            }
            if (nodeInfoMap.find(type) != nodeInfoMap.end())
            {
                // 20221110 MS: Why is the version appended to the 'type' ? (which is 'name::version' in the end)
                type += "::" + version;
            }
        }
        else
        {
            version << model["versions"][versionIndex]["name"];
        }

        if (nodeInfoMap.find(type) != nodeInfoMap.end())
            return false;

        ConfigMap map, tmpMap;
        {
            // 20221110 MS: This is shitty. We want 'type' to remain 'type'. Is this possible?
            if (model.hasKey("type"))
            {
                map["xrock_type"] = model["type"];
            }
            // 20221110 MS: Why is this not kept as 'version'?
            map["modelVersion"] = model["versions"][versionIndex]["name"];
            ConfigVector::iterator it = model["versions"][versionIndex]["interfaces"].begin();
            for (; it != model["versions"][versionIndex]["interfaces"].end(); ++it)
            {
                std::string iDomain = domain;
                if (it->hasKey("domain"))
                {
                    iDomain << (*it)["domain"];
                }
                if (it->hasKey("data"))
                {
                    ConfigMap dataMap;
                    if ((*it)["data"].isMap())
                    {
                        dataMap = (*it)["data"];
                    }
                    else
                    {
                        std::string dataString = (*it)["data"];
                        if (!dataString.empty())
                        {
                            dataMap = ConfigMap::fromYamlString(dataString);
                        }
                    }
                    if (dataMap.hasKey("domain"))
                    {
                        iDomain << dataMap["domain"];
                    }
                }
                if (!it->hasKey("direction") || (std::string)(*it)["direction"] == "INCOMING")
                {
                    ConfigMap interface_;
                    interface_["name"] = (*it)["name"];
                    interface_["type"] = (*it)["type"];
                    interface_["direction"] = "incoming";
                    if (!iDomain.empty())
                    {
                        interface_["domain"] = iDomain;
                    }
                    tmpMap["inputs"].push_back(interface_);
                    ++numInputs;
                }
                else if ((std::string)(*it)["direction"] == "OUTGOING")
                {
                    ConfigMap interface_;
                    interface_["name"] = (*it)["name"];
                    interface_["type"] = (*it)["type"];
                    interface_["direction"] = "outgoing";
                    if (!iDomain.empty())
                    {
                        interface_["domain"] = iDomain;
                    }
                    tmpMap["outputs"].push_back(interface_);
                    ++numOutputs;
                }
                else if ((std::string)(*it)["direction"] == "BIDIRECTIONAL")
                {
                    ConfigMap interface_;
                    interface_["name"] = (*it)["name"];
                    interface_["type"] = (*it)["type"];
                    interface_["direction"] = "bidirectional";
                    if (!iDomain.empty())
                    {
                        interface_["domain"] = iDomain;
                    }
                    map["inputs"].push_back(interface_);
                    map["outputs"].push_back(interface_);
                    ++numInputs;
                    ++numOutputs;
                }
            }
            for (auto it3 : tmpMap["inputs"])
            {
                map["inputs"].push_back(it3);
            }
            for (auto it4 : tmpMap["outputs"])
            {
                map["outputs"].push_back(it4);
            }
        }

        info.numInputs = numInputs;
        info.numOutputs = numOutputs;
        info.map = map;
        info.map["name"] = "";
        info.map["type"] = type;
        info.map["domain"] = domain;
        info.map["modelName"] = model["name"];
        if (!version.empty())
        {
            info.map["modelVersion"] = version;
        }
        if (model["versions"][versionIndex].hasKey("data"))
        {
            info.map["data"] = model["versions"][versionIndex]["data"];
            // unpack the data string for the gui
            if (!(model["versions"][versionIndex]["data"].isMap()))
            {
                info.map["data"] = ConfigMap::fromYamlString(model["versions"][versionIndex]["data"]);
            }
        }
        if (model["versions"][versionIndex].hasKey("defaultConfiguration") &&
            model["versions"][versionIndex]["defaultConfiguration"].hasKey("data"))
        {
            if (model["versions"][versionIndex]["defaultConfiguration"]["data"].isMap())
                info.map["defaultConfiguration"]["data"] = model["versions"][versionIndex]["defaultConfiguration"]["data"];
            else
                info.map["defaultConfiguration"]["data"] = ConfigMap::fromYamlString(model["versions"][versionIndex]["defaultConfiguration"]["data"]);
        }
        else if (model["versions"][versionIndex].hasKey("defaultConfig") &&
                 model["versions"][versionIndex]["defaultConfig"].hasKey("data"))
        {
            if (model["versions"][versionIndex]["defaultConfig"]["data"].isMap())
                info.map["defaultConfiguration"]["data"] = model["versions"][versionIndex]["defaultConfig"]["data"];
            else
                info.map["defaultConfiguration"]["data"] = ConfigMap::fromYamlString(model["versions"][versionIndex]["defaultConfig"]["data"]);
        }
        if (model["versions"][versionIndex].hasKey("components") &&
            model["versions"][versionIndex]["components"].hasKey("configuration") &&
            model["versions"][versionIndex]["components"]["configuration"].hasKey("nodes"))
        {
            ConfigMapHelper::unpackSubmodel(info.map["data"], model["versions"][versionIndex]["components"]["configuration"]["nodes"]);
        }
        info.type = type;
        // NOTE: This info is really needed such that other plugins can distinguish the maps.
        info.map["NodeClass"] = "xrock";
        nodeInfoMap[info.type] = info;

        return true;
    }

    // TODO: Is this function deprecated? Because we normally import orogen models from orogen_to_xrock script
    bool ComponentModelInterface::addOrogenInfo(ConfigMap &model)
    {
        // try to use the template to generate bagel node info
        osg_graph_viz::NodeInfo info;
        int numInputs = 0;
        int numOutputs = 0;
        for (auto it : model)
        {
            std::string libName = it.first;
            for (auto it2 : (ConfigMap)it.second)
            {
                std::string name = libName + "::" + it2.first;
                std::string type = "software::" + name;
                if (nodeInfoMap.find(type) != nodeInfoMap.end())
                    continue;
                ConfigMap map;
                map["modelVersion"] = "v0.1";
                map["modelName"] = name;
                map["name"] = "";
                map["domain"] = "software";
                map["NodeClass"] = "xrock";
                map["type"] = type;
                numInputs = 0;
                numOutputs = 0;
                if (it2.second.hasKey("inputPorts"))
                {
                    for (auto in : it2.second["inputPorts"])
                    {
                        map["inputs"][numInputs]["name"] = in["Name"];
                        map["inputs"][numInputs]["type"] = in["Type"];
                        map["inputs"][numInputs]["direction"] = "incoming";
                        ++numInputs;
                    }
                }
                if (it2.second.hasKey("outputPorts"))
                {
                    for (auto in : it2.second["outputPorts"])
                    {
                        map["outputs"][numOutputs]["name"] = in["Name"];
                        map["outputs"][numOutputs]["type"] = in["Type"];
                        map["outputs"][numOutputs]["direction"] = "outgoing";
                        ++numOutputs;
                    }
                }
                if (it2.second.hasKey("properties"))
                {
                    map["data"]["properties"] = it2.second["properties"];
                }
                map["data"]["framework"] = "Rock";
                info.type = type;
                info.numInputs = numInputs;
                info.numOutputs = numOutputs;
                info.map = map;
                nodeInfoMap[type] = info;
            }
        }

        return true;
    }

    // 20221110 MS: This function adds an entry into the nodeMap
    // todo: document in what cases which addNode function is used
    bool ComponentModelInterface::addNode(unsigned long nodeId, configmaps::ConfigMap *node)
    {
        ConfigMap &map = *node;
        std::string nodeType = map["type"];
        std::string nodeName = map["name"];
        if (nodeType == "DES")
            return true;
        if (nodeMap.find(nodeId) == nodeMap.end())
        {
            if (map.hasKey("defaultConfiguration"))
            {
                if (!map.hasKey("data") ||
                    !map["data"].hasKey("configuration"))
                {
                    map["data"]["configuration"] = map["defaultConfiguration"]["data"];
                }
            }
            if (map.hasKey("data") and
                map["data"].hasKey("framework"))
            {
                if (map["data"]["framework"] == "Rock")
                {
                    map["name"] = mars::utils::replaceString(nodeName, ":", "_");
                }
            }
            nodeMap[nodeId] = map;
            return true;
        }
        return false;
    }

    bool ComponentModelInterface::addNode(unsigned long nodeId,
                        const configmaps::ConfigMap &node)
    {
        ConfigMap map = node;
        return addNode(nodeId, &map);
    }

    // This function adds an entry in the edgeMap (while also checking compatibility)
    bool ComponentModelInterface::addEdge(unsigned long edgeId, configmaps::ConfigMap *edge)
    {
        ConfigMap &map = *edge;

        if (edgeMap.find(edgeId) != edgeMap.end())
            return false;

        // todo: add error handling
        std::string fromNode = map["fromNode"];
        std::string fromNodePort = map["fromNodeOutput"];
        std::string toNode = map["toNode"];
        std::string toNodePort = map["toNodeInput"];
        std::string fromDomain;
        std::string toDomain;
        ConfigMap fromNodeMap;
        for (auto node : nodeMap)
        {
            if (node.second["name"] == fromNode)
            {
                fromDomain << node.second["domain"];
                fromNodeMap = node.second;
                if (node.second["domain"] == "software")
                {
                    // todo: add framework handling
                    if (node.second["xrock_type"] == "system_modelling::task_graph::Task")
                    {
                        if (!map.hasKey("transport"))
                        {
                            map["transport"] = "CORBA";
                            map["type"] = "DATA";
                            map["size"] = "100";
                        }
                    }
                    else if (matchPattern("bagel::*", node.second["xrock_type"]))
                    {
                        if (!map.hasKey("weight"))
                        {
                            map["weight"] = "1.0";
                        }
                    }
                }
                if (fromDomain == "assembly")
                {
                    for (auto it : node.second["outputs"])
                    {
                        if (it["name"] == fromNodePort)
                        {
                            fromDomain << it["domain"];
                            // currently we can't decide wether the software interface of the assembly is from a Rock or
                            // other framework
                            if (fromDomain == "software")
                            {
                                if (!map.hasKey("transport"))
                                {
                                    map["transport"] = "CORBA";
                                    map["type"] = "DATA";
                                    map["size"] = "100";
                                }
                            }
                            break;
                        }
                    }
                }
                if (!toDomain.empty())
                    break;
            }
            if (node.second["name"] == toNode)
            {
                toDomain << node.second["domain"];
                if (toDomain == "assembly")
                {
                    for (auto it : node.second["inputs"])
                    {
                        if (it["name"] == toNodePort)
                        {
                            toDomain << it["domain"];
                            break;
                        }
                    }
                }
                if (!fromDomain.empty())
                    break;
            }
        }

        // add edge domain info
        // check wether ports are of same domain
        if (fromDomain != toDomain)
        {
            return false;
        }
        if (fromDomain == "assembly")
        {
            for (auto it : fromNodeMap["outputs"])
            {
                if ((std::string)it["name"] == fromNodePort)
                {
                    map["domain"] = it["domain"];
                    break;
                }
            }
        }
        else
        {
            map["domain"] << fromDomain;
        }
        edgeMap[edgeId] = map;
        return true;
    }

    bool ComponentModelInterface::addEdge(unsigned long edgeId,
                        const configmaps::ConfigMap &edge)
    {
        ConfigMap map = edge;
        return addEdge(edgeId, &map);
    }

    bool ComponentModelInterface::hasEdge(configmaps::ConfigMap *edge)
    {
        ConfigMap &map = *edge;

        for (auto it = edgeMap.begin(); it != edgeMap.end(); ++it)
        {
            if (it->second["fromNode"] == map["fromNode"] &&
                it->second["toNode"] == map["toNode"] &&
                it->second["fromNodeOutput"] == map["fromNodeOutput"] &&
                it->second["toNodeInput"] == map["toNodeInput"])
            {
                return true;
            }
        }
        return false;
    }

    bool ComponentModelInterface::hasEdge(const configmaps::ConfigMap &edge)
    {
        ConfigMap map = edge;
        return hasEdge(&map);
    }

    bool ComponentModelInterface::groupNodes(unsigned long groupNodeId, unsigned long nodeId)
    {
        // todo: handle deployments
        std::map<unsigned long, ConfigMap>::iterator it = nodeMap.find(groupNodeId);
        if (it != nodeMap.end())
        {
            if ((std::string)it->second["type"] == "software::Deployment")
            {
                return true;
            }
        }
        return false;
    }

    const std::map<std::string, osg_graph_viz::NodeInfo> &ComponentModelInterface::getNodeInfoMap()
    {
        return nodeInfoMap;
    }

    // This function removes a node from the nodeMap
    bool ComponentModelInterface::removeNode(unsigned long nodeId)
    {
        nodeMap.erase(nodeId);
        return true;
    }

    // This function removed an edge from the edgeMap
    bool ComponentModelInterface::removeEdge(unsigned long edgeId)
    {
        if (edgeMap.find(edgeId) == edgeMap.end())
        {
            return true;
        }

        edgeMap.erase(edgeId);
        return true;
    }

    // This function updates an existing node in the nodeMap.
    bool ComponentModelInterface::updateNode(unsigned long nodeId,
                           configmaps::ConfigMap node)
    {
        if (node["type"] == "DES")
            return true;
        std::map<unsigned long, ConfigMap>::iterator it = nodeMap.find(nodeId);
        if (it != nodeMap.end())
        {
            // todo: handle domain namespace in name
            std::string nodeName = node["name"];
            std::string domain = (std::string)node["domain"];
            it->second = node;
            return true;
        }
        return false;
    }

    bool ComponentModelInterface::hasNodeInfo(const std::string &type)
    {
        std::map<std::string, osg_graph_viz::NodeInfo>::iterator it = nodeInfoMap.begin();
        for (; it != nodeInfoMap.end(); ++it)
        {
            if (it->first == type)
            {
                return true;
            }
        }
        return false;
    }

    configmaps::ConfigMap ComponentModelInterface::getNodeInfo(const std::string &type)
    {
        std::map<std::string, osg_graph_viz::NodeInfo>::iterator it = nodeInfoMap.begin();
        for (; it != nodeInfoMap.end(); ++it)
        {
            if (it->first == type)
            {
                return it->second.map;
            }
        }
        return ConfigMap();
    }

    // This function gets called whenever the XRockGui has updates for the current model.
    // E.g. initially the loadComponentModel() function will pass all data to here.
    void ComponentModelInterface::setModelInfo(configmaps::ConfigMap &map)
    {
        // NOTE: basicModel holds the original data. So we just copy over.
        basicModel = map;
        // NOTE: bagelInfo holds the transformed version of the original data to be passed down to the bagelGui.
        ConfigMap bi(basicModel);
        // TODO: Perform transformations
        // When finished, pass the data to bagelInfo
        bagelModel = bi;
    }

    // This function gets called whenever the XRockGui wants to know the current status of the model.
    // It could be that the model has been altered by the bagelGui, so we have to perform inverse trafos here
    configmaps::ConfigMap &ComponentModelInterface::getModelInfo()
    {
        // NOTE: bagelInfo holds the data which might have been altered.
        ConfigMap mi(basicModel);
        // TODO: Peform inverse transformations to update basicModel based on (possibly altered) bagelInfo
        // When finished, update basicModel and return it
        basicModel = mi;
        return basicModel;
    }

    void ComponentModelInterface::resetConfig(configmaps::ConfigMap &map)
    {
        std::string modelName = map["modelName"];
        osg_graph_viz::NodeInfo ndi = nodeInfoMap[modelName];
        ConfigMap &model = ndi.map;

        if (map.hasKey("data"))
        {
            ConfigMap &rMap = map["data"];
            if (map["data"].hasKey("submodel"))
            {
                rMap.erase("submodel");
            }
            if (map["data"].hasKey("configuration"))
            {
                rMap.erase("configuration");
            }
        }
        if (model.hasKey("data"))
        {
            if (model["data"].hasKey("configuration"))
            {
                map["data"]["configuration"] = model["data"]["configuration"];
            }
            if (model["data"].hasKey("submodel"))
            {
                map["data"]["submodel"] = model["data"]["submodel"];
            }
        }
    }

} // end of namespace xrock_gui_model
