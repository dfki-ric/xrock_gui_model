 /**
 * \file Model.cpp
 * \author Malte Langosz
 */

#include "Model.hpp"
#include "ConfigMapHelper.hpp"
#include <osg_graph_viz/Node.hpp>
#include <bagel_gui/BagelGui.hpp>
#include <QMessageBox>

#include <mars/utils/misc.h>
#include <dirent.h>

using namespace bagel_gui;
using namespace configmaps;
using namespace mars::utils;

namespace xrock_gui_model {

  Model::Model(BagelGui *bagelGui) : ModelInterface(bagelGui) {
    std::string confDir = bagelGui->getConfigDir();
    ConfigMap config = ConfigMap::fromYamlFile(confDir+"/config_default.yml", true);
    if(mars::utils::pathExists(confDir+"/config.yml")) {
      config.append(ConfigMap::fromYamlFile(confDir+"/config.yml", true));
    }
    ConfigVector::iterator it = config["xrock_node_definitions"].begin();
    std::vector<std::string> searchPaths;
    for(; it!=config["xrock_node_definitions"].end(); ++it) {
      std::string filename = (*it);
      if(filename[0] == '/') {
        searchPaths.push_back(filename);
      }
      else {
        searchPaths.push_back(confDir+"/"+filename);
      }
    }

    {
      std::vector<std::string>::iterator it = searchPaths.begin();
      for(; it!=searchPaths.end(); ++it) {
        loadNodeInfo(*it);
      }
    }

    osg_graph_viz::NodeInfo info;
    info.numInputs = 0;
    info.numOutputs = 0;
    info.map["name"] = "";
    info.map["type"] = "DES";
    info.map["text"] = "";
    info.map["font_size"] = 28.;
    info.type = "DES";
    info.map["NodeClass"] = "GUINode";
    infoMap[info.type] = info;
    // info.map["NodeClass"] = "Rock";
    // info.map["type"] = "software::Deployment";
    // info.map["modelName"] = "Deployment";
    // info.map["domain"] = "software";
    // info.map["font_size"] = 14.;
    // info.type = "software::Deployment";
    // infoMap[info.type] = info;
    if(config.hasKey("OrogenFolder")) {
      std::string orogenFolder = confDir+"/";
      if(config.hasKey("RockRoot")) {
        orogenFolder << config["RockRoot"];
        if(orogenFolder.back() != '/') {
          orogenFolder += "/";
        }
        if(orogenFolder[0] != '/') {
          orogenFolder = confDir + "/" + orogenFolder;
        }
      }
      orogenFolder += (std::string)config["OrogenFolder"];
      loadNodeInfo(orogenFolder, true);
    }
    edition = "";
  }

  Model::Model(const Model *other) : ModelInterface(other->bagelGui) {
    infoMap = other->infoMap;
    edition = "";
  }

  Model::~Model() {
  }

  ModelInterface* Model::clone() {
    fprintf(stderr, "clone model\n");
    Model *newModel = new Model(this);
    //*newModel = *this;
    return newModel;
  }

  void Model::setEdition(const std::string v) {
    edition = tolower(v);
  }

