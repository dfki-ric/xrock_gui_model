/**
 * \file VersionDialog.hpp
 * \author Malte Langosz
 * \brief Displays known versions of a component and allows to select one
 **/

#ifndef XROCK_GUI_MODEL_VERSION_DIALOG_HPP
#define XROCK_GUI_MODEL_VERSION_DIALOG_HPP

#include <configmaps/ConfigData.h>

#include <QDialog>
#include <QListWidget>

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

    class VersionDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit VersionDialog(XRockGUI *xrockGui);
        ~VersionDialog();
        void requestComponent(const std::string &domain, const std::string &name);

    public slots:
        void selectVersion();
        void versionClicked(const QModelIndex &index);
        void versionActivated(const QModelIndex &index);

    private:
        QListWidget *versions;
        mars::config_map_gui::DataWidget *dw;
        std::string selectedDomain;
        std::string selectedModel;
        std::string selectedVersion;
        XRockGUI *xrockGui;
        configmaps::ConfigMap component;
    };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_VERSION_DIALOG_HPP
