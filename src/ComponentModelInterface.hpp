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
        explicit ComponentModelInterface(const ComponentModelInterface *other);
        ~ComponentModelInterface();

        bagel_gui::ModelInterface *clone();

        // BagelGui calls these function if the user wants to modify the content
        // OR if a new component model is to be loaded into the GUI
        bool addNode(unsigned long nodeId, configmaps::ConfigMap *node);
        bool addEdge(unsigned long egdeId, configmaps::ConfigMap *edge);
        bool addNode(unsigned long nodeId, const configmaps::ConfigMap &node);
        bool addEdge(unsigned long egdeId, const configmaps::ConfigMap &edge);
        bool hasEdge(configmaps::ConfigMap *edge);
        bool hasEdge(const configmaps::ConfigMap &edge);
        void preAddNode(unsigned long nodeId){};
        bool updateNode(unsigned long nodeId,
                        configmaps::ConfigMap node);
        bool updateEdge(unsigned long egdeId,
                        configmaps::ConfigMap edge) { return true; }
        bool removeNode(unsigned long nodeId);
        bool removeEdge(unsigned long edgeId);

        // Import/Export
        // TODO: Why do we have this functions here?
        //void importFromFile(std::string fileName);
        //void exportToFile(std::string fileName);

        bool groupNodes(unsigned long groupNodeId, unsigned long nodeId); // DEPRECATED
        bool loadSubgraphInfo(const std::string &filename,
                              const std::string &absPath) { return false; }

        // TODO: Handle port compatibility!!!
        std::map<unsigned long, std::vector<std::string>> getCompatiblePorts(unsigned long nodeId, std::string outPortName) { return std::map<unsigned long, std::vector<std::string>>(); }
        bool handlePortCompatibility() { return false; }

        // NOTE: accesses the nodeModelMap. This map contains the component model info of all the parts inside this model
        const std::map<std::string, osg_graph_viz::NodeInfo> &getNodeInfoMap(); // PURE VIRTUAL
        bool addNodeInfo(configmaps::ConfigMap &model, std::string version = "");
        bool hasNodeInfo(const std::string &type);
        configmaps::ConfigMap getNodeInfo(const std::string &type);

        // These functions set/get the original model and derive the bagel model from it internally.
        // setModelInfo() will also trigger an GUI update
        void setModelInfo(configmaps::ConfigMap &map); // PURE VIRTUAL
        configmaps::ConfigMap &getModelInfo(); // PURE VIRTUAL

        // NOTE: If requested by the user, this function resets the node configuration to be the default config of the associated component model
        void resetConfig(configmaps::ConfigMap &map);

    private:
        std::map<unsigned long, configmaps::ConfigMap> nodeMap;
        std::map<unsigned long, configmaps::ConfigMap> edgeMap;

        // map which holds the component models of the parts (needed to show their interfaces etc.)
        // it is accessed by an unqiue identifier. The basic model uses domain, name, version keys as a unique identifier.
        std::map<std::string, osg_graph_viz::NodeInfo> nodeInfoMap;

        // This config map should contain the ORIGINAL info of the component model.
        // If this changes the bagel model has to be updated to show the results in the GUI
        configmaps::ConfigMap basicModel;
        // This config map stores the derived bagel model (based on the original model)
        // If this has changed by editing the model in the GUI the orignal model has to be updated
        configmaps::ConfigMap bagelModel;

        void loadNodeInfo(std::string path, bool orogen = false); // NOTE: Needed for bagel/shader stuff. Could be moved to XRockGui itself
        bool addOrogenInfo(configmaps::ConfigMap &model); // DEPRECATED
    };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_HPP
