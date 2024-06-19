#include "ConfigureDialog.hpp"
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

    ConfigureDialog::ConfigureDialog(configmaps::ConfigMap *configuration,
                                     configmaps::ConfigMap &env,
                                     const std::string &type, bool onlyMap,
                                     bool noTreeEdit, configmaps::ConfigMap *dropdown,
                                     configmaps::ConfigMap *urls, std::string fileName) : configuration(configuration), textOnly(false)
    {

        // get data from database
        // QVBoxLayout *vLayout = new QVBoxLayout();
        configMapEdit = NULL;
        dw = NULL;
        statusLabel = NULL;
        if (noTreeEdit)
        {
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
        }
        else
        {
            dw = new mars::config_map_gui::DataWidget(NULL, this, true, false);

            std::vector<std::string> pattern;
            std::vector<std::vector<std::string>> values;
            if (dropdown)
            {
                values.resize(dropdown->size());
                size_t i = 0;
                for (auto it : *dropdown)
                {
                    pattern.push_back("../" + it.first);
                    for (auto it2 : (ConfigVector &)it.second)
                    {
                        values[i].push_back(it2);
                    }
                    ++i;
                }
            }
            else
            {
                pattern.push_back("../activity/type");
                pattern.push_back("../state");
                values.resize(2);
                values[0].push_back("PERIODIC");
                values[0].push_back("FD_DRIVEN");
                values[0].push_back("TRIGGERED");
                values[1].push_back("RUNNING");
                values[1].push_back("STOPPED");
            }
            dw->setDropDownPattern(pattern, values);
            if(urls)
            {
                pattern.clear();
                for(auto it: (*urls)["pattern"])
                {
                    pattern.push_back(it);
                }
                dw->setFilePattern(pattern);
            }
            dw->setConfigMap("", *configuration);
        }

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
            if (dw)
            {
                v->addWidget(dw);
            }
            else if (configFileName.size())
            {
                v->addWidget(configFile);
            }
            else
            {
                v->addWidget(configMapEdit);
            }
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

    ConfigureDialog::ConfigureDialog(std::string *text) : text(text), textOnly(true)
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

    ConfigureDialog::~ConfigureDialog()
    {
        if (dw)
        {
            *configuration = dw->getConfigMap();
        }
        else if (!configFileName.empty())
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

    void ConfigureDialog::textChanged()
    {
        lastTextChanged = ticks;
    }

    void ConfigureDialog::timerEvent(QTimerEvent *event)
    {
        if (lastTextChanged != -1 && ticks - lastTextChanged > 5)
        {
            try
            {
                ConfigMap tmpMap = ConfigMap::fromYamlString(configMapEdit->toPlainText().toStdString());
                statusLabel->setText("valid yaml syntax");
                statusLabel->setStyleSheet("QLabel { background-color : green; color: white;}");
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

} // end of namespace xrock_gui_model
