/**
 * \file ComponentModelEditorWidget.hpp
 * \author Malte Langosz
 * \brief
 **/

#pragma once
#include <mars/main_gui/BaseWidget.h>
#include <configmaps/ConfigMap.hpp>
#include <configmaps/ConfigAtom.hpp>
#include "ComponentModelInterface.hpp"
#include "LinkHardwareSoftwareDialog.hpp"
#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

namespace bagel_gui
{
    class BagelGui;
}

class ModelConfig
{
public:
    std::string name;
    std::string data;
};

namespace xrock_gui_model
{
    class XRockGUI;

    class ComponentModelEditorWidget : public mars::main_gui::BaseWidget
    {
        Q_OBJECT

    public:
        ComponentModelEditorWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                                   bagel_gui::BagelGui *bagelGui, XRockGUI *xrockGui,
                                   QWidget *parent = 0);
        ~ComponentModelEditorWidget();
        void clear();
        void deinit();

    public slots:
        // This function/slot should get called whenever the current model has been changed externally
        void currentModelChanged(bagel_gui::ModelInterface *model);
        // This function/slot should get called whenever we have edited some of the model data (in e.g. the fields)
        void updateModel();

        void setViewFilter(int v);
        void layoutsClicked(const QModelIndex &index);
        void addRemoveLayout();
        void openUrl(const QUrl &);
        void validateYamlSyntax();
        // check whether or not we have a widget handling a certain property
        bool has_prop_widget(const std::string& prop_name);
        // get the text by name
        configmaps::ConfigAtom get_prop_widget_value(const std::string &prop_name);
        // update property widget with the name and value
        void update_prop_widget(const std::string &prop_name, configmaps::ConfigAtom &value);
        // update widget with thr properties and as well as fact information
        void update_widgets(configmaps::ConfigMap &info);
        void removeSelectedType();
        void addType();
        void linkHardware();
        void updateManageHardwareLinkButtonState();

    private:
        std::map<QLabel *, QWidget *> widgets; // Generic list of label:
        bagel_gui::BagelGui *bagelGui;
        XRockGUI *xrockGui;
        bagel_gui::ModelInterface *currentModel = nullptr;

        QListWidget *types;

        QListWidget *layouts;
        std::map<std::string, QCheckBox *> layoutCheckBoxes;
        QLineEdit *layoutName, *uri;
        QTextEdit *includes, *annotations, *interfaces;
        QLabel *dataStatusLabel;
        QPushButton *hardwareLinkBtn;

        void updateCurrentLayout();
    };

} // end of namespace xrock_gui_model

