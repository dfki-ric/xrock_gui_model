#include "ModelWidget.hpp"
#include "ModelLib.hpp"
#include "Model.hpp"
#include "FileDB.hpp"
#include "ConfigureDialog.hpp"
#include "ConfigMapHelper.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QRegExp>
#include <QTextEdit>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <bagel_gui/BagelGui.hpp>
#include <bagel_gui/BagelModel.hpp>
#include <mars/utils/misc.h>
#include <QDesktopServices>


using namespace configmaps;

namespace xrock_gui_model {

  ModelWidget::ModelWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                           bagel_gui::BagelGui *bagelGui, ModelLib *mainLib,
                           QWidget *parent) :
    mars::main_gui::BaseWidget(parent, cfg, "ModelWidget"), bagelGui(bagelGui),
    mainLib(mainLib) {

    ignoreUpdate = false;
    QGridLayout *layout = new QGridLayout();
    QVBoxLayout *vLayout = new QVBoxLayout();
    //QPushButton *button = new QPushButton("add node", this);
    //connect(button, SIGNAL(clicked()), this, SLOT(addNode()));
    edition = "";
    size_t i=0;
    modelPath = cfg->getOrCreateProperty("XRockGUI", "modelPath", ".", this).sValue;
    bagelGui->setLoadPath(modelPath);

    QLabel *l = new QLabel("name");
    layout->addWidget(l, i, 0);
    name = new QLineEdit("");
    layout->addWidget(name, i++, 1);
    connect(name, SIGNAL(textChanged(const QString&)), this, SLOT(updateModelInfo()));

    l = new QLabel("type");
    layout->addWidget(l, i, 0);
    type = new QLineEdit("");
    layout->addWidget(type, i++, 1);
    connect(type, SIGNAL(textChanged(const QString&)), this, SLOT(updateModelInfo()));

    l = new QLabel("version");
    layout->addWidget(l, i, 0);
    version = new QLineEdit("");
    layout->addWidget(version, i++, 1);
    connect(version, SIGNAL(textChanged(const QString&)), this, SLOT(updateModelInfo()));

    l = new QLabel("domain");
    layout->addWidget(l, i, 0);
    domain = new QLineEdit("");
    layout->addWidget(domain, i++, 1);
    connect(domain, SIGNAL(textChanged(const QString&)), this, SLOT(updateModelInfo()));

    l = new QLabel("maturity");
    layout->addWidget(l, i, 0);
    maturity = new QLineEdit("");
    maturity->setEnabled(false);
    layout->addWidget(maturity, i++, 1);
    connect(maturity, SIGNAL(textChanged(const QString&)), this, SLOT(updateModelInfo()));

    l = new QLabel("projectName");
    layout->addWidget(l, i, 0);
    projectName = new QLineEdit("");
    layout->addWidget(projectName, i++, 1);
    connect(projectName, SIGNAL(textChanged(const QString&)), this, SLOT(updateModelInfo()));

    l = new QLabel("designedBy");
    layout->addWidget(l, i, 0);
    designedBy = new QLineEdit("");
    layout->addWidget(designedBy, i++, 1);
    connect(designedBy, SIGNAL(textChanged(const QString&)), this, SLOT(updateModelInfo()));

    // l = new QLabel("includes");
    // layout->addWidget(l, i, 0);
    // includes = new QTextEdit();
    // layout->addWidget(includes, i++, 1);

    l = new QLabel("data");
    layout->addWidget(l, i, 0);
    data = new QTextEdit();
    layout->addWidget(data, i++, 1);
    connect(data, SIGNAL(textChanged()), this, SLOT(updateModelInfo()));

    l = new QLabel("interfaces");
    layout->addWidget(l, i, 0);
    interfaces = new QTextEdit();
    interfaces->setReadOnly(true);
    connect(interfaces, SIGNAL(textChanged()), this, SLOT(updateModelInfo()));
    layout->addWidget(interfaces, i++, 1);
    vLayout->addLayout(layout);
    vLayout->addStretch();

    QGridLayout *gridLayout = new QGridLayout();
    l = new QLabel("Layout:");
    gridLayout->addWidget(l, 0, 0);

    QCheckBox *check = new QCheckBox("mechanics");
    check->setChecked(true);
    connect(check, SIGNAL(stateChanged(int)), this, SLOT(checkMechanics(int)));
    gridLayout->addWidget(check, 0, 1);
    checkMap["mechanics"] = check;

    check = new QCheckBox("electronics");
    check->setChecked(true);
    connect(check, SIGNAL(stateChanged(int)), this, SLOT(checkElectronics(int)));
    gridLayout->addWidget(check, 0, 2);
    checkMap["electronics"] = check;

    check = new QCheckBox("software");
    check->setChecked(true);
    connect(check, SIGNAL(stateChanged(int)), this, SLOT(checkSoftware(int)));
    gridLayout->addWidget(check, 1, 1);
    checkMap["software"] = check;

    check = new QCheckBox("behavior");
    check->setChecked(true);
    connect(check, SIGNAL(stateChanged(int)), this, SLOT(checkBehavior(int)));
    gridLayout->addWidget(check, 1, 2);
    checkMap["behavior"] = check;

    check = new QCheckBox("computation");
    check->setChecked(true);
    connect(check, SIGNAL(stateChanged(int)), this, SLOT(checkComputation(int)));
    gridLayout->addWidget(check, 2, 1);
    checkMap["computation"] = check;

    vLayout->addLayout(gridLayout);
    layouts = new QListWidget();
    vLayout->addWidget(layouts);
    connect(layouts, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(layoutsClicked(const QModelIndex&)));
    layouts->addItem("overview");
    QHBoxLayout *hLayout = new QHBoxLayout();
    layoutName = new QLineEdit("new layout");
    hLayout->addWidget(layoutName);
    QPushButton *b = new QPushButton("add/remove");
    connect(b, SIGNAL(clicked()), this, SLOT(addRemoveLayout()));
    hLayout->addWidget(b);
    vLayout->addLayout(hLayout);
    hLayout = new QHBoxLayout();
    b = new QPushButton("load model");
    connect(b, SIGNAL(clicked()), this, SLOT(requestModel()));
    hLayout->addStretch();
    hLayout->addWidget(b);
    b = new QPushButton("save model");
    connect(b, SIGNAL(clicked()), this, SLOT(storeModel()));
    hLayout->addWidget(b);
    vLayout->addLayout(hLayout);
    setLayout(vLayout);
    currentLayout = "";
    this->clear();
    xrockConfigFilter.push_back("activity");
    xrockConfigFilter.push_back("state");
    xrockConfigFilter.push_back("config_names");
    xrockConfigFilter.push_back("parentName");
    //nodeTypeView->setSelectionMode(QAbstractItemView::SingleSelection);
  }

  ModelWidget::~ModelWidget(void) {
  }

  void ModelWidget::deinit(void) {
    cfg->setPropertyValue("XRockGUI", "modelPath", "value", modelPath);
  }

  void ModelWidget::checkMechanics(int v) {
    bagelGui->setViewFilter("mechanics", v);
  }

  void ModelWidget::checkElectronics(int v) {
    bagelGui->setViewFilter("electronics", v);
  }

  void ModelWidget::checkSoftware(int v) {
    bagelGui->setViewFilter("software", v);
  }

  void ModelWidget::checkBehavior(int v) {
    bagelGui->setViewFilter("behavior", v);
  }

  void ModelWidget::checkComputation(int v) {
    bagelGui->setViewFilter("computation", v);
  }

  void ModelWidget::setEdition(const std::string &domain) {
    this->edition = domain;
    for(auto it: checkMap) {
      if(it.first == domain) {
        it.second->setChecked(true);
      }
      else {
        it.second->setChecked(false);
      }
    }
    handleEditionLayout();
  }

  void ModelWidget::layoutsClicked(const QModelIndex &index) {
    QVariant v = layouts->model()->data(index, 0);
    if(v.isValid()) {
      std::string layout = v.toString().toStdString();
      updateCurrentLayout();
      currentLayout = layout;
      layout += ".yml";
      layoutName->setText(currentLayout.c_str());

      //bagelGui->loadLayout(layout);
      bagelGui->applyLayout(layoutMap[currentLayout]);
    }
  }

  void ModelWidget::addRemoveLayout() {
    std::string name = layoutName->text().toStdString();
    for(int i=0; i<layouts->count(); ++i) {
      QVariant v = layouts->item(i)->data(0);
      if(v.isValid()) {
        std::string layout = v.toString().toStdString();
        if(layout == name) {
          QListWidgetItem *item = layouts->item(i);
          delete item;
          layoutMap.erase(name);
          if(layouts->count() > 0) {
            layouts->setCurrentItem(layouts->item(0));
            currentLayout = layouts->item(0)->data(0).toString().toStdString();
            layoutName->setText(currentLayout.c_str());
            bagelGui->applyLayout(layoutMap[currentLayout]);
          }
          else {
            currentLayout = "";
            layouts->setCurrentItem(0);
          }
          updateModelInfo();
          return;
        }
      }
    }
    updateCurrentLayout();
    ConfigMap layout;
    layouts->addItem(name.c_str());
    layouts->setCurrentItem(layouts->item(layouts->count()-1));
    currentLayout = name;
    updateCurrentLayout();
    //bagelGui->saveLayout(currentLayout + ".yml");
    updateModelInfo();
  }

  void ModelWidget::updateCurrentLayout() {
    if(!currentLayout.empty()) {
      ConfigMap layout = bagelGui->getLayout();
      layoutMap[currentLayout] = layout;
    }
  }

  void ModelWidget::storeModel() {
    updateCurrentLayout();
    ConfigMap map;
    createMap(&map);
    bool success = XRockDB::storeModel(map);
    bagel_gui::BagelModel *model = dynamic_cast<bagel_gui::BagelModel*>(bagelGui->getCurrentModel());
    if(model) {
      // try to save bagel graph
      bagelGui->save(localMap["graphFile"].getString());
    }

    QMessageBox message;
    if(success) {
      message.setText("The model was successfully stored!");
    }
    else {
      message.setText("The model could not be stored!");
    }
    message.exec();
  }

  void ModelWidget::requestModel() {
    mainLib->requestModel();
  }

  void ModelWidget::loadModel() {
    QString fileName = QFileDialog::getOpenFileName(NULL,
                                                    QObject::tr("Select Model"),
                                                    bagelGui->getLoadPath().c_str(),
                                                    QObject::tr("Model Files (*.yml)"),0);
    loadModel(fileName.toStdString());
  }

  void ModelWidget::loadModel(const std::string &file) {
    if(mars::utils::pathExists(file)) {
      modelPath = mars::utils::getPathOfFile(file);
      if(modelPath[modelPath.size()-1] != '/') modelPath.append("/");

      ConfigMap map = ConfigMap::fromYamlFile(file, true);
      map["modelPath"] = modelPath;
      loadModel(map);
    }
  }

  void ModelWidget::loadModel(ConfigMap &map) {
    ConfigMap myMap;
    if(map.hasKey("modelPath")) {
      modelPath << map["modelPath"];
    }
    else {
      modelPath = ".";
    }
    bagelGui->setLoadPath(modelPath);
    //fprintf(stderr, "set load path to: %s\n", modelPath.c_str());
    std::string domainData = mars::utils::tolower(map["domain"]) + "Data";

    // create view clears this widget
    bagelGui->createView("xrock", map["name"]);
    bagelGui->setSmoothLineMode();

    myMap["domain"] = mars::utils::tolower(map["domain"]).c_str();
    myMap["name"]   = map["name"];
    myMap["type"]   = map["type"];
    myMap["versions"][0]["name"] = map["versions"][0]["name"];
    if(map["versions"][0].hasKey("projectName")) {
      myMap["versions"][0]["projectName"] = map["versions"][0]["projectName"];
    }
    if(map["versions"][0].hasKey("designedBy")) {
      myMap["versions"][0]["designedBy"]  = map["versions"][0]["designedBy"];
    }
    if(map["versions"][0].hasKey(domainData) &&
       map["versions"][0][domainData].hasKey("data"))
    {
      myMap["versions"][0][domainData] = map["versions"][0][domainData];
      myMap["versions"][0][domainData]["data"] = ConfigMap::fromYamlString(map["versions"][0][domainData]["data"]);
    }

    if(map["versions"][0].hasKey("maturity")) {
      myMap["versions"][0]["maturity"] = map["versions"][0]["maturity"];
    }
    else {
      myMap["versions"][0]["maturity"] = std::string("INPROGRESS");
    }

    if(map["versions"][0].hasKey("repository")) {
      myMap["versions"][0]["repository"] = map["versions"][0]["repository"];
    }

    if(map["versions"][0].hasKey("interfaces")) {
      myMap["interfaces"] = map["versions"][0]["interfaces"].toYamlString().c_str();
      interfaceMap["interfaces"] = map["versions"][0]["interfaces"];
    }
    else {
      interfaceMap["interfaces"] = ConfigMap();
    }

    if(map["versions"][0].hasKey("defaultConfiguration")) {
      if(map["versions"][0]["defaultConfiguration"].hasKey("data")) {
        myMap["versions"][0]["defaultConfiguration"]["data"] = ConfigMap::fromYamlString(map["versions"][0]["defaultConfiguration"]["data"]);
      }
    }

    if(map["versions"][0].hasKey("components")) {
      //fprintf(stderr, "model graph:\n%s\n", map["versions"][0]["components"].c_str());
      ConfigMap &cMap = map["versions"][0]["components"];
      if(cMap.hasKey("nodes")) {
        loadGraph(cMap);
        interfaces->setReadOnly(true);
      }
    }
    else {
      interfaces->setReadOnly(false);
    }

    if(myMap["versions"][0].hasKey(domainData) &&
       myMap["versions"][0][domainData].hasKey("data"))
    {
      ConfigMap &dataMap = myMap["versions"][0][domainData]["data"];
      if(dataMap.hasKey("gui")) {
        std::string defLayout = dataMap["gui"]["defaultLayout"];
        //ConfigVector::iterator it = dataMap["gui"]["layouts"].begin();
        bagelGui->setLoadPath(modelPath);
        layoutMap = dataMap["gui"]["layouts"];
        for(auto it : layoutMap) {
          layouts->addItem(it.first.c_str());
          if(it.first == defLayout) {
            layouts->setCurrentItem(layouts->item(layouts->count()-1));
            currentLayout = defLayout;
            //bagelGui->loadLayout(currentLayout + ".yml");
            bagelGui->applyLayout(it.second);
          }
        }
      }
      if(dataMap.hasKey("description")) {
        if(dataMap["description"].hasKey("nodes")) {
          for(auto it: dataMap["description"]["nodes"]) {
            bagelGui->addNode("DES", it["name"]);
            bagelGui->updateNodeMap(it["name"], it);
          }
        }
      }
    }

    setModelInfo(myMap);
    updateModelInfo();
    handleEditionLayout();
  }

  void ModelWidget::handleEditionLayout() {
    // if possible set the layout depending on the edition
    if(!edition.empty()) {
      bool found = false;
      if(layoutMap.size()) {
        for(auto it: layoutMap) {
          if(it.first == edition) {
            found = true;
            currentLayout = edition;
            bagelGui->applyLayout(it.second);
            for(int i=0; i<layouts->count(); ++i) {
              QVariant v = layouts->item(i)->data(0);
              if(v.isValid()) {
                std::string layout = v.toString().toStdString();
                if(layout == edition) {
                  layouts->setCurrentItem(layouts->item(i));
                  break;
                }
              }
            }
            break;
          }
        }
      }
      if(!found) {
        // create a layout for the used edition
        updateCurrentLayout();
        layouts->addItem(edition.c_str());
        layouts->setCurrentItem(layouts->item(layouts->count()-1));
        currentLayout = edition;
        updateCurrentLayout();
        updateModelInfo();
      }
    }
  }

  /*
   * todo:
   *       - how to handle visual properties
   */
  void ModelWidget::loadGraph(ConfigMap &map) {
    //ConfigMap map = ConfigMap::fromYamlFile(file);
    ConfigMap config;

    if(map.hasKey("configuration")) {
      config = map["configuration"];
    }

    // create view; set model
    if(map.hasKey("nodes")) {
      ConfigVector::iterator it = map["nodes"].begin();
      ConfigVector pending;
      for(; it!=map["nodes"].end(); ++it) {
        if((*it)["model"]["name"] == "software::Deployment") {
          loadNode((*it), config);
        }
        else {
          pending.push_back(*it);
        }
      }
      for(it=pending.begin(); it!=pending.end(); ++it) {
        loadNode((*it), config);
      }
    }

    if(map.hasKey("edges")) {
      ConfigVector::iterator it = map["edges"].begin();
      for(; it!=map["edges"].end(); ++it) {
        ConfigMap edge;
        std::string name = (*it)["from"]["name"];
        edge["fromNode"] = name;
        edge["fromNodeOutput"] = (*it)["from"]["interface"];

        name << (*it)["to"]["name"];
        edge["toNode"] = name;
        edge["toNodeInput"] = (*it)["to"]["interface"];;

        edge["name"] = (*it)["name"];
        if(it->hasKey("data")) {
          edge.append(ConfigMap::fromYamlString((*it)["data"].getString()));
        }
        edge["smooth"] = true;
        bagelGui->addEdge(edge);
      }
    }
  }

  void ModelWidget::loadNode(ConfigMap &node, ConfigMap &config) {
    std::string domain = mars::utils::tolower(node["model"]["domain"]);
    std::string name = node["name"];
    std::string origName = node["name"];
    std::string modelName = node["model"]["name"];
    std::string modelVersion;
    if(node["model"].hasKey("version")) {
      modelVersion << node["model"]["version"];
    }
    // check if we know the type already
    loadType(domain, modelName, modelVersion);

    std::string type = modelName;
    // todo: only add version if it is not the first default one
    Model *model = dynamic_cast<Model*>(bagelGui->getCurrentModel());
    if (model) {
      if(!modelVersion.empty()) {
        if(model->hasNodeInfo(type + "::" + modelVersion)) {
          type += "::" + modelVersion;
        }
      }
    }
    bagelGui->addNode(type, name);

    ConfigMap data;
    // get node config
    if(config.hasKey("nodes")) {
      ConfigVector::iterator itConf = config["nodes"].begin();
      for(; itConf != config["nodes"].end(); ++itConf) {
        std::string model = (*itConf)["name"];
        if(model == name) {
          if(itConf->hasKey("data")) {
            data["configuration"] = ConfigMap::fromYamlString((*itConf)["data"].getString());
          }
          if(itConf->hasKey("submodel")) {
            ConfigMapHelper::unpackSubmodel(data, (*itConf)["submodel"]);
          }
          break;
        }
      }
    }
    if(config.hasKey("edges")) {
      ConfigVector::iterator itConf = config["edges"].begin();
      for(; itConf != config["edges"].end(); ++itConf) {
        std::string model = (*itConf)["name"];
        if(model == name) {
          data["edge_submodel"] = *itConf;
          break;
        }
      }
    }
    // todo: handle name clashes
    const ConfigMap *nodeMap_ = bagelGui->getNodeMap(name);
    if(!nodeMap_) {
      return;
    }
    ConfigMap nodeMap = *nodeMap_;

    bool updateMap = false;

    if(data.size() > 0) {
      if(data.hasKey("configuration")) {
        if(data["configuration"].hasKey("xrock_config")) {
          for(auto it2: xrockConfigFilter) {
            if(data["configuration"]["xrock_config"].hasKey(it2)) {
              nodeMap[it2] = data["configuration"]["xrock_config"][it2];
            }
          }
          ((ConfigMap&)data["configuration"]).erase("xrock_config");
        }
      }
      nodeMap[domain+"Data"]["data"].appendMap(data);
      updateMap = true;
    }
    ConfigVector::iterator itNodeMap = nodeMap["inputs"].begin();
    for(;itNodeMap != nodeMap["inputs"].end(); ++itNodeMap) {
      // todo: handle origname correctly due to domain
      //std::string iname = origName + ":" + (std::string)(*itNodeMap)["name"];
      ConfigVector::iterator it2 = interfaceMap["interfaces"].begin();
      for(; it2 != interfaceMap["interfaces"].end(); ++it2) {
        if(it2->hasKey("linkToNode") &&
           (*it2)["linkToNode"] == origName &&
           (*it2)["linkToInterface"] == (std::string)(*itNodeMap)["name"] &&
           ((*it2)["direction"] == "INCOMING" || (*it2)["direction"] == "BIDIRECTIONAL"))
        {
          (*itNodeMap)["interface"] = 1;
          (*itNodeMap)["interfaceExportName"] = (*it2)["name"];
          if(it2->hasKey("data")) {
            ConfigMap data = ConfigMap::fromYamlString((*it2)["data"]);
            if(data.hasKey("initValue")) {
              (*itNodeMap)["initValue"] = data["initValue"];
            }
          }
          updateMap = true;
          break;
        }
      }
    }
    itNodeMap = nodeMap["outputs"].begin();
    for(;itNodeMap != nodeMap["outputs"].end(); ++itNodeMap) {
      // todo: handle origname correctly due to domain
      //std::string iname = origName + ":" + (std::string)(*itNodeMap)["name"];
      ConfigVector::iterator it2 = interfaceMap["interfaces"].begin();
      for(; it2 != interfaceMap["interfaces"].end(); ++it2) {
        if(it2->hasKey("linkToNode") &&
           (*it2)["linkToNode"] == origName &&
           (*it2)["linkToInterface"] == (std::string)(*itNodeMap)["name"] &&
           ((*it2)["direction"] == "OUTGOING"))
        {
          (*itNodeMap)["interface"] = 1;
          (*itNodeMap)["interfaceExportName"] = (*it2)["name"];
          updateMap = true;
          break;
        }
      }
    }

    // add DefaultConfiguration to interfaces

    // ConfigMap defaultConfig = getDefaultConfig(domain, modelName, modelVersion);
    // itNodeMap = nodeMap["outputs"].begin();
    // for(;itNodeMap != nodeMap["outputs"].end(); ++itNodeMap) {
    //   if(defaultConfig.hasKey("submodel")) {
    //     ConfigVector::iterator it2 = defaultConfig["submodel"].begin();
    //     for(; it2 != defaultConfig["submodel"].end(); ++it2) {
    //       if(it2->hasKey("name") &&
    //          (std::string)(*it2)["name"] == (std::string)(*itNodeMap)["name"] &&
    //          it2->hasKey("data"))
    //       {
    //         (*itNodeMap)["defaultConfig"] = (*it2)["data"];
    //         updateMap = true;
    //         break;
    //       }
    //     }
    //   }
    // }

    // itNodeMap = nodeMap["inputs"].begin();
    // for(;itNodeMap != nodeMap["inputs"].end(); ++itNodeMap) {
    //   if(defaultConfig.hasKey("submodel")) {
    //     ConfigVector::iterator it2 = defaultConfig["submodel"].begin();
    //     for(; it2 != defaultConfig["submodel"].end(); ++it2) {
    //       if(it2->hasKey("name") &&
    //          (std::string)(*it2)["name"] == (std::string)(*itNodeMap)["name"] &&
    //          it2->hasKey("data"))
    //       {
    //         (*itNodeMap)["defaultConfig"] = (*it2)["data"];
    //         updateMap = true;
    //         break;
    //       }
    //     }
    //   }
    // }

    if(updateMap) {
      bagelGui->updateNodeMap(name, nodeMap);
    }
  }

  ConfigMap ModelWidget::getDefaultConfig(const std::string &domain, const std::string &name, const std::string &version) {
    ConfigMap defaultConfig;
    ConfigMap fullMap = XRockDB::requestModel(domain, name, version);
    if(fullMap["versions"][0].hasKey("defaultConfiguration")) {
      defaultConfig = fullMap["versions"][0]["defaultConfiguration"];
    }
    return defaultConfig;
  }

  void ModelWidget::saveModel() {
    std::string suggestion = name->text().toStdString();
    mars::utils::handleFilenamePrefix(&suggestion, bagelGui->getLoadPath());
    QString fileName = QFileDialog::getSaveFileName(NULL,
                                                    QObject::tr("Select Model"),
                                                    suggestion.c_str(),
                                                    QObject::tr("Model Files (*.yml)"));
    if(!fileName.isNull()) {
      std::string file = fileName.toStdString();
      modelPath = mars::utils::getPathOfFile(file);
      if(modelPath[modelPath.size()-1] != '/') modelPath.append("/");
      bagelGui->setLoadPath(modelPath);
      updateCurrentLayout();
      ConfigMap map;
      createMap(&map);
      map.toYamlFile(file);
    }
  }

  void ModelWidget::createMap(ConfigMap *m) {
    ignoreUpdate = true;
    ConfigMap &map = *m;
    map = localMap;
    map.erase("layouts");
    map.erase("defaultLayout");
    map.erase("editable_interfaces");
    map.erase("interfaces");
    map.erase("graphFile");
    std::string domainl = mars::utils::tolower(localMap["domain"]);
    map["domain"] = mars::utils::toupper(domainl);
    map["versions"][0]["date"] = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();

    std::string domainData = domainl + "Data";
    std::string t;
    t = mars::utils::trim(data->toPlainText().toStdString());
    if(!t.empty()) {
      map["versions"][0][domainData] = ConfigMap::fromYamlString(t);
    }
    if(map["versions"][0].hasKey(domainData)) {
      if(map["versions"][0][domainData].hasKey("data")) {
        std::string dataMap = map["versions"][0][domainData]["data"].toYamlString();
        ConfigMap &tmpMap = map["versions"][0][domainData];
        tmpMap.erase("data");
        map["versions"][0][domainData]["data"] = dataMap;
      }
    }
    ConfigMap components;
    ConfigMap interfaceMap;
    ConfigMap descriptionMap;
    saveGraph(components, interfaceMap, descriptionMap, domainl);
    if(interfaceMap.hasKey("interfaces")) {
      map["versions"][0]["interfaces"] = interfaceMap["interfaces"];
      if(map["type"] == "bagel::subgraph") {
        // create default configuration for interfaces
        ConfigMap merge;
        merge["merge"] = "SUM";
        merge["bias"] = 0.0;
        merge["default"] = 0.0;
        ConfigMap inMap = localMap["versions"][0]["defaultConfiguration"]["data"]["interfaces"];
        localMap["versions"][0]["defaultConfiguration"]["data"]["interfaces"] = ConfigMap();
        for(auto it: interfaceMap["interfaces"]) {
          if(it["direction"] == "INCOMING") {
            // todo: check order or use dict instead of list
            std::string name = it["name"];
            if(inMap.hasKey(name)) {
              localMap["versions"][0]["defaultConfiguration"]["data"]["interfaces"][name] = inMap[name];
            }
            else {
              localMap["versions"][0]["defaultConfiguration"]["data"]["interfaces"][name] = merge;
            }
          }
        }
      }
    }
    // convert back to string
    if(localMap["versions"][0].hasKey("defaultConfiguration")) {
      ConfigMap &defMap = map["versions"][0]["defaultConfiguration"];
      defMap.erase("data");
      defMap["data"] = localMap["versions"][0]["defaultConfiguration"]["data"].toYamlString();
    }
    map["versions"][0]["components"] = components;
    ConfigMap guiMap;
    if(layoutMap.size() > 0) {
      guiMap["layouts"] = layoutMap;
    }
    for(int i=0; i<layouts->count(); ++i) {
      QVariant v = layouts->item(i)->data(0);
      if(v.isValid()) {
        std::string layout = v.toString().toStdString();
        guiMap["defaultLayout"] = layout;
      }
    }
    ConfigMap dataMap;
    if(map["versions"][0].hasKey(domainData) &&
       map["versions"][0][domainData].hasKey("data")) {
      dataMap = ConfigMap::fromYamlString(map["versions"][0][domainData]["data"]);
      if(dataMap.hasKey("gui")) {
        dataMap.erase("gui");
        map["versions"][0][domainData]["data"] = dataMap.toYamlString();
      }
    }
    bool updateMap = false;
    if(guiMap.hasKey("layouts")) {
      dataMap["gui"] = guiMap;
      updateMap = true;
    }
    if(descriptionMap.hasKey("nodes")) {
      dataMap["description"]["nodes"] = descriptionMap["nodes"];
      updateMap = true;
    }
    if(updateMap) {
      map["versions"][0][domainData]["data"] = dataMap.toYamlString();
    }
    ignoreUpdate = false;
  }

  void ModelWidget::saveGraph(ConfigMap &output, ConfigMap &interfaceMap,
                              ConfigMap &descriptionMap,
                              const std::string &saveDomain) {
    ConfigMap map = bagelGui->createConfigMap();
    ConfigMap config;
    ConfigMap tmpInterfaces;
    try {
      tmpInterfaces["i"] = ConfigItem::fromYamlString(interfaces->toPlainText().toStdString());
    } catch(...) {
      fprintf(stderr, "ERROR: cannot load interfaces from text field\n");
      tmpInterfaces = ConfigMap();
    }

    if(map["model"].getString() != "xrock" || !map.hasKey("nodes") || map["nodes"].size() == 0) {
      // load interfaces from text field
      if(tmpInterfaces.hasKey("i")) {
        interfaceMap["interfaces"] = tmpInterfaces["i"];
      }
      return;
    }
    if(map.hasKey("descriptions")) {
      for(auto it: map["descriptions"]) {
        descriptionMap["nodes"].push_back(it);
      }
    }
    ConfigVector::iterator it = map["nodes"].begin();
    for(; it!=map["nodes"].end(); ++it) {
      std::string name = (*it)["name"];
      std::string domain = mars::utils::tolower((*it)["domain"]);
      ConfigMap nodeData, edgeData;
      nodeData["name"] = name;
      bool handleNodeConfig = false;
      bool handleEdgeConfig = false;
      for(auto it2: xrockConfigFilter) {
        if(it->hasKey(it2)) {
          (*it)[domain+"Data"]["data"]["configuration"]["xrock_config"][it2] = (*it)[it2];
        }
      }
      if(it->hasKey(domain+"Data") &&
         (*it)[domain+"Data"].hasKey("data") &&
         (*it)[domain+"Data"]["data"].hasKey("configuration")) {
        nodeData["data"] = (*it)[domain+"Data"]["data"]["configuration"].toYamlString();
        handleNodeConfig = true;
      }
      if(it->hasKey(domain+"Data") &&
         (*it)[domain+"Data"].hasKey("data") &&
         (*it)[domain+"Data"]["data"].hasKey("submodel")) {
        ConfigMapHelper::packSubmodel(nodeData, (*it)[domain+"Data"]["data"]["submodel"]);
        handleNodeConfig = true;
      }
      if((*it)["data"].hasKey("edge_submodel")) {
        // todo: fix this
        edgeData = (*it)["data"]["edge_submodel"];
        handleEdgeConfig = true;
      }
      if(handleNodeConfig) {
        config["nodes"].push_back(nodeData);
      }
      if(handleEdgeConfig) {
        config["edges"].push_back(edgeData);
      }
      ConfigMap node;
      node["name"] = name;//.substr(domain.size()+2);
      node["model"]["domain"] = mars::utils::toupper(domain);
      // remove domain from type
      std::string type = (*it)["modelName"];
      //type = type.substr(domain.size()+2);
      //size_t pos = type.find_last_of(':');
      if(it->hasKey("modelVersion")) {
        node["model"]["version"] = (*it)["modelVersion"];
      }
      node["model"]["name"] = type;
      output["nodes"].push_back(node);
      if(it->hasKey("inputs")) {
        ConfigVector::iterator it2 = (*it)["inputs"].begin();
        for(;it2!=(*it)["inputs"].end(); ++it2) {
          if(it2->hasKey("interface")) {
            if((int)((*it2)["interface"]) == 1 || (int)((*it2)["interface"]) == 2) {
              ConfigMap i, data;
              std::string interfaceName;
              if(it2->hasKey("interfaceExportName")) {
                interfaceName << (*it2)["interfaceExportName"];
              }
              else {
                interfaceName << node["name"];
                interfaceName += ":" + (std::string)(*it2)["name"];
              }
              if(it2->hasKey("initValue")) {
                data["initValue"] = (*it2)["initValue"];
                i["data"] = data.toYamlString();
              }
              i["name"] =  interfaceName;
              i["type"] = (*it2)["type"];
              i["direction"] = mars::utils::toupper((*it2)["direction"]);
              i["linkToNode"] = node["name"];
              i["linkToInterface"] = (*it2)["name"];
              i["domain"] = mars::utils::toupper((*it2)["domain"]);
              if(interfaceMap.hasKey("interfaces")) {
                ConfigVector::iterator it3 = interfaceMap["interfaces"].begin();
                for(; it3!=interfaceMap["interfaces"].end(); ++it3) {
                  if((*it3)["name"] == interfaceName && (*it3)["direction"] == (std::string)i["direction"]) {
                    break;
                  }
                }
                if(it3 == interfaceMap["interfaces"].end()) {
                  interfaceMap["interfaces"].push_back(i);
                }
              }
              else {
                interfaceMap["interfaces"].push_back(i);
              }
            }
          }
        }
      }
      if(it->hasKey("outputs")) {
        ConfigVector::iterator it2 = (*it)["outputs"].begin();
        for(;it2!=(*it)["outputs"].end(); ++it2) {
          if(it2->hasKey("interface")) {
            if((int)((*it2)["interface"]) == 1 || (int)((*it2)["interface"]) == 2) {
              //if((*it2)["direction"] == "bidirectional") continue;
              ConfigMap i;
              std::string interfaceName;
              if(it2->hasKey("interfaceExportName")) {
                interfaceName << (*it2)["interfaceExportName"];
              }
              else {
                interfaceName << node["name"];
                interfaceName += ":" + (std::string)(*it2)["name"];
              }
              if(interfaceMap.hasKey(interfaceName)) continue;
              i["name"] =  interfaceName;
              i["type"] = (*it2)["type"];
              i["direction"] = mars::utils::toupper((*it2)["direction"]);
              i["linkToNode"] = node["name"];
              i["linkToInterface"] = (*it2)["name"];
              i["domain"] = mars::utils::toupper((*it2)["domain"]);
              if(interfaceMap.hasKey("interfaces")) {
                ConfigVector::iterator it3 = interfaceMap["interfaces"].begin();
                for(; it3!=interfaceMap["interfaces"].end(); ++it3) {
                  if((*it3)["name"] == interfaceName && ((*it3)["direction"] == "OUTGOING" || (*it3)["direction"] == "BIDIRECTIONAL")) {
                    break;
                  }
                }
                if(it3 == interfaceMap["interfaces"].end()) {
                  interfaceMap["interfaces"].push_back(i);
                }
              }
              else {
                interfaceMap["interfaces"].push_back(i);
              }
            }
          }
        }
      }
    }

    if(tmpInterfaces.hasKey("i")) {
      for(auto it : tmpInterfaces["i"]) {
        if(!it.hasKey("linkToNode")) {
          ConfigVector::iterator it3 = interfaceMap["interfaces"].begin();
          for(; it3!=interfaceMap["interfaces"].end(); ++it3) {
            if((std::string)(*it3)["name"] == (std::string)it["name"]) {
              break;
            }
          }
          if(it3 == interfaceMap["interfaces"].end()) {
            interfaceMap["interfaces"].push_back(it);
          }
        }
      }
    }
    output["configuration"] = config;
    it = map["edges"].begin();
    // todo: this filter can clash if the tags are used in edge data of graph model
    std::map<std::string, bool> filter;
    filter["fromNode"] = true;
    filter["fromNodeOutput"] = true;
    filter["sourceNode"] = true;
    filter["toNode"] = true;
    filter["toNodeInput"] = true;
    filter["id"] = true;
    filter["vertices"] = true;
    filter["decoupleVertices"] = true;
    for(; it!=map["edges"].end(); ++it) {
      ConfigMap edge;
      //todo: handle domain correctly
      std::string fromName = (*it)["fromNode"];
      std::string toName = (*it)["toNode"];
      edge["from"]["name"] = fromName;
      edge["from"]["interface"] = (*it)["fromNodeOutput"];
      edge["from"]["domain"] = mars::utils::toupper((*it)["domain"]);
      edge["to"]["name"] = toName;
      edge["to"]["interface"] = (*it)["toNodeInput"];
      edge["to"]["domain"] = mars::utils::toupper((*it)["domain"]);
      ConfigMap edgeData;
      ConfigMap::iterator it2 = it->beginMap();
      for(; it2!=it->endMap(); ++it2) {
        if(filter.find(it2->first) == filter.end()) {
          edgeData[it2->first] = it2->second;
        }
      }
      if(edgeData.size() > 0) {
        edge["data"] = edgeData.toYamlString();
      }
      if(!it->hasKey("name")) {
        edge["name"] = (*it)["id"].toString().c_str();
      }
      else {
        edge["name"] = (*it)["name"].getString();
      }
      if(edge.hasKey("direction")) {
        edge["direction"] = mars::utils::toupper(edge["direction"]);
      }
      output["edges"].push_back(edge);
    }
    //output.toYamlFile("da2.yml");
  }

  void ModelWidget::clear() {
    ignoreUpdate = true;
    edition = "";
    name->clear();
    type->clear();
    domain->clear();
    maturity->clear();
    projectName->clear();
    designedBy->clear();
    version->clear();
    //includes->clear();
    data->clear();
    interfaces->clear();
    layouts->clear();
    localMap = ConfigMap();
    layoutMap = ConfigMap();
    ignoreUpdate = false;
  }

  void ModelWidget::loadType(const std::string &domain,
                             const std::string &name,
                             const std::string &version) {
    if(domain == "software" && name == "Deployment") return;
    fprintf(stderr, "check type: %s %s %s\n", domain.c_str(), name.c_str(), version.c_str());
    Model *model = dynamic_cast<Model*>(bagelGui->getCurrentModel());
    if (model) {
      ConfigMap modelMap, nodeInfo;
      std::string type = name;
      if(!model->hasNodeInfo(type)) {
        modelMap = XRockDB::requestModel(domain, name, version, !version.empty());
        model->addNodeInfo(modelMap);//, version);
        bagelGui->updateNodeTypes();
        return;
      }
      nodeInfo = model->getNodeInfo(type);
      if(!version.empty() && nodeInfo["modelVersion"] != version) {
        type += "::" + version;
        if(!model->hasNodeInfo(type)) {
          modelMap = XRockDB::requestModel(domain, name, version, !version.empty());
          model->addNodeInfo(modelMap, version);
          bagelGui->updateNodeTypes();
        }
      }
    }
  }

  void ModelWidget::setModelInfo(configmaps::ConfigMap &model) {
    ignoreUpdate = true;
    localMap = model;
    type->setText(localMap["type"].getString().c_str());
    domain->setText(localMap["domain"].getString().c_str());
    name->setText(localMap["name"].getString().c_str());
    version->setText(localMap["versions"][0]["name"].getString().c_str());
    if(localMap["versions"][0].hasKey("projectName")) {
      projectName->setText(localMap["versions"][0]["projectName"].getString().c_str());
    }
    if(localMap["versions"][0].hasKey("designedBy")) {
      designedBy->setText(localMap["versions"][0]["designedBy"].getString().c_str());
    }
    if(localMap["versions"][0].hasKey("maturity")) {
      maturity->setText(localMap["versions"][0]["maturity"].getString().c_str());
    }
    if(localMap["versions"][0].hasKey("data")) {
      data->setText(localMap["versions"][0]["data"].getString().c_str());
    }
    if(localMap.hasKey("interfaces")) {
      interfaces->setText(localMap["interfaces"].getString().c_str());
      interfaces->setReadOnly((bool)localMap["editable_interfaces"]);
    }
    if(localMap.hasKey("layouts")) {
      layoutMap = localMap["layouts"];
      for(auto it: layoutMap) {
        layouts->addItem(it.first.c_str());
      }
      layouts->setCurrentItem(layouts->item(layouts->count()-1));
    }
    ignoreUpdate = false;
  }

  void ModelWidget::updateModelInfo() {
    if(ignoreUpdate) return;
    bagel_gui::ModelInterface *model = bagelGui->getCurrentModel();

    if (model) {
      localMap["name"]     = name->text().toStdString();
      localMap["type"]     = type->text().toStdString();
      localMap["versions"][0]["name"]  = version->text().toStdString();
      localMap["domain"]   = domain->text().toStdString();
      if(maturity->text().toStdString() != "") {
        localMap["versions"][0]["maturity"] = maturity->text().toStdString();
      }
      if(projectName->text().toStdString() == "") {
        ConfigMap &tmpMap = localMap["versions"][0];
        tmpMap.erase("projectName");
      } else {
        localMap["versions"][0]["projectName"] = projectName->text().toStdString();
      }
      if(designedBy->text().toStdString() == "") {
        ConfigMap &tmpMap = localMap["versions"][0];
        tmpMap.erase("designedBy");
      } else {
        localMap["versions"][0]["designedBy"] = designedBy->text().toStdString();
      }
      if(data->toPlainText().toStdString() == "") {
        ConfigMap &tmpMap = localMap["versions"][0];
        tmpMap.erase("data");
      } else {
        localMap["versions"][0]["data"] = data->toPlainText().toStdString();
      }
      if(interfaces->toPlainText().toStdString() != "") {
        localMap["interfaces"] = interfaces->toPlainText().toStdString();
        localMap["editable_interfaces"] = interfaces->isReadOnly();
      }
      if(layoutMap.size() > 0) {
        localMap["layouts"] = layoutMap;
      }
      else if(localMap.hasKey("layouts")) {
        localMap.erase("layouts");
      }
      for(int i=0; i<layouts->count(); ++i) {
        QVariant v = layouts->item(i)->data(0);
        if(v.isValid()) {
          std::string layout = v.toString().toStdString();
          localMap["defaultLayout"] = layout;
        }
      }
      model->setModelInfo(localMap);
    }
  }

  void ModelWidget::getModelInfo(std::string *domain, std::string *name,
                                 std::string *version) {
    *name = this->name->text().toStdString();
    *domain = this->domain->text().toStdString();
    *version = this->version->text().toStdString();
  }

  void ModelWidget::editLocalMap() {
    ConfigMap env;
    ConfigMap copy = localMap;
    {
      ConfigureDialog cd(&copy, env, "local map", true, true);
      cd.resize(400, 400);
      cd.exec();
    }
    layouts->clear();
    layoutMap = ConfigMap();
    setModelInfo(copy);
    updateModelInfo();
  }

  void ModelWidget::editDescription() {
    ConfigMap env;
    ConfigMap copy = localMap;
    std::string description;
    std::string domainData = copy["domain"];
    domainData += "Data";
    {
      if(copy["versions"][0].hasKey(domainData)) {
        if(copy["versions"][0][domainData].hasKey("data")) {
          if(copy["versions"][0][domainData]["data"].hasKey("description")) {
            if(copy["versions"][0][domainData]["data"]["description"].hasKey("markdown")) {
              description << copy["versions"][0][domainData]["data"]["description"]["markdown"];
            }
          }
        }
      }
      ConfigureDialog cd(&description);
      cd.resize(400, 400);
      cd.exec();
    }
    localMap["versions"][0][domainData]["data"]["description"]["markdown"] = description;
  }

  void ModelWidget::openUrl(const QUrl &link) {
    QDesktopServices::openUrl(link);
  }


} // end of namespace xrock_gui_model
