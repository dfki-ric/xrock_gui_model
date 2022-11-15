/**
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief
 * \version 0.1
 */

#pragma once
#include <iostream>
#include <fstream>

#include <lib_manager/LibInterface.hpp>
#include <mars/main_gui/GuiInterface.h>
#include <mars/main_gui/MenuInterface.h>
#include <bagel_gui/BagelGui.hpp>
#include <bagel_gui/PluginInterface.hpp>
#include <mars/cfg_manager/CFGManagerInterface.h>
#include "DBInterface.hpp"
#include "ToolbarBackend.hpp"

namespace bagel_gui
{
    class BagelModel;
}

namespace xrock_gui_model
{

    class ComponentModelInterface;
    class ComponentModelEditorWidget;

    enum struct MenuActions : int
    {
        LOAD_MODEL = 1,
        SAVE_MODEL = 2,
        TOGGLE_MODEL_WIDGET = 3,
        STORE_MODEL_TO_DB = 4,
        EXPORT_CND = 5,
        ADD_COMPONENT_FROM_DB = 6,
        LOAD_MODEL_FROM_DB = 7,
        IMPORT_HW_TO_BAGEL = 8,
        EDIT_LOCAL_MAP = 10,
        CREATE_BAGEL_MOTION_CONTROL_TASK = 11,
        CREATE_BAGEL_MODEL = 12,
        CREATE_BAGEL_TASK = 13,
        EDIT_MODEL_DESCRIPTION = 14,
        EXPORT_CND_AND_LAUNCH = 15,
        STOP_CND = 16,
        IMPORT_CND = 20,
        SELECT_SERVERLESS = 21,
        SELECT_CLIENT = 22,
        SELECT_MULTIDB = 23,
        RELOAD_MODEL_FROM_DB = 30,
        EXPORT_CND_TFENHANCE = 31
    };

    class XRockGUI : public lib_manager::LibInterface,
                     public mars::main_gui::MenuInterface,
                     public bagel_gui::PluginInterface,
                     public mars::cfg_manager::CFGClient
    {
    public:
        explicit XRockGUI(lib_manager::LibManager *theManager);
        ~XRockGUI();

        // Initializers
        void initConfig();
        void initBagelGui();
        void initMainGui();
        void initBackends();

        // LibInterface methods
        int getLibVersion() const
        {
            return 1;
        }

        const std::string getLibName() const
        {
            return std::string("xrock_gui_model");
        }

        CREATE_MODULE_INFO();

        // CFGClient methods
        virtual void cfgUpdateProperty(mars::cfg_manager::cfgPropertyStruct property);

        // MenuInterface methods
        void menuAction(int action, bool checked = false);
        void loadNodes(bagel_gui::BagelModel *model, configmaps::ConfigMap &nodes, std::string path,
                       std::vector<std::string> *nodesFound);
        void handleModelMap(bagel_gui::BagelModel *model, configmaps::ConfigMap &map, std::string path);
        void mechanicsToBagel(configmaps::ConfigMap &map);
        void currentModelChanged(bagel_gui::ModelInterface *model);
        void changeNodeVersion(const std::string &name);
        void configureNode(const std::string &name);
        void openConfigFile(const std::string &name);
        void configureInPort(const std::string &nodeName, const std::string &portName);
        void configureOutPort(const std::string &nodeName, const std::string &portName);
        void selectVersion(const std::string &version);
        void exportCnd(const configmaps::ConfigMap &map_, const std::string &filename, const std::string &urdf_file = "");
        void exportCnd2(const configmaps::ConfigMap &map_, const std::string &filename);
        void importCND(const std::string &fileName);
        void nodeContextClicked(const std::string name);
        void inPortContextClicked(const std::string name);
        void outPortContextClicked(const std::string name);
        std::vector<std::string> getNodeContextStrings(const std::string &name);
        std::vector<std::string> getInPortContextStrings(const std::string &nodeName,
                                                         const std::string &portName);
        std::vector<std::string> getOutPortContextStrings(const std::string &nodeName,
                                                          const std::string &portName);
        void requestModel();
        void addComponent();

        // public slots:
        void addComponent(const std::string& domain, const std::string& modelName, const std::string& version, std::string nodeName = "");
        void loadComponentModel(const std::string& domain, const std::string& modelName, const std::string& version);
        void loadComponentModelFrom(configmaps::ConfigMap &map);
        void applyConfiguration(configmaps::ConfigMap &map);
        std::unique_ptr<DBInterface> db;

    private:
        std::map<std::string, configmaps::ConfigMap> modelCache;
        mars::main_gui::GuiInterface *gui;
        bagel_gui::BagelGui *bagelGui;
        ComponentModelEditorWidget *widget;
        bool importToBagel;
        std::string versionChangeName, configureNodeName, contextNodeName, contextPortName;
        mars::cfg_manager::CFGManagerInterface *cfg;
        configmaps::ConfigMap env;
        mars::cfg_manager::cfgParamId dbAddress_paramId;
        mars::cfg_manager::cfgParamId dbUser_paramId;
        mars::cfg_manager::cfgParamId dbPassword_paramId;
        std::string lastExecFolder;
        std::string resourcesPath;
        ToolbarBackend *toolbarBackend;

        void loadStartModel();
        void loadModelFromParameter();
        bool loadCart();
        void loadSettingsFromFile(const std::string &filename);
        void writeStatus(const int statusId, const std::string &message);
        void openConfigureInterfaceDialog(const std::string &nodeName,
                                          const std::string &portName,
                                          const std::string &portType);
        void configureComponents(const std::string &name);
        void createBagelModel();
        void createBagelTask();
    };

} // end of namespace xrock_gui_model

