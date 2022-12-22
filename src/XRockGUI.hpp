/**
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief
 * \version 0.1
 */

#pragma once
#include <iostream>
#include <fstream>

#include <mars/main_gui/MainGUI.h>
#include <lib_manager/LibInterface.hpp>
#include <mars/main_gui/GuiInterface.h>
#include <mars/main_gui/MenuInterface.h>
#include <bagel_gui/BagelGui.hpp>
#include <bagel_gui/PluginInterface.hpp>
#include <mars/cfg_manager/CFGManagerInterface.h>
#include "DBInterface.hpp"
#include "ToolbarBackend.hpp"
#include "XRockIOLibrary.hpp"

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
        EDIT_LOCAL_MAP = 10,
        CREATE_BAGEL_MOTION_CONTROL_TASK = 11,
        CREATE_BAGEL_MODEL = 12,
        CREATE_BAGEL_TASK = 13,
        EDIT_MODEL_DESCRIPTION = 14,
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
        void currentModelChanged(bagel_gui::ModelInterface *model);
        void changeNodeVersion(const std::string &name);
        void configureNode(const std::string &name);
        void openConfigFile(const std::string &name);
        void configureInPort(const std::string &nodeName, const std::string &portName);
        void configureOutPort(const std::string &nodeName, const std::string &portName);
        void selectVersion(const std::string &version);
        void exportCnd(const configmaps::ConfigMap &map_, const std::string &filename, const std::string &urdf_file = "");
        void importCND(const std::string &fileName);

        // drop down menu functions on nodes and node interfaces
        // these are triggered by right-clicking in the GUI
        void nodeContextClicked(const std::string name);
        void inPortContextClicked(const std::string name);
        void outPortContextClicked(const std::string name);
        std::vector<std::string> getNodeContextStrings(const std::string &name);
        std::vector<std::string> getInPortContextStrings(const std::string &nodeName,
                                                         const std::string &portName);
        std::vector<std::string> getOutPortContextStrings(const std::string &nodeName,
                                                          const std::string &portName);

        // public slots:
        // This function adds a new component to the current component model (possibly asking the DB for it's model)
        void addComponent(const std::string& domain, const std::string& modelName, const std::string& version, std::string nodeName = "");
        // These function load a component model from DB or from a ConfigMap
        void loadComponentModel(const std::string& domain, const std::string& modelName, const std::string& version);
        void loadComponentModelFrom(configmaps::ConfigMap &map);
        // This function stores the current component model
        bool storeComponentModel();
        // This function applies a ROCK configuration to a ROCK Task with dynamic ports to create a new model
        // TODO: Since the XTypes now know the notion of an dynamic interface, we could remove this and create such functionality
        // in ComponentModelInterface without creating a new model in the database
        void applyConfiguration(configmaps::ConfigMap &map);

        // These functions open a dialog to select a component model to be opened/instantiated and then fetch the info from the database (see below)
        void requestModel();
        void addComponent();
        // Stored a pointer to the currently selected XRock database backend instance
        std::unique_ptr<DBInterface> db;
        XRockIOLibrary *ioLibrary;

    private:
        mars::main_gui::GuiInterface *gui;
        bagel_gui::BagelGui *bagelGui;
        ComponentModelEditorWidget *widget;
        std::string versionChangeName, configureNodeName, contextNodeName, contextPortName;
        mars::cfg_manager::CFGManagerInterface *cfg;
        configmaps::ConfigMap env;
        mars::cfg_manager::cfgParamId dbAddress_paramId;
        mars::cfg_manager::cfgParamId dbUser_paramId;
        mars::cfg_manager::cfgParamId dbPassword_paramId;
        std::string resourcesPath;
        ToolbarBackend *toolbarBackend;

        void loadStartModel();
        void loadModelFromParameter();
        bool loadCart();
        void loadSettingsFromFile(const std::string &filename);
        void openConfigureInterfaceDialog(const std::string &nodeName,
                                          const std::string &portName,
                                          const std::string &portType);
        void configureComponents(const std::string &name);

        // This function creates a new ROCK Task out of an bagel graph
        // TODO: This function is deprecated and should be moved to a script
        void createBagelModel();
        // This function creates a new Bagel Model out of an ASSEMBLY with a SMURF File
        // TODO: This function is deprecated and should be moved to a script
        void createBagelTask();
    };

} // end of namespace xrock_gui_model

