#include "ROCKTASKConfig.hpp"
#include <mars/config_map_gui/DataWidget.h>
#include <mars/utils/misc.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>

#include <sstream>
#include <iostream>
#include <fstream>

#include "../ComponentModelInterface.hpp"
using namespace configmaps;
using namespace bagel_gui;

namespace xrock_gui_model
{
  bool startsWith(const std::string &str, const std::string &prefix) { return str.compare(0, prefix.length(), prefix) == 0; }
  
  ConfigAtom ROCKTASKConfig::propertyToConfigAtom(ConfigItem &property)
  {
    std::string type = property["Type"];

    if (type == "bool")
    {
      return property.hasKey("DefaultVal") ? (bool)property["DefaultVal"] : false;
    }
    else if (type == "::std::string")
    {
      return property.hasKey("DefaultVal") ? (std::string)property["DefaultVal"] : std::string("");
    }
    else if (type == "double" || type == "float")
    {
      return property.hasKey("DefaultVal") ? (double)property["DefaultVal"] : 0.0;
    }
    else if (startsWith(type, "boost::int") || startsWith(type, "boost::uint"))
    {
      return property.hasKey("DefaultVal") ? (std::int32_t)property["DefaultVal"] : 0;
    }
    else
    {
      return ConfigAtom(); //ItemType::UNDEFINED_TYPE;
    }
  }

  void ROCKTASKConfig::initEditablePattern(ConfigMap &newConfig)
  {
      for (auto &property : nodeMap["model"]["versions"][0]["data"]["properties"])
      {
        std::string key = property["Name"].getString();
        std::string type = property["Type"].getString();
        
        if (mars::utils::tolower(key).find("frame") != std::string::npos && type == "::std::string") continue;
        if (key == "bool")
          continue;
        
        ConfigAtom atom = propertyToConfigAtom(property);

        //non-atom
        if (atom.getType() == ConfigAtom::ItemType::UNDEFINED_TYPE) {
          if (newConfig["config"].hasKey(key))
            dwConfig["config"][key] = newConfig["config"][key];
          else
            dwConfig["config"][key] = configmaps::ConfigMap();
        }
        else // atom (str, int, double..)
        {
          if (newConfig["config"].hasKey(key))
            dwConfig["config"][key] = newConfig["config"][key];
          else
            dwConfig["config"][key] = atom;
        }
      }
  }

  void ROCKTASKConfig::initCheckablePattern(ConfigMap &newConfig)
  {
    //std::vector<std::string> pattern;
    for (auto &property : nodeMap["model"]["versions"][0]["data"]["properties"])
    {
      std::string key = property["Name"].getString();
      std::string type = property["Type"].getString();
      if (type == "bool")
      {
        if (newConfig["config"].hasKey(key))
        {
          dwConfig["config"][key] = (bool)newConfig["config"][key];
        }
        else
        {
          if (property.hasKey("DefaultVal"))
          {
            dwConfig["config"][key] = (bool)property["DefaultVal"];
          }
          else
          {
            dwConfig["config"][key] = false;
          }
        }
        checkablePattern.push_back("../config/" + key);
      }
    }
   // dw->setCheckablePattern(pattern);
  }

  void ROCKTASKConfig::initDropDownPattern(ConfigMap &newConfig)
   {
  //   std::vector<std::string> pattern;
  //   std::vector<std::vector<std::string>> values;
    // Activity and state
    dwConfig["activity"]["type"] = "PERIODIC";
    dwConfig["state"] = "PREOPERATIONAL";

    dropDownPattern.push_back("../activity/type");
    dropDownPattern.push_back("../state");

    dropDownValues.resize(dropDownPattern.size());
    dropDownValues[0].push_back("PERIODIC"); // ../activity/type
    dropDownValues[0].push_back("FD_DRIVEN");
    dropDownValues[0].push_back("TRIGGERED");
    dropDownValues[1].push_back("PRREOPERATIONAL"); // ../state
    dropDownValues[1].push_back("RUNNING");
    dropDownValues[1].push_back("STOPPED");

    // Frame names
    for (auto &property : nodeMap["model"]["versions"][0]["data"]["properties"])
    {
      std::string key = property["Name"].getString();
      std::string type = property["Type"].getString();
      if (mars::utils::tolower(key).find("frame") != std::string::npos && type == "::std::string")
      {
        dropDownPattern.push_back("../config/" + key);
        dropDownValues.emplace_back();

        if(newConfig["config"].hasKey(key)) {
          dropDownValues.back().push_back(newConfig["config"][key]);
          dwConfig["config"][key] = newConfig["config"][key];
        }
        else
        {
          if (property.hasKey("DefaultVal"))
          {
            dropDownValues.back().push_back((std::string)property["DefaultVal"]);
            dwConfig["config"][key] = (std::string)property["DefaultVal"];
          }
          else
          {
            dropDownValues.back().push_back("-");
            dwConfig["config"][key] = "-";
          }

        }
        if (globalConfig.hasKey("frameNames"))
        {
          for (auto &frameName : globalConfig["frameNames"])
          {
            dropDownValues.back().push_back((std::string)frameName);
          }
        }
      }
    }
    
  }

