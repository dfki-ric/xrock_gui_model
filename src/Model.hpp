/**
 * \file Model.hpp
 * \author Malte Langosz
 * \brief
 */

#ifndef XROCK_GUI_MODEL_HPP
#define XROCK_GUI_MODEL_HPP

#include <bagel_gui/ModelInterface.hpp>

namespace xrock_gui_model {

  class Model : public bagel_gui::ModelInterface {
  public:
    explicit Model(bagel_gui::BagelGui *bagelGui);
    Model(const Model *other);
    ~Model();

    bagel_gui::ModelInterface* clone();

    bool addNode(unsigned long nodeId, configmaps::ConfigMap *node);
    bool addEdge(unsigned long egdeId, configmaps::ConfigMap *edge);
    bool addNode(unsigned long nodeId, const configmaps::ConfigMap &node);
    bool addEdge(unsigned long egdeId, const configmaps::ConfigMap &edge);
    bool hasEdge(configmaps::ConfigMap *edge);
    bool hasEdge(const configmaps::ConfigMap &edge);
    void preAddNode(unsigned long nodeId) {};
    // todo: handle update name
    bool updateNode(unsigned long nodeId,
                    configmaps::ConfigMap& node);
    bool updateEdge(unsigned long egdeId,
                    configmaps::ConfigMap& edge) {return true;}
    bool removeNode(unsigned long nodeId);
    bool removeEdge(unsigned long edgeId);

    // Import/Export
    void importFromFile(std::string fileName);
    void exportToFile(std::string fileName);

    bool groupNodes(unsigned long groupNodeId, unsigned long nodeId);
    bool loadSubgraphInfo(const std::string &filename,
                          const std::string &absPath) {return false;}
    std::map<unsigned long, std::vector<std::string> > getCompatiblePorts(unsigned long nodeId,  std::string outPortName) {return std::map<unsigned long, std::vector<std::string> >();}
    bool handlePortCompatibility() {return false;}
    const std::map<std::string, osg_graph_viz::NodeInfo>& getNodeInfoMap();
    //void displayWidget( QWidget *pParent );
    bool addNodeInfo(configmaps::ConfigMap &model, std::string version = "" );
    bool hasNodeInfo(const std::string &type);
    configmaps::ConfigMap getNodeInfo(const std::string &type);
    void setModelInfo(configmaps::ConfigMap &map);
    configmaps::ConfigMap& getModelInfo();
    void setEdition(std::string v);
    void resetConfig(configmaps::ConfigMap &map);

  private:
    std::map<unsigned long, configmaps::ConfigMap> nodeMap;
    std::map<unsigned long, configmaps::ConfigMap> edgeMap;
    std::map<std::string, osg_graph_viz::NodeInfo> infoMap;
    configmaps::ConfigMap modelInfo;
    std::string edition;

    void loadNodeInfo(std::string path, bool orogen=false);
    bool addOrogenInfo(configmaps::ConfigMap &model);
  };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_HPP
