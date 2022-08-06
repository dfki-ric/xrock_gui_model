/**
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief
 * \version 0.1
 */

#ifndef XROCK_GUI_MODEL_LIB_HPP
#define XROCK_GUI_MODEL_LIB_HPP

#include <iostream>
#include <fstream>

#include <lib_manager/LibInterface.hpp>
#include <mars/main_gui/GuiInterface.h>
#include <mars/main_gui/MenuInterface.h>
#include <bagel_gui/BagelGui.hpp>
#include <bagel_gui/PluginInterface.hpp>
#include <mars/cfg_manager/CFGManagerInterface.h>
#include "DBInterface.hpp"

namespace bagel_gui
{
  class BagelModel;
}

namespace xrock_gui_model
{

  class Model;
  class ModelWidget;

  class ModelLib : public lib_manager::LibInterface,
                   public mars::main_gui::MenuInterface,
                   public bagel_gui::PluginInterface,
                   public mars::cfg_manager::CFGClient
  {

  public:
    explicit ModelLib(lib_manager::LibManager *theManager);
    ~ModelLib();

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

    Model *getModelInstance();

    // CFGClient methods
    virtual void cfgUpdateProperty(mars::cfg_manager::cfgPropertyStruct _property);

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
    void exportCnd(const configmaps::ConfigMap &map_, const std::string &filename);
    void importCND(const std::string &filename);
    void nodeContextClicked(const std::string name);
    void inPortContextClicked(const std::string name);
    void outPortContextClicked(const std::string name);
    std::vector<std::string> getNodeContextStrings(const std::string &name);
    std::vector<std::string> getInPortContextStrings(const std::string &nodeName,
                                                     const std::string &portName);
    std::vector<std::string> getOutPortContextStrings(const std::string &nodeName,
                                                      const std::string &portName);
    void requestModel();

    // public slots:
    void addComponent(std::string domain, std::string modelName, std::string version, std::string nodeName = "");
    void loadComponent(std::string domain, std::string modelName, std::string version);
    void applyConfiguration(configmaps::ConfigMap &map);
    DBInterface *db;

  private:
    std::map<std::string, configmaps::ConfigMap> modelCache;
    Model *model;
    mars::main_gui::GuiInterface *gui;
    bagel_gui::BagelGui *bagelGui;
    ModelWidget *widget;
    bool importToBagel;
    std::string versionChangeName, configureNodeName, contextNodeName, contextPortName;
    mars::cfg_manager::CFGManagerInterface *cfg;
    configmaps::ConfigMap env;
    mars::cfg_manager::cfgParamId dbAddress_paramId;
    mars::cfg_manager::cfgParamId dbUser_paramId;
    mars::cfg_manager::cfgParamId dbPassword_paramId;
    std::string lastExecFolder;

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

#endif // XROCK_GUI_MODEL_LIB_HPP