  void ROCKTASKConfig::postUpdateCheckablePattern()
  {
    //std::vector<std::string> pattern;
    // Update the dw boolean values 'true' and 'false' should be checkboxes.

     if(dwConfig.hasKey("activity"))
     {
  
        auto it = dwConfig["activity"].beginMap();
        for (; it != dwConfig["activity"].endMap(); ++it)
        {
          auto &[key, value] = *it;
          

          if (value.isAtom() ) {
           
            ConfigAtom& atom = static_cast<ConfigAtom &>(value);
            std::string v = mars::utils::tolower(atom.getString());
            if (v == "false" || v == "true")
            {
              value = ConfigAtom(v == "true");
              checkablePattern.push_back("../activity/" + key);
            }
          }
        }
     }
   
  }



  void ROCKTASKConfig::updateDataWidget(ConfigMap &newConfig)
  {
    dropDownPattern.clear();
    dropDownValues.clear();
    checkablePattern.clear();
    initDropDownPattern(newConfig);
    initCheckablePattern(newConfig);
    initEditablePattern(newConfig);
    dwConfig.updateMap(newConfig);
    postUpdateCheckablePattern();
    dw->setCheckablePattern(checkablePattern);
    dw->setDropDownPattern(dropDownPattern, dropDownValues);
    dw->setConfigMap("", dwConfig);
  }

