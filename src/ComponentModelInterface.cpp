/**
 * \file ComponentModelInterface.cpp
 * \author Malte Langosz
 */

#include "ComponentModelInterface.hpp"
#include "ConfigMapHelper.hpp"
#include <osg_graph_viz/Node.hpp>
#include <bagel_gui/BagelGui.hpp>
#include "XRockGUI.hpp"
#include <QMessageBox>

#include <mars/utils/misc.h>
#include <dirent.h>
#include <iostream>
using namespace bagel_gui;
using namespace configmaps;
using namespace mars::utils;

namespace xrock_gui_model
{

    ComponentModelInterface::ComponentModelInterface(BagelGui *bagelGui, XRockGUI* xrockGui) : ModelInterface(bagelGui), xrockGui(xrockGui)
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
          xrockGui(other->xrockGui),
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
                        addNodeInfo(deriveTypeFromNodeInfo(map), map);
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

    std::string ComponentModelInterface::deriveTypeFrom(const std::string& domain, const std::string& name, const std::string& version)
    {
        return (domain + "::" + name + "::" + version);
    }

    std::string ComponentModelInterface::deriveTypeFromNodeInfo(configmaps::ConfigMap &model)
    {
        return deriveTypeFrom(model["model"]["domain"].getString(), model["model"]["name"].getString(), model["model"]["versions"][0]["name"].getString());
    }

