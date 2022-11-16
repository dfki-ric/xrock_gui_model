/**
 * \file ComponentModelInterface.hpp
 * \author Malte Langosz
 * \brief
 */

#pragma once
#include <bagel_gui/ModelInterface.hpp>

namespace xrock_gui_model
{
    class XRockGUI;

    class ComponentModelInterface : public bagel_gui::ModelInterface
    {
    public:
        explicit ComponentModelInterface(bagel_gui::BagelGui *bagelGui, XRockGUI* xrockGui);
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
        std::string deriveTypeFrom(const std::string& domain, const std::string& name, const std::string& version);
        std::string deriveTypeFromNodeInfo(configmaps::ConfigMap &model);
        bool addNodeInfo(const std::string& type, configmaps::ConfigMap &model);
        bool hasNodeInfo(const std::string &type);
        configmaps::ConfigMap getNodeInfo(const std::string &type);

        // These functions set/get the original model and derive the bagel model from it internally.
        // setModelInfo() will also trigger an GUI update
        void setModelInfo(configmaps::ConfigMap &map); // PURE VIRTUAL
        configmaps::ConfigMap &getModelInfo(); // PURE VIRTUAL
        // This function will register a component model if it is not already registered.
        // If the model is unknown it will request it internally
        bool registerComponentModel(const std::string& domain, const std::string& name, const std::string& version);
        // This function tries to find layout specific info in the given model and will update the layout/positions of the parts
        void applyPartLayout(configmaps::ConfigMap &map);

        // NOTE: If requested by the user, this function resets the node configuration to be the default config of the associated component model
        void resetConfig(configmaps::ConfigMap &map);

    private:
        // We need a reference to the XRockGUI for DB accesses
        XRockGUI* xrockGui;

        std::map<unsigned long, configmaps::ConfigMap> nodeMap;
        std::map<unsigned long, configmaps::ConfigMap> edgeMap;

        // Map which holds a mixed and transformed version of the component models of the parts and the part itself (needed to show their interfaces etc.)
        // it is accessed by an unqiue identifier. The basic model uses domain, name, version keys as a unique identifier.
        std::map<std::string, osg_graph_viz::NodeInfo> nodeInfoMap;

        // This config map should contain the ORIGINAL info of the component model.
        // If this changes the bagel model has to be updated to show the results in the GUI
        // NOTE: The bagel specific stuff based on the basic model is in the node, edge and nodeInfo maps
        configmaps::ConfigMap basicModel;
        // This map stores the ORIGINAL info of the compponent models of the parts.
        std::map<std::string, configmaps::ConfigMap> partModels;

        void loadNodeInfo(std::string path, bool orogen = false); // NOTE: Needed for bagel/shader stuff. Could be moved to XRockGui itself
        bool addOrogenInfo(configmaps::ConfigMap &model); // DEPRECATED
    };
} // end of namespace xrock_gui_model


