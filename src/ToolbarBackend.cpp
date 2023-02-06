/*
 *  Copyright 2022, DFKI GmbH Robotics Innovation Center
 */

#include "ToolbarBackend.hpp"
#include "XRockGUI.hpp"
#include "ComponentModelInterface.hpp"
#include "ComponentModelEditorWidget.hpp"
#include "ImportDialog.hpp"
#include "FileDB.hpp"
// #include "RestDB.hpp"
// #include "ServerlessDB.hpp"
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
    : xrockGui(xrockGui), main_gui(dynamic_cast<mars::main_gui::MainGUI *>(gui))
{
    QToolBar *toolbar = main_gui->getToolbar("Actions");

    // Backends
    cb_backends = new QComboBox;
    std::vector<std::string> backends;

    if (xrockGui->ioLibrary)
    {
        backends = xrockGui->ioLibrary->getBackends();
    }
    else
    {
        backends.push_back("FileDB");
    }
    int i = 0;
    std::string backend;
    for (auto const &e : backends)
    {
        cb_backends->addItem(QString::fromStdString(e));
        if(e == xrockGui->getBackend())
        {
            cb_backends->setCurrentIndex(i);
            backend = e;
        }
        ++i;
    }
    toolbar->addWidget(cb_backends);

    connect(cb_backends, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(on_backend_changed(const QString &)));

    // Db Path
    QLabel *label = new QLabel(" DB Path: ");
    le_db_path = new QLineEdit;
    le_db_path->setText("modkom/component_db");
    le_db_path->setFixedWidth(150);
    ActionLabelPath = toolbar->addWidget(label);
    widgetActionPath = toolbar->addWidget(le_db_path);
    actions.push_back(ActionLabelPath);
    actions.push_back(widgetActionPath);
    connect(le_db_path, SIGNAL(textChanged(const QString &)), this, SLOT(on_db_path_changed(const QString &)));

    // URL
    label = new QLabel(" URL: ");
    le_url = new QLineEdit;
    le_url->setText("http://0.0.0.0");
    label->hide();
    le_url->hide();
    le_url->setFixedWidth(120);
    ActionLabelUrl = toolbar->addWidget(label);
    widgetActionUrl = toolbar->addWidget(le_url);
    actions.push_back(ActionLabelUrl);
    actions.push_back(widgetActionUrl);

    connect(le_url, SIGNAL(textChanged(const QString &)), this, SLOT(on_url_changed(const QString &)));

    // Port
    label = new QLabel(" Port: ");
    le_port = new QLineEdit;
    le_port->setText("8183");
    label->hide();
    le_port->hide();
    le_port->setFixedWidth(100);
    ActionLabelPort = toolbar->addWidget(label);
    widgetActionPort = toolbar->addWidget(le_port);
    actions.push_back(ActionLabelPort);
    actions.push_back(widgetActionPort);
    connect(le_port, SIGNAL(textChanged(const QString &)), this, SLOT(on_port_changed(const QString &)));

    // Graph
    label = new QLabel(" Graph: ");
    le_graph = new QLineEdit;
    //le_graph->setText("graph_test");
    le_graph->setFixedWidth(120);
    ActionLabelGraph = toolbar->addWidget(label);
    widgetActionGraph = toolbar->addWidget(le_graph);
    actions.push_back(ActionLabelGraph);
    actions.push_back(widgetActionGraph);
    connect(le_graph, SIGNAL(textChanged(const QString &)), this, SLOT(on_graph_changed(const QString &)));

    // Load default values 
    if (xrockGui->ioLibrary)
    {

        auto default_config = xrockGui->ioLibrary->getDefaultConfig();
        if (!default_config.empty() )
        {
            if(default_config.hasKey("Serverless"))
            {
                le_db_path->setText(QString::fromStdString((std::string)default_config["Serverless"]["path"]));
                le_graph->setText(QString::fromStdString((std::string)default_config["Serverless"]["graph"]));
            }
            if(default_config.hasKey("Client"))
            {
                std::string full_url = (std::string)default_config["Client"]["url"];
                std::string url= full_url.substr(0, full_url.find_last_of(':'));
                std::string port= full_url.substr(url.size()+1);
                le_url->setText(QString::fromStdString(url));
                le_port->setText(QString::fromStdString(port));
                le_graph->setText(QString::fromStdString((std::string)default_config["Client"]["graph"]));
            }
        }
    }

    // switch initial visibility
    if (backend == "Client")
    {
        widgetActionUrl->setVisible(true);
        ActionLabelUrl->setVisible(true);
        widgetActionPort->setVisible(true);
        ActionLabelPort->setVisible(true);
        widgetActionGraph->setVisible(true);
        ActionLabelGraph->setVisible(true);
        widgetActionPath->setVisible(false);
        ActionLabelPath->setVisible(false);
    }
    else if (backend == "MultiDbClient")
    {
        for (auto &action : actions)
        {
            action->setVisible(false);
        }
    }
}

ToolbarBackend::~ToolbarBackend()
{
    delete cb_backends;
    delete le_db_path;
    delete le_url;
    delete le_port;
    delete le_graph;
}

void ToolbarBackend::on_backend_changed(const QString &new_backend)
{
    if (new_backend == "Serverless")
    {

        widgetActionPath->setVisible(true);
        ActionLabelPath->setVisible(true);
        widgetActionGraph->setVisible(true);
        ActionLabelGraph->setVisible(true);
        widgetActionUrl->setVisible(false);
        ActionLabelUrl->setVisible(false);
        widgetActionPort->setVisible(false);
        ActionLabelPort->setVisible(false);
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_SERVERLESS));
    }
    else if (new_backend == "Client")
    {
        widgetActionUrl->setVisible(true);
        ActionLabelUrl->setVisible(true);
        widgetActionPort->setVisible(true);
        ActionLabelPort->setVisible(true);
        widgetActionGraph->setVisible(true);
        ActionLabelGraph->setVisible(true);
        widgetActionPath->setVisible(false);
        ActionLabelPath->setVisible(false);
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_CLIENT));
    }
    else if (new_backend == "MultiDbClient")
    {
        for (auto &action : actions)
        {
            action->setVisible(false);
        }

        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_MULTIDB));
    }
    else
    {
        throw std::runtime_error("Unhandled backend type " + new_backend.toStdString());
    }
}

void ToolbarBackend::on_db_path_changed(const QString &db_path)
{
    std::string dbPath = mars::utils::pathJoin(std::getenv("AUTOPROJ_CURRENT_ROOT"), db_path.toStdString());
    xrockGui->db->set_dbPath(dbPath);
}

void ToolbarBackend::on_url_changed(const QString &url)
{
    std::string dbAddress = url.toStdString() + ':' + le_port->text().toStdString();
    xrockGui->db->set_dbAddress(dbAddress);
}

void ToolbarBackend::on_port_changed(const QString &port)
{
    std::string dbAddress = le_url->text().toStdString() + ':' + port.toStdString();
    xrockGui->db->set_dbAddress(dbAddress);
}

void ToolbarBackend::on_graph_changed(const QString &graph)
{
    std::string _graph_name = graph.toStdString();
    xrockGui->db->set_dbGraph(_graph_name);
}

std::string ToolbarBackend::get_dbPath()
{
    return le_db_path->text().toStdString();
}

std::string ToolbarBackend::get_dbAddress()
{
    std::string dbAddress = le_url->text().toStdString() + ':' + le_port->text().toStdString();
    return dbAddress;
}

std::string ToolbarBackend::get_graph()
{
    return le_graph->text().toStdString();
}
