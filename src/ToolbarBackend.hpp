/*
 *  Copyright 2022, DFKI GmbH Robotics Innovation Center
 */
#pragma once

#include <main_gui/MenuInterface.h>
#include <string>
#include <functional>
#include <QString>
#include <QObject>
#include <QToolBar>
#include <QLabel>
#include "DBInterface.hpp"
#include <main_gui/MainGUI.h>
#include <cfg_manager/CFGManagerInterface.h>
#include <main_gui/GuiInterface.h>

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
        ToolbarBackend(XRockGUI *xrockGui, mars::main_gui::GuiInterface *gui);
        virtual ~ToolbarBackend();
        /** \brief getters */
        std::string getDbPath();
        std::string getDbAddress();
        std::string getGraph();

    private:
        void hideToolbarWidgets(const QString &backend);
        void showToolbarWidgets(const QString &backend);

    private slots:
        void onBackendChanged(const QString &newBackend);
        void onDbPathChanged(const QString &Path);
        void onUrlChanged(const QString &url);
        void onPortChanged(const QString &port);
        void onGraphChanged(const QString &graph);
        void popUpConfigDialog();

    private:
        XRockGUI *xrockGui;
        mars::main_gui::MainGUI *mainGui;
        QComboBox *cbBackends;
        QLineEdit *leDbPath;
        QLineEdit *leUrl;
        QLineEdit *lePort;
        QLineEdit *leSgraph;
        QLineEdit *leCgraph;
        QAction *widgetActionUrl;
        QAction *widgetActionPath;
        QAction *widgetActionPort;
        QAction *widgetActionGraphS;
        QAction *widgetActionGraphC;
        QAction *ActionLabelUrl;
        QAction *ActionLabelPath;
        QAction *ActionLabelPort;
        QAction *ActionLabelGraph;

        QAction *configDialogAction;
    };

} // end of namespace xrock_gui_model
