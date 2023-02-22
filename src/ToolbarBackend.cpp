/*
 *  Copyright 2022, DFKI GmbH Robotics Innovation Center
 */

#include "ToolbarBackend.hpp"
#include "XRockGUI.hpp"
#include "ComponentModelInterface.hpp"
#include "ComponentModelEditorWidget.hpp"
#include "ImportDialog.hpp"
#include "FileDB.hpp"
#include "VersionDialog.hpp"
#include "ConfigureDialog.hpp"
#include "ConfigMapHelper.hpp"
#include <lib_manager/LibManager.hpp>
#include <bagel_gui/BagelGui.hpp>
#include <bagel_gui/BagelModel.hpp>
#include <mars/main_gui/MainGUI.h>
#include <QLineEdit>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <cstdlib>
#include <mars/utils/misc.h>
using namespace xrock_gui_model;

ToolbarBackend::ToolbarBackend(XRockGUI *xrockGui, mars::main_gui::GuiInterface *gui)
    : xrockGui(xrockGui), mainGui(dynamic_cast<mars::main_gui::MainGUI *>(gui))
{
    QToolBar *toolbar = mainGui->getToolbar("Actions");

    // Backends
    cbBackends = new QComboBox;
    std::vector<std::string> backends;

    if (xrockGui->ioLibrary)
    {
        backends = xrockGui->ioLibrary->getBackends();
    }
    else
    {
        backends.push_back("FileDB");
    }
    for (auto const &e : backends)
        cbBackends->addItem(QString::fromStdString(e));
    cbBackends->setCurrentIndex(cbBackends->findText(QString::fromStdString(xrockGui->getBackend()), Qt::MatchFixedString));
    toolbar->addWidget(cbBackends);

    connect(cbBackends, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(onBackendChanged(const QString &)));

    // Db Path
    QLabel *label = new QLabel(" DB Path: ");
    leDbPath = new QLineEdit;
    leDbPath->setText("modkom/component_db");
    leDbPath->setFixedWidth(150);
    ActionLabelPath = toolbar->addWidget(label);
    widgetActionPath = toolbar->addWidget(leDbPath);
    connect(leDbPath, SIGNAL(textChanged(const QString &)), this, SLOT(onDbPathChanged(const QString &)));

    // URL
    label = new QLabel(" URL: ");
    leUrl = new QLineEdit;
    leUrl->setText("http://0.0.0.0");
    leUrl->setFixedWidth(120);
    ActionLabelUrl = toolbar->addWidget(label);
    widgetActionUrl = toolbar->addWidget(leUrl);
    connect(leUrl, SIGNAL(textChanged(const QString &)), this, SLOT(onUrlChanged(const QString &)));

    // Port
    label = new QLabel(" Port: ");
    lePort = new QLineEdit;
    lePort->setText("8183");
    lePort->setFixedWidth(100);
    ActionLabelPort = toolbar->addWidget(label);
    widgetActionPort = toolbar->addWidget(lePort);
    connect(lePort, SIGNAL(textChanged(const QString &)), this, SLOT(onPortChanged(const QString &)));

    // Graph
    label = new QLabel(" Graph: ");
    leSgraph = new QLineEdit;
    leSgraph->setText("graph_test");
    leSgraph->setFixedWidth(120);
    leCgraph = new QLineEdit;
    leCgraph->setText("graph_test");
    leCgraph->setFixedWidth(120);
    ActionLabelGraph = toolbar->addWidget(label);
    widgetActionGraphS = toolbar->addWidget(leSgraph);
    widgetActionGraphC = toolbar->addWidget(leCgraph);
    connect(leSgraph, SIGNAL(textChanged(const QString &)), this, SLOT(onGraphChanged(const QString &)));
    connect(leCgraph, SIGNAL(textChanged(const QString &)), this, SLOT(onGraphChanged(const QString &)));

    // multidb config dialog icon
    configDialogAction = toolbar->addAction("Open MultiDbClient Config");
    const std::string icon = mars::utils::pathJoin(XROCK_DEFAULT_RESOURCES_PATH, "xrock_gui_model/resources/images/");
    configDialogAction->setIcon(QIcon(QString::fromStdString(icon + "config.png")));
    toolbar->addAction(configDialogAction);
    connect(configDialogAction, SIGNAL(triggered()), this, SLOT(popUpConfigDialog()));

    // Load default values to toolbar widgets if any bundle was selected
    if (xrockGui->ioLibrary)
    {
        auto defaultConfig = xrockGui->ioLibrary->getDefaultConfig();
        if (!defaultConfig.empty())
        {
            if (defaultConfig.hasKey("Client"))
            {
                std::string fullUrl = (std::string)defaultConfig["Client"]["url"];
                std::string url = fullUrl.substr(0, fullUrl.find_last_of(':'));
                std::string port = fullUrl.substr(url.size() + 1);
                leUrl->setText(QString::fromStdString(url));
                lePort->setText(QString::fromStdString(port));
                leCgraph->setText(QString::fromStdString((std::string)defaultConfig["Client"]["graph"]));
            }

            if (defaultConfig.hasKey("Serverless"))
            {
                leDbPath->setText(QString::fromStdString((std::string)defaultConfig["Serverless"]["path"]));
                leSgraph->setText(QString::fromStdString((std::string)defaultConfig["Serverless"]["graph"]));
            }
        }
    }

    // switch initial visibility
    showToolbarWidgets(QString::fromStdString(xrockGui->getBackend()));
}