    // This function actually adds the component model information of a node into the nodeInfoMap.
    bool ComponentModelInterface::addNodeInfo(const std::string& type, configmaps::ConfigMap &model)
    {
        // Check if the type is already known. If so, do nothing
        if (nodeInfoMap.find(type) != nodeInfoMap.end())
            return false;

        // Setup all information in the NodeInfo
        osg_graph_viz::NodeInfo info;
        // It should preserve as much of the orignal model as possible, so we should actually copy everything into info in the beginning!
        info.map["model"] = model;
        // NOTE: This info is really needed such that other plugins can distinguish the maps.
        info.map["NodeClass"] = "xrock";
        info.type = type;
        // NOTE: Make the type visible in the NodeData widget
        info.map["type"] = type;

        // Transform interfaces to inputs/outputs
        // This is needed, because the bagel GUI expects these to be set properly
        int numInputs = 0;
        int numOutputs = 0;
        if (model["versions"][0].hasKey("interfaces"))
        {
            auto interfaces = model["versions"][0]["interfaces"];
            for (auto it : interfaces)
            {
                if (it.hasKey("direction"))
                {
                    const std::string& dir(it["direction"].getString());
                    if ((dir == "INCOMING") || (dir == "BIDIRECTIONAL"))
                    {
                        info.map["inputs"].push_back(it);
                        numInputs++;
                    }
                    if ((dir == "OUTGOING") || (dir == "BIDIRECTIONAL"))
                    {
                        info.map["outputs"].push_back(it);
                        numOutputs++;
                    }
                }
            }
        }
        // Set the input and output numbers in the info map
        info.numInputs = numInputs;
        info.numOutputs = numOutputs;

        // Set the default configuration as configuration starting point
        if (model["versions"][0].hasKey("defaultConfiguration"))
        {
            info.map["configuration"] = model["versions"][0]["defaultConfiguration"];
        }

        // Register the new model in the nodeInfoMap data structure
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
                map["domain"] = "SOFTWARE";
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

    // This function gets called whenever the GUI adds a new node to the canvas
    bool ComponentModelInterface::addNode(unsigned long nodeId, configmaps::ConfigMap *node)
    {
        ConfigMap &map = *node;

        // Check if the node has already been added
        if (nodeMap.find(nodeId) != nodeMap.end())
            return false;

        // TODO: Instead of these 'DES' nodes we should have a property called 'description'
        std::string nodeType = map["type"];
        if (nodeType == "DES")
            return true;
        nodeMap[nodeId] = map;
        return true;
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

        // Check if we already have added the edge
        if (edgeMap.find(edgeId) != edgeMap.end())
            return false;

        // TODO: Move this check to port compatibility function
        //if (fromDomain != toDomain)
        //{
        //    return false;
        //}
        edgeMap[edgeId] = map;
        return true;
    }

    bool ComponentModelInterface::addEdge(unsigned long edgeId,
                        const configmaps::ConfigMap &edge)
    {
        ConfigMap map = edge;
        return addEdge(edgeId, &map);
    }

    // Checks whether an edge between the same nodes and interfaces already exists
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

    // DEPRECATED
    // TODO: Check if this is a pure virtual function. If it is not, remove it completely
    bool ComponentModelInterface::groupNodes(unsigned long groupNodeId, unsigned long nodeId)
    {
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
            return true;

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

    bool ComponentModelInterface::registerComponentModel(const std::string& domain, const std::string& name, const std::string& version)
    {
        const std::string& partType(deriveTypeFrom(domain, name, version));
        if (hasNodeInfo(partType))
            return true;
        // Get map from DB. For this we need a reference to the XRockGui
        ConfigMap partModel = xrockGui->db->requestModel(domain, name, version, true);
        partModels[partType] = partModel;
        // Register the new model
        // NOTE: This function already converts the given basicModel into bagel specific stuff
        if (!addNodeInfo(partType, partModel))
            return false;
        // Once we have updated type info, we need to make the bagelGui aware of it.
        // Only then, the subsequent addNode() will work.
        bagelGui->updateNodeTypes();
        return true;
    }

    // This function gets called whenever the XRockGui has updates for the current model.
    // E.g. initially the loadComponentModel() function will pass all data to here.
    void ComponentModelInterface::setModelInfo(configmaps::ConfigMap &map)
    {
        // NOTE: basicModel holds the original data. So we just copy over.
        basicModel = map;
        //std::cout << "ComponentModelInterface::setModelInfo()\n" << basicModel.toJsonString() << "\n";
        // We now use the basic model to setup the GUI
        if (basicModel["versions"][0].hasKey("components") && basicModel["versions"][0]["components"].hasKey("nodes"))
        {
            auto nodes = basicModel["versions"][0]["components"]["nodes"];
            // At first, we have to create the nodes
            for (auto it : nodes)
            {
                const std::string& name(it["name"].getString());
                if (bagelGui->getNodeMap(name))
                    continue;
                const std::string& modelName(it["model"]["name"].getString());
                const std::string& modelDomain(it["model"]["domain"].getString());
                const std::string& modelVersion(it["model"]["version"].getString());
                // Unfortunately, the basicModel has no URI, so we have to construct a unique type id ourselves
                const std::string& partType(deriveTypeFrom(modelDomain, modelName, modelVersion));
                // Before we can add a node, we first have to check if the model is already known or
                // has to be requested from the DB first
                if (!registerComponentModel(modelDomain, modelName, modelVersion))
                {
                    std::cerr << "ComponentModelInterface::setModelInfo(): could not register " << partType << "\n";
                    continue;
                }
                // Add the part as a node in the bagelGui
                // TODO: Apply node alias if set
                bagelGui->addNode(partType, name);
                // TODO: Apply interface_aliases info
            }
            // After we have done the nodes, we can wire their interfaces together
            if (basicModel["versions"][0]["components"].hasKey("edges"))
            {
                auto edges = basicModel["versions"][0]["components"]["edges"];
                for (auto it : edges)
                {
                    ConfigMap edge(it);
                    edge["fromNode"] = it["from"]["name"];
                    edge["fromNodeOutput"] = it["from"]["interface"];
                    edge["toNode"] = it["to"]["name"];
                    edge["toNodeInput"] = it["to"]["interface"];
                    if (!edge.hasKey("name"))
                    {
                        // If no name exists, we derive a new name
                        edge["name"] = edge["fromNode"].getString()
                            + "_" + edge["fromNodeOutput"].getString()
                            + "_" + edge["toNode"].getString()
                            + "_" + edge["toNodeInput"].getString();
                    }
                    edge["smooth"] = true;
                    if (hasEdge(edge))
                        continue;
                    bagelGui->addEdge(edge);
                }
            }
            // Add configuration update to nodes and edges
            if (basicModel["versions"][0]["components"].hasKey("configuration"))
            {
                if (basicModel["versions"][0]["components"]["configuration"].hasKey("nodes"))
                {
                    auto nodeConfig = basicModel["versions"][0]["components"]["configuration"]["nodes"];
                    for (auto it : nodeConfig)
                    {
                        const std::string& nodeName(it["name"].getString());
                        ConfigMap currentMap = *bagelGui->getNodeMap(nodeName);
                        if (it.hasKey("data"))
                            currentMap["configuration"]["data"] = it["data"];
                        if (it.hasKey("submodel"))
                            currentMap["configuration"]["submodel"] = it["submodel"];
                        bagelGui->updateNodeMap(nodeName, currentMap);
                    }
                }
                // TODO: Handle edge configuration
            }
        }
        // Once we are done creating the nodes, we update their layout
        applyPartLayout(basicModel);
    }

    void ComponentModelInterface::applyPartLayout(configmaps::ConfigMap &map)
    {
        if (!map["versions"][0].hasKey("data"))
            return;
        ConfigMap& dataMap = map["versions"][0]["data"];
        if (!dataMap.hasKey("gui"))
            return;
        if (!dataMap["gui"].hasKey("layouts"))
            return;
        // Found a layout in the component model data field. Lets apply it.
        ConfigMap& layoutMap = dataMap["gui"]["layouts"];
        for (const auto &[layoutName, layoutPositions] : layoutMap)
        {
            bagelGui->applyLayout(layoutPositions);
        }
    }

    // This function gets called whenever the XRockGui wants to know the current status of the model.
    // It could be that the model has been altered by the bagelGui, so we have to perform inverse trafos here
    configmaps::ConfigMap &ComponentModelInterface::getModelInfo()
    {
        // NOTE: bagelInfo holds the data which might have been altered.
        ConfigMap mi(basicModel);
        // NOTE: The toplevel properties have already been updated at this point (see ComponentModelEditorWidget)
        // Update inner components & configuration based on nodeMap
        mi["versions"][0]["components"]["nodes"] = ConfigVector();
        mi["versions"][0]["components"]["configuration"]["nodes"] = ConfigVector();
        for (auto& [id, node] : nodeMap)
        {
            // Update node entry
            ConfigMap n;
            n["name"] = node["name"];
            n["model"]["name"] = node["model"]["name"];
            n["model"]["domain"] = node["model"]["domain"];
            n["model"]["version"] = node["model"]["versions"][0]["name"];
            mi["versions"][0]["components"]["nodes"].push_back(n);
            // Update node configuration entry
            ConfigMap c(node["configuration"]);
            c["name"] = n["name"];
            mi["versions"][0]["components"]["configuration"]["nodes"].push_back(c);
        }
        // Update edges & configuration based on edgeMap
        mi["versions"][0]["components"]["edges"] = ConfigVector();
        mi["versions"][0]["components"]["configuration"]["edges"] = ConfigVector();
        for (auto& [id, edge] : edgeMap)
        {
            ConfigMap e(edge);
            // Update edge entry
            e["from"]["name"] = edge["fromNode"];
            e["from"]["interface"] = edge["fromNodeOutput"];
            e["to"]["name"] = edge["toNode"];
            e["to"]["interface"] = edge["toNodeInput"];
            mi["versions"][0]["components"]["edges"].push_back(e);
            // TODO: Update edge configuration
        }
        // When finished, update basicModel and return it
        // NOTE: There might be leftovers of the bagel specific data which will be ignored by the xtype specific data
        //std::cout << "ComponentModelInterface::getModelInfo():\n" << mi.toJsonString() << "\n";
        basicModel = mi;
        return basicModel;
    }

    void ComponentModelInterface::resetConfig(configmaps::ConfigMap &map)
    {
        if (map["model"]["versions"][0].hasKey("defaultConfiguration"))
        {
            // The node map already contains the default configuration
            map["configuration"] = map["model"]["versions"][0]["defaultConfiguration"];
        }
    }

} // end of namespace xrock_gui_model
