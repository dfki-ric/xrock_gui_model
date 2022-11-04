/**
 * \file ImportDialog.hpp
 * \author Malte Langosz
 * \brief Gets a list of basic_model form database and allows to add one
 **/

#ifndef XROCK_GUI_MODEL_IMPORT_DIALOG_HPP
#define XROCK_GUI_MODEL_IMPORT_DIALOG_HPP

#include "XRockGUI.hpp"

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QWebView>

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

    class ImportDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit ImportDialog(XRockGUI *xrockGui, bool load = false);
        ~ImportDialog();

        static std::string lastDomain, lastFilter;

    public slots:
        void addModel();
        void modelClicked(const QModelIndex &index);
        void versionChanged(const QString &versionName);
        void updateFilter(const QString &filter);
        void changeDomain(const QString &domain);
        void urlClicked(const QUrl &);

    signals:
        void sigLoadComponent(std::string domain, std::string model, std::string version);
        void sigAddComponent(std::string domain, std::string model, std::string version);

    private:
        XRockGUI *xrockGui;
        bool load, ignoreUpdate;
        std::string selectedDomain;
        std::string selectedModel;
        std::string selectedVersion;
        std::vector<std::pair<std::string, std::string>> modelList;
        configmaps::ConfigMap indexMap;

        QListWidget *models;
        QLineEdit *filterPattern;
        QComboBox *domainSelect;
        QComboBox *versionSelect;
        QLabel *versionLabel;
        QWebView *doc;
        mars::config_map_gui::DataWidget *dw;
    };
} // end of namespace xrock_gui_model

#endif // XROCK_GUI_MODEL_IMPORT_DIALOG_HPP