  ROCKTASKConfig::ROCKTASKConfig(XRockGUI *xrockGui, configmaps::ConfigMap *configuration,
                                 configmaps::ConfigMap &env,
                                 configmaps::ConfigMap &globalConfig,
                                 const std::string &type, bool onlyMap,
                                 bool noTreeEdit, configmaps::ConfigMap *dropdown, std::string fileName) : xrockGui(xrockGui), configuration(configuration), globalConfig(globalConfig), textOnly(false)
  {
    configMapEdit = nullptr;
    dw = nullptr;
    statusLabel = nullptr;
    nodeMap = xrockGui->getBagelGui()->getCurrentTabView()->getView()->getSelectedNodeMap(); // TODO: will be better to pass in nodeMap from XROCKGUI.cpp
    std::string nodeName = nodeMap["model"]["name"];
    this->setWindowTitle(QString::fromStdString(nodeName+" Configuration"));
    if (fileName.size())
    {
      // open file
      configFile = new QTextEdit();
      configFileName = fileName;
      std::ifstream t(configFileName.c_str());
      std::stringstream buffer;
      buffer << t.rdbuf();
      configFileContent = buffer.str();
      configFile->setText(configFileContent.c_str());
    }
    else
    {
      configMapEdit = new QTextEdit();
      configMapEdit->setText(configuration->toYamlString().c_str());
      connect(configMapEdit, SIGNAL(textChanged()), this, SLOT(textChanged()));
      statusLabel = new QLabel();
      statusLabel->setText("valid yaml syntax");
    }
    dw = new mars::config_map_gui::DataWidget(nullptr, this, true, false);

    ConfigMap &config = *configuration;
    updateDataWidget(config);

    connect(dw, SIGNAL(mapChanged()), this, SLOT(mapChanged()));

    QVBoxLayout *v = new QVBoxLayout();
    v->setContentsMargins(0, 0, 0, 0);
    if (!onlyMap)
    {
      QLabel *l = new QLabel("config file:");
      configFile = new QTextEdit();
      configFileName = "";
      // try to open config file
      {
        // contruct path starting with RockRoot
        // todo: check override behavior
        std::string bundle = env["BundelEnvVarName"];
        std::string path = env["ConfigDir"];
        if (env.hasKey("RockRoot"))
        {
          path << env["RockRoot"];
        }
        const char *envS;
        if ((envS = getenv("AUTOPROJ_CURRENT_ROOT")))
        {
          path = envS;
        }
        if (path.back() != '/')
          path += "/";
        if (env.hasKey("BundleFolder"))
        {
          path += (std::string)env["BundleFolder"];
        }
        else
          path += "bundles/";
        if (path.back() != '/')
          path += "/";
        if ((envS = getenv(bundle.c_str())))
        {
          path += envS;
          if (path.back() != '/')
            path += "/";
          if (env.hasKey("BundelOrogenConfigFolder"))
          {
            path += (std::string)env["BundelOrogenConfigFolder"];
          }
          else
            path += "config/orogen/";
          if (path.back() != '/')
            path += "/";
          path += type + ".yml";
          std::cerr << "check for config file: " << path.c_str() << std::endl;
          if (mars::utils::pathExists(path))
          {
            configFileName = path;
            std::ifstream t(path.c_str());
            std::stringstream buffer;
            buffer << t.rdbuf();
            configFileContent = buffer.str();
            configFile->setText(configFileContent.c_str());
          }
          else
          {
            // todo: generate configuration file
          }
        }
      }
      QSplitter *split = new QSplitter(Qt::Vertical, nullptr);
      if (dw)
      {
        split->addWidget(dw);
      }
      else
      {
        split->addWidget(configMapEdit);
      }
      QVBoxLayout *vLayout2 = new QVBoxLayout();
      vLayout2->setContentsMargins(0, 0, 0, 0);
      vLayout2->addWidget(l);
      vLayout2->addWidget(configFile);
      QWidget *w2 = new QWidget();
      w2->setLayout(vLayout2);
      split->addWidget(w2);
      v->addWidget(split);
    }
    else
    {
      v->addWidget(dw);
      v->addWidget(configMapEdit);
      configMapEdit->setText(dwConfig.toYamlString().c_str());
    }

    if (configMapEdit)
    {
      statusLabel->setStyleSheet("QLabel { background-color : green; color: white; }");
      v->addWidget(statusLabel);
      lastTextChanged = 0;
      ticks = 0;
      startTimer(100);
    }
    QPushButton *button = new QPushButton("close");
    v->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(close()));
    setLayout(v);
  }

  ROCKTASKConfig::ROCKTASKConfig(std::string *text) : text(text), textOnly(true)
  {
    configMapEdit = nullptr;
    dw = nullptr;
    statusLabel = nullptr;
    configMapEdit = new QTextEdit();
    configMapEdit->setText(text->c_str());
    connect(configMapEdit, SIGNAL(textChanged()), this, SLOT(textChanged()));
    QVBoxLayout *v = new QVBoxLayout();
    v->setContentsMargins(0, 0, 0, 0);
    v->addWidget(configMapEdit);
    QPushButton *button = new QPushButton("close");
    v->addWidget(button);
    connect(button, SIGNAL(clicked()), this, SLOT(close()));
    setLayout(v);
  }

  ROCKTASKConfig::~ROCKTASKConfig()
  {
    if (!configFileName.empty())
    {
      std::string content = configFile->toPlainText().toStdString();
      if (content != configFileContent)
      {
        FILE *f = fopen(configFileName.c_str(), "w");
        fprintf(f, "%s", content.c_str());
        fclose(f);
      }
    }
    else
    {
      if (textOnly)
      {
        *text = configMapEdit->toPlainText().toStdString();
      }
      else
      {
        try
        {
          ConfigMap tmpMap = ConfigMap::fromYamlString(configMapEdit->toPlainText().toStdString());
          *configuration = tmpMap;
        }
        catch (...)
        {
          std::cerr << "Error converting config into yaml map!" << std::endl;
        }
      }
    }

  }

  void ROCKTASKConfig::textChanged()
  {
    lastTextChanged = ticks;
  }

  void ROCKTASKConfig::timerEvent(QTimerEvent *event)
  {


    if (lastTextChanged != -1 && ticks - lastTextChanged > 5)
    {
      try
      {
        ConfigMap newConfig = ConfigMap::fromYamlString(configMapEdit->toPlainText().toStdString());
        statusLabel->setText("valid yaml syntax");
        statusLabel->setStyleSheet("QLabel { background-color : green; color: white;}");

        updateDataWidget(newConfig);

        fprintf(stderr, "dw updated from text edit");
      }
      catch (...)
      {
        statusLabel->setText("invalid yaml syntax");
        statusLabel->setStyleSheet("QLabel { background-color : red; color: white;}");
      }
      lastTextChanged = -1;
      ticks = -1;
    }
    ticks += 1;
  }

  QDialog *ROCKTASKConfigLoader::createDialog(configmaps::ConfigMap *configuration,
                                              configmaps::ConfigMap &env,
                                              configmaps::ConfigMap &globalConfig)
  {
    QDialog *d = new ROCKTASKConfig(xrockGui, configuration, env, globalConfig, "ROCK::TASK", true, true);
    return d;
  }

  void ROCKTASKConfig::mapChanged()
  {
    dwConfig = dw->getConfigMap();
    configMapEdit->setText(dwConfig.toYamlString().c_str());
    lastTextChanged = -1;
    ticks = -1;
  }

} // end of namespace xrock_gui_model
