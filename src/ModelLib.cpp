/**
 * \file ModelLib.cpp
 * \author Malte Langosz
 * \brief
 */

#include "ModelLib.hpp"
#include "Model.hpp"
#include "ModelWidget.hpp"
#include "ImportDialog.hpp"
#include "FileDB.hpp"
#include "RestDB.hpp"
#include "ServerlessDB.hpp"
#include "VersionDialog.hpp"
#include "ConfigureDialog.hpp"
#include "ConfigMapHelper.hpp"

#include <lib_manager/LibManager.hpp>
#include <bagel_gui/BagelGui.hpp>
#include <bagel_gui/BagelModel.hpp>
#include <mars/main_gui/MainGUI.h>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <mars/utils/misc.h>
#include <QWebView>
#include <QUuid>
#include <QDateTime>
#include <iostream>
#include <fstream>

using namespace lib_manager;
using namespace bagel_gui;
using namespace configmaps;
using namespace mars::utils;

namespace xrock_gui_model
{

  std::string getHtml2(const std::string &markdown)
  {
    std::string cmd = "echo \"" + markdown + "\" | python -m markdown";
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
      throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get()))
    {
      if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
        result += buffer.data();
    }
    return result;
  }

  ModelLib::ModelLib(lib_manager::LibManager *theManager) : lib_manager::LibInterface(theManager), model(NULL)
  {
    fprintf(stderr, "create model\n");

    importToBagel = false;
    cfg = libManager->getLibraryAs<mars::cfg_manager::CFGManagerInterface>("cfg_manager", true);
    std::string confDir = ".";
    
    resourcesPath = XROCK_DEFAULT_RESOURCES_PATH;
    if(cfg) {
      std::string resourcesPathConfig = cfg->getOrCreateProperty("Preferences",
                                                     "resources_path",
                                                     "").sValue;
      if(resourcesPathConfig != "") {
        resourcesPath = resourcesPathConfig;
      }
      cfg->getPropertyValue("Config", "config_path", "value", &confDir);
    
      env = ConfigMap::fromYamlFile(confDir + "/config_default.yml", true);
      env["AUTOPROJ_CURRENT_ROOT"] = getenv("AUTOPROJ_CURRENT_ROOT");
      // try to load environment
      if (mars::utils::pathExists(confDir + "/config.yml"))
      {
        env.append(ConfigMap::fromYamlFile(confDir + "/config.yml"));
      }
      env["ConfigDir"] = confDir;
      std::string defaultAddress = "../../../bagel/bagel_db";
      mars::utils::handleFilenamePrefix(&defaultAddress, confDir);
      if (env.hasKey("dbType"))
      {
        if (env["dbType"] == "RestDB")
        {
          defaultAddress = "http://localhost:8183";
        }
      }

      std::string confDir2 = confDir + "/XRockGUI.yml";
      if (mars::utils::pathExists(confDir2))
      {
        cfg->loadConfig(confDir2.c_str());
      }

      mars::cfg_manager::cfgPropertyStruct prop_dbAddress;
      prop_dbAddress = cfg->getOrCreateProperty("XRockGUI", "dbAddress",
                                                defaultAddress, this);
      db = NULL;
      if (env.hasKey("dbType") and env["dbType"] == "RestDB")
      {
        db = new RestDB();
        std::cout << "Using restdb" << std::endl;
      }
      else if (env.hasKey("dbType") and env["dbType"] == "ServerlessDB" and env.hasKey("dbPath"))
      {
        std::string dbAbsPath = mars::utils::pathJoin(env["AUTOPROJ_CURRENT_ROOT"], env["dbPath"].toString());
        db = new ServerlessDB(dbAbsPath);
        std::cout << "Using serverless where db path is: " << dbAbsPath << std::endl;
      }
      else if (env.hasKey("dbType") and env["dbType"] == "MultiDB")
      {
        // todo
      }
      
      else
      {
        prop_dbAddress.sValue = mars::utils::pathJoin(confDir, prop_dbAddress.sValue);
        db = new FileDB();
      }
      db->set_dbAddress(prop_dbAddress.sValue);
      dbAddress_paramId = prop_dbAddress.paramId;
    }
    bagelGui = libManager->getLibraryAs<BagelGui>("bagel_gui");
    if (bagelGui)
    {
      model = new Model(bagelGui);
      bagelGui->addModelInterface("xrock", model);
      bagelGui->createView("xrock", "Model");
      bagelGui->addPlugin(this);
    }
    else
    {
      std::cerr << "ERROR: ModelLib: was not able to get bagel_gui!" << std::endl;
    }
    gui = libManager->getLibraryAs<mars::main_gui::GuiInterface>("main_gui");
    if (gui)
    {
      const std::string icon = mars::utils::pathJoin(resourcesPath, "xrock_gui_model/resources/images/");
      std::cout << icon << std::endl;
      gui->addGenericMenuAction("../File/Import/Model", static_cast<int>(MenuActions::LOAD_MODEL), this);
      gui->addGenericMenuAction("../File/Import/CNDModel", static_cast<int>(MenuActions::IMPORT_CND), this);
      gui->addGenericMenuAction("../File/Export/Model", static_cast<int>(MenuActions::SAVE_MODEL), this);
      gui->addGenericMenuAction("../File/Export/CNDModel", static_cast<int>(MenuActions::EXPORT_CND), this);
      gui->addGenericMenuAction("../File/Export/CNDModel With tf_enhance", static_cast<int>(MenuActions::EXPORT_CND_TFENHANCE), this);
      gui->addGenericMenuAction("../Database/Add Component", static_cast<int>(MenuActions::ADD_COMPONENT_FROM_DB), this);
      gui->addGenericMenuAction("../Database/Store Model", static_cast<int>(MenuActions::STORE_MODEL_TO_DB), this);
      gui->addGenericMenuAction("../Database/Load Model", static_cast<int>(MenuActions::LOAD_MODEL_FROM_DB), this);
      gui->addGenericMenuAction("../Database/HardToSoft", static_cast<int>(MenuActions::IMPORT_HW_TO_BAGEL), this);
      gui->addGenericMenuAction("../Windows/ModelWidget", static_cast<int>(MenuActions::TOGGLE_MODEL_WIDGET), this);
      gui->addGenericMenuAction("../Expert/Edit Description", static_cast<int>(MenuActions::EDIT_MODEL_DESCRIPTION), this);
      gui->addGenericMenuAction("../Expert/Edit Local Map", static_cast<int>(MenuActions::EDIT_LOCAL_MAP), this);
      gui->addGenericMenuAction("../Expert/Create Bagel Model", static_cast<int>(MenuActions::CREATE_BAGEL_MODEL), this);
      gui->addGenericMenuAction("../Expert/Create Bagel Task", static_cast<int>(MenuActions::CREATE_BAGEL_TASK), this);
      gui->addGenericMenuAction("../Expert/Launch CND", static_cast<int>(MenuActions::EXPORT_CND_AND_LAUNCH), this);
      gui->addGenericMenuAction("../Expert/Stop CND", static_cast<int>(MenuActions::STOP_CND), this);
      gui->addGenericMenuAction("../Actions/Load Model", static_cast<int>(MenuActions::LOAD_MODEL_FROM_DB), this, 0,
                                icon + "load.png", true);
      gui->addGenericMenuAction("../Actions/Add Component", static_cast<int>(MenuActions::ADD_COMPONENT_FROM_DB), this, 0,
                                icon + "add.png", true);
      gui->addGenericMenuAction("../Actions/Save Model", static_cast<int>(MenuActions::STORE_MODEL_TO_DB), this, 0,
                                icon + "save.png", true);
      gui->addGenericMenuAction("../Actions/Reload", static_cast<int>(MenuActions::RELOAD_MODEL_FROM_DB), this, 0,
                                icon + "reload.png", true);

      mars::main_gui::MainGUI *main_gui = dynamic_cast<mars::main_gui::MainGUI *>(gui);
      main_gui->mainWindow_p()->setWindowIcon(QIcon(":/images/xrock_gui.ico"));
      main_gui->addComboBoxToToolbar("Actions", DBInterface::loadBackends(), [this, main_gui](std::string new_backend)
                                     {
                                      std::cout << "backend changed  " << new_backend << std::endl;
                                      if(new_backend == "Serverless")
                                        {
                                          menuAction(static_cast<int>(MenuActions::SELECT_SERVERLESS));
                                          main_gui->disableToolbarLineEdit({2,3});
                                          main_gui->enableToolbarLineEdit({1});
                                        }
                                      else if (new_backend == "Client")
                                        {
                                          menuAction(static_cast<int>(MenuActions::SELECT_CLIENT));
                                          main_gui->disableToolbarLineEdit({1});
                                          main_gui->enableToolbarLineEdit({2,3,4});
                                        }
                                      else if (new_backend == "MultiDbClient")
                                        menuAction(static_cast<int>(MenuActions::SELECT_MULTIDB)); });
      main_gui->addLineEditToToolbar(1, "Actions", " db path: ", "modkom/component_db",
                                     [this](std::string text)
                                     {
                                       ServerlessDB *sdb = dynamic_cast<ServerlessDB *>(db);
                                       sdb->set_dbPath(text);
                                     });
      main_gui->addLineEditToToolbar(2, "Actions", " URL: ", "http://0.0.0.0", [this, main_gui](std::string text)
                                     {
                                       RestDB *rdb = dynamic_cast<RestDB *>(db);
                                       std::string port = main_gui->getToolbarLineEditText(3);
                                       std::cout << " port no " << port << std::endl;
                                       std::string dbAddress = text + ':' + port;
                                       rdb->set_dbAddress(dbAddress);
                                       std::cout << "URL changed  " << text << std::endl;
                                     });
      main_gui->addLineEditToToolbar(3, "Actions", " port: ", "8183", [this, main_gui](std::string text)
                                     { 
                                       RestDB *rdb = dynamic_cast<RestDB *>(db);
                                       std::string url = main_gui->getToolbarLineEditText(2);
                                       std::string dbAddress= url + ':' + text;
                                       rdb->set_dbAddress(dbAddress); });
      main_gui->addLineEditToToolbar(4, "Actions", " graph: ", "graph_test", [this](std::string text)
                                     {db->set_dbGraph(text);});
      main_gui->disableToolbarLineEdit({2, 3});

      widget = new ModelWidget(cfg, bagelGui, this);
      if (!widget->getHiddenCloseState())
      {
        gui->addDockWidget((void *)widget, 1);
        bagelGui->updateViewSize();
      }
      std::string loadGraph;
      cfg->getPropertyValue("Config", "model", "value", &loadGraph);
      if (!loadGraph.empty())
      {
        widget->loadModel(loadGraph);
      }
      bagelGui->setSmoothLineMode();
    }
    else
    {
      std::cerr << "ModelLib: was not able to get main_gui" << std::endl;
    }
    loadSettingsFromFile("generalsettings.yml");
    loadModelFromParameter();
  }

  ModelLib::~ModelLib()
  {
    widget->deinit();
    if (gui)
      libManager->releaseLibrary("main_gui");
    if (bagelGui)
      libManager->releaseLibrary("bagel_gui");
    if (cfg)
    {
      std::string confDir = ".";
      cfg->getPropertyValue("Config", "config_path", "value", &confDir);
      confDir += "/XRockGUI.yml";
      cfg->writeConfig(confDir.c_str(), "XRockGUI");
      libManager->releaseLibrary("cfg_manager");
    }
    writeStatus(0, "closed fine");
  }

  void ModelLib::loadStartModel()
  {
    if (cfg)
    {
      std::string domain = "";
      std::string modelName = "";
      std::string version = "";
      std::string edition = "";
      cfg->getPropertyValue("Config", "domain", "value", &domain);
      cfg->getPropertyValue("Config", "model", "value", &modelName);
      cfg->getPropertyValue("Config", "version", "value", &version);
      cfg->getPropertyValue("Config", "edition", "value", &edition);
      edition = mars::utils::toupper(edition);
      if (domain != std::string("") &&
          modelName != std::string("") &&
          version != std::string("") &&
          edition != std::string(""))
      {
        // set modelInfo with given information
        ConfigMap modelInfo;
        modelInfo["name"] = modelName;
        modelInfo["type"] = std::string("system_modelling::") + (edition == "COMPUTATION" ? std::string("hardware_graph") : std::string("task_graph")) + std::string("::Network");
        modelInfo["domain"] = edition;
        modelInfo["data"] = std::string("");
        modelInfo["version"] = version;
        modelInfo["interfaces"] = std::string("");
        modelInfo["editable_interfaces"] = false;
        modelInfo["layouts"] = std::string("");
        widget->setModelInfo(modelInfo);

      }
      else
      {
        fprintf(stderr, "loadStartModel: no model given\n");
      }
    }
  }

  void ModelLib::loadModelFromParameter()
  {
    if (cfg)
    {
      std::string domain = "";
      std::string modelName = "";
      std::string version = "";
      std::string edition = "";
      cfg->getPropertyValue("Config", "domain", "value", &domain);
      cfg->getPropertyValue("Config", "model", "value", &modelName);
      cfg->getPropertyValue("Config", "version", "value", &version);
      cfg->getPropertyValue("Config", "edition", "value", &edition);
      if (domain != std::string("") &&
          modelName != std::string("") &&
          version != std::string(""))
      {
        loadComponent(domain, modelName, version);
        Model *model = dynamic_cast<Model *>(bagelGui->getCurrentModel());
        if (model)
        {
          model->setEdition(edition);
        }
        widget->setEdition(mars::utils::tolower(edition));
      }
      else
      {
        std::cerr << "loadStartModel: no model given" << std::endl;
      }
    }
  }

  void ModelLib::loadSettingsFromFile(const std::string &filename)
  {
    std::string workspace = "";
    if (cfg)
    {
      cfg->getPropertyValue("Config", "workspace", "value", &workspace);
      env["wsd"] = workspace;
    }
    try
    {
      ConfigMap gConfig = ConfigMap::fromYamlFile(workspace + "/" + filename);
      const std::string user = gConfig["database"]["username"];
      const std::string password = gConfig["database"]["password"];
      const std::string url = gConfig["database"]["url"];
      if (cfg)
      {
        cfg->setPropertyValue("XRockGUI", "dbAddress", "value", url);
        cfg->setPropertyValue("XRockGUI", "dbUser", "value", user);
        cfg->setPropertyValue("XRockGUI", "dbPassword", "value", password);
      }
      std::cout << "loadSettingsFromFile: settings loaded from file: " << filename << std::endl;
    }
    catch (std::invalid_argument &e)
    {
      QMessageBox::critical(nullptr, "Error", e.what(), QMessageBox::Ok);
    }
    catch (...)
    {
      std::stringstream ss;
      ss << "loadSettingsFromFile: ERROR while loading: " << workspace << '/' << filename;
      std::cerr << ss.str() << std::endl;
      
    }
  }

  void ModelLib::writeStatus(const int statusId, const std::string &message)
  {
    std::ofstream outputFile("status.yml");
    outputFile << "status : " << statusId << std::endl;
    outputFile << "message : " << message << std::endl;
    outputFile.close();
  }

  Model *ModelLib::getModelInstance()
  {
    return model;
  }

  void ModelLib::menuAction(int action, bool checked)
  {
    mars::main_gui::MainGUI *main_gui = dynamic_cast<mars::main_gui::MainGUI *>(gui);
    switch (static_cast<MenuActions>(action))
    {
        case MenuActions::LOAD_MODEL:
        {
          widget->loadModel();
          break;
        }
        case MenuActions::SAVE_MODEL:
        {
          widget->saveModel();
          break;
        }
        case MenuActions::TOGGLE_MODEL_WIDGET:
        {
          if (widget->isHidden())
          {
            gui->addDockWidget((void *)widget, 1);
          }
          else
          {
            gui->removeDockWidget((void *)widget, 1);
          }
          break;
        }
        case MenuActions::STORE_MODEL_TO_DB: // store model
        {
          widget->storeModel();
          break;
        }
        case MenuActions::EXPORT_CND:
        {
          QString fileName = QFileDialog::getSaveFileName(NULL, QObject::tr("Select Model"),
                                                          "export.cnd", QObject::tr("YAML syntax (*.cnd)"), 0,
                                                          QFileDialog::DontUseNativeDialog);
          if (!fileName.isNull())
          {
            ConfigMap map = bagelGui->createConfigMap();
            exportCnd(map, fileName.toStdString());
          }
          break;
        }
        case MenuActions::ADD_COMPONENT_FROM_DB: // add component from database
        {
          ImportDialog id(this, false);
          id.exec();
          break;
        }
        case MenuActions::LOAD_MODEL_FROM_DB: // load model from database
        {
          requestModel();
          break;
        }
        case MenuActions::IMPORT_HW_TO_BAGEL: // import hardware information into bagel model
        {
            // 20221102 MS: Why is this here? Has nothing todo with XROCK.
          importToBagel = true;
          ImportDialog id(this, true);
          id.exec();
          break;
        }
        case MenuActions::EDIT_LOCAL_MAP:
        {
            // 20221102 MS: Did not work for me. Is this relevant?
          widget->editLocalMap();
          break;
        }
        case MenuActions::CREATE_BAGEL_MOTION_CONTROL_TASK:
        {
            // 20221102 MS: Why is this here? Has nothing todo with XROCK.
          ModelInterface *model = bagelGui->getCurrentModel();
          if (model)
          {
            ConfigMap localMap = model->getModelInfo();
            std::string domain = "software";
            std::string type = "system_modelling::task_graph::Task";
            std::string name = "behavior_graph::MotionControlTask.yml";
            std::string version = localMap["name"];
            std::string graphPath = "tmp/bagel/" + version;
            std::string graphFile;
            handleFilenamePrefix(&graphPath, env["wsd"].getString());
            createDirectory(graphPath);
            graphFile = graphPath + "/" + version + ".yml";
            bagelGui->setLoadPath(graphPath);
            if (pathExists(graphFile))
            {
              bagelGui->load(graphFile);
            }
            else
            {
              bagelGui->createView("bagel", version);
            }
            std::string smurfPath = "tmp/models/assembly/";
            handleFilenamePrefix(&smurfPath, env["wsd"].getString());
            smurfPath += version + "/" + localMap["versions"][0]["name"].getString() + "/smurf/" + version + ".smurf";
            BagelModel *bagelModel = dynamic_cast<BagelModel *>(bagelGui->getCurrentModel());
            if (bagelModel)
            {
              bagelModel->importSmurf(smurfPath);
              ConfigMap map;
              map["domain"] = domain;
              map["name"] = name;
              map["type"] = type;
              map["versions"][0]["name"] = version;
              map["versions"][0]["maturity"] = "INPROGRESS";
              handleFilenamePrefix(&graphFile, getCurrentWorkingDir());

              map["graphFile"] = graphFile;
              ConfigMap interfaces;
              interfaces["i"][0]["direction"] = "OUTGOING";
              interfaces["i"][0]["name"] = "joint_commands";
              interfaces["i"][0]["type"] = "::base::samples::Joints";
              map["interfaces"] = interfaces["i"].toYamlString();

              map["versions"][0]["defaultConfiguration"]["data"]["config"]["graphFilename"] = graphFile;
              widget->setModelInfo(map);
            }
          }
          break;
        }
        case MenuActions::CREATE_BAGEL_MODEL:
        {
            // 20221102 MS: Why is this here? Has nothing todo with XROCK.
          createBagelModel();
          break;
        }
        case MenuActions::CREATE_BAGEL_TASK:
        {
            // 20221102 MS: Why is this here? Has nothing todo with XROCK.
          createBagelTask();
          break;
        }
        case MenuActions::EDIT_MODEL_DESCRIPTION:
        {
          widget->editDescription();
          break;
        }
        case MenuActions::EXPORT_CND_AND_LAUNCH:
        {
            // 20221102 MS: Move to extra function ...
          ModelInterface *model = bagelGui->getCurrentModel();
          if (!model)
          {
            return;
          }
          ConfigMap modelInfo = model->getModelInfo();
          std::string uuid = QUuid::createUuid().toString().toStdString();
          std::string path = mars::utils::pathJoin("tmp", uuid);
          lastExecFolder = path;
          mars::utils::createDirectory(path);
          configmaps::ConfigMap map = bagelGui->createConfigMap();
          std::string name = modelInfo["name"];
          if (name.size() == 0)
          {
            name = "tmp_model_name";
          }
          std::string cndName = name + ".cnd";
          std::string cndNamePre = name + "_pre.cnd";
          std::string cndPath = mars::utils::pathJoin(path, cndName);
          exportCnd2(map, cndPath);
          if (!mars::utils::pathExists(cndPath))
          {
            printf("ERROR: CND file was not exported successfully!\n");
            break;
          }
          std::string pwd = mars::utils::getCurrentWorkingDir();
          chdir(path.c_str());
          std::ofstream file;
          file.open(name + ".orogen");
          file << "require 'cnd_orogen'" << std::endl;
          file << std::endl;
          file << "name '" << name << "'" << std::endl;
          file << "cnd_model = ::CndOrogen.load_cnd_model('" << name << ".cnd')" << std::endl;
          file << "::CndOrogen.load_orogen_project_from_cnd(self, cnd_model)" << std::endl;
          file.close();
#ifdef __APPLE__
          std::string rockLaunch = "mac-rock-launch";
          // create pseudo manifest
          file.open("manifest.xml");
          file.close();
          std::string cmd = "orogen --transports=corba,typelib --extensions=cpp_proxies,modelExport " + name + ".orogen";
          printf("Call %s\n", cmd.c_str());
          system(cmd.c_str());
          cmd = "amake";
          printf("Call %s\n", cmd.c_str());
          system(cmd.c_str());
#else
          std::string rockLaunch = "rock-launch";
          std::string cmd = "cnd-orogen " + cndName;
          printf("Call %s\n", cmd.c_str());
          system(cmd.c_str());
#endif
          if (mars::utils::pathExists(cndNamePre))
          {
            cmd = rockLaunch + " " + cndNamePre;
            printf("Call %s\n", cmd.c_str());
            system(cmd.c_str());
          }
          cmd = rockLaunch + " " + cndName;
          printf("Call %s\n", cmd.c_str());
          system(cmd.c_str());
          chdir(pwd.c_str());
          break;
        }
        case MenuActions::STOP_CND:
        {
          ModelInterface *model = bagelGui->getCurrentModel();
          if (!model)
          {
            return;
          }
          ConfigMap modelInfo = model->getModelInfo();
          std::string path = lastExecFolder;
          std::string name = modelInfo["name"];
          if (name.size() == 0)
          {
            name = "tmp_model_name";
          }
          std::string cndNamePre = name + "_pre.cnd";
          std::string cndPathPre = mars::utils::pathJoin(path, cndNamePre);
          std::string cndPath = mars::utils::pathJoin(path, "shutdown.cnd");
          std::string cmd;
#ifdef __APPLE__
          std::string rockLaunch = "mac-rock-launch";
#else
          std::string rockLaunch = "rock-launch";
#endif
          if (mars::utils::pathExists(cndPathPre))
          {
            cmd = rockLaunch + " " + cndPathPre;
            printf("Call %s\n", cmd.c_str());
            system(cmd.c_str());
          }
          cmd = rockLaunch + " " + cndPath;
          printf("Call %s\n", cmd.c_str());
          system(cmd.c_str());
          break;
        }
        case MenuActions::IMPORT_CND:
        {
          QString fileName = QFileDialog::getOpenFileName(NULL, QObject::tr("Select Model"),
                                                          ".", QObject::tr("YAML syntax (*.cnd)"), 0,
                                                          QFileDialog::DontUseNativeDialog);
          if (!fileName.isNull())
          {
            importCND(fileName.toStdString());
          }
          break;
        }
        case MenuActions::SELECT_SERVERLESS: // Serverless
        {
            // 20221102 MS: If db is already set who deletes the existing backend? Possible memory leak
          db = new (db) ServerlessDB(mars::utils::pathJoin(env["AUTOPROJ_CURRENT_ROOT"], env["dbPath"].getString())); //Todo get this from a textfield
          break;
        }
        case MenuActions::SELECT_CLIENT: // Client
        {
            // 20221102 MS: If db is already set who deletes the existing backend? Possible memory leak
          db = new (db) RestDB();

          if (!db->isConnected())
          {
            std::string msg = "Server is not running! Please run server using command:\njsondb -d "+main_gui->getToolbarLineEditText(1);
            QMessageBox::warning(nullptr, "Warning", msg.c_str(), QMessageBox::Ok);
          }
          break;
        }
        case MenuActions::SELECT_MULTIDB: // MultiDbClient
        {
            // 20221102 MS: If db is already set who deletes the existing backend? Possible memory leak
          //TODO: implement
          std::cout << "integration of MultiDbClient still required" << std::endl;
          break;
        }
        case MenuActions::RELOAD_MODEL_FROM_DB: // Reload
        {
            // 20221102 MS: We also have a class variable model. Shadowing issues can arise.
          Model *model = dynamic_cast<Model *>(bagelGui->getCurrentModel());
          if (model)
          {
            bagelGui->closeCurrentTab();
            ConfigMap currentModel = model->getModelInfo();

            if (currentModel.hasKey("name"))
            {
              ConfigMap newModel = db->requestModel(currentModel["domain"], currentModel["name"], currentModel["versions"][0]["name"], true);
              // load updated model in new tab
              widget->loadModel(newModel);
            }
            else
              QMessageBox::warning(nullptr, "Warning", "Nothing to reload.", QMessageBox::Ok);
          }

          break;
        }
        case MenuActions::EXPORT_CND_TFENHANCE:
        {
          QString fileName = QFileDialog::getSaveFileName(NULL, QObject::tr("Select Model"),
                                                          "export.cnd", QObject::tr("YAML syntax (*.cnd)"), 0,
                                                          QFileDialog::DontUseNativeDialog);
     
         QString urdf_file = QFileDialog::getOpenFileName(NULL, QObject::tr("Select urdf_file"),
                                                          ".", QObject::tr("YAML syntax (*.urdf)"), 0,
                                                          QFileDialog::DontUseNativeDialog);
          if (!fileName.isNull())
          {
            ConfigMap map = bagelGui->createConfigMap();
            exportCnd(map, fileName.toStdString(), urdf_file.toStdString());
          }
          break;
        }
        default:
        {
            throw std::out_of_range("Cannot handle action " + std::to_string(action));
            break;
        }
    }
  }

  void ModelLib::createBagelTask()
  {
    ModelInterface *model = bagelGui->getCurrentModel();
    if (model)
    {
      QMessageBox message;
      ConfigMap localMap = model->getModelInfo();
      std::string domain = "software";
      std::string name = "behavior_graph::MotionControlTask";
      std::string version = localMap["version"][0]["name"];
      std::string graphFile = "tmp/bagel/" + localMap["name"].getString() + "/" + version + "/" + localMap["name"].getString() + ".yml";
      handleFilenamePrefix(&graphFile, env["wsd"]);

      // check if we can load a model as template
      ConfigMap map = db->requestModel(domain, name, "template");
      // if the map is empty create the model info
      if (map.empty())
      {
        message.setText("Unable to load template model for MotionControlTask!");
        message.exec();
        return;
      }
      map["versions"][0]["name"] = version;
      map["versions"][0]["maturity"] = "INPROGRESS";
      ConfigMap config;
      if (map["versions"][0]["defaultConfiguration"].hasKey("data"))
      {
        if (map["versions"][0]["defaultConfiguration"]["data"].isMap())
          config = map["versions"][0]["defaultConfiguration"]["data"];
        else
          config = ConfigMap::fromYamlString(map["versions"][0]["defaultConfiguration"]["data"]);
      }
      config["config"]["graphFilename"] = graphFile;
      map["versions"][0]["defaultConfiguration"]["data"] = config.toYamlString();
      bool success = db->storeModel(map);
      if (success)
      {
        message.setText("The Bagel Task was successfully stored!");
      }
      else
      {
        message.setText("The Bagel Task could not be stored!");
      }
      message.exec();
    }
  }

  void ModelLib::createBagelModel()
  {
    ModelInterface *model = bagelGui->getCurrentModel();
    if (model)
    {
      ConfigMap localMap = model->getModelInfo();
      std::string domain = "software";
      std::string type = "bagel::subgraph";
      std::string name = "bagel::" + localMap["name"].getString();
      std::string version = localMap["name"];
      // check if we can load the model from the database
      ConfigMap map = db->requestModel(domain, name, version);
      // if the map is empty create the model info
      if (map.empty())
      {
        map["domain"] = domain;
        map["name"] = name;
        map["type"] = type;
        map["versions"][0]["name"] = version;
        map["versions"][0]["maturity"] = "INPROGRESS";
      }
      widget->loadModel(map);

      // load smurf file and add components if not already included
      std::string smurfPath = "tmp/models/assembly/";
      handleFilenamePrefix(&smurfPath, env["wsd"].getString());
      smurfPath += version + "/" + localMap["versions"][0]["name"].getString() + "/smurf/";
      std::string smurfFile = smurfPath + version + ".smurf";
      ConfigMap smurfMap = ConfigMap::fromYamlFile(smurfFile);
      ConfigMap motorMap;
      if (smurfMap.hasKey("files"))
      {
        std::string file;
        for (auto it : smurfMap["files"])
        {
          file << it;
          if (file.find("motors") != std::string::npos)
          {
            motorMap = ConfigMap::fromYamlFile(smurfPath + "/" + file);
          }
        }
      }
      if (motorMap.hasKey("motors"))
      {
        ConfigMap versionMap;
        if (map.hasKey("versions"))
        {
          for (auto it : map["versions"])
          {
            if (it["name"] == version)
            {
              versionMap = it;
              break;
            }
          }
        }
        ConfigVector::iterator it = motorMap["motors"].begin();
        double step = 22.0;
        double n = (motorMap["motors"].size() * 1.) * step;
        for (; it != motorMap["motors"].end(); ++it)
        {
          std::string motorName = (*it)["name"];
          bool found = false;
          if (versionMap.hasKey("components"))
          {
            for (auto node : versionMap["components"]["nodes"])
            {
              if (node["name"] == motorName)
              {
                found = true;
                break;
              }
            }
          }
          if (!found)
          {
            addComponent("software", "PIPE", "v1.0.0", motorName);
            // todo: change the output interface name and toggle interface
            ConfigMap nodeMap = *(bagelGui->getNodeMap(motorName));
            nodeMap["outputs"][0]["interface"] = 1;
            nodeMap["outputs"][0]["interfaceExportName"] = motorName + "/des_angle";
            bagelGui->updateNodeMap(motorName, nodeMap);
          }

          n -= step;
        }
      }
      widget->loadType("software", "PIPE", "v1.0.0");
      widget->loadType("software", "SIN", "v1.0.0");
      widget->loadType("software", "ASIN", "v1.0.0");
      widget->loadType("software", "MOD", "v1.0.0");
      widget->loadType("software", "POW", "v1.0.0");
      widget->loadType("software", "TAN", "v1.0.0");
      widget->loadType("software", "COS", "v1.0.0");
      widget->loadType("software", "ABS", "v1.0.0");
      widget->loadType("software", "DIV", "v1.0.0");
      widget->loadType("software", "Timer", "v1.0.0");
      widget->loadType("software", "Modulated_Sine", "v1.0.0");
      widget->setEdition("software");
    }
  }

  void ModelLib::requestModel()
  {
    importToBagel = false;
    ImportDialog id(this, true);
    id.exec();
  }
  void ModelLib::addComponent()
  {
    importToBagel = false;
    ImportDialog id(this, false);
    id.exec();
  }

  void ModelLib::addComponent(std::string domain, std::string modelName, std::string version, std::string nodeName)
  {
    // create type name by using domain as namespace
    domain = mars::utils::tolower(domain);
    std::string type = modelName;

    widget->loadType(domain, modelName, version);
    Model *model = dynamic_cast<Model *>(bagelGui->getCurrentModel());
    if (model)
    {
      if (model->hasNodeInfo(type + "::" + version))
      {
        type = type + "::" + version;
      }
      if (nodeName.empty())
      {
        nodeName = modelName;
      }
      bagelGui->addNode(type, nodeName);
    }
  }

  void ModelLib::loadComponent(std::string domain, std::string modelName, std::string version)
  {
    ConfigMap map = db->requestModel(domain, modelName, version, !version.empty());
    std::cout << "loadComponent: " << map.toJsonString() << std::endl;
    if (importToBagel)
    {
      mechanicsToBagel(map);
    }
    else
    {
      widget->loadModel(map);
    }
  }

  void ModelLib::loadNodes(bagel_gui::BagelModel *model,
                           configmaps::ConfigMap &nodes, std::string path,
                           std::vector<std::string> *nodesFound)
  {
    ConfigVector::iterator it = nodes["nodes"].begin();
    for (; it != nodes["nodes"].end(); ++it)
    {
      if ((*it)["model"]["domain"] == "mechanics")
      {
        std::string modelName = (*it)["model"]["name"];
        std::string name = (*it)["name"];
        if (!path.empty())
        {
          name = path + "_" + name;
        }
        if (modelName == "motor_universal")
        {
          if (!model->hasNode(name))
          {
            bagelGui->addNode("OUTPUT", name);
          }
          nodesFound->push_back((*it)["name"]);
        }
        else
        {
          ConfigMap map;
          std::map<std::string, ConfigMap>::iterator mt = modelCache.find(modelName);
          if (mt != modelCache.end())
          {
            map = mt->second;
          }
          else
          {
            map = db->requestModel("mechanics", modelName, std::string("v1"), true);
            modelCache[modelName] = map;
          }
          handleModelMap(model, map, name);
        }
      }
    }
  }

  void ModelLib::handleModelMap(bagel_gui::BagelModel *model,
                                configmaps::ConfigMap &map,
                                std::string path)
  {
    ConfigMap cMap;
    std::vector<std::string> nodesFound;
    if (map.hasKey("versions"))
    {
      if (map["versions"][0].hasKey("components"))
      {
        cMap = ConfigMap::fromYamlString(map["versions"][0]["components"]);
      }
    }
    if (cMap.hasKey("nodes"))
    {
      loadNodes(model, cMap, path, &nodesFound);
    }
    // check if we have links in data
    if (map["versions"][0].hasKey("data"))
    {
      ConfigMap data;
      if (map["versions"][0]["data"].isMap())
        data = map["versions"][0]["mechanicsData"]["data"];
      else
        data = ConfigMap::fromYamlString(map["versions"][0]["data"]);
      if (data.hasKey("bagel_control"))
      {
        std::string bagelControl = data["bagel_control"];
        if (!model->hasNodeInfo(bagelControl))
        {
          // todo: handle path via config or database?
          QFileInfo fi("../mars_bagel/bagel/");
          model->loadSubgraphInfo(bagelControl, fi.absoluteFilePath().toStdString());
          bagelGui->updateNodeTypes();
        }
        std::string nodeName = path + "_" + bagelControl.substr(0, bagelControl.size() - 4);
        if (!model->hasNode(nodeName))
        {
          bagelGui->addNode(bagelControl, nodeName);
        }
        const ConfigMap *nodeMap_ = bagelGui->getNodeMap(nodeName);
        if (!nodeMap_)
        {
          return;
        }
        ConfigMap nodeMap = *nodeMap_;
        std::vector<std::string>::iterator it = nodesFound.begin();
        for (; it != nodesFound.end(); ++it)
        {
          if (!model->hasConnection(path + "_" + *it))
          {
            // create edge info if available
            ConfigVector::iterator it2 = nodeMap["outputs"].begin();
            for (; it2 != nodeMap["outputs"].end(); ++it2)
            {
              if ((*it2)["name"] == *it)
              {
                ConfigMap edge;
                edge["fromNode"] = nodeName;
                edge["fromNodeOutput"] = *it;
                edge["toNode"] = path + "_" + *it;
                edge["toNodeInput"] = "in1";
                edge["weight"] = 1.0;
                edge["smooth"] = true;
                bagelGui->addEdge(edge);
                break;
              }
            }
          }
        }
      }
    }
  }

  void ModelLib::mechanicsToBagel(configmaps::ConfigMap &map)
  {
    bagel_gui::BagelModel *model = dynamic_cast<bagel_gui::BagelModel *>(bagelGui->getCurrentModel());
    if (!model)
    {
      bagelGui->createView("bagel", "control");
      bagelGui->setSmoothLineMode();
      bagelGui->setLoadPath("../mars_bagel/bagel/");
      model = dynamic_cast<bagel_gui::BagelModel *>(bagelGui->getCurrentModel());
      if (!model)
      {
        return;
      }
    }
    modelCache.clear();
    handleModelMap(model, map, "");
  }

  void ModelLib::currentModelChanged(bagel_gui::ModelInterface *model)
  {
    widget->clear();
    if (model)
    {
      widget->setModelInfo(model->getModelInfo());
    }
  }

  void ModelLib::changeNodeVersion(const std::string &name)
  {
    versionChangeName = name;
    const ConfigMap *node_ = bagelGui->getNodeMap(name);
    if (!node_)
      return;
    ConfigMap node = *node_;
    VersionDialog vd(this);
    std::string domain = node["domain"];
    std::string type = node["modelName"];
    vd.requestComponent(domain, type);
    vd.exec();
  }

  void ModelLib::configureNode(const std::string &name)
  {
    ConfigMap node = *(bagelGui->getNodeMap(name));
    ConfigMap config;
    if (node["data"].hasKey("configuration"))
    {
      config = node["data"]["configuration"];
    }
    else if (node.hasKey("defaultConfiguration"))
    {
      config = node["defaultConfiguration"];
    }
    {
      ConfigureDialog cd(&config, env, node["modelName"], true, true);
      cd.resize(400, 400);
      cd.exec();
    }
    node["data"]["configuration"] = config;
    bagelGui->updateNodeMap(name, node);
  }

  void ModelLib::openConfigFile(const std::string &name)
  {
    ConfigMap node = *(bagelGui->getNodeMap(name));
    ConfigMap config;
    // check for selected bundle
    char *envs = getenv("ROCK_BUNDLE");
    if (envs)
    {
      // check for config file in bundle folder
      std::string bundleName = envs;
      envs = getenv("ROCK_BUNDLE_PATH");
      if (envs)
      {
        std::string bundlePath = envs;
        // only if both is set we continue
        std::string configFile = node["modelName"];
        configFile += ".yml";
        std::string path = mars::utils::pathJoin(bundlePath, bundleName);
        path = mars::utils::pathJoin(path, "config");
        path = mars::utils::pathJoin(path, "orogen");
        path = mars::utils::pathJoin(path, configFile);
        if (mars::utils::pathExists(path))
        {
          printf("found config file: %s\n", path.c_str());
          ConfigureDialog cd(NULL, env, node["modelName"], true, true, NULL, path);
          cd.resize(400, 400);
          cd.exec();
        }
      }
      else
      {
        printf("No bundle path set in env: ROCK_BUNDLE_PATH\n");
      }
    }
    else
    {
      printf("No bundle selected in env: ROCK_BUNDLE\n  Use bundle-sel to select one!\n");
    }
  }

  void ModelLib::configureComponents(const std::string &name)
  {
    ConfigMap node = *(bagelGui->getNodeMap(name));
    ConfigMap config;
    if (node["data"].hasKey("submodel"))
    {
      config["submodel"] = node["data"]["submodel"];
    }
    {
      ConfigureDialog cd(&config, env, node["modelName"], true, true);
      cd.resize(400, 400);
      cd.exec();
    }
    node["data"]["submodel"] = config["submodel"];
    bagelGui->updateNodeMap(name, node);
  }

  void ModelLib::configureOutPort(const std::string &nodeName, const std::string &portName)
  {
    openConfigureInterfaceDialog(nodeName, portName, "outputs");
  }

  void ModelLib::configureInPort(const std::string &nodeName, const std::string &portName)
  {
    openConfigureInterfaceDialog(nodeName, portName, "inputs");
  }

  void ModelLib::openConfigureInterfaceDialog(const std::string &nodeName,
                                              const std::string &portName,
                                              const std::string &portType)
  {
    if (portType != "outputs" &&
        portType != "inputs")
    {
      std::cout << "ModelLib::openConfigureInterfaceDialog: wrong portType!" << std::endl;
      return;
    }
    ConfigMap node = *(bagelGui->getNodeMap(nodeName));
    ConfigMap config;
    std::string type = node["xrock_type"];
    bool bagelType = matchPattern("bagel::*", type);
    ConfigVector::iterator it = node[portType].begin();
    ConfigItem *subMap = NULL;
    ConfigMap dropdown;
    for (; it != node[portType].end(); ++it)
    {
      if ((std::string)(*it)["name"] == portName)
      {
        try
        {
          if (bagelType)
          {
            std::cerr << "configure bagel port" << std::endl;
            std::vector<std::string> keys = {"data", "configuration", "interfaces", contextPortName};
            subMap = ConfigMapHelper::getSubItem(node, keys);
            if (portType == "inputs" and subMap and subMap->isMap())
            {
              dropdown["merge"][0] = "SUM";
              dropdown["merge"][1] = "PRODUCT";
              dropdown["merge"][2] = "MIN";
              dropdown["merge"][3] = "MAX";
              config = *subMap;
            }
            if (it->hasKey("interfaceExportName"))
            {
              config["interfaceExportName"] = (*it)["interfaceExportName"];
            }
            if (it->hasKey("initValue"))
            {
              config["initValue"] = (*it)["initValue"];
            }
          }
          else
          {
            config = ConfigMap::fromYamlString((*it)["defaultConfig"].getString());
          }
        }
        catch (...)
        {

          std::stringstream ss;
          ss << "defaultConfig is not a valid YAML ";
          QMessageBox::critical(nullptr, "Error", QString::fromStdString(ss.str()), QMessageBox::Ok);
          // what to do if the defaultConfig is not a valid YAML
        }
        break;
      }
    }
    {
      ConfigureDialog cd(&config, env, node["modelName"], true, false, &dropdown);
      cd.resize(400, 400);
      cd.exec();
    }
    try
    {
      if (bagelType)
      {
        if (config.hasKey("interfaceExportName"))
        {
          (*it)["interfaceExportName"] = config["interfaceExportName"];
        }
        if (config.hasKey("initValue"))
        {
          (*it)["initValue"] = config["initValue"];
        }
        if (subMap)
        {
          (*subMap)["merge"] = config["merge"];
          (*subMap)["bias"] = config["bias"];
          (*subMap)["default"] = config["default"];
        }
      }
      else
      {
        (*it)["defaultConfig"] = config.toYamlString();
      }
      bagelGui->updateNodeMap(nodeName, node);
    }
    catch (...)
    {
      std::stringstream ss;
      ss << "defaultConfig is not a valid YAML ";
      QMessageBox::critical(nullptr, "Error", QString::fromStdString(ss.str()), QMessageBox::Ok);
    }
  }

  /*
   * todo: handle node configuration
   *       warn if edges can not be reconnected (dropdown optional)
   */
  void ModelLib::selectVersion(const std::string &version)
  {
    Model *model = dynamic_cast<Model *>(bagelGui->getCurrentModel());
    if (model)
    {
      std::vector<ConfigMap> edgeList;
      ConfigMap graph = bagelGui->createConfigMap();
      for (auto it : graph["edges"])
      {
        if (it["fromNode"] == versionChangeName)
        {
          edgeList.push_back(it);
        }
        else if (it["toNode"] == versionChangeName)
        {
          edgeList.push_back(it);
        }
      }
      ConfigMap node = *(bagelGui->getNodeMap(versionChangeName));
      bagelGui->removeNode(versionChangeName);
      std::string domain = node["domain"];
      std::string name = node["modelName"];
      std::string versionName = version;
      std::string type = name + "::" + versionName;
      widget->loadType(domain, name, versionName);
      if (!model->hasNodeInfo(type))
      {
        type = name;
      }

      bagelGui->addNode(type, versionChangeName);
      return;
      ConfigMap nodeMap = *(bagelGui->getNodeMap(versionChangeName));
      // update node configuration
      {
        nodeMap["pos"] = node["pos"];
        for (auto it : node["inputs"])
        {
          ConfigVector::iterator it2 = nodeMap["inputs"].begin();
          for (; it2 != nodeMap["inputs"].end(); ++it2)
          {
            if ((*it2)["name"].getString() == it["name"].getString())
            {
              if (it.hasKey("interface"))
              {
                (*it2)["interface"] = it["interface"];
              }
              if (it.hasKey("interfaceExportName"))
              {
                (*it2)["interfaceExportName"] = it["interfaceExportName"];
              }
              if (it.hasKey("initValue"))
              {
                (*it2)["initValue"] = it["initValue"];
              }
              break;
            }
          }
        }
        for (auto it : node["outputs"])
        {
          ConfigVector::iterator it2 = nodeMap["outputs"].begin();
          for (; it2 != nodeMap["outputs"].end(); ++it2)
          {
            if ((*it2)["name"].getString() == it["name"].getString())
            {
              if (it.hasKey("interface"))
              {
                (*it2)["interface"] = it["interface"];
              }
              if (it.hasKey("interfaceExportName"))
              {
                (*it2)["interfaceExportName"] = it["interfaceExportName"];
              }
              break;
            }
          }
        }
        if (node.hasKey("data"))
        {
          nodeMap["data"].updateMap(node["data"]);
        }
      }

      bagelGui->updateNodeMap(versionChangeName, nodeMap);

      for (auto it : edgeList)
      {
        it.erase("vertices");
        it.erase("decoupleVertices");
        bagelGui->addEdge(it);
      }
    }
  }

  void trimMap(ConfigItem &item)
  {
    if (item.isMap())
    {
      ConfigMap::iterator it = item.beginMap();
      while (it != item.endMap())
      {
        if (it->second.isAtom())
        {
          std::string value = mars::utils::trim(it->second.toString());
          if (value.empty())
          {
            item.erase(it);
            it = item.beginMap();
          }
          else
          {
            ++it;
          }
        }
        else if (it->second.isMap() || it->second.isVector())
        {
          // todo: handle empty map
          trimMap(it->second);
          if (it->second.size() == 0)
          {
            item.erase(it);
            it = item.beginMap();
          }
          else
          {
            ++it;
          }
        }
        else
        {
          item.erase(it);
          it = item.beginMap();
        }
      }
    }
    else if (item.isVector())
    {

      ConfigVector::iterator it = item.begin();
      while (it != item.end())
      {
        if (it->isAtom())
        {
          std::string value = mars::utils::trim(it->toString());
          if (value.empty())
          {
            item.erase(it);
            it = item.begin();
          }
          else
          {
            ++it;
          }
        }
        else if (it->isMap() || it->isVector())
        {
          trimMap(*it);
          if (it->size() == 0)
          {
            item.erase(it);
            it = item.begin();
          }
          else
          {
            ++it;
          }
        }
        else
        {
          item.erase(it);
          it = item.begin();
        }
      }
    }
  }

  void ModelLib::exportCnd(const configmaps::ConfigMap &map_,
                           const std::string &filename, const std::string &urdf_file)
  {
    // ask user to pick a urdf file
    
    ConfigMap map = bagelGui->getCurrentModel()->getModelInfo();
    std::stringstream cnd_export;
    cnd_export << "export_cnd -m " << map["name"].getString()
               << " -v " << map["versions"][0]["name"].getString()
               << " -o " << filename;
    if(!urdf_file.empty()) 
    {
      cnd_export << " -t --tf_enhance -u " << urdf_file;
    }          
    if(dynamic_cast<ServerlessDB*>(db)){
      cnd_export << " -b Serverless ";
    }
    else if(dynamic_cast<RestDB*>(db)){
      cnd_export << " -b Client ";
    }
   // else if(dynamic_cast<MultiDB*>(db)){
    //  cnd_export << " -b multidb ";
    //}

    int ret = std::system(cnd_export.str().c_str());
    if(ret == EXIT_SUCCESS)
       QMessageBox::information(nullptr, "Export","Successfully exported",  QMessageBox::Ok);
    else
       QMessageBox::critical(nullptr, "Export", QString::fromStdString("Failed to export cnd with code: " + std::to_string(ret)), QMessageBox::Ok);

  }
  void ModelLib::exportCnd2(const configmaps::ConfigMap &map_,
                           const std::string &filename) 
  {
      ConfigMap map = map_;
       ConfigMap output;
       ConfigMap nameMap;
       ConfigMap dNameMap;
       bool haveMarsTask = false;
       bool compileDeployments = false;
       // handle file path and  node order
       for (auto node : map["nodes"])
       {
         if (node["type"] == "software::Deployment")
         {
           std::string name = node["name"];
           output["deployments"][name]["deployer"] = "orogen";
           output["deployments"][name]["process_name"] = name;
           output["deployments"][name]["hostID"] = "local";
           if (node.hasKey("softwareData") and node["softwareData"].hasKey("data") and
               node["softwareData"]["data"].hasKey("configuration"))
           {
             ConfigItem item(node["softwareData"]["data"]["configuration"]);
             trimMap(item);
             ConfigMap m = item;
             if (m.hasKey("deployer"))
             {
               output["deployments"][name]["deployer"] = m["deployer"];
             }
             if (m.hasKey("process_name"))
             {
               output["deployments"][name]["process_name"] = m["process_name"];
             }
             if (m.hasKey("hostID"))
             {
               output["deployments"][name]["hostID"] = m["hostID"];
             }
           }
           dNameMap[name] = output["deployments"][name]["process_name"];
         }
         else
         {
           if (node["domain"] == "software")
           {
             std::string name = node["name"];
             nameMap[name] = 1;
             ConfigItem item(node["softwareData"]["data"]);
             trimMap(item);
             ConfigMap m = item;
             if (m.hasKey("properties"))
             {
               ConfigVector props;
               for (auto prop : m["properties"])
               {
                 if (prop.hasKey("Value"))
                 {
                   props.push_back(prop);
                 }
               }
               m.erase("properties");
               if (props.size() > 0)
               {
                 m["properties"] = props;
               }
             }
             if (m.hasKey("configuration"))
             {
               ConfigMap m2 = m["configuration"];
               m.erase("configuration");
               m.updateMap(m2);
             }
             if (m.hasKey("description"))
             {
               m.erase("description");
             }
             std::string type = node["modelName"];
             if (type == "mars::Task")
             {
               haveMarsTask = true;
             }
             output["tasks"][name] = m;
             output["tasks"][name]["type"] = type;
             if (node.hasKey("parentName") and node["parentName"].getString().size() > 0)
             {
               std::string parent = node["parentName"];
               std::vector<std::string> arrName = mars::utils::explodeString(':', node["type"]);
               std::string process_name = "orogen_default_" + arrName[0] + "__" + arrName[2];
               output["deployments"][parent]["taskList"][name] = process_name;
               if (output["deployments"][parent]["taskList"].size() > 1)
               {
                 compileDeployments = true;
                 output["deployments"][parent]["process_name"] = dNameMap[parent];
               }
               else
               {
                 output["deployments"][parent]["process_name"] = process_name;
               }
             }
             else
             {
               std::string depName = name + "_deployment";
               output["deployments"][depName]["deployer"] = "orogen";
               output["deployments"][depName]["hostID"] = "local";
               std::vector<std::string> arrName = mars::utils::explodeString(':', node["type"]);
               std::string process_name = "orogen_default_" + arrName[0] + "__" + arrName[2];
               output["deployments"][depName]["taskList"][name] = process_name;
               output["deployments"][depName]["process_name"] = process_name;
               dNameMap[depName] = depName;
             }
           }
         }
       }
       if (compileDeployments)
       {
         for (ConfigMap::iterator it = output["deployments"].beginMap();
              it != output["deployments"].endMap(); ++it)
         {
           for (ConfigMap::iterator nt = it->second["taskList"].beginMap();
                nt != it->second["taskList"].endMap(); ++nt)
           {
             nt->second = nt->first;
           }
           output["deployments"][it->first]["process_name"] = dNameMap[it->first];
         }
       }
       int i = 0;
       for (auto edge : map["edges"])
       {
         ConfigMap m;
         if (edge.hasKey("transport"))
         {
           m["transport"] = edge["transport"];
         }
         if (edge.hasKey("type"))
         {
           m["type"] = edge["type"];
         }
         if (edge.hasKey("size"))
         {
           m["size"] = edge["size"];
         }
         std::string name = edge["fromNode"];
         if (!nameMap.hasKey(name))
           continue;
         // remove domian namespace
         m["from"]["task_id"] = name;
         m["from"]["port_name"] = edge["fromNodeOutput"];
         // remove domian namespace
         name << edge["toNode"];
         if (!nameMap.hasKey(name))
           continue;
         m["to"]["task_id"] = name;
         m["to"]["port_name"] = edge["toNodeInput"];
         char buffer[100];
         // todo: use snprintf
         sprintf(buffer, "%d", i++);
         output["connections"][std::string(buffer)] = m;
       }
       output.toYamlFile(filename);
       if (haveMarsTask)
       {
         // generate pre cnd because we have to first load mars::Task then
         // the plugin tasks
         for (ConfigMap::iterator it = output["tasks"].beginMap();
              it != output["tasks"].endMap(); ++it)
         {
           if (it->second["type"] != "mars::Task")
           {
             it->second["state"] = "PRE_OPERATIONAL";
           }
         }
         std::string preFile = filename;
         mars::utils::removeFilenameSuffix(&preFile);
         if (output.hasKey("connections"))
         {
           output.erase("connections");
         }
         output.toYamlFile(preFile + "_pre.cnd");
       }
       // write shutdown cnd
       std::string shutdownFile = mars::utils::pathJoin(mars::utils::getPathOfFile(filename), "shutdown.cnd");
       FILE *f = fopen(shutdownFile.c_str(), "w");
       fprintf(f, "deployments:\n\ntasks:\n\nconnections:\n\n");
       fclose(f);
}

  void ModelLib::importCND(const std::string &fileName)
  {
    ConfigMap map;
    ConfigMap cnd = ConfigMap::fromYamlFile(fileName);
    std::string name = fileName;
    mars::utils::removeFilenamePrefix(&name);
    mars::utils::removeFilenameSuffix(&name);
    map["name"] = name;
    map["domain"] = "SOFTWARE";
    map["type"] = "CND";
    map["versions"][0]["name"] = "v0.0.1";
    map["versions"][0]["projectName"] = "";
    map["versions"][0]["designedBy"] = "";
    map["versions"][0]["date"] = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    map["versions"][0]["components"]["nodes"] = ConfigVector();
    map["versions"][0]["components"]["edges"] = ConfigVector();
    if (cnd.hasKey("tasks"))
    {
      for (auto it : (ConfigMap)cnd["tasks"])
      {
        std::cerr << "task name: " << it.first.c_str() << std::endl;
        ConfigMap node;
        node["name"] = it.first.c_str();
        node["model"]["domain"] = "SOFTWARE";
        node["model"]["version"] = "v0.0.1";
        node["model"]["name"] = it.second["type"];
        map["versions"][0]["components"]["nodes"].push_back(node);
        ConfigMap config;
        config["data"] = it.second.toYamlString();
        config["name"] = node["name"];
        map["versions"][0]["components"]["configuration"]["nodes"].push_back(config);
      }
    }
    map["modelPath"] = mars::utils::getPathOfFile(fileName);
    map.toYamlFile("da.yml");
    widget->loadModel(map);
  }

  void ModelLib::nodeContextClicked(const std::string name)
  {

    if (name == "change version")
    {
      changeNodeVersion(contextNodeName);
    }
    else if (name == "configure node")
    {
      configureNode(contextNodeName);
    }
    else if (name == "configure components")
    {
      configureComponents(contextNodeName);
    }
    else if (name == "reset configuration")
    {
      Model *model = dynamic_cast<Model *>(bagelGui->getCurrentModel());
      if (model)
      {
        ConfigMap map = *(bagelGui->getNodeMap(contextNodeName));
        model->resetConfig(map);
        bagelGui->updateNodeMap(contextNodeName, map);
      }
    }
    else if (name == "open ROCK config file")
    {
      openConfigFile(contextNodeName);
    }
    else if (name == "open model")
    {
      ConfigMap node = *(bagelGui->getNodeMap(contextNodeName));
      std::string domain = node["domain"];
      std::string version = node["modelVersion"];
      std::string model_name = node["modelName"];
      loadComponent(domain, model_name, version);
    }
    else if (name == "show description")
    {
      ConfigMap node = *(bagelGui->getNodeMap(contextNodeName));
      std::string domain = node["domain"];
      std::string version = node["modelVersion"];
      std::string model_name = node["modelName"];
      QWebView *doc = new QWebView();
      doc->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
      widget->connect(doc, SIGNAL(linkClicked(const QUrl &)), widget, SLOT(openUrl(const QUrl &)));
      ConfigMap modelMap = db->requestModel(domain, model_name, version, true);
      if (modelMap["versions"][0].hasKey("data"))
      {
        {
          ConfigMap dataMap;
          if (modelMap["versions"][0]["data"].isMap())
            dataMap = modelMap["versions"][0]["data"];
          else
            dataMap = ConfigMap::fromYamlString(modelMap["versions"][0]["data"]);
          if (dataMap.hasKey("description"))
          {
            if (dataMap["description"].hasKey("markdown"))
            {
              std::string md = dataMap["description"]["markdown"];
              doc->setHtml(getHtml2(md).c_str());
            }
          }
        }
      }
      doc->show();
    }
    else if (name == "apply configuration")
    {
      ConfigMap node = *(bagelGui->getNodeMap(contextNodeName));
      applyConfiguration(node);
    }
  }

  void ModelLib::inPortContextClicked(const std::string name)
  {
    if (name == "configure interface")
    {
      configureInPort(contextNodeName, contextPortName);
      return;
    }
    double biasValue = 0.0;
    std::string merge;
    if (name == "SUM merge")
    {
      merge = "SUM";
    }
    else if (name == "PRODUCT merge")
    {
      merge = "PRODUCT";
      biasValue = 1.0;
    }
    else if (name == "MIN merge")
    {
      merge = "MIN";
      biasValue = 1000;
    }
    else if (name == "MAX merge")
    {
      merge = "MAX";
      biasValue = -1000;
    }

    ConfigMap node = *(bagelGui->getNodeMap(contextNodeName));
    node["data"]["configuration"]["interfaces"][contextPortName]["merge"] = merge;
    node["data"]["configuration"]["interfaces"][contextPortName]["bias"] = biasValue;
    bagelGui->updateNodeMap(contextNodeName, node);
  }

  void ModelLib::outPortContextClicked(const std::string name)
  {
    if (name == "configure interface")
    {
      configureOutPort(contextNodeName, contextPortName);
    }
  }

  std::vector<std::string> ModelLib::getNodeContextStrings(const std::string &name)
  {
    // get node map
    // check domain foo
    // save context name
    std::vector<std::string> r;
    r.push_back("change version");
    r.push_back("configure node");
    r.push_back("configure components");
    r.push_back("reset configuration");
    r.push_back("open ROCK config file");
    r.push_back("apply configuration");
    r.push_back("open model");
    r.push_back("show description");
    // todo: add bundle handling for node configuration
 
    contextNodeName = name;
    return r;
  }

  std::vector<std::string> ModelLib::getInPortContextStrings(const std::string &nodeName, const std::string &portName)
  {
    // get node map
    // check domain foo
    // save context name
    contextNodeName = nodeName;
    contextPortName = portName;
    std::vector<std::string> r;
    r.push_back("configure interface");
    ConfigMap node = *(bagelGui->getNodeMap(nodeName));
    std::string type = node["xrock_type"];
    if (matchPattern("bagel::*", type))
    {
      r.push_back("SUM merge");
      r.push_back("PRODUCT merge");
      r.push_back("MIN merge");
      r.push_back("MAX merge");
    }
    return r;
  }

  std::vector<std::string> ModelLib::getOutPortContextStrings(const std::string &nodeName, const std::string &portName)
  {
    // get node map
    // check domain foo
    // save context name
    contextNodeName = nodeName;
    contextPortName = portName;
    std::vector<std::string> r;
    r.push_back("configure interface");
    return r;
  }

  void ModelLib::applyConfiguration(configmaps::ConfigMap &map)
  {
    if (map["domain"] != "software")
      return;
    Model *model = dynamic_cast<Model *>(bagelGui->getCurrentModel());
    if (!model)
      return;
    std::string configFile = "temp_task_config.yml";
    std::string modelFile = "temp_task_model.yml";
    // 1. get configuration from map
    // 2. store configuration in temp yaml file
    std::string cmd = "xrock-resolve-ports";
#ifdef __APPLE__
    {
      std::string c = getenv("AUTOPROJ_CURRENT_ROOT");
      c += "/install/bin/";
      cmd = "DYLD_LIBRARY_PATH=$MYLD_LIBRARY_PATH ruby " + c + cmd;
    }
#endif
    if (map.hasKey("data") and
        map["data"].hasKey("configuration") and
        map["data"]["configuration"].hasKey("config"))
    {
      std::string yaml = "--- name:default\n" + map["data"]["configuration"]["config"].toYamlString();
      std::ofstream file;
      file.open(configFile);
      file << yaml;
      file.close();
      cmd += " -c " + configFile;
      cmd += " -o " + modelFile + " ";
      cmd += map["modelName"].getString();
      cmd += " default";
    }
    else if (map.hasKey("data") and
             map["data"].hasKey("configuration") and
             map["data"]["configuration"].hasKey("config_names"))
    {
      cmd += " -o " + modelFile + " ";
      cmd += map["modelName"].getString();
      for (auto it : map["data"]["configuration"]["config_names"])
      {
        cmd += " " + (std::string)it;
      }
    }
    else
    {
      return;
    }
    // 3. execute rock-instantiate -c temp_file.yml -o temp_model.yml modelname default
    printf("execute: %s\n", cmd.c_str());
    system(cmd.c_str());
    // 4. load orogen model as new version of modelname
    if (!mars::utils::pathExists(modelFile))
    {
      printf("ERROR: executing rock-instantiate\n");
      return;
    }
    configmaps::ConfigMap info = model->getModelInfo();
    std::string version = info["name"];
    versionChangeName << map["name"];
    version += "_" + info["versions"][0]["name"].getString() + "_" + versionChangeName;
    cmd = "orogen_to_xrock --modelname " + map["modelName"].getString() + " --model_file " + modelFile + " --version_name " + version;
#ifdef __APPLE__
    {
      std::string c = getenv("AUTOPROJ_CURRENT_ROOT");
      c += "/install/bin/";
      cmd = "DYLD_LIBRARY_PATH=$MYLD_LIBRARY_PATH python " + c + cmd;
    }
#endif
    printf("execute: %s\n", cmd.c_str());
    system(cmd.c_str());
    // 5. switch node to new version
    selectVersion(version);
  }

  void ModelLib::cfgUpdateProperty(mars::cfg_manager::cfgPropertyStruct p)
  {
    if (p.paramId == dbAddress_paramId)
    {
      db->set_dbAddress(p.sValue);
    }
    else if (p.paramId == dbUser_paramId)
    {
    }
  }

} // end of namespace xrock_gui_model

DESTROY_LIB(xrock_gui_model::ModelLib)
CREATE_LIB(xrock_gui_model::ModelLib)
