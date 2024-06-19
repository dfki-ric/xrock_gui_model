#include "MARSIMUConfig.hpp"
#include <config_map_gui/DataWidget.h>
#include <mars_utils/misc.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>

#include <sstream>
#include <iostream>
#include <fstream>

using namespace configmaps;

namespace xrock_gui_model
{

    MARSIMUConfig::MARSIMUConfig(configmaps::ConfigMap *configuration,
                                 configmaps::ConfigMap &env,
                                 configmaps::ConfigMap &globalConfig,
                                 const std::string &type, bool onlyMap,
                                 bool noTreeEdit, configmaps::ConfigMap *dropdown, std::string fileName) : configuration(configuration), textOnly(false)
    {

        // get data from database
        // QVBoxLayout *vLayout = new QVBoxLayout();
        this->setWindowTitle("mars::IMU Configuration");
        configMapEdit = NULL;
        dw = NULL;
        statusLabel = NULL;
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

        ConfigMap &config = *configuration;
        dwConfig["activity"]["type"] = "PERIODIC";
        dwConfig["state"] = "PREOPERATIONAL";
        dwConfig["config"]["imu_frame"] = "-";
        dwConfig["config"]["world_frame"] = "-";
        dwConfig["config"]["provide_orientation"] = true;
        dwConfig["config"]["provide_velocity"] = true;
        dwConfig["config"]["provide_position"] = true;

        dw = new mars::config_map_gui::DataWidget(NULL, this, true, false);

        std::vector<std::string> pattern;
        pattern.push_back("../activity/type");
        pattern.push_back("../state");
        pattern.push_back("../config/imu_frame");
        pattern.push_back("../config/world_frame");
        values.resize(pattern.size());

        values[0].push_back("PERIODIC");
        values[0].push_back("FD_DRIVEN");
        values[0].push_back("TRIGGERED");
        values[1].push_back("PRREOPERATIONAL");
        values[1].push_back("RUNNING");
        values[1].push_back("STOPPED");

        values[2].push_back("-");
        values[3].push_back("-");
        if(globalConfig.hasKey("frameNames"))
        {
            for(auto &it: globalConfig["frameNames"])
            {
                values[2].push_back((std::string)it);
                values[3].push_back((std::string)it);
            }
        }
        dw->setDropDownPattern(pattern, values);

        pattern.clear();
        pattern.push_back("../config/provide_orientation");
        pattern.push_back("../config/provide_velocity");
        pattern.push_back("../config/provide_position");
        dw->setCheckablePattern(pattern);
        
        if(config.hasKey("activity") &&
           config["activity"].hasKey("type"))
        {
            dwConfig["activity"]["type"] = config["activity"]["type"];            
        }
        if(config.hasKey("state"))
        {
            dwConfig["state"] = config["state"];
        }
        if(config.hasKey("config"))
        {
            if(config["config"].hasKey("provide_orientation"))
            {
                dwConfig["config"]["provide_orientation"] = (bool)config["config"]["provide_orientation"];
            }
            if(config["config"].hasKey("provide_velocity"))
            {
                dwConfig["config"]["provide_velocity"] = (bool)config["config"]["provide_velocity"];
            }
            if(config["config"].hasKey("provide_position"))
            {
                dwConfig["config"]["provide_position"] = (bool)config["config"]["provide_position"];
            }
            if(config["config"].hasKey("world_frame"))
            {
                std::string frame = config["config"]["world_frame"];
                for(auto &it: values[2])
                {
                    if(it == frame)
                    {
                        dwConfig["config"]["world_frame"] = frame;
                        break;
                    }
                }
            }
            if(config["config"].hasKey("imu_frame"))
            {
                std::string frame = config["config"]["imu_frame"];
                for(auto &it: values[2])
                {
                    if(it == frame)
                    {
                        dwConfig["config"]["imu_frame"] = frame;
                        break;
                    }
                }
            }
        }

        dw->setConfigMap("", dwConfig);
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
            QSplitter *split = new QSplitter(Qt::Vertical, NULL);
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
        // if(configFileName.size()) {
        //   QPushButton *button = new QPushButton("save");
        //   v->addWidget(button);
        //   connect(button, SIGNAL(clicked()), this, SLOT(save()));
        // }
        setLayout(v);
    }

