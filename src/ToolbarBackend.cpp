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
    for (auto const &e : backends)
        cb_backends->addItem(QString::fromStdString(e));
    cb_backends->setCurrentIndex(cb_backends->findText(QString::fromStdString(xrockGui->getBackend()), Qt::MatchFixedString));
    toolbar->addWidget(cb_backends);

    connect(cb_backends, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(on_backend_changed(const QString &)));

    // Db Path
    QLabel *label = new QLabel(" DB Path: ");
    le_db_path = new QLineEdit;
    le_db_path->setText("modkom/component_db");
    le_db_path->setFixedWidth(150);
    ActionLabelPath = toolbar->addWidget(label);
    widgetActionPath = toolbar->addWidget(le_db_path);
    connect(le_db_path, SIGNAL(textChanged(const QString &)), this, SLOT(on_db_path_changed(const QString &)));

    // URL
    label = new QLabel(" URL: ");
    le_url = new QLineEdit;
    le_url->setText("http://0.0.0.0");
    le_url->setFixedWidth(120);
    ActionLabelUrl = toolbar->addWidget(label);
    widgetActionUrl = toolbar->addWidget(le_url);
    connect(le_url, SIGNAL(textChanged(const QString &)), this, SLOT(on_url_changed(const QString &)));

    // Port
    label = new QLabel(" Port: ");
    le_port = new QLineEdit;
    le_port->setText("8183");
    le_port->setFixedWidth(100);
    ActionLabelPort = toolbar->addWidget(label);
    widgetActionPort = toolbar->addWidget(le_port);
    connect(le_port, SIGNAL(textChanged(const QString &)), this, SLOT(on_port_changed(const QString &)));

    // Graph
    label = new QLabel(" Graph: ");
    le_s_graph = new QLineEdit;
    le_s_graph->setText("graph_test");
    le_s_graph->setFixedWidth(120);
    le_c_graph = new QLineEdit;
    le_c_graph->setText("graph_test");
    le_c_graph->setFixedWidth(120);
    ActionLabelGraph = toolbar->addWidget(label);
    widgetActionGraphS = toolbar->addWidget(le_s_graph);
    widgetActionGraphC = toolbar->addWidget(le_c_graph);
    connect(le_s_graph, SIGNAL(textChanged(const QString &)), this, SLOT(on_graph_changed(const QString &)));
    connect(le_c_graph, SIGNAL(textChanged(const QString &)), this, SLOT(on_graph_changed(const QString &)));
    //multidb config dialog icon 
    configDialogAction = toolbar->addAction("Open MultiDbClient Config");
    const std::string icon = mars::utils::pathJoin(XROCK_DEFAULT_RESOURCES_PATH, "xrock_gui_model/resources/images/");
    configDialogAction->setIcon(QIcon(QString::fromStdString(icon + "config.png")));
    toolbar->addAction(configDialogAction);
    connect(configDialogAction, SIGNAL(triggered()), this, SLOT(popUpConfigDialog()));


    // Load default values to toolbar widgets if any bundle was selected
    if (xrockGui->ioLibrary)
    {
        auto default_config = xrockGui->ioLibrary->getDefaultConfig();
        if (!default_config.empty())
        {

            if (default_config.hasKey("Client"))
            {
                std::string full_url = (std::string)default_config["Client"]["url"];
                std::string url = full_url.substr(0, full_url.find_last_of(':'));
                std::string port = full_url.substr(url.size() + 1);
                le_url->setText(QString::fromStdString(url));
                le_port->setText(QString::fromStdString(port));
                le_c_graph->setText(QString::fromStdString((std::string)default_config["Client"]["graph"]));
            }

            if (default_config.hasKey("Serverless"))
            {
                le_db_path->setText(QString::fromStdString((std::string)default_config["Serverless"]["path"]));
                le_s_graph->setText(QString::fromStdString((std::string)default_config["Serverless"]["graph"]));
            }
        }
    }

    // switch initial visibility
    show_toolbar_widgets(QString::fromStdString(xrockGui->getBackend()));
}

ToolbarBackend::~ToolbarBackend()
{
    delete cb_backends;
    delete le_db_path;
    delete le_url;
    delete le_port;
    delete le_c_graph;
    delete le_s_graph;
}

void ToolbarBackend::hide_toolbar_widgets(const QString &backend)
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
    else if(backend == "MultiDbClient")
    {
        configDialogAction->setVisible(false);
    }
}
void ToolbarBackend::show_toolbar_widgets(const QString &backend)
{
    if (backend == "Client")
    {
        widgetActionUrl->setVisible(true);
        ActionLabelUrl->setVisible(true);
        widgetActionPort->setVisible(true);
        ActionLabelPort->setVisible(true);
        widgetActionGraphC->setVisible(true);
        ActionLabelGraph->setVisible(true);
        hide_toolbar_widgets("Serverless");
    }
    else if (backend == "Serverless")
    {
        widgetActionPath->setVisible(true);
        ActionLabelPath->setVisible(true);
        widgetActionGraphS->setVisible(true);
        ActionLabelGraph->setVisible(true);
        hide_toolbar_widgets("Client");
    }
    else if (backend == "MultiDbClient"){
        hide_toolbar_widgets("Serverless");
        hide_toolbar_widgets("Client");
        widgetActionGraphS->setVisible(false);
        widgetActionGraphC->setVisible(false);
        ActionLabelGraph->setVisible(false);
        configDialogAction->setVisible(true);
    }
}

void ToolbarBackend::on_backend_changed(const QString &new_backend)
{
    show_toolbar_widgets(new_backend);
    if (new_backend == "Serverless")
    {
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_SERVERLESS));
    }
    else if (new_backend == "Client")
    {
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_CLIENT));
    }
    else if (new_backend == "MultiDbClient")
    {
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_MULTIDB));
    }
    else if (new_backend == "FileDB")
    {
        xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_FILEDB));
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
    if(cb_backends->currentText() == "Serverless")
        return le_s_graph->text().toStdString();
    else if (cb_backends->currentText() == "Client")
        return le_c_graph->text().toStdString();
    else
        throw std::runtime_error("could not get graph for selected backend: "+cb_backends->currentText().toStdString());
void ToolbarBackend::popUpConfigDialog()
{
    xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_MULTIDB));
}