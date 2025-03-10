/**
 * \file XRockGUI.cpp
 * \author Malte Langosz
 * \brief
 */

#include "XRockGUI.hpp"
#include "ComponentModelInterface.hpp"
#include "ComponentModelEditorWidget.hpp"
#include "ImportDialog.hpp"
#include "BasicModelHelper.hpp"
#include "FileDB.hpp"

#include "MultiDBConfigDialog.hpp"
#include "VersionDialog.hpp"
#include "BuildModuleDialog.hpp"
#include "ConfigureDialog.hpp"
#include "ConfigMapHelper.hpp"

#include "plugins/MARSIMUConfig.hpp"

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
#include <iomanip> // for std::put_time()

#include "utils/WaitCursorRAII.hpp"
#include <smurf_parser/SMURFParser.h>

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

    XRockGUI::XRockGUI(lib_manager::LibManager *theManager) : lib_manager::LibInterface(theManager), ioLibrary(NULL)
    {
        initConfig();
        initBagelGui();
        initMainGui();

        // register config plugins
        ConfigureDialogLoader *l = new MARSIMUConfigLoader();
        configPlugins["mars::IMU"] = l;

        loadSettingsFromFile("generalsettings.yml");
        loadModelFromParameter();
    }

    void XRockGUI::initConfig()
    {
        cfg = libManager->getLibraryAs<mars::cfg_manager::CFGManagerInterface>("cfg_manager", true);
        std::string confDir = ".";

        resourcesPath = XROCK_DEFAULT_RESOURCES_PATH;
        if (cfg)
        {
            std::string resourcesPathConfig = cfg->getOrCreateProperty("Preferences",
                                                                       "resources_path",
                                                                       "")
                                                  .sValue;
            if (resourcesPathConfig != "")
            {
                resourcesPath = resourcesPathConfig;
            }
            cfg->getPropertyValue("Config", "config_path", "value", &confDir);
            bool noXRockIOLib = false;
            std::string arg3;
            cfg->getPropertyValue("Config", "arg3", "value", &arg3);
            if(arg3 == "--no-xrock")
            {
                noXRockIOLib = true;
            }
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
                if (env["dbType"] == "Client")
                {
                    defaultAddress = "http://localhost:8183";
                }
            }
            else
            {
                env["dbType"] = "FileDB";
            }

            std::string confDir2 = confDir + "/XRockGUI.yml";
            std::string confDir3 = confDir + "/MultiDB.yml";
            if (mars::utils::pathExists(confDir2))
            {
                cfg->loadConfig(confDir2.c_str());
            }
            if (mars::utils::pathExists(confDir3))
            {
                cfg->loadConfig(confDir3.c_str());
            }
            mars::cfg_manager::cfgPropertyStruct prop_dbAddress;
            prop_dbAddress = cfg->getOrCreateProperty("XRockGUI", "dbAddress",
                                                      defaultAddress, this);
            env["backend"] = env["dbType"];
	    ioLibrary = NULL;
	    if(!noXRockIOLib)
	    {
	        ioLibrary = libManager->getLibraryAs<XRockIOLibrary>("xrock_io_library", false);
	    }
            if(ioLibrary)
            {
                ConfigMap dbConfig = ioLibrary->getDefaultConfig();
                if(!dbConfig.empty())
                {
                    fprintf(stderr, "---    default config\n%s", dbConfig.toYamlString().c_str());
                    std::string dbType = dbConfig.begin()->first;
                    ConfigMap &config = dbConfig.begin()->second;
                    env["backend"] = dbType;
                    if(dbType == "Serverless")
                    {
                        env["dbType"] = "Serverless";
                        env["dbPath"] = config["dbPath"];
                        env["dbGraph"] = config["dbGraph"];
                        db.reset(ioLibrary->getDB(env));
                    }
                    else if(dbType == "Client")
                    {
                        env["dbType"] = "Client";
                        db.reset(ioLibrary->getDB(env));
                        db->setDbAddress(config["url"]);
                    }
                    else if(dbType == "MultiDbClient")
                    {
                        env["dbType"] = "MultiDbClient";
                        env["multiDBConfig"] = config.toJsonString();
                        db.reset(ioLibrary->getDB(env));
                        fprintf(stderr, "---    Set MultiDB from default config\n");
                    }
                    else
                    {
                        // todo: print error config db key wrong
                        db.reset(ioLibrary->getDB(env));
                    }
                }
                else
                {
                    db.reset(ioLibrary->getDB(env));
                }
            }
            else
            {
                env["backend"] = "FileDB";
                env["dbType"] = "FileDB";
                // if we don't have a ioLibrary we only support FileDB
               db.reset(new FileDB());
            }
            if(env["dbType"] == "FileDB")
            {
                prop_dbAddress.sValue = mars::utils::pathJoin(confDir, prop_dbAddress.sValue);
                db->setDbAddress(prop_dbAddress.sValue);
                dbAddress_paramId = prop_dbAddress.paramId;
            }
        }
        else
            std::cerr << "Error: Failed to load library cfg_manager!" << std::endl;
    }

    void XRockGUI::initBagelGui()
    {
        bagelGui = libManager->getLibraryAs<BagelGui>("bagel_gui");
        if (bagelGui)
        {
            // Register this gui
            bagelGui->addPlugin(this);
            // NOTE: addModelInterface() is actually a registerModelInterface() function to setup a factory
            ComponentModelInterface* model = new ComponentModelInterface(bagelGui, this);
            if(env["dbType"] == "FileDB")
            {
                model->setSimpleTypeGen();
            }

            bagelGui->addModelInterface("xrock", model);
            // Preload the canvas with already defined models
            if (env.hasKey("initLoadModels") and (bool)env["initLoadModels"] == true)
            {
                std::vector<std::pair<std::string, std::string>> models = db->requestModelListByDomain("SOFTWARE");
                for(auto it: models)
                {
                    ConfigMap modelMap = db->requestModel("SOFTWARE", it.first, "");
                    model->addNodeInfo(model->deriveTypeFromNodeInfo(modelMap), modelMap);
                }
            }
        }
        else
        {
            std::cerr << "ERROR: XRockGUI: was not able to get bagel_gui!" << std::endl;
        }
    }

    void XRockGUI::initMainGui()
    {
        gui = libManager->getLibraryAs<mars::main_gui::GuiInterface>("main_gui");
        if (gui)
        {
            const std::string icon = mars::utils::pathJoin(resourcesPath, "xrock_gui_model/resources/images/");
            gui->addGenericMenuAction("../File/Import/Model", static_cast<int>(MenuActions::LOAD_MODEL), this);
            gui->addGenericMenuAction("../File/Import/CNDModel", static_cast<int>(MenuActions::IMPORT_CND), this);
            gui->addGenericMenuAction("../File/Export/Model", static_cast<int>(MenuActions::SAVE_MODEL), this);
            gui->addGenericMenuAction("../File/Export/CNDModel", static_cast<int>(MenuActions::EXPORT_CND), this);
            gui->addGenericMenuAction("../File/Export/CNDModel With tf_enhance", static_cast<int>(MenuActions::EXPORT_CND_TFENHANCE), this);
            gui->addGenericMenuAction("../Database/New Model", static_cast<int>(MenuActions::NEW_MODEL), this);
            gui->addGenericMenuAction("../Database/Add Component", static_cast<int>(MenuActions::ADD_COMPONENT_FROM_DB), this);
            gui->addGenericMenuAction("../Database/Store Model", static_cast<int>(MenuActions::STORE_MODEL_TO_DB), this);
            gui->addGenericMenuAction("../Database/Load Model", static_cast<int>(MenuActions::LOAD_MODEL_FROM_DB), this);
            gui->addGenericMenuAction("../Windows/ComponentModelEditorWidget", static_cast<int>(MenuActions::TOGGLE_MODEL_WIDGET), this);
            gui->addGenericMenuAction("../Expert/Edit Description", static_cast<int>(MenuActions::EDIT_MODEL_DESCRIPTION), this);
            gui->addGenericMenuAction("../Expert/Edit Local Map", static_cast<int>(MenuActions::EDIT_LOCAL_MAP), this);
            gui->addGenericMenuAction("../Expert/Create Bagel Model", static_cast<int>(MenuActions::CREATE_BAGEL_MODEL), this);
            gui->addGenericMenuAction("../Expert/Create Bagel Task", static_cast<int>(MenuActions::CREATE_BAGEL_TASK), this);
            gui->addGenericMenuAction("../Actions/New Model", static_cast<int>(MenuActions::NEW_MODEL), this, 0,
                                      icon +"new_model.png", true);
            gui->addGenericMenuAction("../Actions/Load Model", static_cast<int>(MenuActions::LOAD_MODEL_FROM_DB), this, 0,
                                      icon + "load.png", true);
            gui->addGenericMenuAction("../Actions/Add Component", static_cast<int>(MenuActions::ADD_COMPONENT_FROM_DB), this, 0,
                                      icon + "add.png", true);
            gui->addGenericMenuAction("../Actions/Save Model", static_cast<int>(MenuActions::STORE_MODEL_TO_DB), this, 0,
                                      icon + "save.png", true);
            gui->addGenericMenuAction("../Actions/Reload", static_cast<int>(MenuActions::RELOAD_MODEL_FROM_DB), this, 0,
                                      icon + "reload.png", true);
            gui->addGenericMenuAction("../Actions/Remove", static_cast<int>(MenuActions::REMOVE_MODEL_FROM_DB), this, 0,
                                      icon + "remove.png", true);
            gui->addGenericMenuAction("../Actions/Build", static_cast<int>(MenuActions::BUILD_MODULE_TO_DB), this, 0,
                                      icon + "build.png", true);
            gui->addGenericMenuAction("../Implements/Abstract_gui", static_cast<int>(MenuActions::RUN_ABSTRACT_GUI), this, 0,
                                      icon + "abstract.png", true);
            gui->addGenericMenuAction("../Edit/Global Variabls/Edit", static_cast<int>(MenuActions::EDIT_GLOBAL_VARIABLES), this, 0, "", true);
            gui->addGenericMenuAction("../Edit/Global Variabls/Load from Model", static_cast<int>(MenuActions::EDIT_LOAD_GLOBAL_VARIABLES), this, 0, "", true);
            gui->addGenericMenuAction("../Edit/Global Variabls/Store to Model", static_cast<int>(MenuActions::EDIT_STORE_GLOBAL_VARIABLES), this, 0, "", true);
            gui->addGenericMenuAction("../Edit/Frames/Edit", static_cast<int>(MenuActions::EDIT_FRAMES), this, 0, "", true);
            gui->addGenericMenuAction("../Edit/Frames/Load from Smurf", static_cast<int>(MenuActions::EDIT_LOAD_FRAMES_FROM_SMURF), this, 0, "", true);
            gui->addGenericMenuAction("../Edit/Frames/Load from Model", static_cast<int>(MenuActions::EDIT_LOAD_FRAMES), this, 0, "", true);
            gui->addGenericMenuAction("../Edit/Frames/Store to Model", static_cast<int>(MenuActions::EDIT_STORE_FRAMES), this, 0, "", true);

            mars::main_gui::MainGUI *mainGui = dynamic_cast<mars::main_gui::MainGUI *>(gui);
            mainGui->mainWindow_p()->setWindowIcon(QIcon(":/images/xrock_gui.ico"));

            toolbarBackend = new ToolbarBackend(this, gui);

            widget = new ComponentModelEditorWidget(cfg, bagelGui, this);
            if (!widget->getHiddenCloseState())
            {
                gui->addDockWidget((void *)widget, 1);
                bagelGui->updateViewSize();
            }
            //std::string loadGraph;
            //cfg->getPropertyValue("Config", "model", "value", &loadGraph);
            //if (!loadGraph.empty())
            //{
            //    widget->loadModel(loadGraph);
            //}
            bagelGui->setSmoothLineMode();
        }
        else
        {
            std::cerr << "XRockGUI: was not able to get main_gui" << std::endl;
        }
    }

    XRockGUI::~XRockGUI()
    {
        widget->deinit();
        if (gui)
            libManager->releaseLibrary("main_gui");
        if (bagelGui)
            libManager->releaseLibrary("bagel_gui");
        if(ioLibrary)
            libManager->releaseLibrary("xrock_io_library");
        if (cfg)
        {
            std::string confDir = ".";
            cfg->getPropertyValue("Config", "config_path", "value", &confDir);
            confDir += "/XRockGUI.yml";
            cfg->writeConfig(confDir.c_str(), "XRockGUI");
            libManager->releaseLibrary("cfg_manager");
        }
        delete toolbarBackend;
    }

    void XRockGUI::loadStartModel()
    {
        if (cfg)
        {
            std::string domain = "";
            std::string modelName = "";
            std::string version = "";
            cfg->getPropertyValue("Config", "domain", "value", &domain);
            cfg->getPropertyValue("Config", "model", "value", &modelName);
            cfg->getPropertyValue("Config", "version", "value", &version);
            if (domain != std::string("") &&
                modelName != std::string("") &&
                version != std::string("")) 
            {
                // set modelInfo with given information
                ConfigMap modelInfo;
                modelInfo["name"] = modelName;
                modelInfo["type"] = std::string("system_modelling::") +  std::string("::Network");
                modelInfo["domain"] = domain;
                modelInfo["data"] = std::string("");
                modelInfo["version"] = version;
                modelInfo["interfaces"] = std::string("");
                modelInfo["editable_interfaces"] = false;
                modelInfo["layouts"] = std::string("");
                //widget->update(modelInfo);
            }
            else
            {
                std::cerr << "loadStartModel: no model given" << std::endl;
            }
        }
    }

    void XRockGUI::loadModelFromParameter()
    {
        if (cfg)
        {
            std::string domain = "";
            std::string modelName = "";
            std::string version = "";
            cfg->getPropertyValue("Config", "domain", "value", &domain);
            cfg->getPropertyValue("Config", "model", "value", &modelName);
            cfg->getPropertyValue("Config", "version", "value", &version);
            if (domain != std::string("") &&
                modelName != std::string("") &&
                version != std::string(""))
            {
                loadComponentModel(domain, modelName, version);
            }
            else
            {
                std::cerr << "loadStartModel: no model given" << std::endl;
            }
        }
    }

    void XRockGUI::loadSettingsFromFile(const std::string &filename)
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

    void XRockGUI::menuAction(int action, bool checked)
    {
        try
        {
            switch (static_cast<MenuActions>(action))
            {
            case MenuActions::LOAD_MODEL:
            {
               
                QString fileName = QFileDialog::getOpenFileName(NULL, QObject::tr("Select Model File"),
                                                                ".", QObject::tr("YAML syntax (*.yml)"), 0,
                                                                QFileDialog::DontUseNativeDialog);

                ConfigMap map = configmaps::ConfigMap::fromYamlFile(fileName.toStdString());
                BasicModelHelper::convertFromLegacyModelFormat(map);
                loadComponentModelFrom(map);

                break;
            }
            case MenuActions::SAVE_MODEL:
            {
                ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());
                if (!model)
                    return;
                ConfigMap map = model->getModelInfo();
                QString fileName = QString::fromStdString(map["name"].getString() + ".yml");
                fileName = QFileDialog::getSaveFileName(NULL, QObject::tr("Select Model File"),
                                                        fileName, QObject::tr("YAML syntax (*.yml)"), 0,
                                                        QFileDialog::DontUseNativeDialog);
                BasicModelHelper::convertToLegacyModelFormat(map);
                map.toYamlFile(fileName.toStdString());
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
                if (!storeComponentModel())
                {
                    QMessageBox::critical(nullptr, "Error", "Could not store component model to database", QMessageBox::Ok);
                    break;
                }
                QMessageBox::information(nullptr, "Success", "Component model has been successfully stored into database", QMessageBox::Ok);
                break;
            }
            case MenuActions::BUILD_MODULE_TO_DB: // save and build module
            {
                ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());
                if (!model)
                    return;
                BuildModuleDialog dialog(this);
                dialog.exec();
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
                addComponent();
                break;
            }
            case MenuActions::NEW_MODEL: // create new, empty model
            {
                newComponentModel();
                break;
            }
            case MenuActions::LOAD_MODEL_FROM_DB: // load model from database
            {
                requestModel();
                break;
            }
            case MenuActions::EDIT_LOCAL_MAP:
            {
                ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());
                if (!model)
                    return;
                ConfigMap basicModel = model->getModelInfo();
                {
                    ConfigureDialog cd(&basicModel, env, "Basic Model", true, true);
                    cd.resize(400, 400);
                    cd.exec();
                }
                model->setModelInfo(basicModel);
                break;
            }

            // Edit->Global Variables
            case MenuActions::EDIT_GLOBAL_VARIABLES:
            {
                ConfigMap globalMap = bagelGui->getGlobalConfig();
                ConfigMap map;
                if(globalMap.hasKey("globalVariables"))
                {
                    map = globalMap["globalVariables"];
                }
                {
                    ConfigureDialog cd(&map, env, "Global Variables", true, true);
                    cd.resize(400, 400);
                    cd.exec();
                }
                globalMap["globalVariables"] = map;
                bagelGui->setGlobalConfig(globalMap);
                break;
            }
            case MenuActions::EDIT_LOAD_GLOBAL_VARIABLES:
            {

                ModelInterface *model = bagelGui->getCurrentModel();
                if (model)
                {
                    ConfigMap basicModel = model->getModelInfo();
                    if(basicModel["versions"][0].hasKey("data") &&
                       basicModel["versions"][0]["data"].hasKey("globalVariables"))
                    {
                        ConfigMap globalConfig = bagelGui->getGlobalConfig();
                        globalConfig["globalVariables"] = basicModel["versions"][0]["data"]["globalVariables"];
                        bagelGui->setGlobalConfig(globalConfig);
                    }

                }
                break;
            }
            case MenuActions::EDIT_STORE_GLOBAL_VARIABLES:
            {

                ModelInterface *model = bagelGui->getCurrentModel();
                if (model)
                {
                    ConfigMap basicModel = model->getModelInfo();
                    ConfigMap globalMap = bagelGui->getGlobalConfig();
                    basicModel["versions"][0]["data"]["globalVariables"] = globalMap["globalVariables"];
                    model->setModelInfo(basicModel);
                }
                break;
            }

            // Edit->Frames
            case MenuActions::EDIT_FRAMES:
            {
                ConfigMap globalMap = bagelGui->getGlobalConfig();
                ConfigMap map;
                map["frames"] = ConfigVector();
                if(globalMap.hasKey("frameNames"))
                {
                    map["frames"] = globalMap["frameNames"];
                }
                {
                    ConfigureDialog cd(&map, env, "Frames", true, true);
                    cd.resize(400, 400);
                    cd.exec();
                }
                if(map.hasKey("frames"))
                {
                    globalMap["frameNames"] = map["frames"];
                }
                bagelGui->setGlobalConfig(globalMap);
                break;
            }
            case MenuActions::EDIT_LOAD_FRAMES_FROM_SMURF:
            {
                ConfigMap map;
                map["prefix"] = "";
                map["filename"] = "";
                ConfigMap pattern;
                pattern["pattern"][0] = "../filename";
                {
                    ConfigureDialog cd(&map, env, "Load Frames From Smurf", true, false, NULL, &pattern);
                    cd.resize(400, 400);
                    cd.exec();
                }
                urdf::ModelInterfaceSharedPtr model;
                ConfigMap entityconfig;
                std::string filename = map["filename"];
                std::string path = getPathOfFile(filename);
                path = pathJoin(getCurrentWorkingDir(), path);
                removeFilenamePrefix(&filename);
                fprintf(stderr, "load frames from: %s - %s\n", path.c_str(), filename.c_str());
                model = smurf_parser::parseFile(&entityconfig, path, filename, true);
                std::map<std::string, urdf::LinkSharedPtr >::iterator it;
                ConfigMap frames;
                std::string prefix = map["prefix"];
                std::string frame;
                for(it = model->links_.begin(); it != model->links_.end(); ++it)
                {
                    if(prefix != "")
                    {
                        frame = prefix + "." + it->first;
                    }
                    else
                    {
                        frame = it->first;
                    }
                    frames["frameNames"].push_back(frame);
                }
                //fprintf(stderr, "frames:\n%s\n", frames.toYamlString().c_str());
                ConfigMap globalConfig = bagelGui->getGlobalConfig();
                globalConfig["frameNames"] = frames["frameNames"];
                bagelGui->setGlobalConfig(globalConfig);                
                break;
            }
            case MenuActions::EDIT_LOAD_FRAMES:
            {

                ModelInterface *model = bagelGui->getCurrentModel();
                if (model)
                {
                    ConfigMap basicModel = model->getModelInfo();
                    if(basicModel["versions"][0].hasKey("data") &&
                       basicModel["versions"][0]["data"].hasKey("frameNames"))
                    {
                        ConfigMap globalConfig = bagelGui->getGlobalConfig();
                        globalConfig["frameNames"] = basicModel["versions"][0]["data"]["frameNames"];
                        bagelGui->setGlobalConfig(globalConfig);
                    }

                }
                break;
            }
            case MenuActions::EDIT_STORE_FRAMES:
            {

                ModelInterface *model = bagelGui->getCurrentModel();
                if (model)
                {
                    ConfigMap basicModel = model->getModelInfo();
                    ConfigMap globalMap = bagelGui->getGlobalConfig();
                    basicModel["versions"][0]["data"]["frameNames"] = globalMap["frameNames"];
                    model->setModelInfo(basicModel);
                }
                break;
            }

            case MenuActions::CREATE_BAGEL_MOTION_CONTROL_TASK:
            {
                // 20221102 MS: Why is this here? Has nothing todo with XROCK.
                ModelInterface *model = bagelGui->getCurrentModel();
                if (model)
                {
                    ConfigMap localMap = model->getModelInfo();
                    std::string domain = "SOFTWARE";
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
                        model->setModelInfo(map);
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
                // TODO: Once we have a toplvl property, we would not need this anymore. Then the ComponentModelEditorWidget would have a field for it.
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
                if (ioLibrary)
                {
                    env["dbType"] = "Serverless";
                    env["dbPath"] = toolbarBackend->getDbPath();
                    db.reset(ioLibrary->getDB(env));
                    db->setDbGraph(toolbarBackend->getGraph());
                }
                break;
            }
            case MenuActions::SELECT_CLIENT: // Client
            {
                if (ioLibrary)
                {
                    env["dbType"] = "Client";
                    env["dbAddress"] = toolbarBackend->getDbAddress();
                    env["dbGraph"] = toolbarBackend->getGraph();
                    db.reset(ioLibrary->getDB(env));
                    db->setDbGraph(toolbarBackend->getGraph());
                    db->setDbAddress(toolbarBackend->getDbAddress());
                    if (!db->isConnected())
                    {
                        std::string msg = "Server is not running! Please run server using command:\njsondb -d " + toolbarBackend->getDbPath();
                        QMessageBox::warning(nullptr, "Warning", msg.c_str(), QMessageBox::Ok);
                    }
                }
                break;
            }

            case MenuActions::SELECT_MULTIDB: // MultiDbClient
            {
                if (ioLibrary)
                {
                    std::string multidb_config_path = bagelGui->getConfigDir() + "/MultiDBConfig.yml";
                    MultiDBConfigDialog dialog(multidb_config_path, ioLibrary);
                    dialog.exec();
                    if (mars::utils::pathExists(multidb_config_path))
                    {
                        ConfigMap multidb_config = configmaps::ConfigMap::fromYamlFile(multidb_config_path);
                        env["dbType"] = "MultiDbClient";
                        env["multiDBConfig"] = multidb_config.toJsonString();
                        db.reset(ioLibrary->getDB(env));
                        if (multidb_config["main_server"]["type"] == "Client" or
                            std::any_of(multidb_config["import_servers"].begin(), multidb_config["import_servers"].end(), [](ConfigItem &is)
                                        { return is["type"] == "Client"; }))
                        {
                            std::string msg = "MultiDB is requesting a client server! Please run server using command:\njsondb -d YOUR_DB_PATH";
                            QMessageBox::warning(nullptr, "Warning", msg.c_str(), QMessageBox::Ok);
                        }
                    }
                }
                break;
            }
            case MenuActions::SELECT_FILEDB: // FileDB
            {
                if (!ioLibrary)
                {
                    db.reset(new FileDB());
                }
                break;
            }
            case MenuActions::RELOAD_MODEL_FROM_DB: // Reload
            {
                if (ModelInterface *m = bagelGui->getCurrentModel())
                {
                    ConfigMap currentModel = m->getModelInfo();

                    if (currentModel.hasKey("name"))
                    {
                        bagelGui->closeCurrentTab();
                        // Reload component model from DB (creating a new TAB)
                        loadComponentModel(currentModel["domain"], currentModel["name"], currentModel["versions"][0]["name"]);
                    }
                    else
                        QMessageBox::warning(nullptr, "Warning", "Nothing to reload.", QMessageBox::Ok);
                }

                break;
            }
            case MenuActions::REMOVE_MODEL_FROM_DB: // Remove model
            {
                if (QMessageBox::question(nullptr, "Warning", "Are you sure you want to remove the current Model?", QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
                {
                    if (ModelInterface *m = bagelGui->getCurrentModel())
                    {
                        ConfigMap currentModel = m->getModelInfo();
                        if (currentModel.hasKey("uri"))
                        {
                            bool removed;
                            {
                                WaitCursorRAII _;
                                removed = db->removeModel((std::string)currentModel["uri"]);
                            }
                            if (removed)
                            {
                                bagelGui->closeCurrentTab();
                                QMessageBox::information(nullptr, "Success", "Model removed successfully!", QMessageBox::Ok);
                            }
                            else
                                QMessageBox::critical(nullptr, "Error", "Failed to remove model.", QMessageBox::Ok);
                        }
                        else
                            QMessageBox::warning(nullptr, "Warning", "Cannot remove model due missing uri.", QMessageBox::Ok);
                    }
                }
                break;
            }
            case MenuActions::EXPORT_CND_TFENHANCE:
            {
                QString urdf_file = QFileDialog::getOpenFileName(NULL, QObject::tr("Select urdf_file"),
                                                                 ".", QObject::tr("YAML syntax (*.urdf)"), 0,
                                                                 QFileDialog::DontUseNativeDialog);

                QString fileName = QFileDialog::getSaveFileName(NULL, QObject::tr("Select Model"),
                                                                "export.cnd", QObject::tr("YAML syntax (*.cnd)"), 0,
                                                                QFileDialog::DontUseNativeDialog);
                if (!urdf_file.isNull())
                {
                    ConfigMap map = bagelGui->createConfigMap();
                    exportCnd(map, fileName.toStdString(), urdf_file.toStdString());
                }
                break;
            }
            case MenuActions::RUN_ABSTRACT_GUI:
            {
                runAbstractGui();
                break;
            }

            default:
            {
                throw std::out_of_range("Cannot handle action " + std::to_string(action));
            }
            }
        }
        catch (const std::exception &e)
        {
            QMessageBox::critical(nullptr, "Error", QString::fromStdString(e.what()), QMessageBox::Ok);
        }
    }
    void XRockGUI::createBagelTask()
    {
        ModelInterface *model = bagelGui->getCurrentModel();
        if (model)
        {
            QMessageBox message;
            ConfigMap localMap = model->getModelInfo();
            std::string domain = "SOFTWARE";
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

    // TODO: Check if this function should belong here, because it does not operate on a component model interface but a bagel interface
    void XRockGUI::createBagelModel()
    {
        ModelInterface *model = bagelGui->getCurrentModel();
        if (model)
        {
            ConfigMap localMap = model->getModelInfo();
            std::string domain = "SOFTWARE";
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
            loadComponentModelFrom(map);

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
                        addComponent("SOFTWARE", "PIPE", "v1.0.0", motorName);
                        // todo: change the output interface name and toggle interface
                        ConfigMap nodeMap = *(bagelGui->getNodeMap(motorName));
                        nodeMap["outputs"][0]["interface"] = 1;
                        nodeMap["outputs"][0]["interfaceExportName"] = motorName + "/des_angle";
                        bagelGui->updateNodeMap(motorName, nodeMap);
                    }

                    n -= step;
                }
            }
            //widget->loadType("SOFTWARE", "PIPE", "v1.0.0");
            //widget->loadType("SOFTWARE", "SIN", "v1.0.0");
            //widget->loadType("SOFTWARE", "ASIN", "v1.0.0");
            //widget->loadType("SOFTWARE", "MOD", "v1.0.0");
            //widget->loadType("SOFTWARE", "POW", "v1.0.0");
            //widget->loadType("SOFTWARE", "TAN", "v1.0.0");
            //widget->loadType("SOFTWARE", "COS", "v1.0.0");
            //widget->loadType("SOFTWARE", "ABS", "v1.0.0");
            //widget->loadType("SOFTWARE", "DIV", "v1.0.0");
            //widget->loadType("SOFTWARE", "Timer", "v1.0.0");
            //widget->loadType("SOFTWARE", "Modulated_Sine", "v1.0.0");
        }
    }

    void XRockGUI::requestModel()
    {
        ImportDialog id(this, ImportDialog::Intention::LOAD_CM);
        id.exec();
    }
    void XRockGUI::addComponent()
    {
        ImportDialog id(this, ImportDialog::Intention::ADD_CM);
        id.exec();
    }

    void XRockGUI::newComponentModel()
    {
        // Create view will setup a NEW instance of a component model interface
        bagelGui->createView("xrock", "New Model");
        ComponentModelInterface* model = dynamic_cast<ComponentModelInterface*>(bagelGui->getCurrentModel());
        // Set the model info of the ComponentModelInterface
        ConfigMap emptyModel = db->getEmptyComponentModel();
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        emptyModel["date"] = oss.str();
        emptyModel["versions"][0]["date"] = oss.str();
        model->addLayout("default_layout");
        model->setModelInfo(emptyModel);
        // Afterwards we have to (re-)trigger the currentModelChanged() function
        currentModelChanged(model);
    }

    // This function adds a new part to an already opened component model
    void XRockGUI::addComponent(const std::string& domain, const std::string& modelName, const std::string& version, std::string nodeName)
    {
        ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());
        if (!model)
            return;
        const std::string& type(model->deriveTypeFrom(domain, modelName, version));
        if (!model->registerComponentModel(domain, modelName, version))
        {
            std::cerr << "XRockGUI::addComponent(): Could not register component model " << type << "\n";
            return;
        }
        if (nodeName.empty())
        {
            nodeName = modelName;
        }
        bagelGui->addNode(type, nodeName);
    }

    // This function loads a component model from an already existing config map
    void XRockGUI::loadComponentModelFrom(configmaps::ConfigMap &map)
    {
        // Create view will setup a NEW instance of a component model interface
        bagelGui->createView("xrock", map["name"]);
        ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());

        if (!map["versions"][0]["data"].hasKey("gui"))
            map["versions"][0]["data"]["gui"] = ConfigMap();
        if (!map["versions"][0]["data"]["gui"].hasKey("defaultLayout"))
            map["versions"][0]["data"]["gui"]["defaultLayout"] = "software";

        // Set the model info of the ComponentModelInterface
        model->setModelInfo(map);
        // Afterwards we have to (re-)trigger the currentModelChanged() function
        currentModelChanged(model);
    }

    // This function loads a component model from DB
    void XRockGUI::loadComponentModel(const std::string &domain, const std::string &modelName, const std::string &version)
    {
        ConfigMap map;
        {
            WaitCursorRAII _;
            map = db->requestModel(domain, modelName, version, !version.empty());
        }
        loadComponentModelFrom(map);
    }

    // This function stores the current component model
    bool XRockGUI::storeComponentModel()
    {
        ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());
        if (!model)
            return false;
        // Get the current model info
        ConfigMap map = model->getModelInfo();
        // Store the returned info to db
        bool saved;
        {
            WaitCursorRAII _;
            saved = db->storeModel(map);
        }
        return saved;
    }

    void XRockGUI::currentModelChanged(bagel_gui::ModelInterface *model)
    {
        widget->clear();
        widget->currentModelChanged(model);
    }

    void XRockGUI::changeNodeVersion(const std::string &name)
    {
        versionChangeName = name;
        const ConfigMap *node_ = bagelGui->getNodeMap(name);
        if (!node_)
            return;
        ConfigMap node = *node_;
        VersionDialog vd(this);
        // When we request to change the version of a node, we have to search for alternative component models
        std::string domain = node["model"]["domain"];
        std::string type = node["model"]["name"];
        vd.requestComponent(domain, type);
        vd.exec();
    }

    void XRockGUI::configureNode(const std::string &name)
    {
        ConfigMap node = *(bagelGui->getNodeMap(name));
        ConfigMap config;
        if(node.hasKey("configuration"))
        {
            // Preload the current configuration
            if(node["configuration"].hasKey("data"))
            {
                config = node["configuration"]["data"];
            }
        }
        {
            std::string type = node["model"]["name"];
            std::map<std::string, ConfigureDialogLoader*>::iterator it;
            it = configPlugins.find(type);
            if(it != configPlugins.end())
            {
                ConfigMap globalConfig = bagelGui->getGlobalConfig();
                QDialog *d = it->second->createDialog(&config, env, globalConfig);
                d->setWindowTitle(QString::fromStdString("Configure Node " + name));
                d->exec();
                delete d;
            }
            else
            {
                ConfigureDialog cd(&config, env, node["model"]["name"], true, true);
                cd.setWindowTitle(QString::fromStdString("Configure Node " + name));
                cd.resize(400, 400);
                cd.exec();
            }
        }
        // Update the node configuration
        node["configuration"]["data"] = config;
        bagelGui->updateNodeMap(name, node);
    }
    
    void XRockGUI::configureEdge(const std::string &name)
    {
        ConfigMap edge = *(bagelGui->getEdgeMap(name));
        ConfigMap config;
        if (edge.hasKey("configuration"))
        {
            // Preload the current configuration
            config = edge["configuration"];

        }

        {
            ConfigureDialog cd(&config, env, name, true, true);
            cd.setWindowTitle(QString::fromStdString("Configure Edge " + name));
            cd.resize(400, 400);
            cd.exec();
        }
        // Update the edge configuration
        edge["configuration"] = config;
        bagelGui->updateEdgeMap(name, edge);
    }

    void XRockGUI::openConfigFile(const std::string &name)
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
                std::string configFile = node["model"]["name"];
                configFile += ".yml";
                std::string path = mars::utils::pathJoin(bundlePath, bundleName);
                path = mars::utils::pathJoin(path, "config");
                path = mars::utils::pathJoin(path, "orogen");
                path = mars::utils::pathJoin(path, configFile);
                if (mars::utils::pathExists(path))
                {
                    printf("found config file: %s\n", path.c_str());
                    ConfigureDialog cd(NULL, env, node["modelName"], true, true, NULL, NULL, path);
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

    void XRockGUI::configureComponents(const std::string &name)
    {
        ConfigMap node = *(bagelGui->getNodeMap(name));
        ConfigMap config;
        if(!node.hasKey("configuration"))
        {
            node["configuration"] = ConfigMap();
        }
        if(!node["configuration"].hasKey("submodel"))
        {
            node["configuration"]["submodel"] = ConfigVector();
        }
        // Since the data fields in there are strings, we have convert them to ConfigMaps
        ConfigMapHelper::unpackSubmodel(config, node["configuration"]["submodel"]);
        {
            ConfigureDialog cd(&config, env, node["model"]["name"], true, true);
            cd.resize(400, 400);
            cd.exec();
        }
        // Afterwards we have to repack them into strings
        ConfigMapHelper::packSubmodel(node["configuration"], config["submodel"]);
        bagelGui->updateNodeMap(name, node);
    }

    void XRockGUI::configureOutPort(const std::string &nodeName, const std::string &portName)
    {
        openConfigureInterfaceDialog(nodeName, portName, "outputs");
    }

    void XRockGUI::configureInPort(const std::string &nodeName, const std::string &portName)
    {
        openConfigureInterfaceDialog(nodeName, portName, "inputs");
    }

    // TODO: Does this only work for bagel nodes? Where does this 'defaultConfig' come from?
    void XRockGUI::openConfigureInterfaceDialog(const std::string &nodeName,
                                                const std::string &portName,
                                                const std::string &portType)
    {
        if (portType != "outputs" &&
            portType != "inputs")
        {
            std::cout << "XRockGUI::openConfigureInterfaceDialog: wrong portType!" << std::endl;
            return;
        }
        ConfigMap node = *(bagelGui->getNodeMap(nodeName));
        ConfigMap config;
        std::string type = node["model"]["type"];
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
                        std::vector<std::string> keys = {"configuration", "data", "interfaces", contextPortName};
                        subMap = ConfigMapHelper::getSubItem(node, keys);
                        if (portType == "inputs" and subMap and subMap->isMap())
                        {
                            dropdown["merge"][0] = "SUM";
                            dropdown["merge"][1] = "PRODUCT";
                            dropdown["merge"][2] = "MIN";
                            dropdown["merge"][3] = "MAX";
                            config = *subMap;
                        }
                        else if(node["model"]["versions"][0].hasKey("defaultConfiguration"))
                        {
                            ConfigMap &defaultConfig = node["model"]["versions"][0]["defaultConfiguration"];
                            if(defaultConfig.hasKey("data") && defaultConfig["data"].hasKey("interfaces"))
                            {
                                for(auto &[key, value]: (ConfigMap)defaultConfig["data"]["interfaces"])
                                {
                                    if(key == portName)
                                    {
                                        config = value;
                                        break;
                                    }
                                }
                            }
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
                        // TODO: This is not correct. Where does this come from? The basic model does not have such a property?
                        if(node["model"]["versions"][0].hasKey("defaultConfiguration"))
                        {
                            ConfigMap &defaultConfig = node["model"]["versions"][0]["defaultConfiguration"];
                            if(defaultConfig.hasKey("data") && defaultConfig["data"].hasKey("interfaces"))
                            {
                                for(auto &[key, value]: (ConfigMap)defaultConfig["data"]["interfaces"])
                                {
                                    if(key == portName)
                                    {
                                        config = value;
                                        break;
                                    }
                                }
                            }
                        }
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
            ConfigureDialog cd(&config, env, node["model"]["name"], true, false, &dropdown);
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
                // TODO: This is not correct. Where does this come from? The basic model does not have such a property?
                //(*it)["defaultConfig"] = config.toYamlString();
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

    void XRockGUI::selectVersion(const std::string &version)
    {
        ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());
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
            std::string domain = node["model"]["domain"];
            std::string name = node["model"]["name"];
            std::string versionName = version;
            std::string type = model->deriveTypeFrom(domain, name, version);
            if (!model->hasNodeInfo(type))
            {
                model->registerComponentModel(domain, name, version);
            }

            bagelGui->addNode(type, versionChangeName);

            // TODO: Reconnect ports if needed
            // TODO: Handle node configuration

            return;
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

    void XRockGUI::exportCnd(const configmaps::ConfigMap &map_,
                             const std::string &filename, const std::string &urdf_file)
    {
        // ask user to pick a urdf file

        ConfigMap map = bagelGui->getCurrentModel()->getModelInfo();
        std::stringstream cnd_export;
        cnd_export << "xrock-export-cnd -m " << map["name"].getString()
                   << " -v " << map["versions"][0]["name"].getString()
                   << " -o " << filename;
        if (!urdf_file.empty())
        {
            cnd_export << " -t --tf_enhance -u " << urdf_file;
        }
        cnd_export << " -b " << (std::string)env["dbType"];
        if (env["dbType"] == "Serverless")
        {
            cnd_export << " --db_address " << toolbarBackend->getDbPath();
        }
        else if (env["dbType"] == "Client")
        {
            cnd_export << " --db_address " << toolbarBackend->getDbAddress();  
        }
        cnd_export << " --db_graph " <<  toolbarBackend->getGraph();
      
        int ret = std::system(cnd_export.str().c_str());
        if (ret == EXIT_SUCCESS)
            QMessageBox::information(nullptr, "Export", "Successfully exported", QMessageBox::Ok);
        else
            QMessageBox::critical(nullptr, "Export", QString::fromStdString("Failed to export cnd with code: " + std::to_string(ret)), QMessageBox::Ok);
    }

    // this function runs the executable of python abstract gui which helps to perform implements relation
    void XRockGUI::runAbstractGui()
    {
        std::stringstream abstract_gui;
        abstract_gui << "abstract_gui_cli --backend " << (std::string)env["dbType"] ;

        if (env["dbType"] == "Serverless")
        {
            abstract_gui << " --path " << toolbarBackend->getDbPath();
            abstract_gui << " --graph " <<  toolbarBackend->getGraph();
        }
        else if (env["dbType"] == "Client")
        {
            abstract_gui << " --url " << toolbarBackend->getDbAddress();  
        abstract_gui << " --graph " <<  toolbarBackend->getGraph();
        }
        else if (env["dbType"] == "MultiDbClient")
        {
            std::string multidb_config_path = bagelGui->getConfigDir() + "/MultiDBConfig.yml";
            abstract_gui << " --multi_db_config_path " <<  multidb_config_path;  
        }
        abstract_gui << " &";
        std::cout << "CMD: " << abstract_gui.str() << std::endl;
        std::system(abstract_gui.str().c_str());
    }

    void XRockGUI::importCND(const std::string &fileName)
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
        map["versions"][0]["data"]["gui"] = ConfigMap();

        std::map<std::string, std::pair<int, int>> nodePositions;
        int minX = 0;
        int minY = 0;
        int maxX = 2000;
        int maxY = 2000;
        for (auto it : (ConfigMap)cnd["tasks"])
        {
            int x = rand() % (maxX - minX) + minX;
            int y = rand() % (maxY - minY) + minY;
            nodePositions[it.first.c_str()] = std::make_pair(x, y);

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
        if (cnd.hasKey("connections"))
        {
            // Iterate through the connections and create edges
            for (auto &connection : (ConfigMap)cnd["connections"])
            {
                ConfigMap edge;
                ConfigMap newFromSection;
                ConfigMap fromSection = connection.second["from"];
                newFromSection["domain"] = "SOFTWARE";
                newFromSection["interface"] = fromSection["port_name"];
                newFromSection["name"] = fromSection["task_id"];
                edge["from"] = newFromSection;
                edge["data"] = connection.second["data"];
                edge["name"] = connection.first;
                ConfigMap newToSection;
                ConfigMap toSection = connection.second["to"];
                newToSection["domain"] = "SOFTWARE";
                newToSection["interface"] = toSection["port_name"];
                newToSection["name"] = toSection["task_id"];
                edge["to"] = newToSection;
                // Push the edge into the "edges" array
                map["versions"][0]["components"]["edges"].push_back(edge);
            }
        }

        ConfigMap &guiData = map["versions"][0]["data"]["gui"];
        for (const auto &it : nodePositions)
        {
            const std::string &nodeName = it.first;
            int x = it.second.first;
            int y = it.second.second;

            if (!guiData["layouts"]["software"].hasKey(nodeName))
            {
                // Create a layout entry if it doesn't exist
                guiData["layouts"]["software"][nodeName] = ConfigMap();
            }

            guiData["layouts"]["software"][nodeName]["x"] = std::to_string(x);
            guiData["layouts"]["software"][nodeName]["y"] = std::to_string(y);
        }
        // TODO: The next lines have to be refactored
        map["modelPath"] = mars::utils::getPathOfFile(fileName);
        map.toYamlFile("da.yml");
        loadComponentModelFrom(map);
    }

    void XRockGUI::nodeContextClicked(const std::string name)
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
            ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());
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
            std::string domain = node["model"]["domain"];
            std::string model_name = node["model"]["name"];
            std::string version = node["model"]["versions"][0]["name"];
            loadComponentModel(domain, model_name, version); 
        }
        // TODO: We should add a property called 'description' to the xtypes
        else if (name == "show description")
        {
            ConfigMap node = *(bagelGui->getNodeMap(contextNodeName));
            std::string domain = node["model"]["domain"];
            std::string version = node["model"]["versions"][0]["name"];
            std::string model_name = node["model"]["name"];
            QWebView *doc = new QWebView();
            doc->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
            widget->connect(doc, SIGNAL(linkClicked(const QUrl &)), widget, SLOT(openUrl(const QUrl &)));
            ConfigMap modelMap;
            {
                WaitCursorRAII _;
                modelMap = db->requestModel(domain, model_name, version, true);
            }
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
        // else if (name == "select implementation...")
        // {
        //     showImplementationsDialog();
        // }
    }

    void XRockGUI::edgeContextClicked(const std::string name)
    {
        if (name == "configure edge")
        {
            configureEdge(contextEdgeName);
        }
    }

    void XRockGUI::inPortContextClicked(const std::string name)
    {
        if (name == "configure interface")
        {
            configureInPort(contextNodeName, contextPortName);
            return;
        }
        // TODO: Move the bagel specific stuff to an extra function after checking if the interface is a bagel interface or not
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

    void XRockGUI::outPortContextClicked(const std::string name)
    {
        if (name == "configure interface")
        {
            configureOutPort(contextNodeName, contextPortName);
        }
    }

    std::vector<std::string> XRockGUI::getNodeContextStrings(const std::string &name)
    {
        // Get the node map to show context dependent on node properties (see below)
        ConfigMap map = *(bagelGui->getNodeMap(name));
        std::vector<std::string> r;
        r.push_back("change version");
        r.push_back("configure node");
        // Make configure nodes only visible if the component actually has inner parts
        if (map["model"]["versions"][0].hasKey("components") && map["model"]["versions"][0]["components"].hasKey("nodes"))
        {
            r.push_back("configure components");
        }
        r.push_back("reset configuration");
        // Only software nodes can have a ROCK config file and can have 'apply configuration'
        if (map["model"]["domain"] == "SOFTWARE")
        {
            r.push_back("open ROCK config file");
            r.push_back("apply configuration");
        }
        
        r.push_back("open model");
        r.push_back("show description");

        contextNodeName = name;
        return r;
    }

    std::vector<std::string> XRockGUI::getEdgeContextStrings(const std::string &name)
    {
        // Get the edge map to show context dependent on edge properties
        contextEdgeName = name;
        std::vector<std::string> r;
        r.push_back("configure edge");
        return r;
    }

    std::vector<std::string> XRockGUI::getInPortContextStrings(const std::string &nodeName, const std::string &portName)
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

    std::vector<std::string> XRockGUI::getOutPortContextStrings(const std::string &nodeName, const std::string &portName)
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

    void XRockGUI::applyConfiguration(configmaps::ConfigMap &map)
    {
        // This function is restricted to software domain because it deals with ROCK task configuration only
        if (map["model"]["domain"] != "SOFTWARE")
            return;
        ComponentModelInterface *model = dynamic_cast<ComponentModelInterface *>(bagelGui->getCurrentModel());
        if (!model)
            return;
        ConfigMap modelMap = model->getModelInfo();
        std::string modelName = modelMap["name"].getString() + "_" + modelMap["versions"][0]["name"].getString();

        versionChangeName << map["name"];
        if(map.hasKey("alias") and map["alias"] != "")
        {
            versionChangeName << map["alias"];
        }
        std::string configFile = "temp_task_config.yml";
        std::string modelFile = map["model"]["name"].getString()+".yml";
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
        if (map.hasKey("configuration") and
            map["configuration"].hasKey("data") and
            map["configuration"]["data"].hasKey("config"))
        {
            std::string yaml = "--- name:default\n" + map["configuration"]["data"]["config"].toYamlString();
            std::ofstream file;
            file.open(configFile);
            file << yaml;
            file.close();
            cmd += " -c " + configFile;
            cmd += " -o " + modelFile + " ";
            cmd += map["model"]["name"].getString();
            cmd += " default";
        }
        else if (map.hasKey("configuration") and
                 map["configuration"].hasKey("data") and
                 map["configuration"]["data"].hasKey("config_names"))
        {
            cmd += " -o " + modelFile + " ";
            cmd += map["model"]["name"].getString();
            for (auto it : map["configuration"]["data"]["config_names"])
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
        std::string new_version = map["model"]["versions"][0]["name"].getString() + "_" + modelName;
        std::string c = getenv("AUTOPROJ_CURRENT_ROOT");
        ConfigMap dbConfig = ConfigMap::fromYamlString(env["multiDBConfig"]);
        std::string dbPath = pathJoin(c, dbConfig["main_server"]["path"].getString());
        std::string dbGraph = dbConfig["main_server"]["graph"];
        cmd = "xrock-orogen-to-xrock --model_name " + map["model"]["name"].getString() + " --model_file " + modelFile + " --model_version " + new_version + " -a " + dbPath + " -g " + dbGraph;
#ifdef __APPLE__
        {
            c += "/install/bin/";
            cmd = "DYLD_LIBRARY_PATH=$MYLD_LIBRARY_PATH python " + c + cmd;
        }
#endif
        printf("execute: %s\n", cmd.c_str());
        system(cmd.c_str());
        // 5. switch node to new version
        selectVersion(new_version);
    }

    void XRockGUI::cfgUpdateProperty(mars::cfg_manager::cfgPropertyStruct property)
    {
        if (property.paramId == dbAddress_paramId)
        {
            db->setDbAddress(property.sValue);
        }
        else if (property.paramId == dbUser_paramId)
        {
            // TODO: Is something missing here?
        }
    }

    std::string XRockGUI::getBackend()
    {
        return env["backend"].getString();
    }

    bool XRockGUI::handleAlias()
    {
        return (env["dbType"] != "FileDB");
    }

} // end of namespace xrock_gui_model

DESTROY_LIB(xrock_gui_model::XRockGUI)
CREATE_LIB(xrock_gui_model::XRockGUI)
