/*
 *  Copyright 2022, DFKI GmbH Robotics Innovation Center
 */
#pragma once

#include <mars/main_gui/MenuInterface.h>
#include <string>
#include <functional>
#include <QString>
#include <QObject>
#include <QToolBar>
#include <QLabel>
#include "DBInterface.hpp"
#include <mars/main_gui/MainGUI.h>
#include <mars/cfg_manager/CFGManagerInterface.h>
#include <mars/main_gui/GuiInterface.h>

class QLineEdit;
class QWidget;
class QLabel;
class QComboBox;

namespace xrock_gui_model
{
    class XRockGUI;

    /**
     * \brief ToolbarBackend creates all the menu items related to backend manipulation.
     */
    class ToolbarBackend : public QObject
    {
        Q_OBJECT
    public:
        /** \brief The constructor adds the actions to the File menu */
        ToolbarBackend(XRockGUI *xrockGui, mars::main_gui::GuiInterface *gui, DBInterface *db);
        virtual ~ToolbarBackend();
       /** \brief getters */
        std::string get_dbPath();
        std::string get_dbAddress();
        std::string get_graph();

    private slots:
        void on_backend_changed(const QString &new_backend);
        void on_db_path_changed(const QString &str);
        void on_url_changed(const QString &url);
        void on_port_changed(const QString &port);
        void on_graph_changed(const QString &graph);

    private:
        XRockGUI *xrockGui;
        DBInterface *db;
        mars::main_gui::MainGUI *main_gui;
        QComboBox *cb_backends;
        QLineEdit *le_db_path;
        QLineEdit *le_url;
        QLineEdit *le_port;
        QLineEdit *le_graph;
    };

} // end of namespace xrock_gui_model