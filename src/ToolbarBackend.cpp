/*
 *  Copyright 2022, DFKI GmbH Robotics Innovation Center
 */


#include "ToolbarBackend.hpp"
#include "XRockGUI.hpp"
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
#include <QLineEdit>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <cstdlib>
#include <mars/utils/misc.h>
using namespace xrock_gui_model;

ToolbarBackend::ToolbarBackend(XRockGUI *xrockGui, mars::main_gui::GuiInterface *gui, DBInterface *db)
    : xrockGui(xrockGui), db(db), main_gui(dynamic_cast<mars::main_gui::MainGUI *>(gui))
{
  QToolBar *toolbar = main_gui->getToolbar("Actions");

  // Backends
  cb_backends = new QComboBox;
  for (auto const &e : DBInterface::loadBackends())
  {
    cb_backends->addItem(QString::fromStdString(e));
  }
  toolbar->addWidget(cb_backends);
  connect(cb_backends, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(on_backend_changed(const QString &)));

  // Db Path
  QLabel *label = new QLabel(" DB Path: ");
  le_db_path = new QLineEdit;
  le_db_path->setText("modkom/component_db");
  le_db_path->setFixedWidth(150);
  toolbar->addWidget(label);
  toolbar->addWidget(le_db_path);
  connect(le_db_path, SIGNAL(textChanged(const QString &)), this, SLOT(on_db_path_changed(const QString &)));

  // URL
  label = new QLabel(" URL: ");
  le_url = new QLineEdit;
  le_url->setText("http://0.0.0.0");
  le_url->setFixedWidth(120);
  toolbar->addWidget(label);
  toolbar->addWidget(le_url);
  connect(le_url, SIGNAL(textChanged(const QString &)), this, SLOT(on_url_changed(const QString &)));

  // Port
  label = new QLabel(" Port: ");
  le_port = new QLineEdit;
  le_port->setText("8183");
  le_port->setFixedWidth(100);
  toolbar->addWidget(label);
  toolbar->addWidget(le_port);
  connect(le_port, SIGNAL(textChanged(const QString &)), this, SLOT(on_port_changed(const QString &)));

  // Graph
  label = new QLabel(" Graph: ");
  le_graph = new QLineEdit;
  le_graph->setText("graph_test");
  le_graph->setFixedWidth(120);
  toolbar->addWidget(label);
  toolbar->addWidget(le_graph);
  connect(le_graph, SIGNAL(textChanged(const QString &)), this, SLOT(on_graph_changed(const QString &)));
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
    xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_SERVERLESS));
    main_gui->disableToolbarLineEdit({2, 3});
    main_gui->enableToolbarLineEdit({1});
  }
  else if (new_backend == "Client")
  {
    xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_CLIENT));
    main_gui->disableToolbarLineEdit({1});
    main_gui->enableToolbarLineEdit({2, 3, 4});
  }
  else if (new_backend == "MultiDbClient")
    xrockGui->menuAction(static_cast<int>(MenuActions::SELECT_MULTIDB));
}

void ToolbarBackend::on_db_path_changed(const QString &db_path)
{
  if (ServerlessDB* sdb = dynamic_cast<ServerlessDB*>(db))
  {
    std::string dbPath = mars::utils::pathJoin(std::getenv("AUTOPROJ_CURRENT_ROOT"), db_path.toStdString());
    sdb->set_dbPath(dbPath);
  }
}
void ToolbarBackend::on_url_changed(const QString &url)
{
  if (RestDB* rdb = dynamic_cast<RestDB* >(db))
  {
    std::string dbAddress = url.toStdString() + ':' + le_port->text().toStdString();
    rdb->set_dbAddress(dbAddress);
  }
}
void ToolbarBackend::on_port_changed(const QString &port)
{
  if (RestDB* rdb = dynamic_cast<RestDB* >(db))
  {
    std::string dbAddress = le_url->text().toStdString() + ':' + port.toStdString();
    rdb->set_dbAddress(dbAddress);
  }
}

void ToolbarBackend::on_graph_changed(const QString &graph)
{
  db->set_dbGraph(graph.toStdString());
}
