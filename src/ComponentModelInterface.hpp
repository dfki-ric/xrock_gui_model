/**
 * \file ComponentModelInterface.hpp
 * \author Malte Langosz
 * \brief
 */

#ifndef XROCK_GUI_MODEL_HPP
#define XROCK_GUI_MODEL_HPP

#include <bagel_gui/ModelInterface.hpp>

namespace xrock_gui_model
{

    class ComponentModelInterface : public bagel_gui::ModelInterface
    {
    public:
        explicit ComponentModelInterface(bagel_gui::BagelGui *bagelGui);
        ComponentModelInterface(const ComponentModelInterface *other);
        ~ComponentModelInterface();

        bagel_gui::ModelInterface *clone();

        bool addNode(unsigned long nodeId, configmaps::ConfigMap *node);
        bool addEdge(unsigned long egdeId, configmaps::ConfigMap *edge);
        bool addNode(unsigned long nodeId, const configmaps::ConfigMap &node);
        bool addEdge(unsigned long egdeId, const configmaps::ConfigMap &edge);
        bool hasEdge(configmaps::ConfigMap *edge);
        bool hasEdge(const configmaps::ConfigMap &edge);
        void preAddNode(unsigned long nodeId){};
        // todo: handle update name
        bool updateNode(unsigned long nodeId,
                        configmaps::ConfigMap node);
        bool updateEdge(unsigned long egdeId,
                        configmaps::ConfigMap edge) { return true; }
        bool removeNode(unsigned long nodeId);
        bool removeEdge(unsigned long edgeId);

        // Import/Export
        // TODO: Why do we have this functions here?
        void importFromFile(std::string fileName);
        void exportToFile(std::string fileName);

        bool groupNodes(unsigned long groupNodeId, unsigned long nodeId);
        bool loadSubgraphInfo(const std::string &filename,
                              const std::string &absPath) { return false; }
        std::map<unsigned long, std::vector<std::string>> getCompatiblePorts(unsigned long nodeId, std::string outPortName) { return std::map<unsigned long, std::vector<std::string>>(); }
        bool handlePortCompatibility() { return false; }
        const std::map<std::string, osg_graph_viz::NodeInfo> &getNodeInfoMap();
        bool addNodeInfo(configmaps::ConfigMap &model, std::string version = "");
        bool hasNodeInfo(const std::string &type);
        configmaps::ConfigMap getNodeInfo(const std::string &type);

        // These functions set/get the original model and derive the bagel model from it
        void setModelInfo(configmaps::ConfigMap &map);
        configmaps::ConfigMap &getModelInfo();

        void setEdition(std::string v);
        void resetConfig(configmaps::ConfigMap &map);

    private:
        // TODO: Do we need all these maps? What do they do? Especially the infoMap
        std::map<unsigned long, configmaps::ConfigMap> nodeMap;
        std::map<unsigned long, configmaps::ConfigMap> edgeMap;
        std::map<std::string, osg_graph_viz::NodeInfo> infoMap;

        // This config map should contain the ORIGINAL info of the component model.
        // If this changes the bagel model has to be updated to show the results in the GUI
        configmaps::ConfigMap modelInfo;
        // This config map stores the derived bagel model (based on the original model)
        // If this has changed by editing the model in the GUI the orignal model has to be updated
        configmaps::ConfigMap modelInfo;

        // TODO: What is edition?
        std::string edition;

        void loadNodeInfo(std::string path, bool orogen = false);
        bool addOrogenInfo(configmaps::ConfigMap &model);
    };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_HPP
