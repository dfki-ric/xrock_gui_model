/**
 * \file BuildModuleDialog.hpp
 * \brief Displays unresolved abstract component models to be resolved 
 *        by selecting each implementation.
 **/

#pragma once
#include <configmaps/ConfigData.h>

#include <QDialog>
#include <QListWidget>
#include <QApplication>
#include <QProcess>
#include <QEventLoop>
#include <QComboBox>
#include <QLabel>

namespace mars
{
    namespace config_map_gui
    {
        class DataWidget;
    }
}

namespace xrock_gui_model
{
    class XRockGUI;
    class BuildModuleDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit BuildModuleDialog(XRockGUI *xrockGui);
        ~BuildModuleDialog();

        // void requestComponent(const std::string &domain, const std::string &name);

    public slots:
        void onBuildButtonClicked();

    private:
        QPushButton *button;
        QLineEdit *moduleNameEdit;

        QListWidget *listWidget;

    private:
        XRockGUI *xrockGui;

        //     std::string acm_uri;
        // std::string selectedDomain;
        std::string name;
        std::map<QLabel*, QComboBox *> selectedImplementationsWidgets; // to cache the infor of lable and Qcomboboy
        std::string uriToplvlcm;
        // std::string selectedVersion;
        };
    } // end of namespace xrock_gui_model