  void Model::loadNodeInfo(std::string path, bool orogen) {
    if(path[path.size()-1] != '/') {
      path += "/";
    }
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
      // go through all entities
      while ((ent = readdir (dir)) != NULL) {
        std::string file = ent->d_name;

        if (file.find(".yml", file.size() - 4, 4) != std::string::npos) {
          // try to load the yaml-file
          //fprintf(stderr, "load: %s\n", file.c_str());
          ConfigMap map = ConfigMap::fromYamlFile(path + file);
          if(orogen) {
            addOrogenInfo(map);
          }
          else {
            addNodeInfo(map);
          }
        } else if (file.find(".", 0, 1) != std::string::npos) {
          // skip ".*"
        } else {
          // go into the next dir
          loadNodeInfo(path + file + "/", orogen);
        }
      }
      closedir (dir);
    } else {
      // this is not a directory
      fprintf(stderr, "Specified path '%s' is not a valid directory\n",
              path.c_str());
    }
    return;
  }

  bool Model::addNodeInfo(ConfigMap &model, std::string version) {
    if(!model.hasKey("domain")) return false;
    // try to use the template to generate bagel node info
    osg_graph_viz::NodeInfo info;
    int numInputs = 0;
    int numOutputs = 0;
    int versionIndex = 0;
    std::string domain = model["domain"];
    domain = tolower(domain);
    std::string type = model["name"];

    if(!version.empty()) {
      unsigned int i=0;
      for(auto it: model["versions"]) {
        if(it["name"] == version) {
          versionIndex = i;
          break;
        }
        ++i;
      }
      if(i>=model["versions"].size()) {
        fprintf(stderr, "version \"%s\" is not part of model \"%s\"\n",
                version.c_str(), type.c_str());
        return false;
      }
      if(infoMap.find(type) != infoMap.end()) {
        type += "::" + version;
      }
    }
    else {
      version << model["versions"][versionIndex]["name"];
    }

    if(infoMap.find(type) != infoMap.end()) return false;
    //fprintf(stderr, "map:\n%s\n", map.toYamlString().c_str());
    ConfigMap map, tmpMap;
    {
      if(model.hasKey("type")) {
        map["xrock_type"] = model["type"];
      }
      map["modelVersion"] = model["versions"][versionIndex]["name"];
      ConfigVector::iterator it = model["versions"][versionIndex]["interfaces"].begin();
      for(; it!=model["versions"][versionIndex]["interfaces"].end(); ++it) {
        std::string iDomain = domain;
        if(it->hasKey("domain")) {
          iDomain << (*it)["domain"];
        }
        if(it->hasKey("data")) {
          ConfigMap dataMap;
          if((*it)["data"].isMap()) {
            dataMap = (*it)["data"];
          }
          else {
            std::string dataString = (*it)["data"];
            if(!dataString.empty()) {
              fprintf(stderr, "load interface data: %s %s\n", dataString.c_str(), type.c_str());
              dataMap = ConfigMap::fromYamlString(dataString);
              fprintf(stderr, "got it\n");
            }
          }
          if(dataMap.hasKey("domain")) {
            iDomain << dataMap["domain"];
          }
        }
        if(!it->hasKey("direction") || (std::string)(*it)["direction"] == "INCOMING") {
          ConfigMap interface_;
          interface_["name"] = (*it)["name"];
          interface_["type"] = (*it)["type"];
          interface_["direction"] = "incoming";
          if(!iDomain.empty()) {
            interface_["domain"] = tolower(iDomain);
          }
          tmpMap["inputs"].push_back(interface_);
          ++numInputs;
        }
        else if((std::string)(*it)["direction"] == "OUTGOING") {
          ConfigMap interface_;
          interface_["name"] = (*it)["name"];
          interface_["type"] = (*it)["type"];
          interface_["direction"] = "outgoing";
          if(!iDomain.empty()) {
            interface_["domain"] = tolower(iDomain);
          }
          tmpMap["outputs"].push_back(interface_);
          ++numOutputs;
        }
        else if((std::string)(*it)["direction"] == "BIDIRECTIONAL") {
          ConfigMap interface_;
          interface_["name"] = (*it)["name"];
          interface_["type"] = (*it)["type"];
          interface_["direction"] = "bidirectional";
          if(!iDomain.empty()) {
            interface_["domain"] = tolower(iDomain);
          }
          map["inputs"].push_back(interface_);
          map["outputs"].push_back(interface_);
          ++numInputs;
          ++numOutputs;
        }
      }
      for(auto it: tmpMap["inputs"]) {
        map["inputs"].push_back(it);
      }
      for(auto it: tmpMap["outputs"]) {
        map["outputs"].push_back(it);
      }
    }

    info.numInputs = numInputs;
    info.numOutputs = numOutputs;
    info.map = map;
    info.map["name"] = "";
    info.map["type"] = type;
    info.map["domain"] = domain;
    info.map["modelName"] = model["name"];
    if(!version.empty()) {
      info.map["modelVersion"] = version;
    }
    if(model["versions"][versionIndex].hasKey(domain+"Data")) {
      if(model["versions"][versionIndex][domain+"Data"].hasKey("data")) {
        info.map[domain+"Data"]= model["versions"][versionIndex][domain+"Data"];
        // unpack the data string for the gui
        info.map[domain+"Data"]["data"] = ConfigMap::fromYamlString(model["versions"][versionIndex][domain+"Data"]["data"]);
      }
    }
    if(model["versions"][versionIndex].hasKey("defaultConfiguration") &&
       model["versions"][versionIndex]["defaultConfiguration"].hasKey("data")) {
      info.map["defaultConfiguration"] = ConfigMap::fromYamlString(model["versions"][versionIndex]["defaultConfiguration"]["data"]);
    }
    else if(model["versions"][versionIndex].hasKey("defaultConfig") &&
       model["versions"][versionIndex]["defaultConfig"].hasKey("data")) {
      info.map["defaultConfiguration"] = ConfigMap::fromYamlString(model["versions"][versionIndex]["defaultConfig"]["data"]);
    }
    if(model["versions"][versionIndex].hasKey("components") &&
       model["versions"][versionIndex]["components"].hasKey("configuration") &&
       model["versions"][versionIndex]["components"]["configuration"].hasKey("nodes")) {
      ConfigMapHelper::unpackSubmodel(info.map[domain+"Data"]["data"], model["versions"][versionIndex]["components"]["configuration"]["nodes"]);
    }
    info.type = type;
    info.map["NodeClass"] = "xrock";
    infoMap[info.type] = info;

    return true;
  }

  bool Model::addOrogenInfo(ConfigMap &model) {
    // try to use the template to generate bagel node info
    osg_graph_viz::NodeInfo info;
    int numInputs = 0;
    int numOutputs = 0;
    for(auto it: model) {
      std::string libName = it.first;
      for(auto it2: (ConfigMap)it.second) {
        std::string name = libName + "::" + it2.first;
        std::string type = "software::" + name;
        if(infoMap.find(type) != infoMap.end()) continue;
        ConfigMap map;
        map["modelVersion"] = "v0.1";
        map["modelName"] = name;
        map["name"] = "";
        map["domain"] = "software";
        map["NodeClass"] = "xrock";
        map["type"] = type;
        numInputs = 0;
        numOutputs = 0;
        if(it2.second.hasKey("inputPorts")) {
          for(auto in: it2.second["inputPorts"]) {
            map["inputs"][numInputs]["name"] = in["Name"];
            map["inputs"][numInputs]["type"] = in["Type"];
            map["inputs"][numInputs]["direction"] = "incoming";
            ++numInputs;
          }
        }
        if(it2.second.hasKey("outputPorts")) {
          for(auto in: it2.second["outputPorts"]) {
            map["outputs"][numOutputs]["name"] = in["Name"];
            map["outputs"][numOutputs]["type"] = in["Type"];
            map["outputs"][numOutputs]["direction"] = "outgoing";
            ++numOutputs;
          }
        }
        if(it2.second.hasKey("properties")) {
          map["softwareData"]["data"]["properties"] = it2.second["properties"];
        }
        map["softwareData"]["data"]["framework"] = "Rock";
        info.type = type;
        info.numInputs = numInputs;
        info.numOutputs = numOutputs;
        info.map = map;
        infoMap[type] = info;
      }
    }

    return true;
  }

  // todo: document in what cases which addNode function is used
  bool Model::addNode(unsigned long nodeId, configmaps::ConfigMap *node) {
    ConfigMap &map = *node;
    std::string nodeType = map["type"];
    std::string nodeName = map["name"];
    if(nodeType == "DES") return true;
    if(nodeMap.find(nodeId) == nodeMap.end()) {
      if(map.hasKey("defaultConfiguration")) {
        std::string domainData = map["domain"];
        domainData += "Data";
        if(!map.hasKey(domainData) ||
           !map[domainData].hasKey("data") ||
           !map[domainData]["data"].hasKey("configuration")) {
          map[domainData]["data"]["configuration"] = map["defaultConfiguration"];
        }
      }
      nodeMap[nodeId] = map;
      return true;
    }
    return false;
  }

  bool Model::addNode(unsigned long nodeId,
                      const configmaps::ConfigMap &node) {
    ConfigMap map = node;
    return addNode(nodeId, &map);
  }

  bool Model::addEdge(unsigned long edgeId, configmaps::ConfigMap *edge) {
    ConfigMap &map = *edge;

    if(edgeMap.find(edgeId) != edgeMap.end()) return false;

    // todo: add error handling
    std::string fromNode = map["fromNode"];
    std::string fromNodePort = map["fromNodeOutput"];
    std::string toNode = map["toNode"];
    std::string toNodePort = map["toNodeInput"];
    std::string fromDomain;
    std::string toDomain;
    ConfigMap fromNodeMap;
    for(auto node: nodeMap) {
      if(node.second["name"] == fromNode) {
        fromDomain << node.second["domain"];
        fromNodeMap = node.second;
        if(node.second["domain"] == "software") {
          // todo: add framework handling
          if(node.second["xrock_type"] == "system_modelling::task_graph::Task") {
            if(!map.hasKey("transport")) {
              map["transport"] = "CORBA";
              map["type"] = "DATA";
              map["size"] = "100";
            }
          }
          else if(matchPattern("bagel::*", node.second["xrock_type"])) {
            if(!map.hasKey("weight")) {
              map["weight"] = "1.0";
            }
          }
        }
        if(fromDomain == "assembly") {
          for(auto it: node.second["outputs"]) {
            if(it["name"] == fromNodePort) {
              fromDomain << it["domain"];
              // currently we can't decide wether the software interface of the assembly is from a Rock or
              // other framework
              if(fromDomain == "software") {
                if(!map.hasKey("transport")) {
                  map["transport"] = "CORBA";
                  map["type"] = "DATA";
                  map["size"] = "100";
                }
              }
              break;
            }
          }
        }
        if(!toDomain.empty()) break;
      }
      if(node.second["name"] == toNode) {
        toDomain << node.second["domain"];
        if(toDomain == "assembly") {
          for(auto it: node.second["inputs"]) {
            if(it["name"] == toNodePort) {
              toDomain << it["domain"];
              break;
            }
          }
        }
        if(!fromDomain.empty()) break;
      }
    }

    // add edge domain info
    // check wether ports are of same domain
    if(fromDomain != toDomain) {
      return false;
    }
    if(fromDomain == "assembly") {
      for(auto it: fromNodeMap["outputs"]) {
        if((std::string)it["name"] == fromNodePort) {
          map["domain"] = it["domain"];
          break;
        }
      }
    }
    else {
      map["domain"] << fromDomain;
    }
    edgeMap[edgeId] = map;
    return true;
  }

  bool Model::addEdge(unsigned long edgeId,
                          const configmaps::ConfigMap &edge) {
    ConfigMap map = edge;
    return addEdge(edgeId, &map);
  }

  bool Model::hasEdge(configmaps::ConfigMap *edge) {
    ConfigMap &map = *edge;

    for (auto it=edgeMap.begin(); it!=edgeMap.end(); ++it) {
      if (it->second["fromNode"]       == map["fromNode"]       &&
          it->second["toNode"]         == map["toNode"]         &&
          it->second["fromNodeOutput"] == map["fromNodeOutput"] &&
          it->second["toNodeInput"]    == map["toNodeInput"]
         )
      {
        return true;
      }
    }
    return false;
  }

  bool Model::hasEdge(const configmaps::ConfigMap &edge) {
    ConfigMap map = edge;
    return hasEdge(&map);
  }

  bool Model::groupNodes(unsigned long groupNodeId, unsigned long nodeId) {
    // todo: handle deployments
    std::map<unsigned long, ConfigMap>::iterator it = nodeMap.find(groupNodeId);
    if(it != nodeMap.end()) {
      if((std::string)it->second["type"] == "software::Deployment") {
        return true;
      }
    }
    return false;
  }

  const std::map<std::string, osg_graph_viz::NodeInfo>& Model::getNodeInfoMap() {
    return infoMap;
  }

  bool Model::removeNode(unsigned long nodeId) {
    if(!edition.empty()) {
      ConfigMap node = nodeMap[nodeId];
      std::string nodeDomain = tolower((std::string)node["domain"]);
      std::string nodeName = node["name"];
      if(nodeDomain != edition) {
        if(nodeDomain != "assembly") {
          return false;
        }
        else {
          // check if there are no edges in other domains
          for(auto it: edgeMap) {
            if((std::string)it.second["fromNode"] == nodeName) {
              if(tolower((std::string)it.second["domain"]) != edition) {
                return false;
              }
            }
            if((std::string)it.second["toNode"] == nodeName) {
              if(tolower((std::string)it.second["domain"]) != edition) {
                return false;
              }
            }
          }
        }
      }
    }

    nodeMap.erase(nodeId);
    return true;
  }

  bool Model::removeEdge(unsigned long edgeId) {
    if(edgeMap.find(edgeId) == edgeMap.end()) {
      return true;
    }

    ConfigMap &edge = edgeMap[edgeId];
    if(!edition.empty() && tolower((std::string)edge["domain"]) != edition) {
      return false;
    }

    edgeMap.erase(edgeId);
    return true;
  }

  bool Model::updateNode(unsigned long nodeId,
                         configmaps::ConfigMap node) {
    if(node["type"] == "DES") return true;
    std::map<unsigned long, ConfigMap>::iterator it = nodeMap.find(nodeId);
    if(it != nodeMap.end()) {
      // todo: handle domain namespace in name
      std::string nodeName = node["name"];
      std::string domain = tolower((std::string)node["domain"]);
      if(!edition.empty()) {
        if(edition != domain && nodeName != it->second["name"].getString()) {
          fprintf(stderr, "ERROR: change node name of non %s nodes is not allowed!", edition.c_str());
          return false;
        }
      }

      //if(domain.empty()) return false;
      //domain += "::";
      //if(nodeName.find(domain) != 0) return false;
      it->second = node;
      return true;
    }
    return false;
  }

  bool Model::hasNodeInfo(const std::string &type) {
    std::map<std::string, osg_graph_viz::NodeInfo>::iterator it = infoMap.begin();
    for(; it!=infoMap.end(); ++it) {
      if(it->first == type) {
        return true;
      }
    }
    return false;
  }

  configmaps::ConfigMap Model::getNodeInfo(const std::string &type) {
    std::map<std::string, osg_graph_viz::NodeInfo>::iterator it = infoMap.begin();
    for(; it!=infoMap.end(); ++it) {
      if(it->first == type) {
        return it->second.map;
      }
    }
    return ConfigMap();
  }


  void Model::setModelInfo(configmaps::ConfigMap &map) {
    modelInfo = map;
  }

  configmaps::ConfigMap& Model::getModelInfo() {
    return modelInfo;
  }

  void Model::resetConfig(configmaps::ConfigMap &map) {
    std::string domain = map["domain"];
    std::string domainData = domain+"Data";
    std::string modelName = map["modelName"];
    std::string modelVersion = map["modelVersion"];
    osg_graph_viz::NodeInfo ndi = infoMap[modelName];
    ConfigMap &model = ndi.map;

    if(map[domainData].hasKey("data")) {
      ConfigMap &rMap = map[domainData]["data"];
      if(map[domainData]["data"].hasKey("submodel")) {
        rMap.erase("submodel");
      }
      if(map[domainData]["data"].hasKey("configuration")) {
        rMap.erase("configuration");
      }
    }
    if(model.hasKey(domainData) &&
       model[domainData].hasKey("data")) {
      if(model[domainData]["data"].hasKey("configuration")) {
        map[domainData]["data"]["configuration"] = model[domainData]["data"]["configuration"];
      }
      if(model[domainData]["data"].hasKey("submodel")) {
        map[domainData]["data"]["submodel"] = model[domainData]["data"]["submodel"];
      }
    }
  }

} // end of namespace xrock_gui_model