    MARSIMUConfig::MARSIMUConfig(std::string *text) : text(text), textOnly(true)
    {

        configMapEdit = NULL;
        dw = NULL;
        statusLabel = NULL;
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

    MARSIMUConfig::~MARSIMUConfig()
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

    void MARSIMUConfig::textChanged()
    {
        lastTextChanged = ticks;
    }

    void MARSIMUConfig::timerEvent(QTimerEvent *event)
    {
        if (lastTextChanged != -1 && ticks - lastTextChanged > 5)
        {
            try
            {
                ConfigMap tmpMap = ConfigMap::fromYamlString(configMapEdit->toPlainText().toStdString());
                statusLabel->setText("valid yaml syntax");
                statusLabel->setStyleSheet("QLabel { background-color : green; color: white;}");
                // update dwMap ...
                if(tmpMap.hasKey("activity") &&
                   tmpMap["activity"].hasKey("type"))
                {
                    dwConfig["activity"]["type"] = tmpMap["activity"]["type"];            
                }
                dwConfig["state"] = "PREOPERATIONAL";
                if(tmpMap.hasKey("state"))
                {
                    dwConfig["state"] = tmpMap["state"];
                }
                if(tmpMap.hasKey("config"))
                {
                    if(tmpMap["config"].hasKey("world_frame"))
                    {
                        std::string frame = tmpMap["config"]["world_frame"];
                        dwConfig["config"]["world_frame"] = "-";
                        for(auto &it: values[2])
                        {
                            if(it == frame)
                            {
                                dwConfig["config"]["world_frame"] = frame;
                                break;
                            }
                        }
                    }
                    if(tmpMap["config"].hasKey("imu_frame"))
                    {
                        std::string frame = tmpMap["config"]["imu_frame"];
                        dwConfig["config"]["imu_frame"] = "-";
                        for(auto &it: values[2])
                        {
                            if(it == frame)
                            {
                                dwConfig["config"]["imu_frame"] = frame;
                                break;
                            }
                        }
                    }
                    if(tmpMap["config"].hasKey("provide_orientation"))
                    {
                        dwConfig["config"]["provide_orientation"] = (bool)tmpMap["config"]["provide_orientation"];
                    }
                    if(tmpMap["config"].hasKey("provide_velocity"))
                    {
                        dwConfig["config"]["provide_velocity"] = (bool)tmpMap["config"]["provide_velocity"];
                    }
                    if(tmpMap["config"].hasKey("provide_position"))
                    {
                        dwConfig["config"]["provide_position"] = (bool)tmpMap["config"]["provide_position"];
                    }
                }
                dw->setConfigMap("", dwConfig);
                fprintf(stderr, ".");
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

    QDialog* MARSIMUConfigLoader::createDialog(configmaps::ConfigMap *configuration,
                                               configmaps::ConfigMap &env,
                                               configmaps::ConfigMap &globalConfig)
    {
        QDialog *d = new MARSIMUConfig(configuration, env, globalConfig, "mars::IMU", true, true);
        return d;
    }

    void MARSIMUConfig::mapChanged()
    {
        dwConfig = dw->getConfigMap();
        ConfigMap &config = *configuration;
        config["activity"]["type"] = dwConfig["activity"]["type"];
        config["state"] = dwConfig["state"];
        if(dwConfig["world_frame"] != "-")
        {
            config["config"]["world_frame"] = dwConfig["config"]["world_frame"];
        }
        if(dwConfig["imu_frame"] != "-")
        {
            config["config"]["imu_frame"] = dwConfig["config"]["imu_frame"];
        }
        config["config"]["provide_orientation"] = dwConfig["config"]["provide_orientation"];
        config["config"]["provide_velocity"] = dwConfig["config"]["provide_velocity"];
        config["config"]["provide_position"] = dwConfig["config"]["provide_position"];
        configMapEdit->setText(config.toYamlString().c_str());
        lastTextChanged = -1;
        ticks = -1;
    }

} // end of namespace xrock_gui_model