ToolbarBackend::~ToolbarBackend()
{
    delete cbBackends;
    delete leDbPath;
    delete leUrl;
    delete lePort;
    delete leCgraph;
    delete leSgraph;
}

void ToolbarBackend::hideToolbarWidgets(const QString &backend)
{
    if (backend == "Client")
    {
        widgetActionUrl->setVisible(false);
        ActionLabelUrl->setVisible(false);
        widgetActionPort->setVisible(false);
        ActionLabelPort->setVisible(false);
        widgetActionGraphC->setVisible(false);
        configDialogAction->setVisible(false);
    }
    else if (backend == "Serverless")
    {
        widgetActionPath->setVisible(false);
        ActionLabelPath->setVisible(false);
        widgetActionGraphS->setVisible(false);
        configDialogAction->setVisible(false);
    }
    else if (backend == "MultiDbClient")
    {
        configDialogAction->setVisible(false);
    }
}
void ToolbarBackend::showToolbarWidgets(const QString &backend)
{
    if (backend == "Client")
    {
        widgetActionUrl->setVisible(true);
        ActionLabelUrl->setVisible(true);
        widgetActionPort->setVisible(true);
        ActionLabelPort->setVisible(true);
        widgetActionGraphC->setVisible(true);
        ActionLabelGraph->setVisible(true);
        hideToolbarWidgets("Serverless");
    }
    else if (backend == "Serverless")
    {
        widgetActionPath->setVisible(true);
        ActionLabelPath->setVisible(true);
        widgetActionGraphS->setVisible(true);
        ActionLabelGraph->setVisible(true);
        hideToolbarWidgets("Client");
    }
    else if (backend == "MultiDbClient")
    {
        hideToolbarWidgets("Serverless");
        hideToolbarWidgets("Client");
        widgetActionGraphS->setVisible(false);
        widgetActionGraphC->setVisible(false);
        ActionLabelGraph->setVisible(false);
        configDialogAction->setVisible(true);
    }
}

void ToolbarBackend::onBackendChanged(const QString &newBackend)
{
    showToolbarWidgets(newBackend);
    if (newBackend == "Serverless")
    {
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_SERVERLESS));
    }
    else if (newBackend == "Client")
    {
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_CLIENT));
    }
    else if (newBackend == "MultiDbClient")
    {
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_MULTIDB));
    }
    else if (newBackend == "FileDB")
    {
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_FILEDB));
    }
    else
    {
        throw std::runtime_error("Unhandled backend type " + newBackend.toStdString());
    }
}

void ToolbarBackend::onDbPathChanged(const QString &Path)
{
    std::string dbPath = mars::utils::pathJoin(std::getenv("AUTOPROJ_CURRENT_ROOT"), Path.toStdString());
    xrockGui->db->set_dbPath(dbPath);
}

void ToolbarBackend::onUrlChanged(const QString &url)
{
    std::string dbAddress = url.toStdString() + ':' + lePort->text().toStdString();
    xrockGui->db->set_dbAddress(dbAddress);
}

void ToolbarBackend::onPortChanged(const QString &port)
{
    std::string dbAddress = leUrl->text().toStdString() + ':' + port.toStdString();
    xrockGui->db->set_dbAddress(dbAddress);
}

void ToolbarBackend::onGraphChanged(const QString &graph)
{
    std::string _graph_name = graph.toStdString();
    xrockGui->db->set_dbGraph(_graph_name);
}

std::string ToolbarBackend::getDbPath()
{
    return leDbPath->text().toStdString();
}

std::string ToolbarBackend::getdbAddress()
{
    std::string dbAddress = leUrl->text().toStdString() + ':' + lePort->text().toStdString();
    return dbAddress;
}

std::string ToolbarBackend::getGraph()
{
    if (cbBackends->currentText() == "Serverless")
        return leSgraph->text().toStdString();
    else if (cbBackends->currentText() == "Client")
        return leCgraph->text().toStdString();
    else
        throw std::runtime_error("could not get graph for selected backend: " + cbBackends->currentText().toStdString());
}

void ToolbarBackend::popUpConfigDialog()
{
    xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_MULTIDB));
}